#include "arch/ts7200.h"
#include "lib/assert.h"
#include "lib/bwio.h"

int main() {
  assert(1 == 2);
  bwputstr(COM2, "Hello kernel!\r\n");
}
