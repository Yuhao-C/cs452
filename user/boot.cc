#include "boot.h"

#include "k1.h"
#include "lib/bwio.h"
#include "user/task.h"

void boot() {
  // for (int i = 0; i < 8; i += 2) {
  //   int tid = create(i, k1);
  //   bwprintf(COM2, "Created: %d\n\r", tid);
  // }
  bwputstr(COM2, "FirstUserTask: exiting\n\r");
  exit();
}