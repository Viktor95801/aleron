#include "aleron.h"
#include "crc.h"
#include "util.h"
#include "vendor/stb_ds.h"

// add raw
static void builder_addr(char **sb, const char *str)
{
        assert(str);

        size_t len = strlen(str);
        i32 where = arraddnindex(*sb, len);
        strncpy(*sb + where, str, len);
}

static void builder_add(char **sb, const char *str)
{
        assert(str);

        size_t len = strlen(str);
        i32 where = arraddnindex(*sb, len);
        strncpy(*sb + where, str, len);
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

static i32 var_name_counter = 0;

static void codegen_expr(Ast ast, char *sb, const char *var_name);
static void codegen_expr_lit(NodeLit *lit, char *sb, const char *var_name);
static void codegen_expr_binary(NodeBinop *bin, char *sb, const char *var_name,
                                int *ssa_index);

static void codegen_expr_lit(NodeLit *lit, char *sb, const char *var_name)
{
        assert(lit->kind == LK_INT);
        builder_add(&sb, format("  %%%s =w copy %.*s", var_name,
                                (int)lit->str.count, lit->str.data));
}

static void codegen_expr_binary(NodeBinop *bin, char *sb, const char *var_name,
                                int *ssa_index)
{
        const char *rc_str(left_name, format("l%d", var_name_counter));
        const char *rc_str(right_name, format("r%d", var_name_counter++));
        codegen_expr(bin->left, sb, left_name);
        codegen_expr(bin->right, sb, right_name);

        builder_add(&sb,
                    format("  %%%s =w %s %%%s, %%%s", var_name,
                           binopk_to_str(bin->kind), left_name, right_name));
}

static void codegen_expr(Ast ast, char *sb, const char *var_name)
{
        assert(ast);
        i32 ssa_index = 0;

        switch (ast->kind) {
        case NK_BINOP: {
                codegen_expr_binary(&ast->as.binop, sb, var_name, &ssa_index);
        } break;
        case NK_LIT: {
                codegen_expr_lit(&ast->as.lit, sb, var_name);
        } break;

        default:
                error("invalid expression");
                break;
        }

        builder_add(&sb, format("  ret %%%s", var_name));
}

// must free
char *codegen(Ast ast)
{
        assert(ast);
        char *builder = NULL;
        arrsetcap(builder, 1024);

        builder_add(&builder, format("export function w $main() {\n"
                                     "@start"));

        codegen_expr(ast, builder, "x");

        builder_add(&builder, "}");
        builder_null(&builder);

        return builder;
}

void codegen_destroy(char *data)
{
        assert(data);
        arrfree(data);
}
