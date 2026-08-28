#include "aleron.h"
#include "util.h"
#include "vendor/utest.h"

#include "crc.h"
#include "vendor/stb_ds.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// ain't really testing it, here just for the funzies
UTEST(parser, ast_dump)
{
        Ast rca(ast) = new_binop(BINOP_ADD,
                                 new_binop(BINOP_SUB, new_ival(7), new_ival(2)),
                                 new_ival(5));

        FILE *file = tmpfile();
        ast_dump(ast, file);

        rewind(file);
        char buf[512];
        while (fgets(buf, sizeof(buf), file)) {
                fputs(buf, stdout);
        }
        fclose(file);
}

typedef struct {
        const char *src;
        char *pos;
        ScanResult pscan, cscan, nscan;
} Parser;
void init_parser(Parser *p, const char *source)
{
        p->src = source;
        p->pos = (char *)source;
}
UTEST(parser, tests)
{
}
