#ifndef LIB_LOG_H_
#define LIB_LOG_H_

#if ENABLE_DISPLAY
#define log(args...) ((void)0)
#else
#define log(args...) __log_func(__FILE__, __LINE__, __FUNCTION__, args)
#endif

void __log_func(const char* file, int line, const char* func, const char* fmt,
                ...);

#endif  // LIB_LOG_H_