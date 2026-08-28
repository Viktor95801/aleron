#include "util.h"

// from raylib
const char *format(const char *text, ...)
{
        static char buffers[4][1024] = { 0 };
        static int index = 0;

        char *currentBuffer = buffers[index];
        memset(currentBuffer, 0, 1024);
        if (text == NULL) {
                return currentBuffer;
        }

        va_list args;
        va_start(args, text);
        int requiredByteCount = vsnprintf(currentBuffer, 1024, text, args);
        va_end(args);

        if (requiredByteCount >= 1024) {
                char *truncBuffer = buffers[index] + 1024 - 4;
                snprintf(truncBuffer, 4, "...");
        }

        index += 1;
        if (index >= 4) {
                index = 0;
        }

        return currentBuffer;
}
