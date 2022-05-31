# CS452 Kernel

## Group Member

- Qishen Wu \<q246wu@uwaterloo.ca\>
- Yuhao Chen \<y627chen@uwaterloo.ca\>

## Setup

```bash
git clone https://git.uwaterloo.ca/y627chen/cs452.git
cd cs452
make
make install # transfer kmain.elf to the tftp server
```

- Executable file: `kern/kmain.elf`

## File Structure

```dockerfile
cs452/
├── include/
│   ├── kern/
│   │   ├── arch/
│   │   │   └── ts7200.h # ts7200 registers definition
│   │   ├── common.h  # declare common type and helper
│   │   ├── kmem.h    # declare constants for memory sections
│   │   ├── message.h # declare kernel send-receive-reply handler
│   │   ├── sys.h     # declare kExit (exit kernel)
│   │   ├── syscall_code.h # define syscall codes
│   │   ├── syscall.h # declare Trapframe and syscall functions
│   │   └── task.h    # declare kernel task handler
│   ├── lib/
│   │   ├── assert.h # declare assert utility
│   │   ├── bwio.h   # declare busy-wait I/O routines
│   │   ├── hashtable.h # declare hashtable
│   │   ├── queue.h # declare queue
│   │   ├── string.h # declare string wrapper
│   │   └── timer.h # declare timer functions
│   └── user/
│       ├── message.h # declare user send-receive-reply
│       └── task.h # declare user task interface
├── kern/
│   ├── lib/
│   │   └── sys.cc # define kExit (exit kernel)
│   ├── message/
│   │   ├── message_user.S # define message SWI instruction
│   │   └── message.cc # define kern send-receive-reply handler
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
│   ├── bwio.cc   # define busy-wait I/O routines
│   ├── hashtable.cc # define hashtable
│   ├── queue.cc  # define queue
│   ├── string.cc # define string wrapper
│   └── timer.cc  # define timer functions
├── user/
│   ├── include/
│   │   ├── boot.h # declare first user task and its priority
│   │   ├── k1.h   # declare k1 user task
│   │   ├── name_server.h # declare name server
│   │   ├── perf_test.h # declare performance test
│   │   └── rps.h # declare RPS server and Client
│   ├── tasks/
│   │   ├── k1.cc   # define k1 user task
│   │   ├── name_server.h # define name server
│   │   ├── perf_test.h # define performance
│   │   └── rps.cc  # define RPS server and Client
│   └── boot.cc  # user program entry
├── .gitignore
├── linker.ld
├── Makefile
└── README.md
```

## Kernel Description

### Context Switch

`taskSchedule` -> `taskActivate` -> `leaveKernel` -> `userMode` -> _Execute User Program_ -> _Software Interrupt_ -> `handleSWI` -> `enterKernel`

- `taskSchedule`: Fetch a top-priority task in the task ready queue.
- `taskActivate`: Sets the fetched task's state to `Active`, then calls `leaveKernel`.
- `leaveKernel`: Calls `userMode`. Return to `taskActivate`.
- `userMode (asm)`: Saves kernel context (`r4`~`r11`, `lr`) on the stack. Restore user program's context from its trapframe. User program starts to execute.
- `handleSWI (asm)`: Saves user program's context on the stack. Restores kernel context. Calls `enterKernel`. Return to `leaveKernel`.
- `enterKernel`: Based on the `SWI` code, calls the corresponding handler. Reschedules on every `enterKernel`. Return to `handleSWI`.

#### Priority Queues

`include/kern/task.h`

A wrapper struct that provides interface for easily manipulating priority queues. It is implemented as an array of queues. Each priority level corresponds to one queue. Each queue is a singly-linked list. The linkage is stored in each Task Descriptor in the `nextReady` field.

- `void enqueue(TaskDescriptor *task)`

  push a task to the end of its priority queue

- `TaskDescriptor *dequeue(int priority)`

  pop a task from the front of a specific priority queue corresponding to the input priority

- `TaskDescriptor *dequeue()`

  pop a task from the front of the highest non-empty priority queue

#### Task Descriptors

`include/kern/task.h`

A struct that stores task related state, including

- `int tid` Task id
- `TaskDescriptor *parent` Task's parent task
- `int priority` Task's priority
- `TaskDescriptor *nextReady` Next ready task after curent task
- `State state` Task's running state
- `Trapframe tf` Task's Trapframe

#### Trapframe

`include/kern/syscall.h`

A struct that stores the user state (`r1`~`r14`, `lr_svc`, `spsr`) before context switch and restore them when switched back.

#### System Parameters and Limitations

- `include/kern/task.h`:
  - `USER_STACK_SIZE` (stack size for each task): **128 KB**
  - `NUM_TASKS` (maximum number of tasks): **64**
  - `NUM_PRIORITY_LEVELS` (number of priority levels): **8** _(0 to 7 inclusive, 0 is the highest)_

Note: The number of tasks and the stack size for each task can be made larger by modifying `include/kern/task.h`, as long as `0x1000000 - USER_STACK_SIZE * NUM_TASKS > __bss_end`.

### Message Passing

- `send()` and `receive()` (sender first)
  - The sender is enqueued into the receiver's send queue and the sender's state changes to _send-blocked_ in `send()`.
  - When the receiver calls `receive()`, the sender is dequeued from the send queue and data is copied from the sender to the receiver. The sender becomes _reply-blocked_. The receiver becomes _ready_ due to rescheduling.
- `send()` and `receive()` (receiver first)
  - The receiver becomes _receive-blocked_ in `receive()`.
  - When the sender calls `send()`, data is copied from the sender to the receiver. There is no need to enqueue the sender into a send queue because the receiver is already _receive-blocked_ which means it is ready to receive a message. Then the sender becomes _reply-blocked_. The receiver becomes _ready_.
- `reply()`
  - When the receiver calls `reply()`, the sender should always be _reply-blocked_.
    - If the sender is not _reply-blocked_, it means that the sender have not sent anything and the `reply()` call is invalid. `reply()` will just return.
  - Data of the reply is copied from the receiver to the sender. Then the sender becomes _ready_ and is enqueue into the ready queue. The receiver becomes _ready_ afterwards and is enqueued too due to rescheduling. Note that the sender is enqueued first, so that we the sender and the receiver has the same priority, the sender will run first.

#### Send Queues

Each task has a send queue which stores all tasks that are trying to send message to the task.
The task queues are implemented as a linked list where the linkage is stored as a pointer in `TaskDescriptor::nextSend`. To allow efficient enqueue, a pointer to the end of each queue is stored in `TaskDescriptor::lastSend`. The front of a task's send queue is also stored in `TaskDescriptor::nextSend`.

Notice that although each task has a send queue individually, we only maintain one `nextSend` pointer in each `TaskDescriptor`. This is because a task cannot be in more than one send queues at the same time. When the task is present in a send queue, it is always _send-blocked_ and waiting for the receiver to receive its message, so the task cannot send to another receiver at the same time. This also allows us to store both the front of a queue and the linked-list linkage at the same field `nextSend`. The `nextSend` field in a _send-blocked_ task stores the linked-list linkage, while the same field in a task that is not _send-blocked_ stores the front of its send queue. When a task is not _send-blocked_, it is not trying to send anything as thus not in any send queue. This ensures we can distinguish the different interpretations of `nextSend` and each send queue is standalone and the linkages are not mixed.

### Name Server

The name server is running in the highest priority so it can reply as soon as possible to avoid blocking other tasks. Since we assign tid in the order of creation and the name server is always the first task created by the boot task, the tid of the name server is always `1`.

`registerAs()` and `whoIs()` is simply wrapper functions that calls `send()` to the name server and obtain reply.

#### Hash Table

We use a hash table to store the mapping between task name and tid. The task name is stored as a `String`, a wrapper for `const char *` to allow overloading `operator==()` for string comparison. Since we do not have dynamic memory allocation, when using the hash table with strings, we must make sure that the `const char *` points to a valid address, that is, either in the static area or on stack when the stack is valid.

## Program Output

### K2: Rock-Paper-Scissors

```
 1| [RPS Player 3]: 🙋	Sign Up
 2| [RPS Player 4]: 🙋	Sign Up
 3| [RPS Server]: matched [Player 3] and [Player 4]
 4| [RPS Player 5]: 🙋	Sign Up
 5| [RPS Player 6]: 🙋	Sign Up
 6| [RPS Server]: matched [Player 5] and [Player 6]
 7| [RPS Player 7]: 🙋	Sign Up
 8| [RPS Player 3]: 👊	Rock
 9| [RPS Player 4]: 🖐️	Paper
10| [RPS Player 5]: 👊	Rock
11| [RPS Player 6]: 🖐️	Paper
12| [RPS Player 4]: 🥳	Win
13| [RPS Player 4]: ✌️	Scissors
14| [RPS Player 3]: 😭	Lose
15| [RPS Player 3]: 👊	Rock
16| [RPS Player 6]: 🥳	Win
17| [RPS Player 6]: ✌️	Scissors
18| [RPS Player 5]: 😭	Lose
19| [RPS Player 5]: 💨	Quit
20| [RPS Player 3]: 🥳	Win
21| [RPS Player 3]: 💨	Quit
22| [RPS Player 4]: 😭	Lose
23| [RPS Player 4]: 💨	Quit
24| [RPS Player 6]: 🏳️	Opponent Quit
25| [RPS Server]: matched [Player 7] and [Player 6]
26| [RPS Player 7]: 🖐️	Paper
27| [RPS Player 6]: 👊	Rock
28| [RPS Player 6]: 😭	Lose
29| [RPS Player 6]: 🖐️	Paper
30| [RPS Player 7]: 🥳	Win
31| [RPS Player 7]: ✌️	Scissors
32| [RPS Player 7]: 🥳	Win
33| [RPS Player 7]: 💨	Quit
34| [RPS Player 6]: 😭	Lose
35| [RPS Player 6]: 💨	Quit
```

We create 5 players, where `Player x` represent the player task whose tid is `x`.

**Player behavior:**

- `Player 3`, `Player 4`, `Player 7`: sign up, play **two** games, and then quit.

- `Player 5`: sign up, play a **single** game, and then quit.

- `Player 6`: wants to play **two** games. If the opponent quits before two games finish, it will sign up again to play another two games.

First, `Players 3~7` sign up in their sequential order. The server matches `Player 3` and `Player 4`, then matches `Player 5` and `Player 6`. `Player 7` is not matched at first.

The following matches are played concurrently on the server:

|         | Player 3    | Player 4        |
| ------- | ----------- | --------------- |
| Round 1 | Rock (Lose) | Paper (Win)     |
| Round 2 | Rock (Win)  | Scissors (Lose) |
|         | Quit        | Quit            |

|         | Player 5    | Player 6    |
| ------- | ----------- | ----------- |
| Round 1 | Rock (Lose) | Paper (Win) |
| Round 2 | Quit        | Scissors    |

Now since `Player 5` quitted before two games finish, `Player 6` signs up again. And now server matches `Player 6` and `Player 7`.

|         | Player 6     | Player 7       |
| ------- | ------------ | -------------- |
| Round 1 | Rock (Lose)  | Paper (Win)    |
| Round 2 | Paper (Lose) | Scissors (Win) |
|         | Quit         | Quit           |

Now all players have quitted.

### K1

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
