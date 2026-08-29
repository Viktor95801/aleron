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

static void codegen_expr(Ast ast, char **sb, const char *var_name,
                         int *ssa_index);
static void codegen_expr_lit(NodeLit *lit, char **sb, const char *var_name);
static void codegen_expr_unary(NodeUnaop *una, char **sb, const char *var_name,
                               int *ssa_index);
static void codegen_expr_binary(NodeBinop *bin, char **sb, const char *var_name,
                                int *ssa_index);

static void codegen_expr_lit(NodeLit *lit, char **sb, const char *var_name)
{
        assert(lit->kind == LK_INT);
        builder_add(sb, format("  %%%s =w copy %.*s", var_name,
                               (int)lit->str.count, lit->str.data));
}

static void codegen_expr_unary(NodeUnaop *una, char **sb, const char *var_name,
                               int *ssa_index)
{
        assert(una->kind == UNAOP_NEG);
        const char *rc_str(name, format("tmp%d", (*ssa_index)++));
        codegen_expr(una->expr, sb, name, ssa_index);

        builder_add(sb, format("  %%%s =w mul %%%s, -1", var_name, name));
}

static void codegen_expr_binary(NodeBinop *bin, char **sb, const char *var_name,
                                int *ssa_index)
{
        const char *rc_str(left_name, format("l%d", *ssa_index));
        const char *rc_str(right_name, format("r%d", (*ssa_index)++));
        codegen_expr(bin->left, sb, left_name, ssa_index);
        codegen_expr(bin->right, sb, right_name, ssa_index);

        builder_add(sb,
                    format("  %%%s =w %s %%%s, %%%s", var_name,
                           binopk_to_str(bin->kind), left_name, right_name));
}

static void codegen_expr(Ast ast, char **sb, const char *var_name,
                         int *ssa_index)
{
        assert(ast);

        switch (ast->kind) {
        case NK_UNAOP: {
                codegen_expr_unary(&ast->as.unaop, sb, var_name, ssa_index);
                return;
        } break;

        case NK_BINOP: {
                codegen_expr_binary(&ast->as.binop, sb, var_name, ssa_index);
                return;
        } break;

        case NK_LIT: {
                codegen_expr_lit(&ast->as.lit, sb, var_name);
                return;
        } break;

        case NK_BAD:
        }
        error(format("invalid expression %d", ast->kind));
}

// must free
char *codegen(Ast ast)
{
        assert(ast);
        char *builder = NULL;
        arrsetcap(builder, 1024);

        builder_add(&builder, "export function w $main() {\n"
                              "@start");

        int ssa_index = 0;
        codegen_expr(ast, &builder, "x", &ssa_index);

        builder_add(&builder, "  ret %x");
        builder_add(&builder, "}");
        builder_null(&builder);

        return builder;
}

void codegen_destroy(char *data)
{
        builder_destroy(data);
}
