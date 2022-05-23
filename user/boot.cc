#include "boot.h"

#include "k1.h"
#include "lib/bwio.h"
#include "user/task.h"

void boot() {
  int priority[] = {0, 0, 2, 2};
  for (int p : priority) {
    int tid = create(p, k1);
    bwprintf(COM2, "Created: %d\n\r", tid);
  }
  bwputstr(COM2, "FirstUserTask: exiting\n\r");
  exit();
}