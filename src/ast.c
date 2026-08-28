#include "aleron.h"

#include <assert.h>
#include <stdio.h>

#include "crc.h"
#include "util.h"
#include "vendor/sv.h"

void destroy_node(void *ptr)
{
        assert(ptr);
        Node *node = ptr;
        switch (node->kind) {
        case NK_BINOP: {
                NodeBinop binop = node->as.binop;
                del(binop.left);
                del(binop.right);
        } break;
        case NK_LIT: {
        } break;

        case NK_BAD:
        case NK_end:
                break;
        }
}

Node *new_node(NodeKind kind)
{
        assert(kind >= NK_BAD && kind < NK_end);

        Node *result = with_deleter(sizeof(Node), destroy_node);
        memset(result, 0, sizeof(Node));
        result->kind = kind;

        return result;
}

Node *new_lit(LiteralKind kind, String_View str)
{
        Node *result = new_node(NK_LIT);
        NodeLit *lit = &result->as.lit;
        lit->kind = kind;
        lit->str = str;

        return result;
}

Node *new_binop(BinopKind kind, Node *left, Node *right)
{
        assert(kind >= 0 && kind <= BINOP_end);
        assert(left);
        assert(right);

        Node *result = new_node(NK_BINOP);
        NodeBinop *binop = &result->as.binop;
        binop->kind = kind;
        binop->left = left;
        binop->right = right;

        return result;
}

static inline void dump_indent(FILE *file, u32 indent)
{
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

        case BINOP_end:
        }

        assert(0 && format("%s:%d %s unrecognized %d", __FILE__, __LINE__,
                           __FUNCTION__, kind));
}

static void dump_literal(NodeLit *node, FILE *file)
{
        assert(node);
        assert(file);

        fprintf(file, "i" SV_Fmt, (int)node->str.count, node->str.data);
        switch (node->kind) {
        case LK_INT: {
        } break;
        }
}

static void dump_node(Node *node, FILE *file, u32 indent);
static void dump_binop(NodeBinop *node, FILE *file, u32 indent)
{
        assert(node);
        assert(file);

        fprintf(file, "binary(%s,\n", binopk_to_str(node->kind));

        dump_indent(file, indent);
        dump_node(node->left, file, indent + 1);
        fputs(",\n", file);

        dump_indent(file, indent);
        dump_node(node->right, file, indent + 1);

        fputs(")", file);
}

static void dump_node(Node *node, FILE *file, u32 indent)
{
        assert(node);
        assert(file);

        switch (node->kind) {
        case NK_LIT:
                dump_literal(&node->as.lit, file);
                break;
        case NK_BINOP:
                dump_binop(&node->as.binop, file, indent);
                break;
        default:
                assert(0 && format("%s:%d %s unrecognized %d", __FILE__,
                                   __LINE__, __FUNCTION__, node->kind));
        }
}

void ast_dump(Ast ast, FILE *file)
{
        assert(file);
        dump_node(ast, file, 0);
        fputc('\n', file);
}
