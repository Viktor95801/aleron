#!/bin/bash
@LSAN_OPTIONS=suppressions=suppressions.txt ./test_runner "$@"
