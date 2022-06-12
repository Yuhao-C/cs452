#include "k1.h"

#include "lib/io.h"
#include "user/task.h"

void k1() {
  println(COM2, "Task id: %d, Parent task id: %d", myTid(), myParentTid());
  yield();
  println(COM2, "Task id: %d, Parent task id: %d", myTid(), myParentTid());
  exit();
}