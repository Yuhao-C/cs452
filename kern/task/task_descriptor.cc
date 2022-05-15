#include "task_private.h"

TaskDescriptor::TaskDescriptor(TaskDescriptor* parent, int priority)
    : parent{parent},
      priority{priority},
      nextReady{nullptr},
      nextSend{nullptr},
      state{State::kReady},
      stackPtr{0},
      retVal{0},
      spsr{0} {
  tid = tidCounter++;
}

TaskDescriptor::TaskDescriptor() : TaskDescriptor{nullptr, -1} {}
