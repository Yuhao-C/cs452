#include "lib/math.h"

unsigned int log2(unsigned int x) {
  return sizeof(unsigned int) * 8 - __builtin_clz(x) - 1;
}

int mod(int a, int b) {
  int m = a % b;
  if (m < 0) {
    m = (b < 0) ? m - b : m + b;
  }
  return m;
}
