#include "aleron.h"

#include <stdio.h>
#include <stdlib.h>

// TODO: fileset so we can print the line and col as well
void verror_at(const char *source, i32 loc, char *fmt, va_list ap)
{
        fprintf(stderr, "%s\n", source);
        fprintf(stderr, "%*s", loc, ""); // print pos spaces.
        fprintf(stderr, "^ ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        exit(1);
}

void error_at(const char *source, i32 loc, char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        verror_at(source, loc, fmt, ap);
}
