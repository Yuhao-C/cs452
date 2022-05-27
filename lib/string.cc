#include "lib/string.h"

String::String() : s{nullptr} {}
String::String(const char *s) : s{s} {}

const char *String::cStr() const { return s; }

bool operator==(const String &s1, const String &s2) {
  const char *a = s1.s;
  const char *b = s2.s;
  if (!a || !b) {
    return false;
  }
  while (*a && *b) {
    if (*a != *b) {
      return false;
    }
    ++a;
    ++b;
  }
  return !(*a) && !(*b);
}

void strCopy(const char *src, char *dst, int len) {
  int copiedLen = 0;
  assert(dst != nullptr);
  assert(src != nullptr);
  while (*src && copiedLen < len) {
    *dst = *src;
    ++dst;
    ++src;
    ++copiedLen;
  }
  if (copiedLen == len) {
    *(dst - 1) = 0;
  } else {
    *(dst) = 0;
  }
}
