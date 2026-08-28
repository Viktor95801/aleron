#ifndef ALERON_H
#define ALERON_H

#include "util.h"

#pragma region error

void verror_at(const char *source, i32 loc, char *fmt, va_list ap);
void error_at(const char *source, i32 loc, char *fmt, ...);

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
} TokenKind;

typedef struct {
        TokenKind kind;
} Token;

typedef struct {
        Token token;
        i16 len;
        i32 pos;
        char *error;
} ScanResult;

ScanResult next_token(const char *original, char **src);

#pragma endregion

#endif // ALERON_H
