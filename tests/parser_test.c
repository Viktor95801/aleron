#include "aleron.h"
#include "util.h"
#include "vendor/utest.h"

#include "crc.h"
#include "vendor/stb_ds.h"
#include <asm-generic/errno-base.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// ain't really testing it, here just for the funzies
UTEST(parser, ast_dump)
{
        Ast rca(ast) = new_binop(BINOP_ADD,
                                 new_binop(BINOP_SUB, new_lit(LK_INT, SV("2")),
                                           new_lit(LK_INT, SV("1"))),
                                 new_lit(LK_INT, SV("5")));

        FILE *file = tmpfile();
        ast_dump(ast, file);

        rewind(file);
        char buf[512];
        while (fgets(buf, sizeof(buf), file)) {
                fputs(buf, stdout);
        }
        fclose(file);
}

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

void init_parser(Parser *p, const char *source)
{
        p->src = source;
        p->pos = (char *)source;

        advance(p);
        advance(p);
}

static bool consume(Parser *p, TokenKind kind)
{
        if (p->ctok.kind == kind) {
                advance(p);
                return true;
        }
        return false;
}

static void expect(Parser *p, TokenKind kind, const char *what)
{
        if (!consume(p, kind)) {
                error_at(p->src, p->ctok.str.data, "expected %s", what);
        }
}

static Node *expr(Parser *p);
static Node *eadd(Parser *p);
static Node *emul(Parser *p);
static Node *eprimary(Parser *p);

static Node *parse_int(Parser *p);

Ast parse(const char *source)
{
        Parser p = {};
        init_parser(&p, source);

        Node *ast = expr(&p);

        return ast;
}

static Node *expr(Parser *p)
{
        return eadd(p);
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
        Node *left = eprimary(p);

        for (;;) {
                if (consume(p, TK_MUL)) {
                        left = new_binop(BINOP_MUL, left, eprimary(p));
                        continue;
                }
                if (consume(p, TK_DIV)) {
                        left = new_binop(BINOP_DIV, left, eprimary(p));
                        continue;
                }

                return left;
        }
}

static Node *eprimary(Parser *p)
{
        if (consume(p, TK_OPAREN)) {
                Node *node = expr(p);
                expect(p, TK_CPAREN, "')'");
                return node;
        }

        if (p->ctok.kind == TK_INT) {
                Node *node = new_lit(LK_INT, p->ctok.str);
                advance(p);
                return node;
        }

        error_at(p->src, p->ctok.str.data, "expected an expression");
}

UTEST(parser, ast_from_str)
{
        Ast rca(ast) = parse("1 * 2 + 3 + 1");
        ast_dump(ast, stdout);
}
