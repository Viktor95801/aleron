#include "util.h"

#include <stdio.h>
#include <string.h>

// from raylib
const char *format(const char *text, ...)
{
        static char buffers[4][4096] = { 0 };
        static int index = 0;

        char *currentBuffer = buffers[index];
        memset(currentBuffer, 0, 4096);
        if (text == NULL) {
                return currentBuffer;
        }

        va_list args;
        va_start(args, text);
        int requiredByteCount = vsnprintf(currentBuffer, 4096, text, args);
        va_end(args);

        if (requiredByteCount >= 4096) {
                char *truncBuffer = buffers[index] + 4096 - 4;
                snprintf(truncBuffer, 4, "...");
        }

        index += 1;
        if (index >= 4) {
                index = 0;
        }

        return currentBuffer;
}
