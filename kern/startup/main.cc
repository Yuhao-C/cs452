#include "arch/ts7200.h"
#include "common.h"
#include "lib/assert.h"
#include "lib/bwio.h"
#include "lib/sys.h"

int main() {
  // store main's return address for exiting the kernel
  register addr_t lr asm("r14");
  exitAddr = lr;

  assert(1 == 2);
  bwputstr(COM2, "Hello kernel!\r\n");
}
