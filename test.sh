#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./tmcc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 0
assert 42 42
assert 255 255
assert 21 "5+20-4"
assert 116 "213-125+28"
assert 41 " 12 + 34 - 5 "
assert 28 " 59 - 21 - 10 "
assert 47 "5 + 6 * 7"
assert 15 " 5 * (9 - 6)"
assert 4 "(3 + 5) / 2"
assert 10 "-10+20"
assert 5 "-3*+5+20"
assert 1 "0 == 0"
assert 0 "23 == 79"
assert 0 "0 != 0"
assert 1 "23 != 79"
assert 1 "23 < 79"
assert 0 "51 < 13"
assert 1 "24 <= 24"
assert 0 "24 <= 23"
assert 0 "23 > 79"
assert 1 "51 > 13"
assert 1 "24 >= 24"
assert 0 "24 >= 25"

echo OK