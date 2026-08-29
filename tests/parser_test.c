#include "aleron.h"
#include "util.h"
#include "vendor/utest.h"

#include "crc.h"
#include "vendor/stb_ds.h"
#include <asm-generic/errno-base.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// ain't really testing it, here just for the funzies
UTEST(parser, ast_dump)
{
        Ast rca(ast) = new_binop(BINOP_ADD,
                                 new_binop(BINOP_SUB, new_lit(LK_INT, SV("2")),
                                           new_lit(LK_INT, SV("1"))),
                                 new_lit(LK_INT, SV("5")));

        FILE *file = tmpfile();
        ast_dump(ast, file);

        rewind(file);
        char buf[512];
        while (fgets(buf, sizeof(buf), file)) {
                fputs(buf, stdout);
        }
        fclose(file);
}

UTEST(parser, ast_from_str)
{
        Ast rca(ast) = parse("-1 * 2 + -+-+-(3 + 1)");
        ast_dump(ast, stdout);
}
