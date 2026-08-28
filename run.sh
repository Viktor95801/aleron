#!/bin/sh
make -j16 && LSAN_OPTIONS=suppressions=suppressions.txt ./aleron "$@"
