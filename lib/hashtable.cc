#include "lib/hashtable.h"

#include "lib/assert.h"
#include "lib/string.h"

int hash(const String &s) {
  int h = 0;
  const char *c = s.cStr();
  assert(c != nullptr);
  while (*c) {
    h *= 31;
    h += *c;
    ++c;
  }
  return h;
}
