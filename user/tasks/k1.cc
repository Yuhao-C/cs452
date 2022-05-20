#include "k1.h"

#include "lib/bwio.h"
#include "user/task.h"

void k1() {
  bwprintf(COM2, "Task id: %d, Parent task id: %d\n\r", myTid(), myParentTid());
  yield();
  bwprintf(COM2, "Task id: %d, Parent task id: %d\n\r", myTid(), myParentTid());
  exit();
}