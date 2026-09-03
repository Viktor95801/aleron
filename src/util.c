#include "util.h"

#include <__stdarg_va_list.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

const char *formatv(const char *fmt, va_list arg)
{
        static char buffers[16][2048] = { 0 };
        static int index = 0;

        char *currentBuffer = buffers[index];
        memset(currentBuffer, 0, 2048);
        if (fmt == NULL) {
                return currentBuffer;
        }

        int requiredByteCount = vsnprintf(currentBuffer, 2048, fmt, arg);

        if (requiredByteCount >= 2048) {
                char *truncBuffer = buffers[index] + 2048 - 4;
                snprintf(truncBuffer, 4, "...");
        }

        index += 1;
        if (index >= 8) {
                index = 0;
        }

        return currentBuffer;
}

// from raylib
const char *format(const char *fmt, ...)
{
        va_list arg;

        va_start(arg);
        const char *result = formatv(fmt, arg);
        va_end(arg);

        return result;
}
