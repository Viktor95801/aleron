#!/bin/bash

assert_cleanup() {
  rm -f ./tmp
  rm -f ./tmp.s
  rm -f ./tmp.ssa
}
assert_print() {
        if [ "$actual" = "$expected" ]; then
          echo "$input => $actual"
        else
          echo "$input => $expected expected, but got $actual"
          exit 1
        fi
}
assert() {
  expected="$1"
  input="$2"

  ./aleron "$input" > tmp.ssa || exit
  qbe -o tmp.s tmp.ssa || exit
  clang -static -o tmp tmp.s || exit
  ./tmp
  actual="$?"

        assert_print
}

assert 0 0
assert 42 42
assert 21 '5+20-4'
assert 1 '22-26+5'
assert 41 '  12 +   34 - 5 '
assert 3 '2 * 3 / 2'

echo OK
assert_cleanup
