#!/usr/bin/env bash

find src lib \( -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' \) ! -path './src/display/ui/**/*' ! -path './src/display/drivers/**/*' | xargs clang-format -i
