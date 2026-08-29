#include "aleron.h"

#include <ctype.h>
#include <string.h>

#include "crc.h"
#include "util.h"

// here for tracing
inline char *scanner_next_char(char **src)
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
                while (isdigit(**src)) {
                        scanner_next_char(src);
                }
                auto result = (ScanResult){
                        .token = { TK_INT, { *src - start, start } }
                };
                return result;
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

        case '(':
                singleCharCase(TK_OPAREN);
        case ')':
                singleCharCase(TK_CPAREN);

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
