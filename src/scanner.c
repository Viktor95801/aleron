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
                bool invalid_number = false;
                while (isalnum(**src)) {
                        if (isalpha(**src)) {
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
                                              (int)result.token.str.count,
                                              result.token.str.data);
                }
                return result;
        }

        if (isalpha)

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

                case ';':
                        singleCharCase(TK_SEMI);

                // we want it to stay here forever
                case '\0': {
                        return (ScanResult){ .token = { TK_EOF, { 1, *src } } };
                }
                default: {
                        auto result = (ScanResult){
                                .token = { TK_INVALID, { 1, *src } },
                                .error = strdup(
                                        format("stray '%c' in source\n", **src))
                        };
                        scanner_next_char(src);
                        return result;
                }
                }
#undef singleCharCase
}
