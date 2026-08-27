#include <stdio.h>
#include <stdlib.h>

#define CRC_IMPLEMENTATION
#define CRC_THREAD_SAFE
#include "crc.h"

int main(int argc, char *argv[])
{
        if (argc != 2) {
                printf("Usage: %s <input>\n", argv[0]);
                exit(0);
        }
        printf("export function w $main() {\n"
               "  @start\n"
               "  ret %s\n"
               "}\n",
               argv[1]);
}
