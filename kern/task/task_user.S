#include "kern/syscall_code.h"

	.text
	.align 2
	.global create
	.type create, %function
create:
  swi   SYS_CREATE
  bx    lr


	.text
	.align 2
	.global myTid
	.type myTid, %function
myTid:
  swi   SYS_TID
  bx    lr


	.text
	.align 2
	.global myParentTid
	.type myParentTid, %function
myParentTid:
  swi   SYS_PARENT_TID
  bx    lr


	.text
	.align 2
	.global yield
	.type yield, %function
yield:
  swi   SYS_YIELD
  bx    lr


	.text
	.align 2
	.global exit
	.type exit, %function
exit:
  swi   SYS_EXIT
  bx    lr