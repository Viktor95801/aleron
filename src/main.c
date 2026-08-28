#include <stdio.h>

#include "aleron.h"
#include "crc.h"

int main(int argc, char *argv[])
{
        if (argc != 2) {
                printf("Usage: %s <input>\n", argv[0]);
                exit(0);
        }

        char *src = argv[1];
        const char *original = src;
        for (ScanResult scan = next_token(&src); scan.token.kind != TK_EOF;
             scan = next_token(&src)) {
                printf("%d " SV_Fmt "\n", scan.token.kind,
                       (int)scan.token.str.count, scan.token.str.data);
                scan = next_token(&src);
        }

        src = argv[1];
        original = src;
        int x_index = 0;
        printf("export function w $main() {\n"
               "@start\n"
               "  %%x%d =w copy %li\n",
               x_index, strtol(src, &src, 10)); // consumes first token

        for (ScanResult scan = next_token(&src); scan.token.kind != TK_EOF;
             scan = next_token(&src)) {
                const Token token = scan.token;
                switch (scan.token.kind) {
                case TK_INVALID: {
                        error_at(original, token.str.data, scan.error);
                }

                case TK_ADD: {
                        scan = next_token(&src);
                        if (scan.token.kind == TK_INT) {
                                printf("  %%x%d =w add %%x%d, " SV_Fmt "\n",
                                       x_index + 1, x_index,
                                       (int)token.str.count, token.str.data);
                                x_index++;
                                continue;
                        }
                        error_at(original, token.str.data,
                                 "expected int, got '%.*s'\n", token.str.count,
                                 token.str.data);
                } break;
                case TK_SUB: {
                        scan = next_token(&src);
                        if (scan.token.kind == TK_INT) {
                                printf("  %%x%d =w sub %%x%d, " SV_Fmt "\n",
                                       x_index + 1, x_index,
                                       (int)token.str.count, token.str.data);
                                x_index++;
                                continue;
                        }
                        error_at(original, token.str.data,
                                 "expected int, got '%.*s'\n", token.str.count,
                                 token.str.data);
                } break;
                case TK_MUL: {
                        scan = next_token(&src);
                        if (scan.token.kind == TK_INT) {
                                printf("  %%x%d =w mul %%x%d, " SV_Fmt "\n",
                                       x_index + 1, x_index,
                                       (int)token.str.count, token.str.data);
                                x_index++;
                                continue;
                        }
                        error_at(original, token.str.data,
                                 "expected int, got '%.*s'\n", token.str.count,
                                 token.str.data);
                } break;
                case TK_DIV: {
                        scan = next_token(&src);
                        if (scan.token.kind == TK_INT) {
                                printf("  %%x%d =w div %%x%d, " SV_Fmt "\n",
                                       x_index + 1, x_index,
                                       (int)token.str.count, token.str.data);
                                x_index++;
                                continue;
                        }
                        error_at(original, token.str.data,
                                 "expected int, got '%.*s'\n", token.str.count,
                                 token.str.data);
                } break;
                }
        }
        printf("  ret %%x%d\n"
               "}\n",
               x_index);

        return 0;
}
