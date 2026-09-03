#include "aleron.h"

#include "vendor/stb_ds.h"
#include <string.h>

typedef struct {
        const char *src;
        char *pos;
        Token ptok, ctok, ntok;
} Parser;

static void advance(Parser *p)
{
        ScanResult scan = next_token(&p->pos);
        if (scan.error) {
                error_at(p->src, scan.token.str.data, scan.error);
        }
        p->ptok = p->ctok;
        p->ctok = p->ntok;
        p->ntok = scan.token;

        // printf("%d %.*s\n", p->ptok.kind, p->ptok.len, p->src + p->ptok.pos);
        // printf("%d %.*s\n", p->ctok.kind, p->ctok.len, p->src + p->ctok.pos);
        // printf("%d %.*s\n", p->ntok.kind, p->ntok.len, p->src + p->ntok.pos);
        // puts("--");
}

static void init_parser(Parser *p, const char *source)
{
        memset(p, 0, sizeof(Parser));
        p->src = source;
        p->pos = (char *)source;

        advance(p);
        advance(p);
}

static bool check(Parser *p, TokenKind kind)
{
        return p->ctok.kind == kind;
}
static bool consume(Parser *p, TokenKind kind)
{
        if (check(p, kind)) {
                advance(p);
                return true;
        }
        return false;
}

static void expect(Parser *p, TokenKind kind, const char *what)
{
        if (!consume(p, kind)) {
                error_at(p->src, p->ctok.str.data, "expected: %s", what);
        }
}

static void unclosed(Parser *p, const char *start, const char *what)
{
        error_at(p->src, start, "unclosed: %s", what);
}

static Node *expr(Parser *p);
static Node *eass(Parser *p);
static Node *eadd(Parser *p);
static Node *emul(Parser *p);
static Node *eunary(Parser *p);
static Node *eprimary(Parser *p);

static Node *sstmt(Parser *p);
static Node *sblock(Parser *p);
static Node *sexpr(Parser *p);
static Node *sif(Parser *p);
static Node *sfor(Parser *p);
static Node *sreturn(Parser *p);

Ast parse(const char *source)
{
        Parser p = {};
        init_parser(&p, source);

        if (consume(&p, TK_EOF)) {
                error("Empty file provided. Try looking into our TODO: docs lol");
        }
        Node *st = sstmt(&p);

        expect(&p, TK_EOF, "eof");
        return st;
}

static Node *sstmt(Parser *p)
{
        switch (p->ctok.kind) {
        case KW_IF:
                return sif(p);
        case KW_RETURN:
                return sreturn(p);
        case KW_FOR:
                return sfor(p);

        case TK_OCURLY:
                return sblock(p);
        default:
        }

        return sexpr(p);
}

static Node *sblock(Parser *p)
{
        expect(p, TK_OCURLY, "{");
        const char *start = p->ptok.str.data;

        Node **list = NULL;

        while (!check(p, TK_CCURLY) && !check(p, TK_EOF)) {
                Node *st = sstmt(p);
                arrpush(list, st);
        }
        if (!consume(p, TK_CCURLY)) {
                unclosed(p, start, "{");
        }
        Node *block = new_stblock(list);

        return block;
}

static Node *sexpr(Parser *p)
{
        Node *e = expr(p);
        Node *se = new_stexpr(e);
        expect(p, TK_SEMI, ";");

        return se;
}

static Node *sfor(Parser *p)
{
        expect(p, KW_FOR, "for");

        Node *cond_or_init = NULL;
        if (check(p, TK_SEMI)) {
                cond_or_init = new_lit(LK_INT, SV("0"));
                goto PARSE_FOR;
        } else if (!check(p, TK_OCURLY)) {
                cond_or_init = expr(p);
        } else {
                cond_or_init = new_lit(LK_INT, SV("1"));
        }

        // this is a for, not a while
PARSE_FOR:
        if (consume(p, TK_SEMI)) {
                Node *init = cond_or_init;
                Node *cond = expr(p);
                expect(p, TK_SEMI, ";");

                Node *post = NULL;
                if (!check(p, TK_OCURLY)) {
                        post = expr(p);
                } else {
                        post = new_lit(LK_INT, SV("0"));
                }

                Node *block = sblock(p);
                return new_stfor(init, cond, post, block);
        }
        Node *block = sblock(p);

        return new_stfwhile(cond_or_init, block);
}

static Node *sif(Parser *p)
{
        expect(p, KW_IF, "if");

        Node *cond = expr(p);
        Node *block = sblock(p);
        Node *ifnot = NULL;
        if (consume(p, KW_ELSE)) {
                ifnot = sblock(p);
        }

        return new_stif(cond, block, ifnot);
}

static Node *sreturn(Parser *p)
{
        expect(p, KW_RETURN, "return");
        Node *e = expr(p);
        expect(p, TK_SEMI, ";");

        return new_streturn(e);
}

static Node *expr(Parser *p)
{
        return eass(p);
}

static Node *eass(Parser *p)
{
        Node *left = eadd(p);
        const char *possible_ident_pos = p->ptok.str.data;
        if (consume(p, TK_ASS)) {
                if (left->kind != NKEx_LIT || left->as.lit.kind != LK_ID) {
                        error_at(p->src, possible_ident_pos,
                                 "expected a variable name");
                }
                left = new_binop(BINOP_ASS, left, expr(p));
        }

        return left;
}

static Node *eadd(Parser *p)
{
        Node *left = emul(p);

        for (;;) {
                if (consume(p, TK_ADD)) {
                        left = new_binop(BINOP_ADD, left, emul(p));
                        continue;
                }
                if (consume(p, TK_SUB)) {
                        left = new_binop(BINOP_SUB, left, emul(p));
                        continue;
                }

                return left;
        }
}

static Node *emul(Parser *p)
{
        Node *left = eunary(p);

        for (;;) {
                if (consume(p, TK_MUL)) {
                        left = new_binop(BINOP_MUL, left, eunary(p));
                        continue;
                }
                if (consume(p, TK_DIV)) {
                        left = new_binop(BINOP_DIV, left, eunary(p));
                        continue;
                }

                return left;
        }
}

static Node *eunary(Parser *p)
{
        if (consume(p, TK_ADD)) {
                return eunary(p);
        }
        if (consume(p, TK_SUB)) {
                return new_unary(UNAOP_NEG, eunary(p));
        }

        Node *result = eprimary(p);

        for (;;) {
                if (consume(p, TK_MUL)) {
                        result = new_unary(UNAOP_STAR, result);
                        continue;
                }
                if (consume(p, TK_AMP)) {
                        result = new_unary(UNAOP_ADDR, result);
                        continue;
                }

                break;
        }

        return result;
}

static Node *eprimary(Parser *p)
{
        if (consume(p, TK_OPAREN)) {
                const char *start = p->ptok.str.data;
                Node *node = expr(p);
                if (!consume(p, TK_CPAREN)) {
                        unclosed(p, start, "(");
                }
                return node;
        }

        if (check(p, TK_ID)) {
                Node *node = new_lit(LK_ID, p->ctok.str);
                advance(p);
                return node;
        }

        if (check(p, TK_INT)) {
                Node *node = new_lit(LK_INT, p->ctok.str);
                advance(p);
                return node;
        }

        error_at(p->src, p->ctok.str.data, "expected an expression");
}
