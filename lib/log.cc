#include "lib/log.h"

#include "lib/bwio.h"
#include "lib/io.h"

void __log_func(const char* file, int line, const char* func, const char* fmt,
                ...) {
  printf(COM2, "[%s:%d] ", func, line);
  va_list va;

  va_start(va, fmt);
  format(COM2, fmt, va);
  va_end(va);
  putstr(COM2, "\n\r");
}
