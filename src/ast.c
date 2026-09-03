#include "aleron.h"

#include <assert.h>
#include <stdio.h>

#include "crc.h"
#include "util.h"
#include "vendor/stb_ds.h"
#include "vendor/sv.h"

void destroy_node(void *ptr)
{
        assert(ptr);
        Node *node = ptr;
        switch (node->kind) {
        case NKSt_BLOCK: {
                NodeStBlock block = node->as.stblock;
                for (int i = 0; i < arrlen(block.list); ++i) {
                        del(block.list[i]);
                }
                arrfree(block.list);
        } break;
        case NKSt_RETURN: {
                NodeStReturn stret = node->as.streturn;
                del(stret.expr);
        } break;
        case NKSt_IF: {
                NodeStIf stif = node->as.stif;
                del(stif.block);
                del(stif.cond);
                if (stif.ifnot) {
                        del(stif.ifnot);
                }
        } break;
        case NKSt_FOR_WHILE: {
                NodeStForWhile stwhile = node->as.stfwhile;
                del(stwhile.cond);
                del(stwhile.block);
        } break;
        case NKSt_EXPR: {
                NodeStExpr stexpr = node->as.stexpr;
                del(stexpr.expr);
        } break;

        case NKEx_BINOP: {
                NodeBinop binop = node->as.binop;
                del(binop.left);
                del(binop.right);
        } break;
        case NKEx_UNAOP: {
                NodeUnaop unaop = node->as.unaop;
                del(unaop.expr);
        } break;
        case NKEx_LIT: {
        } break;

        case NK_BAD: {
        } break;
        }
}

Node *new_node(NodeKind kind)
{
        Node *result = with_deleter(sizeof(Node), destroy_node);
        memset(result, 0, sizeof(Node));
        result->kind = kind;

        return result;
}

Node *new_stblock(Node **list)
{
        Node *result = new_node(NKSt_BLOCK);
        NodeStBlock *block = &result->as.stblock;
        block->list = list;

        return result;
}

Node *new_stif(Node *cond, Node *block, Node *ifnot)
{
        Node *result = new_node(NKSt_IF);
        NodeStIf *stif = &result->as.stif;
        stif->block = block;
        stif->cond = cond;
        stif->ifnot = ifnot;

        return result;
}

Node *new_stfwhile(Node *cond, Node *block)
{
        Node *result = new_node(NKSt_FOR_WHILE);
        NodeStForWhile *stfwhile = &result->as.stfwhile;

        stfwhile->cond = cond;
        stfwhile->block = block;

        return result;
}

Node *new_stexpr(Node *expr)
{
        Node *result = new_node(NKSt_EXPR);
        NodeStExpr *stexpr = &result->as.stexpr;
        stexpr->expr = expr;

        return result;
}

Node *new_streturn(Node *expr)
{
        Node *result = new_node(NKSt_RETURN);
        NodeStReturn *stret = &result->as.streturn;
        stret->expr = expr;

        return result;
}

Node *new_lit(LiteralKind kind, String_View str)
{
        Node *result = new_node(NKEx_LIT);
        NodeLit *lit = &result->as.lit;
        lit->kind = kind;
        lit->str = str;

        return result;
}

Node *new_binop(BinopKind kind, Node *left, Node *right)
{
        assert(left);
        assert(right);

        Node *result = new_node(NKEx_BINOP);
        NodeBinop *binop = &result->as.binop;
        binop->kind = kind;
        binop->left = left;
        binop->right = right;

        return result;
}

Node *new_unary(UnaopKind kind, Node *inside)
{
        assert(inside);

        Node *result = new_node(NKEx_UNAOP);
        NodeUnaop *unaop = &result->as.unaop;
        unaop->kind = kind;
        unaop->expr = inside;

        return result;
}

static inline void dump_indent(FILE *file, u32 indent)
{
        return;
        for (i32 i = indent; i >= 0; --i) {
                fputs("  ", file);
        }
}

const char *binopk_to_str(BinopKind kind)
{
        switch (kind) {
        case BINOP_ADD:
                return "add";
        case BINOP_SUB:
                return "sub";
        case BINOP_MUL:
                return "mul";
        case BINOP_DIV:
                return "div";
        case BINOP_ASS:
                return "ass";
        }

        assert(0 && format("unrecognized %d", kind));
}

static const char *unaopk_to_str(UnaopKind kind)
{
        switch (kind) {
        case UNAOP_NEG:
                return "neg";
        }

        assert(0 && format("unrecognized %d", kind));
}

static void dump_node(Node *node, FILE *file, u32 indent);
static void dump_stblock(NodeStBlock *node, FILE *file, u32 indent)
{
        assert(node);
        assert(file);

        fprintf(file, "block{");

        for (int i = 0; i < arrlen(node->list); ++i) {
                dump_indent(file, indent);
                dump_node(node->list[i], file, indent + 1);
        }

        dump_indent(file, indent - 1);
        fputs("}", file);
}

static void dump_streturn(NodeStReturn *node, FILE *file, u32 indent)
{
        assert(node);
        assert(file);

        fprintf(file, "return{");

        dump_indent(file, indent);
        dump_node(node->expr, file, indent + 1);

        dump_indent(file, indent - 1);
        fputs("}", file);
}

static void dump_stfwhile(NodeStForWhile *node, FILE *file, u32 indent)
{
        assert(node);
        assert(file);

        fprintf(file, "while(");
        dump_node(node->cond, file, indent);

        fprintf(file, ") ");
        dump_node(node->block, file, indent + 1);
}

static void dump_stif(NodeStIf *node, FILE *file, u32 indent)
{
        assert(node);
        assert(file);

        fprintf(file, "if(");
        dump_node(node->cond, file, indent);

        fprintf(file, ") ");
        dump_node(node->block, file, indent + 1);

        if (node->ifnot) {
                fprintf(file, " else ");
                dump_node(node->ifnot, file, indent + 1);
        }
}

static void dump_stexpr(NodeStExpr *node, FILE *file, u32 indent)
{
        assert(node);
        assert(file);

        fprintf(file, "expr{");

        dump_indent(file, indent);
        dump_node(node->expr, file, indent + 1);

        dump_indent(file, indent - 1);
        fputs("}", file);
}

static void dump_literal(NodeLit *node, FILE *file)
{
        assert(node);
        assert(file);

        switch (node->kind) {
        case LK_INT:
                fprintf(file, SV_Fmt ":int", Mtokstr_fmt(*node));
                break;
        case LK_ID:
                fprintf(file, SV_Fmt ":id", Mtokstr_fmt(*node));
                break;
        }
}

static void dump_node(Node *node, FILE *file, u32 indent);
static void dump_unaop(NodeUnaop *node, FILE *file, u32 indent)
{
        assert(node);
        assert(file);

        fprintf(file, "unary(%s,", unaopk_to_str(node->kind));

        dump_indent(file, indent);
        dump_node(node->expr, file, indent + 1);

        dump_indent(file, indent - 1);
        fputs(")", file);
}

static void dump_binop(NodeBinop *node, FILE *file, u32 indent)
{
        assert(node);
        assert(file);

        fprintf(file, "binary(%s,", binopk_to_str(node->kind));

        dump_indent(file, indent);
        dump_node(node->left, file, indent + 1);
        fputs(",", file);

        dump_indent(file, indent);
        dump_node(node->right, file, indent + 1);

        fputs(")", file);
}

static void dump_node(Node *node, FILE *file, u32 indent)
{
        assert(node);
        assert(file);

        switch (node->kind) {
        case NKSt_BLOCK:
                dump_stblock(&node->as.stblock, file, indent);
                return;
        case NKSt_RETURN:
                dump_streturn(&node->as.streturn, file, indent);
                return;
        case NKSt_IF:
                dump_stif(&node->as.stif, file, indent);
                return;
        case NKSt_FOR_WHILE:
                dump_stfwhile(&node->as.stfwhile, file, indent);
                return;
        case NKSt_EXPR:
                dump_stexpr(&node->as.stexpr, file, indent);
                return;

        case NKEx_LIT:
                dump_literal(&node->as.lit, file);
                return;
        case NKEx_BINOP:
                dump_binop(&node->as.binop, file, indent);
                return;
        case NKEx_UNAOP:
                dump_unaop(&node->as.unaop, file, indent);
                return;

        case NK_BAD:
        }

        assert(0 && format("unrecognized %d", node->kind));
}

void ast_dump(Ast ast, FILE *file)
{
        assert(ast);
        assert(file);
        dump_node(ast, file, 0);
        fputc('\n', file);
}
