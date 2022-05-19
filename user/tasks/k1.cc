#include "k1.h"

#include "lib/bwio.h"
#include "user/task.h"

void k1() {
  bwprintf(COM2, "Task id: %u, Parent task id: %u\n\r", myTid(), myTid());
  yield();
  bwprintf(COM2, "Task id: %u, Parent task id: %u\n\r", myTid(), myTid());
  exit();
}