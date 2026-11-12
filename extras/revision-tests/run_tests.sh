#!/bin/sh
set -eu
cd "$(dirname "$0")"
inc="../../src"
for cc in gcc clang; do
 for family in STM32F1xx STM32G0xx; do
  extra=""
  if [ "$family" = STM32G0xx ]; then extra="-DMOCK_HSE_DIV31"; fi
  echo "TEST $cc $family"
  "$cc" -std=c11 -Wall -Wextra -Werror -pedantic -I mock -I "$inc" -D"$family" $extra -fsanitize=undefined -fno-omit-frame-pointer ../../src/flprogRtcStm32.c test_rtc.c -o "/tmp/flprog_rtc_test_${cc}_${family}"
  "/tmp/flprog_rtc_test_${cc}_${family}"
 done
done
