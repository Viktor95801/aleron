#include "aleron.h"
#include "util.h"
#include "vendor/utest.h"

#include "scanner.c"

struct {
        char *name, *it;
        TokenKind wants[32];
} lexer_test_cases[] = {
        { "Empty", "", { TK_EOF } },
        { "Invalid", "$", { TK_INVALID } },
        { "Operators", "+ -/*", { TK_ADD, TK_SUB, TK_DIV, TK_MUL, TK_EOF } },
        { "Integer",
          "  32 - 12 + 4  ",
          { TK_INT, TK_SUB, TK_INT, TK_ADD, TK_INT, TK_EOF } },
        { "Grouping",
          " (21-2)",
          { TK_OPAREN, TK_INT, TK_SUB, TK_INT, TK_CPAREN, TK_EOF } },
        { "Return", "return;", { KW_RETURN, TK_SEMI } }
};
const i32 lexer_test_amount =
        sizeof(lexer_test_cases) / sizeof(*lexer_test_cases);

UTEST(lexer, tests)
{
        for (i32 i = 0; i < lexer_test_amount; ++i) {
                auto test_case = lexer_test_cases[i];
                char *src = test_case.it;
                ScanResult scan = next_token(&src);
                for (i32 j = 0; scan.token.kind != TK_EOF;
                     scan = next_token(&src), ++j) {
                        if (scan.token.kind == TK_INVALID) {
                                free(scan.error);
                        }
                        ASSERT_EQ(scan.token.kind, test_case.wants[j]);
                }
        }
}
