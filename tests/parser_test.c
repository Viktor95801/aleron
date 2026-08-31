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

// UTEST(ast, dump)
// {
//         Node **list = NULL;
//         arrpush(list, new_stexpr(new_binop(BINOP_ADD, new_lit(LK_INT,
//         SV("1")),
//                                            new_lit(LK_INT, SV("1")))));
//         Node *rca(node) = new_stblock(list);
//         ast_dump(node, stdout);
// }

UTEST(parser, ast_iter)
{
        Ast rca(ast) = parse("1 + 2; v= 4 + 1 - 2;");
        ast_dump(ast, stdout);
}
