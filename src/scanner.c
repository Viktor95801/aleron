#include "aleron.h"

#include <ctype.h>
#include <string.h>

#include "crc.h"
#include "util.h"

#define _isalpha(c) (isalpha(c) || (c) == '_')
#define _isalnum(c) (isalnum(c) || (c) == '_')

const char *token_to_str(Token *token)
{
        switch (token->kind) {
        case TK_INVALID:
                return format("invalid:%.*s", Mtokstr_fmt(*token));
        case TK_EOF:
                return "eof";
        case TK_ID:
                return format("ident:%.*s", Mtokstr_fmt(*token));
        case TK_INT:
                return format("int:%.*s", Mtokstr_fmt(*token));

        case TK_ADD:
                return "+";
        case TK_SUB:
                return "-";
        case TK_MUL:
                return "*";
        case TK_DIV:
                return "/";
        case TK_ASS:
                return "=";

        case TK_OPAREN:
                return "(";
        case TK_CPAREN:
                return ")";
        case TK_SEMI:
                return ";";
        }
}

// here for tracing
char *scanner_next_char(char **src)
{
        // printf("*src = %s\n", *src);
        return (*src)++;
}

ScanResult next_token(char **src)
{
        while (isspace(**src)) {
                scanner_next_char(src);
        }

        if (isdigit(**src)) {
                char *start = *src;
                bool invalid_number = false;
                while (_isalnum(**src)) {
                        if (_isalpha(**src)) {
                                invalid_number = true;
                        }
                        scanner_next_char(src);
                }
                auto result = (ScanResult){
                        .token = { TK_INT, { *src - start, start } }
                };
                if (invalid_number) {
                        result.token.kind = TK_INVALID;
                        result.error = format("number contains letters: " SV_Fmt
                                              "\n",
                                              Mtokstr_fmt(result.token));
                }
                return result;
        }

        if (_isalpha(**src)) {
                char *start = *src;
                while (_isalnum(**src)) {
                        scanner_next_char(src);
                }
                return (ScanResult){ .token = { TK_ID,
                                                { *src - start, start } } };
        }

#define singleCharCase(kind)                                     \
                                                                 \
        return (ScanResult)                                      \
        {                                                        \
                .token = { kind, { 1, scanner_next_char(src) } } \
        }

        switch (**src) {
        case '+':
                singleCharCase(TK_ADD);
        case '-':
                singleCharCase(TK_SUB);
        case '*':
                singleCharCase(TK_MUL);
        case '/':
                singleCharCase(TK_DIV);
        case '=':
                singleCharCase(TK_ASS);

        case '(':
                singleCharCase(TK_OPAREN);
        case ')':
                singleCharCase(TK_CPAREN);

        case ';':
                singleCharCase(TK_SEMI);

        // we want it to stay here forever
        case '\0': {
                return (ScanResult){ .token = { TK_EOF, { 1, *src } } };
        }
        default: {
                auto result = (ScanResult){
                        .token = { TK_INVALID, { 1, *src } },
                        .error = strdup(format("stray '%c' in source\n", **src))
                };
                scanner_next_char(src);
                return result;
        }
        }
#undef singleCharCase
}
