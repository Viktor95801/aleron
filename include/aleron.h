#ifndef ALERON_H
#define ALERON_H

#include <stdarg.h>
#include <stdio.h>

#include "vendor/sv.h"

#pragma region error

__attribute__((noreturn)) void error(const char *fmt, ...);
__attribute__((noreturn)) void verror_at(const char *source, const char *loc,
                                         char *fmt, va_list ap);
__attribute__((noreturn)) void error_at(const char *source, const char *loc,
                                        char *fmt, ...);

#pragma endregion

#pragma region scanner

typedef enum {
        TK_INVALID = -1,
        TK_EOF,

        TK_ID,
        TK_INT,

        TK_ADD,
        TK_SUB,
        TK_MUL,
        TK_DIV,

        TK_OPAREN,
        TK_CPAREN,

        TK_SEMI,
} TokenKind;

typedef struct {
        TokenKind kind;
        String_View str;
} Token;

typedef struct {
        Token token;
        const char *error;
} ScanResult;

#define Mtokstr_fmt(tok) (int)(tok).str.count, (tok).str.data

// uses format() buffer, so not thread safe and all
const char *token_to_str(Token *token);
ScanResult next_token(char **src);

#pragma endregion

#pragma region ast

typedef enum {
        NK_BAD,

        NKEx_LIT,
        NKEx_BINOP,
        NKEx_UNAOP,

        NKSt_BLOCK,
        NKSt_EXPR,
} NodeKind;

typedef struct Node Node;
typedef enum {
        LK_INT,
} LiteralKind;
typedef struct NodeLit {
        LiteralKind kind;
        String_View str;
} NodeLit;

typedef enum {
        BINOP_ADD,
        BINOP_SUB,
        BINOP_MUL,
        BINOP_DIV,
} BinopKind;
typedef struct NodeBinop {
        BinopKind kind;
        Node *left, *right;
} NodeBinop;

typedef enum {
        UNAOP_NEG,
} UnaopKind;
typedef struct {
        UnaopKind kind;
        Node *expr;
} NodeUnaop;

typedef struct {
        Node **list; // stb_ds darray
} NodeStBlock;

typedef struct {
        Node *expr;
} NodeStExpr;

struct Node {
        NodeKind kind;
        union {
                NodeLit lit;
                NodeUnaop unaop;
                NodeBinop binop;

                NodeStBlock stblock;
                NodeStExpr stexpr;
        } as;
};

typedef Node *Ast;

void destroy_node(void *ptr);
Node *new_stblock(Node **list);
Node *new_stexpr(Node *expr);

Node *new_node(NodeKind kind);
Node *new_lit(LiteralKind kind, String_View str);
Node *new_unary(UnaopKind kind, Node *inside);
Node *new_binop(BinopKind kind, Node *left, Node *right);

const char *binopk_to_str(BinopKind kind);
void ast_dump(Ast ast, FILE *file);

#pragma endregion

#pragma region parser

Ast parse(const char *source);

#pragma endregion

#pragma region codegen

char *codegen(Ast ast);
void codegen_destroy(char *data);

#pragma endregion

#endif // ALERON_H
