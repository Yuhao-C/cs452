#ifndef KERN_LIB_ASSERT_H_
#define KERN_LIB_ASSERT_H_

#ifdef NDEBUG /* required by ANSI standard */
#define kAssert(cond) ((void)0)
#define assert(cond) ((void)0)
#else
#define kAssert(cond) \
  ((cond) ? (void)0   \
          : __k_assert_func(__FILE__, __LINE__, __PRETTY_FUNCTION__, #cond))
#define assert(cond) \
  ((cond) ? (void)0  \
          : __assert_func(__FILE__, __LINE__, __PRETTY_FUNCTION__, #cond))
#endif

void __k_assert_func(const char* file, int line, const char* func,
                     const char* cond);

void __assert_func(const char* file, int line, const char* func,
                   const char* cond);

#endif  // KERN_LIB_ASSERT_H_