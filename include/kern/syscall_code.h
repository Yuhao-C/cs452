#ifndef KERN_SYSCALL_CODE_H_
#define KERN_SYSCALL_CODE_H_

#define IRQ_TC3UI 51
#define IRQ_UART1 52
#define IRQ_UART2 54

#define SYS_CREATE 64
#define SYS_TID 65
#define SYS_PARENT_TID 66
#define SYS_YIELD 67
#define SYS_EXIT 68
#define SYS_SEND 69
#define SYS_RECEIVE 70
#define SYS_REPLY 71
#define SYS_AWAIT_EVENT 72
#define SYS_SHUTDOWN 73

#define SYS_IDLE_TIME 128

#define SYSCALL_FUNC(name, code) \
  .text;                         \
  .align 2;                      \
  .global name;                  \
  .type name, % function;        \
  name:;                         \
  swi code;                      \
  bx lr;

#endif  // KERN_SYSCALL_CODE_H_