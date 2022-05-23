# CS452 Kernel

## Group Member

- Qishen Wu \<q246wu@uwaterloo.ca\>
- Yuhao Chen \<y627chen@uwaterloo.ca\>

## Setup

```bash
make
make install # transfer kmain.elf to tftp server
```

- Executable file: `kern/kmain.elf`

## File Structure

```sh
cs452/
├── include/
│   ├── kern/
│   │   ├── arch/
│   │   │   └── ts7200.h # ts7200 registers definition
│   │   ├── common.h  # declare common type and helper
│   │   ├── kmem.h    # declare constants for memory sections
│   │   ├── sys.h     # declare kExit (exit kernel)
│   │   ├── syscall_code.h # define syscall codes
│   │   ├── syscall.h # declare Trapframe and syscall functions
│   │   └── task.h    # declare kernel task handle
│   ├── lib/
│   │   ├── assert.h # declare assert utility
│   │   └── bwio.h   # declare busy-wait I/O routines
│   └── user/
│       └── task.h # declare user task interface
├── kern/
│   ├── lib/
│   │   └── sys.cc # define kExit (exit kernel)
│   ├── syscall/
│   │   ├── exception.S # save and restore user and kernel stack
│   │   └── syscall.cc  # define enterKernel and leaveKernel
│   ├── task/
│   │   ├── priority_queues.cc # define PriorityQueues class
│   │   ├── task_descriptor.cc # define TaskDescriptor class
│   │   ├── task_kern.cc       # define kernel task handler
│   │   └── task_user.S        # define user task handler
│   └── kmain.cc  # kernel entry
├── lib/
│   ├── assert.cc # define assert utility
│   └── bwio.cc   # define busy-wait I/O routines
├── user/
│   ├── include/
│   │   ├── boot.h # declare first user task and its priority 
│   │   └── k1.h   # declare k1 user task
│   ├── tasks/
│   │   └── k1.cc  # define k1 user task
│   └── boot.cc  # define first uesr task
├── .gitignore
├── linker.ld
├── Makefile
└── README.md
```

## Data Structures

### Priority Queues
`include/kern/task.h`

A wrapper struct that provides interface for easily manipulating priority queues. It is implemented as an array of queues. Each priority level corresponds to one queue. Each queue is a singly-linked list. The linkage is stored in each Task Descriptor in the `nextReady` field.

- `void enqueue(TaskDescriptor *task)`
  
  push a task to the end of its priority queue

- `TaskDescriptor *dequeue(int priority)` 
  
  pop a task from the front of a specific priority queue corresponding to the input priority

- `TaskDescriptor *dequeue()`

  pop a task from the front of the highest non-empty priority queue
  


### Task Descriptors
`include/kern/task.h`

A struct that stores task related state, including
- `int tid` Task id
- `TaskDescriptor *parent` Task's parent task
- `int priority` Task's priority
- `TaskDescriptor *nextReady` Next ready task after curent task
- `State state` Task's running state 
- `Trapframe tf` Task's Trapframe

### Trapframe
`include/kern/syscall.h`


A struct that stores the user state (`r1`~`r14`, `lr_svc`, `spsr`) before context switch and restore them when switched back.

## System Parameters and Limitations


- `include/kern/task.h`:
  - `USER_STACK_SIZE` (stack size for each task): **128 KB**
  - `NUM_TASKS` (maximum number of tasks): **64**
  - `NUM_PRIORITY_LEVELS` (number of priority levels): **8** *(0 to 7 inclusive, 0 is the highest)*

Note: The number of tasks and the stack size for each task can be made larger by modifying `include/kern/task.h`, as long as `0x1000000 - USER_STACK_SIZE * NUM_TASKS > __bss_end`.


## Program Output

```
 1| Task id: 1, Parent task id: 0
 2| Task id: 1, Parent task id: 0
 3| Created: 1
 4| Task id: 2, Parent task id: 0
 5| Task id: 2, Parent task id: 0
 6| Created: 2
 7| Created: 3
 8| Created: 4
 9| FirstUserTask: exiting
10| Task id: 3, Parent task id: 0
11| Task id: 4, Parent task id: 0
12| Task id: 3, Parent task id: 0
13| Task id: 4, Parent task id: 0
```

Task 0 (FirstUserTask) has priority 1. Tasks 1 and 2 have priority 0. Task 3 and 4 have priority 2. Note that priority 0 is the highest.

Task 0 starts by creating Task 1. `create()` triggers reschedule and Task 1 has a higher priority, so Task 1 is executed. Task 1 calls `myTid()` and then `parentTid()`. Although both functions trigger rescheduling, Task 1 has a higher priority, therefore it continues execution after both calls and print Line 1 before going back to Task 0.
Task 1 then yields. Now both Task 1 and Task 0 are ready. But Task 1 has a higher priority and therefore Task 1 continues execution and prints Line 2 in the same way of printing Line 1. After this, Task 1 exits. Now, only Task 0 is in the ready queue. So it continues execution and print Line 3.

Then Task 0 creates Task 2. Similar to Task 1, Task 2 has a higher priority and starts execution and print Line 4 before Task 0 returns from `create()`. Task 2 then yields, but as the highest-priority task in the ready queue. It continues execution and prints Line 5 and exits. Now, only Task 0 is in the ready queue. So it continues execution and print Line 6.

Then Task 0 creates Task 3, which has a lower priority than Task 0. So after leaving kernel mode Task 0 continues execution and print Line 7. This happens again when Task 0 creates Task 4 and prints Line 8. Then Task 0 prints Line 9 and exits.

Now Tasks 3 and 4 are in the ready queue and have the same priority. Task 3 calls `myTid()` which triggers rescheduling and switches to Task 4. Then Task 4 calls `myTid()` and triggers rescheduling and switches to Task 3. Then Task 3 calls `parentTid()` and switches to Task 4. Then Task 4 calls `parentTid()` and switches to Task 3. Then Task 3 prints Line 10 and yields, which triggers rescheduling and switch to Task 4. Then Task 4 prints Line 11 and yields and switch back to Task 3. The bounce between Tasks 3 and 4 repeated for Line 12 and Line 13. Task 3 exits after printing Line 12. Task 4 exits after printing Line 13.

Now there is no task in the ready queue and the kernel exits.