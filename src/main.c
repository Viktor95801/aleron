#define CRC_IMPLEMENTATION
#define CRC_THREAD_SAFE
#include "crc.h"

#include "util.h"

typedef enum {
        TK_INVALID = -1,
        TK_EOF,

        TK_INT,

        TK_ADD,
        TK_SUB,
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

        switch (**src) {
        case '+': {
                return (ScanResult){ .token = { TK_ADD },
                                     .len = 1,
                                     .pos = scanner_next_char(src) - original };
        }
        case '-': {
                return (ScanResult){ .token = { TK_SUB },
                                     .len = 1,
                                     .pos = scanner_next_char(src) - original };
        }
        // sub here cuz we want it to stay here forever
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
                        .error = strdup(format("stray '%c' in source", **src))
                };
                scanner_next_char(src);
                return result;
        }
        }
}

int main(int argc, char *argv[])
{
        if (argc != 2) {
                printf("Usage: %s <input>\n", argv[0]);
                exit(0);
        }

        char *src = argv[1];
        const char *original = src;
        int x_index = 0;
        printf("export function w $main() {\n"
               "@start\n"
               "  %%x%d =w copy %li\n",
               x_index, strtol(src, &src, 10)); // consumes first token

        for (ScanResult scan = next_token(original, &src);
             scan.token.kind != TK_EOF; scan = next_token(original, &src)) {
                switch (scan.token.kind) {
                case TK_INVALID: {
                        puts(scan.error);
                        free(scan.error);
                        return 1;
                }

                case TK_ADD: {
                        scan = next_token(original, &src);
                        if (scan.token.kind != TK_INT) {
                                printf("expected int, got '%.*s'\n", scan.len,
                                       original + scan.pos);
                                if (scan.error) {
                                        puts(scan.error);
                                        free(scan.error);
                                }
                                return 1;
                        }
                        printf("  %%x%d =w add %%x%d, %li\n", x_index + 1,
                               x_index, atol(original + scan.pos));
                        x_index++;
                } break;
                case TK_SUB: {
                        scan = next_token(original, &src);
                        if (scan.token.kind != TK_INT) {
                                printf("expected int, got '%.*s'\n", scan.len,
                                       original + scan.pos);
                                return 1;
                        }
                        printf("  %%x%d =w sub %%x%d, %li\n", x_index + 1,
                               x_index, atol(original + scan.pos));
                        x_index++;
                } break;

                default:
                        unreachable();
                }
        }
        printf("  ret %%x%d\n"
               "}\n",
               x_index);

        return 0;
}
