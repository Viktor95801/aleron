#include <stdio.h>

#include "aleron.h"
#include "crc.h"
#include <string.h>

enum { STAGE_AST, STAGE_IR, STAGE_LEX };

int main(int argc, char *argv[])
{
        if (argc <= 1 || argc >= 4) {
                printf("Usage: %s [-e={ast,ir,lex}] <input>\n", argv[0]);
                exit(0);
        }

        int file_id = 1;
        int stage = STAGE_IR;
        if (argc == 2) {
                goto COMPILE;
        }

        file_id = 2;
        if (strcmp(argv[1], "-e=ast") == 0) {
                stage = STAGE_AST;
        } else if (strcmp(argv[1], "-e=lex") == 0) {
                stage = STAGE_LEX;
        } else if (strcmp(argv[1], "-e=ir") == 0) {
                stage = STAGE_IR;
        } else {
                printf("Unknown stage (choose from -e='ast','ir','lex'): %s",
                       argv[1]);
                return 1;
        }

COMPILE:
        if (stage == STAGE_LEX) {
                char *src = argv[file_id];
                int exit_code = 0;
                for (ScanResult scan = next_token(&src);
                     scan.token.kind != TK_EOF; scan = next_token(&src)) {
                        printf("%s\n", token_to_str(&scan.token));
                        if (scan.error) {
                                exit_code = 1;
                                printf("ERROR: %s", scan.error);
                        }
                }

                return exit_code;
        }
        Ast rca(ast) = parse(argv[file_id]);
        if (stage == STAGE_AST) {
                ast_dump(ast, stdout);
                return 0;
        }
        char *ssa = codegen(ast);
        puts(ssa);
        codegen_destroy(ssa);

        return 0;
}
