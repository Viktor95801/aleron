#!/bin/bash

assert_cleanup() {
  rm -f ./tmp
  rm -f ./tmp.s
  rm -f ./tmp.ssa
}
assert() {
  expected="$1"
  input="$2"

  ./aleron "$input" > tmp.ssa || (assert_cleanup && exit)
  qbe -o tmp.s tmp.ssa || (assert_cleanup && exit)
  gcc -static -o tmp tmp.s || (assert_cleanup && exit)
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    assert_cleanup
    exit 1
  fi
}

assert 0 0
assert 42 42

echo OK
assert_cleanup
