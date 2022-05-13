#include "task.h"

void stackSwitch(TaskDescriptor &curr, TaskDescriptor &next) {
  asm(R"(
        sub sp, sp, #36
        str r4, [sp, #0]
        str r5, [sp, #4]
        str r6, [sp, #8]
        str r7, [sp, #12]
        str r8, [sp, #16]
        str r9, [sp, #20]
        str r10, [sp, #24]
        str r11, [sp, #28]
        str lr, [sp, #32]
    )");
}
