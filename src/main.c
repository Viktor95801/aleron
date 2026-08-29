#include <stdio.h>

#include "aleron.h"
#include "crc.h"

int main(int argc, char *argv[])
{
        if (argc != 2) {
                printf("Usage: %s <input>\n", argv[0]);
                exit(0);
        }

        Ast rca(ast) = parse(argv[1]);
        char *ssa = codegen(ast);
        puts(ssa);
        codegen_destroy(ssa);

        return 0;
}
