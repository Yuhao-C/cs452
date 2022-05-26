#include "kern/syscall_code.h"

SYSCALL_FUNC(create, SYS_CREATE);

SYSCALL_FUNC(myTid, SYS_TID);

SYSCALL_FUNC(myParentTid, SYS_PARENT_TID);

SYSCALL_FUNC(yield, SYS_YIELD);

SYSCALL_FUNC(exit, SYS_EXIT);
