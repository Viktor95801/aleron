#include "aleron.h"
#include "crc.h"
#include "util.h"
#include "vendor/stb_ds.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

// returns whether this returns or not. qbe doesnt allow code after a jmp or
// something like that, so this is useful
static bool codegen_stmt(Node *node, char **sb);
static bool codegen_stmt_block(NodeStBlock *node, char **sb);
static void codegen_stmt_return(NodeStReturn *node, char **sb);
static void codegen_stmt_if(NodeStIf *node, char **sb);
static void codegen_stmt_for(NodeStFor *node, char **sb);
static void codegen_stmt_fwhile(NodeStForWhile *node, char **sb);
static void codegen_stmt_expr(NodeStExpr *node, char **sb);

static void codegen_expr(Ast ast, char **sb, const char *var_name);
static void codegen_expr_lit(NodeLit *lit, char **sb, const char *var_name);
static void codegen_expr_unaop(NodeUnaop *una, char **sb, const char *var_name);
static void codegen_expr_binop(NodeBinop *bin, char **sb, const char *var_name);

static bool codegen_stmt(Node *node, char **sb)
{
        assert(node);
        switch (node->kind) {
        case NKSt_EXPR:
                codegen_stmt_expr(&node->as.stexpr, sb);
                return false;
        case NKSt_BLOCK:
                return codegen_stmt_block(&node->as.stblock, sb);

        case NKSt_RETURN:
                codegen_stmt_return(&node->as.streturn, sb);
                return true;
        case NKSt_IF:
                codegen_stmt_if(&node->as.stif, sb);
                return false;
        case NKSt_FOR_WHILE:
                codegen_stmt_fwhile(&node->as.stfwhile, sb);
                return false;
        case NKSt_FOR:
                codegen_stmt_for(&node->as.stfor, sb);
                return false;

        case NKEx_BINOP:
        case NKEx_LIT:
        case NKEx_UNAOP:
        case NK_BAD:
        }

        error(format("invalid statement %d", node->kind));
}

static bool codegen_stmt_block(NodeStBlock *node, char **sb)
{
        assert(node);

        for (int i = 0; i < arrlen(node->list); ++i) {
                bool terminated = codegen_stmt(node->list[i], sb);
                if (terminated) {
                        return true;
                }
        }
        return false;
}

static void codegen_stmt_return(NodeStReturn *node, char **sb)
{
        const char *rc_str(name, format(".tmp%zu", next_reg++));
        codegen_expr(node->expr, sb, name);

        builder_add(sb, format("  %%.ret =w copy %%%s", name));
        builder_add(sb, format("  jmp @.ret"));
}

static void codegen_stmt_if(NodeStIf *node, char **sb)
{
        builder_add(sb, "");
        const char *rc_str(cond, format(".cond%zu", next_reg++));
        codegen_expr(node->cond, sb, cond);

        size_t if_num = next_reg++;
        builder_add(sb, format("  jnz %%%s, @.if%zu, @.else%zu", cond, if_num,
                               if_num));

        builder_add(sb, format("@.if%zu", if_num));
        bool terminated = codegen_stmt(node->block, sb);
        if (!terminated) {
                builder_add(sb, format("  jmp @.endif%zu", if_num));
        }

        builder_add(sb, format("@.else%zu", if_num));
        if (node->ifnot) {
                terminated = codegen_stmt(node->ifnot, sb);
        }
        if (!terminated) {
                builder_add(sb, format("  jmp @.endif%zu", if_num));
        }

        builder_add(sb, format("@.endif%zu", if_num));
}

static void codegen_stmt_for(NodeStFor *node, char **sb)
{
        size_t for_num = next_reg++;
}

static void codegen_stmt_fwhile(NodeStForWhile *node, char **sb)
{
        size_t while_num = next_reg++;
        builder_add(sb, format("\n@.check%zu", while_num));
        const char *rc_str(cond, format(".cond%zu", next_reg++));
        codegen_expr(node->cond, sb, cond);

        builder_add(sb, format("  jnz %%%s, @.loop%zu, @.endloop%zu", cond,
                               while_num, while_num));

        builder_add(sb, format("@.loop%zu", while_num));
        bool terminated = codegen_stmt(node->block, sb);
        if (!terminated) {
                builder_add(sb, format("  jmp @.check%zu", while_num));
        }

        builder_add(sb, format("@.endloop%zu", while_num));
}

static void codegen_stmt_expr(NodeStExpr *node, char **sb)
{
        codegen_expr(node->expr, sb, ".sexpr");
}

static void codegen_expr_lit(NodeLit *lit, char **sb, const char *var_name)
{
        switch (lit->kind) {
                // TODO: this doesnt handle the possibility of an undeclared
                // identifier. the actual todo is to build a semantic analyser
                // to make sure the types are correct and the scopes are working
                // properly
                // TODO: make sure dierct register assignings are as functional
                // as stack alloc4, loadw and storew
        case LK_ID:
                builder_add(sb, format("  %%%s =w loadw %%.var." SV_Fmt,
                                       var_name, Mtokstr_fmt(*lit)));
                break;
        case LK_INT:
                builder_add(sb, format("  %%%s =w copy %.*s", var_name,
                                       (int)lit->str.count, lit->str.data));
                break;
        }
}

static void codegen_expr_unaop(NodeUnaop *una, char **sb, const char *var_name)
{
        assert(una->kind == UNAOP_NEG);
        const char *rc_str(name, format(".tmp.una%zu", next_reg++));
        codegen_expr(una->expr, sb, name);

        builder_add(sb, format("  %%%s =w mul %%%s, -1", var_name, name));
}

static void codegen_expr_binop(NodeBinop *bin, char **sb, const char *var_name)
{
        if (bin->kind == BINOP_ASS) {
                const char *rc_str(right_name, // intermediates
                                   format(".tmp.ass%zu", next_reg++));
                codegen_expr(bin->right, sb, right_name);
                builder_add(sb, // declare local var
                            format("  %%.var." SV_Fmt " =l alloc4 1",
                                   Mtokstr_fmt(bin->left->as.lit)));
                builder_add(sb, // assign
                            format("  storew %%%s, %%.var." SV_Fmt, right_name,
                                   Mtokstr_fmt(bin->left->as.lit)));

                // output of the expression
                builder_add(sb, format("  %%%s =w copy %%%s", var_name,
                                       right_name));
                return;
        }

        const char *rc_str(left_name, format(".tmp.l%zu", next_reg++));
        codegen_expr(bin->left, sb, left_name);
        const char *rc_str(right_name, format(".tmp.r%zu", next_reg++));
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
        case NKSt_RETURN:
        case NKSt_IF:
        case NKSt_BLOCK:
        case NKSt_EXPR:
        case NKSt_FOR_WHILE:
        case NKSt_FOR:
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

        builder_add(&sb, "  %.ret =w copy 0");
        codegen_stmt(ast, &sb);

        builder_add(&sb, "\n@.ret");
        builder_add(&sb, "  ret %.ret");
        builder_add(&sb, "}");
        builder_null(&sb);

        return sb;
}

void codegen_destroy(char *data)
{
        builder_destroy(data);
}
