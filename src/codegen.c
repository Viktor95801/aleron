#include "aleron.h"
#include "crc.h"
#include "util.h"
#include "vendor/stb_ds.h"
#include <__stdarg_va_list.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// add raw
/* static void builder_addr( const char *str)
{
        assert(str);

        size_t len = strlen(str);
        i32 where = arraddnindex(*sb                        , len);
        strncpy(*sb + where                                 , str , len);
}
 */

char *sb = NULL;

ATT_FORMAT(1, 2) static void builder_add(const char *fmt, ...)
{
        assert(fmt);

        va_list arg;

        va_start(arg, fmt);

        const char *result = formatv(fmt, arg);
        size_t len = strlen(result);
        char *where = arraddnptr(sb, len);

        memcpy(where, result, len);
        arrpush(sb, '\n');

        va_end(arg);
}

static void builder_null()
{
        arrpush(sb, '\0');
}

static void builder_destroy()
{
        assert(sb);
        arrfree(sb);
}

static u64 next_reg = 0;

// returns whether this returns or not. qbe doesnt allow code after a jmp or
// something like that, so this is useful
static bool codegen_stmt(Node *node);
static bool codegen_stmt_block(NodeStBlock *node);
static void codegen_stmt_return(NodeStReturn *node);
static void codegen_stmt_if(NodeStIf *node);
static void codegen_stmt_for(NodeStFor *node);
static void codegen_stmt_fwhile(NodeStForWhile *node);
static void codegen_stmt_expr(NodeStExpr *node);

static void codegen_expr(Ast ast, const char *var_name);
static void codegen_expr_lit(NodeLit *lit, const char *var_name);
static void codegen_expr_unaop(NodeUnaop *una, const char *var_name);
static void codegen_expr_binop(NodeBinop *bin, const char *var_name);

static void codegen_addr(Node *node, const char *var_name);

static bool codegen_stmt(Node *node)
{
        assert(node);
        switch (node->kind) {
        case NKSt_EXPR:
                codegen_stmt_expr(&node->as.stexpr);
                return false;
        case NKSt_BLOCK:
                return codegen_stmt_block(&node->as.stblock);

        case NKSt_RETURN:
                codegen_stmt_return(&node->as.streturn);
                return true;
        case NKSt_IF:
                codegen_stmt_if(&node->as.stif);
                return false;
        case NKSt_FOR_WHILE:
                codegen_stmt_fwhile(&node->as.stfwhile);
                return false;
        case NKSt_FOR:
                codegen_stmt_for(&node->as.stfor);
                return false;

        case NKEx_BINOP:
        case NKEx_LIT:
        case NKEx_UNAOP:
        case NK_BAD:
        }

        error(format("invalid statement %d", node->kind));
}

static bool codegen_stmt_block(NodeStBlock *node)
{
        assert(node);

        for (int i = 0; i < arrlen(node->list); ++i) {
                bool terminated = codegen_stmt(node->list[i]);
                if (terminated) {
                        return true;
                }
        }
        return false;
}

static void codegen_stmt_return(NodeStReturn *node)
{
        const char *rc_str(name, format(".tmp%zu", next_reg++));
        codegen_expr(node->expr, name);

        builder_add("  %%.ret =l copy %%%s", name);
        builder_add("  jmp @.ret");
}

static void codegen_stmt_if(NodeStIf *node)
{
        builder_add("");
        const char *rc_str(cond, format(".cond%zu", next_reg++));
        codegen_expr(node->cond, cond);

        size_t if_num = next_reg++;
        builder_add("  jnz %%%s, @.if%zu, @.else%zu", cond, if_num, if_num);

        builder_add("@.if%zu", if_num);
        bool terminated = codegen_stmt(node->block);
        if (!terminated) {
                builder_add("  jmp @.endif%zu", if_num);
        }

        builder_add("@.else%zu", if_num);
        if (node->ifnot) {
                terminated = codegen_stmt(node->ifnot);
        }
        if (!terminated) {
                builder_add("  jmp @.endif%zu", if_num);
        }

        builder_add("@.endif%zu", if_num);
}

static void codegen_stmt_for(NodeStFor *node)
{
        size_t for_num = next_reg++;
        builder_add("\n@.init%zu", for_num);
        codegen_expr(node->init, ".init");

        builder_add("@.check%zu", for_num);
        const char *rc_str(cond, format(".cond%zu", next_reg++));
        codegen_expr(node->cond, cond);

        builder_add("  jnz %%%s, @.loop%zu, @.endloop%zu", cond, for_num,
                    for_num);

        builder_add("@.loop%zu", for_num);
        bool terminated = codegen_stmt(node->block);

        if (!terminated) {
                builder_add("");
                codegen_expr(node->post, ".post");
                builder_add("  jmp @.check%zu", for_num);
        }

        builder_add("@.endloop%zu", for_num);
}

static void codegen_stmt_fwhile(NodeStForWhile *node)
{
        size_t while_num = next_reg++;
        builder_add("\n@.check%zu", while_num);
        const char *rc_str(cond, format(".cond%zu", next_reg++));
        codegen_expr(node->cond, cond);

        builder_add("  jnz %%%s, @.loop%zu, @.endloop%zu", cond, while_num,
                    while_num);

        builder_add("@.loop%zu", while_num);
        bool terminated = codegen_stmt(node->block);
        if (!terminated) {
                builder_add("  jmp @.check%zu", while_num);
        }

        builder_add("@.endloop%zu", while_num);
}

static void codegen_stmt_expr(NodeStExpr *node)
{
        codegen_expr(node->expr, ".sexpr");
}

static void codegen_expr_lit(NodeLit *lit, const char *var_name)
{
        switch (lit->kind) {
                // TODO: this doesnt handle the possibility of an undeclared
                // identifier. the actual todo is to build a semantic analyser
                // to make sure the types are correct and the scopes are working
                // properly
                // TODO: make sure dierct register assignings are as functional
                // as stack alloc4, loadw and storew
        case LK_ID:
                builder_add("  %%%s =l loadw %%.var." SV_Fmt, var_name,
                            Mtokstr_fmt(*lit));
                break;
        case LK_INT:
                builder_add("  %%%s =l copy %.*s", var_name,
                            (int)lit->str.count, lit->str.data);
                break;
        }
}

static void codegen_expr_unaop(NodeUnaop *node, const char *var_name)
{
        switch (node->kind) {
        case UNAOP_NEG: {
                const char *rc_str(name, format(".tmp.una%zu", next_reg++));
                codegen_expr(node->expr, name);

                builder_add("  %%%s =l mul %%%s, -1", var_name, name);
                return;
        }
        case UNAOP_ADDR: {
                error("todo: unaop_addr and codegen_addr");
                return;
        }
        case UNAOP_STAR: {
                const char *rc_str(name, format(".tmp.una%zu", next_reg++));
                codegen_expr(node->expr, name);

                builder_add("  %%%s =l loadw %%%s", var_name, name);

                return;
        }
        }
        error(".");
}

static void codegen_expr_binop(NodeBinop *node, const char *var_name)
{
        if (node->kind == BINOP_ASS) {
                const char *rc_str(right_name, // intermediates
                                   format(".tmp.ass%zu", next_reg++));
                codegen_expr(node->right, right_name);
                builder_add( // declare local var
                        "  %%.var." SV_Fmt " =l alloc4 1",
                        Mtokstr_fmt(node->left->as.lit));
                builder_add( // assign
                        "  storew %%%s, %%.var." SV_Fmt, right_name,
                        Mtokstr_fmt(node->left->as.lit));

                // output of the expression
                builder_add("  %%%s =l copy %%%s", var_name, right_name);
                return;
        }

        const char *rc_str(left_name, format(".tmp.l%zu", next_reg++));
        codegen_expr(node->left, left_name);
        const char *rc_str(right_name, format(".tmp.r%zu", next_reg++));
        codegen_expr(node->right, right_name);

        builder_add("  %%%s =l %s %%%s, %%%s", var_name,
                    binopk_to_str(node->kind), left_name, right_name);
}

static void codegen_expr(Node *node, const char *var_name)
{
        assert(node);

        switch (node->kind) {
        case NKEx_UNAOP: {
                codegen_expr_unaop(&node->as.unaop, var_name);
                return;
        } break;

        case NKEx_BINOP: {
                codegen_expr_binop(&node->as.binop, var_name);
                return;
        } break;

        case NKEx_LIT: {
                codegen_expr_lit(&node->as.lit, var_name);
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

// static void codegen_addr(Node *node, const char *var_name)
// {
//         if (node->kind == NKEx_LIT && node->as.lit.kind == LK_ID) {
//                 NodeLit *n = &node->as.lit;

//                 return;
//         } else if (node->kind == NKEx_UNAOP &&
//                    node->as.unaop.kind == UNAOP_ADDR) {
//                 NodeUnaop *n = &node->as.unaop;
//                 codegen_expr(, const char *var_name) return;
//         }

//         error("not an lvalue");
// }

// must free
char *codegen(Ast ast)
{
        assert(ast);
        arrsetcap(sb, 1024);

        builder_add("export function w $main() {\n"
                    "@start");

        builder_add("  %%.ret =l copy 0");
        codegen_stmt(ast);

        builder_add("\n@.ret");
        builder_add("  ret %%.ret");
        builder_add("}");
        builder_null();

        return sb;
}

void codegen_destroy(char *_)
{
        builder_destroy();
}
