#include "lib/math.h"

unsigned int log2(unsigned int x) {
  return sizeof(unsigned int) * 8 - __builtin_clz(x) - 1;
}