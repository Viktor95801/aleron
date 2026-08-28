#include <stdio.h>

#define CRC_IMPLEMENTATION
#define CRC_THREAD_SAFE
#include "crc.h"

#include "aleron.h"

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
                        error_at(original, scan.pos, scan.error);
                }

                case TK_ADD: {
                        scan = next_token(original, &src);
                        if (scan.token.kind == TK_INT) {
                                printf("  %%x%d =w add %%x%d, %li\n",
                                       x_index + 1, x_index,
                                       atol(original + scan.pos));
                                x_index++;
                                continue;
                        }
                        error_at(original, scan.pos,
                                 "expected int, got '%.*s'\n", scan.len,
                                 original + scan.pos);
                } break;
                case TK_SUB: {
                        scan = next_token(original, &src);
                        if (scan.token.kind == TK_INT) {
                                printf("  %%x%d =w sub %%x%d, %li\n",
                                       x_index + 1, x_index,
                                       atol(original + scan.pos));
                                x_index++;
                                continue;
                        }
                        error_at(original, scan.pos,
                                 "expected int, got '%.*s'\n", scan.len,
                                 original + scan.pos);
                } break;
                case TK_MUL: {
                        scan = next_token(original, &src);
                        if (scan.token.kind == TK_INT) {
                                printf("  %%x%d =w mul %%x%d, %li\n",
                                       x_index + 1, x_index,
                                       atol(original + scan.pos));
                                x_index++;
                                continue;
                        }
                        error_at(original, scan.pos,
                                 "expected int, got '%.*s'\n", scan.len,
                                 original + scan.pos);
                } break;
                case TK_DIV: {
                        scan = next_token(original, &src);
                        if (scan.token.kind == TK_INT) {
                                printf("  %%x%d =w div %%x%d, %li\n",
                                       x_index + 1, x_index,
                                       atol(original + scan.pos));
                                x_index++;
                                continue;
                        }
                        error_at(original, scan.pos,
                                 "expected int, got '%.*s'\n", scan.len,
                                 original + scan.pos);
                } break;
                }
        }
        printf("  ret %%x%d\n"
               "}\n",
               x_index);

        return 0;
}
