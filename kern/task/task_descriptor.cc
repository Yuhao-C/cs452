#include "kern/task.h"

TaskDescriptor::TaskDescriptor(TaskDescriptor* parent, int priority, int tid)
    : tid{tid},
      parent{parent},
      priority{priority},
      nextReady{nullptr},
      nextSend{nullptr},
      state{State::kReady},
      retVal{0} {}

TaskDescriptor::TaskDescriptor() : TaskDescriptor{nullptr, -1, -1} {}
