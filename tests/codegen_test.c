#include <assert.h>

#include "aleron.h"
#include "crc.h"
#include "util.h"
#include "vendor/stb_ds.h"
#include "vendor/utest.h"
#include <stdio.h>
#include <string.h>

/*
UTEST(codegen, string_builder)
{
        char *sb = (void *)0;
        builder_add(&sb, "Hello, World!");
        builder_null(&sb);
        ASSERT_STREQ(sb, "Hello, World!\n");
        builder_destroy(sb);
}
 */

UTEST(codegen, gen)
{
        Ast rca(ast) = parse("(1 + 2) * 3");
        ast_dump(ast, stdout);
        char *res = codegen(ast);
        puts(res);

        codegen_destroy(res);
}
