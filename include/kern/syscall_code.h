#ifndef KERN_SYSCALL_CODE_H_
#define KERN_SYSCALL_CODE_H_

#define SYS_CREATE 0
#define SYS_TID 1
#define SYS_PARENT_TID 2
#define SYS_YIELD 3
#define SYS_EXIT 4
#define SYS_SEND 5
#define SYS_RECEIVE 6
#define SYS_REPLY 7

#define SYSCALL_FUNC(name, code) \
  .text;                         \
  .align 2;                      \
  .global name;                  \
  .type name, % function;        \
  name:;                         \
  swi code;                      \
  bx lr;

#endif  // KERN_SYSCALL_CODE_H_