#ifndef KERN_LIB_ASSERT_H_
#define KERN_LIB_ASSERT_H_

#include "arch/ts7200.h"
#include "lib/bwio.h"

#ifndef NDEBUG
#define assert(condition)                                                   \
  {                                                                         \
    if (!(condition)) {                                                     \
      bwprintf(COM2,                                                        \
               "\033[31mAssertion failed at %s:%d inside %s\n\rCondition: " \
               "%s\n\r\033[0m",                                             \
               __FILE__, __LINE__, __FUNCTION__, #condition);               \
    }                                                                       \
  }
#else
#define assert(condition) (condition)
#endif

#endif