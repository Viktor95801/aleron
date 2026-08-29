#include "aleron.h"

#include <stdio.h>
#include <stdlib.h>

void error(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        exit(1);
}

// TODO: fileset so we can print the line and col as well
void verror_at(const char *source, const char *loc, char *fmt, va_list ap)
{
        fprintf(stderr, "%s\n", source);
        fprintf(stderr, "%*s", (int)(loc - source), ""); // print pos spaces.
        fprintf(stderr, "^ ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        exit(1);
}

void error_at(const char *source, const char *loc, char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        verror_at(source, loc, fmt, ap);
}
