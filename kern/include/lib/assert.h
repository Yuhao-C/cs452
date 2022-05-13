#ifndef KERN_LIB_ASSERT_H_
#define KERN_LIB_ASSERT_H_

#include "arch/ts7200.h"
#include "lib/bwio.h"

#ifdef NDEBUG /* required by ANSI standard */
#define assert(cond) ((void)0)
#else
#define assert(cond) \
  ((cond) ? (void)0  \
          : __assert_func(__FILE__, __LINE__, __PRETTY_FUNCTION__, #cond))
#endif

void __assert_func(const char* file, int line, const char* func,
                   const char* cond);

#endif  // KERN_LIB_ASSERT_H_