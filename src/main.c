#include <stdio.h>

#include "aleron.h"
#include "crc.h"
#include <string.h>

enum { STAGE_AST, STAGE_IR };

int main(int argc, char *argv[])
{
        if (argc <= 1 || argc >= 4) {
                printf("Usage: %s [-e={ast,ir}] <input>\n", argv[0]);
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
        } else if (strcmp(argv[1], "-e=ir") == 0) {
                stage = STAGE_IR;
        } else {
                printf("Unknown stage (choose from 'ast','ir'): %s", argv[1]);
                return 1;
        }

COMPILE:
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
