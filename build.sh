#!/usr/bin/env bash

gcc \
  -std=c99 \
  -O3 \
  -Wall \
  -Wextra \
  -Wno-missing-braces \
  -Wno-unused-variable \
  -Wno-unused-parameter \
  -fwrapv \
  -fno-strict-aliasing \
  $(find source -name '*.c') \
  -Isource \
  -o mariosim
