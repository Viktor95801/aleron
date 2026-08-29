#ifndef ALERON_H
#define ALERON_H

#include <stdarg.h>
#include <stdio.h>

#include "vendor/sv.h"

#pragma region error

__attribute__((noreturn)) void error(char *fmt, ...);
__attribute__((noreturn)) void verror_at(const char *source, const char *loc,
                                         char *fmt, va_list ap);
__attribute__((noreturn)) void error_at(const char *source, const char *loc,
                                        char *fmt, ...);

#pragma endregion

#pragma region scanner

typedef enum {
        TK_INVALID = -1,
        TK_EOF,

        TK_INT,

        TK_ADD,
        TK_SUB,
        TK_MUL,
        TK_DIV,

        TK_OPAREN,
        TK_CPAREN,
} TokenKind;

typedef struct {
        TokenKind kind;
        String_View str;
} Token;

typedef struct {
        Token token;
        char *error;
} ScanResult;

ScanResult next_token(char **src);

#pragma endregion

#pragma region ast

typedef enum {
        NK_BAD,
        NK_LIT,

        NK_BINOP,
        NK_end
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
        BINOP_end,
} BinopKind;
typedef struct NodeBinop {
        BinopKind kind;
        Node *left, *right;
} NodeBinop;

struct Node {
        NodeKind kind;
        union {
                NodeLit lit;
                NodeBinop binop;
        } as;
};

typedef Node *Ast;

void destroy_node(void *ptr);
Node *new_node(NodeKind kind);
Node *new_lit(LiteralKind kind, String_View str);
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
