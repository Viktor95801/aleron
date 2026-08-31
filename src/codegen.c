#include "aleron.h"
#include "crc.h"
#include "util.h"
#include "vendor/stb_ds.h"
#include <assert.h>

// add raw
/* static void builder_addr(char **sb, const char *str)
{
        assert(str);

        size_t len = strlen(str);
        i32 where = arraddnindex(*sb, len);
        strncpy(*sb + where, str, len);
}
 */

static void builder_add(char **sb, const char *str)
{
        assert(str);

        size_t len = strlen(str);
        i32 where = arraddnindex(*sb, len);
        memcpy(*sb + where, str, len);
        arrpush(*sb, '\n');
}

static void builder_null(char **sb)
{
        arrpush(*sb, '\0');
}

static void builder_destroy(char *sb)
{
        assert(sb);
        arrfree(sb);
}

static u64 next_reg = 0;

static void codegen_stmt(Node *node, char **sb);
static void codegen_stmt_block(NodeStBlock *node, char **sb);
static void codegen_stmt_expr(NodeStExpr *node, char **sb);

static void codegen_expr(Ast ast, char **sb, const char *var_name);
static void codegen_expr_lit(NodeLit *lit, char **sb, const char *var_name);
static void codegen_expr_unaop(NodeUnaop *una, char **sb, const char *var_name);
static void codegen_expr_binop(NodeBinop *bin, char **sb, const char *var_name);

static void codegen_stmt(Node *node, char **sb)
{
        assert(node);
        switch (node->kind) {
        case NKSt_EXPR: {
                codegen_stmt_expr(&node->as.stexpr, sb);
                return;
        } break;
        case NKSt_BLOCK: {
                codegen_stmt_block(&node->as.stblock, sb);
                return;
        } break;

        case NKEx_BINOP:
        case NKEx_LIT:
        case NKEx_UNAOP:
        case NK_BAD:
        }

        error(format("invalid statement %d", node->kind));
}

static void codegen_stmt_block(NodeStBlock *node, char **sb)
{
        assert(node);

        for (int i = 0; i < arrlen(node->list); ++i) {
                codegen_stmt(node->list[i], sb);
        }
}

static void codegen_stmt_expr(NodeStExpr *node, char **sb)
{
        codegen_expr(node->expr, sb, "t");
}

static void codegen_expr_lit(NodeLit *lit, char **sb, const char *var_name)
{
        assert(lit->kind == LK_INT);
        builder_add(sb, format("  %%%s =w copy %.*s", var_name,
                               (int)lit->str.count, lit->str.data));
}

static void codegen_expr_unaop(NodeUnaop *una, char **sb, const char *var_name)
{
        assert(una->kind == UNAOP_NEG);
        const char *rc_str(name, format("tmp%zu", next_reg++));
        codegen_expr(una->expr, sb, name);

        builder_add(sb, format("  %%%s =w mul %%%s, -1", var_name, name));
}

static void codegen_expr_binop(NodeBinop *bin, char **sb, const char *var_name)
{
        const char *rc_str(left_name, format("l%zu", next_reg++));
        codegen_expr(bin->left, sb, left_name);
        const char *rc_str(right_name, format("r%zu", next_reg++));
        codegen_expr(bin->right, sb, right_name);

        builder_add(sb,
                    format("  %%%s =w %s %%%s, %%%s", var_name,
                           binopk_to_str(bin->kind), left_name, right_name));
}

static void codegen_expr(Node *node, char **sb, const char *var_name)
{
        assert(node);

        switch (node->kind) {
        case NKEx_UNAOP: {
                codegen_expr_unaop(&node->as.unaop, sb, var_name);
                return;
        } break;

        case NKEx_BINOP: {
                codegen_expr_binop(&node->as.binop, sb, var_name);
                return;
        } break;

        case NKEx_LIT: {
                codegen_expr_lit(&node->as.lit, sb, var_name);
                return;
        } break;

        case NK_BAD:
        case NKSt_BLOCK:
        case NKSt_EXPR:
        }
        error(format("invalid expression %d", node->kind));
}

// must free
char *codegen(Ast ast)
{
        assert(ast);
        char *sb = NULL;
        arrsetcap(sb, 1024);

        builder_add(&sb, "export function w $main() {\n"
                         "@start");

        builder_add(&sb, "  %t =w copy 0");
        codegen_stmt(ast, &sb);

        builder_add(&sb, "  ret %t");
        builder_add(&sb, "}");
        builder_null(&sb);

        return sb;
}

void codegen_destroy(char *data)
{
        builder_destroy(data);
}
