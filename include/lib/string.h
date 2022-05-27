#ifndef LIB_STRING_H_
#define LIB_STRING_H_

#include "lib/assert.h"
#include "lib/bwio.h"

class String {
  const char *s;

 public:
  String();
  String(const char *s);
  const char *cStr() const;

  friend bool operator==(const String &s1, const String &s2);
};

void strCopy(const char *src, char *dst, int len);

#endif  // LIB_STRING_H_
