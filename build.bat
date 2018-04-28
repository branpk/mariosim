
gcc ^
  -DWIN32 ^
  -std=c99 ^
  -O3 ^
  -Wall ^
  -Wextra ^
  -Wno-missing-braces ^
  -Wno-unused-variable ^
  -Wno-unused-parameter ^
  source/*.c ^
  source/sm64/*.c ^
  -fwrapv ^
  -fno-strict-aliasing ^
  -Isource ^
  -o mariosim.exe
