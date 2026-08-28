#include "aleron.h"

#include <ctype.h>
#include <string.h>

#include "crc.h"

// here for tracing
inline char *scanner_next_char(char **src)
{
        // printf("*src = %s\n", *src);
        return (*src)++;
}

ScanResult next_token(const char *original, char **src)
{
        while (isspace(**src)) {
                scanner_next_char(src);
        }

        if (isdigit(**src)) {
                char *start = *src;
                while (isdigit(**src)) {
                        scanner_next_char(src);
                }
                auto result = (ScanResult){ .token = { TK_INT },
                                            .len = *src - start,
                                            .pos = start - original };
                return result;
        }

#define singleCharCase(kind)                             \
                                                         \
        return (ScanResult)                              \
        {                                                \
                .token = { kind }, .len = 1,             \
                .pos = scanner_next_char(src) - original \
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
                return (ScanResult){ .token = { TK_EOF },
                                     .len = 1,
                                     .pos = *src - original };
        }
        default: {
                auto result = (ScanResult){
                        .token = { TK_INVALID },
                        .len = 1,
                        .pos = *src - original,
                        .error = strdup(format("stray '%c' in source\n", **src))
                };
                scanner_next_char(src);
                return result;
        }
        }
#undef singleCharCase
}
