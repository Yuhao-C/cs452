#include "kern/message.h"

#include "kern/syscall.h"
#include "kern/task.h"
#include "lib/assert.h"

int msgCopy(const char *src, int srcLen, char *dst, int dstLen) {
  if (srcLen < dstLen) {
    dstLen = srcLen;
  }
  for (int i = 0; i < dstLen; ++i) {
    dst[i] = src[i];
  }
  return dstLen;
}

void msgCopy(TaskDescriptor *sender, TaskDescriptor *receiver) {
  int *senderTid = (int *)receiver->tf.r0;
  *senderTid = sender->tid;

  const char *senderBuf = (const char *)sender->tf.r1;
  int senderMsgLen = (int)sender->tf.r2;
  char *receiverBuf = (char *)receiver->tf.r1;
  int receiverMsgLen = (int)receiver->tf.r2;
  int copiedLen = msgCopy(senderBuf, senderMsgLen, receiverBuf, receiverMsgLen);

  receiver->tf.r0 = copiedLen;
}

void msgSend() {
  int tid = curTask->tf.r0;

  // TODO: check condition for return -2

  if (!isTidValid(tid)) {
    curTask->tf.r0 = -1;
    taskYield();
    return;
  }

  TaskDescriptor *sender = curTask;
  TaskDescriptor *receiver = &tasks[tid];

  if (receiver->state == TaskDescriptor::State::kReceiveBlocked) {
    // receiver first
    receiver->state = TaskDescriptor::State::kReady;
    readyQueues.enqueue(receiver);
    sender->state = TaskDescriptor::State::kReplyBlocked;
    msgCopy(sender, receiver);
  } else {
    // sender first
    sender->state = TaskDescriptor::State::kSendBlocked;
    receiver->enqueueSender(sender);
  }
}

void msgReceive() {
  TaskDescriptor *sender = curTask->dequeueSender();
  TaskDescriptor *receiver = curTask;

  if (sender) {
    // sender first
    kAssert(sender->state == TaskDescriptor::State::kSendBlocked);
    sender->state = TaskDescriptor::State::kReplyBlocked;
    receiver->nextSend = sender->nextSend;
    msgCopy(sender, receiver);
    taskYield();
  } else {
    // receiver first
    receiver->state = TaskDescriptor::State::kReceiveBlocked;
  }
}

void msgReply() {
  int tid = (int)curTask->tf.r0;
  const char *reply = (const char *)curTask->tf.r1;
  int replyLen = (int)curTask->tf.r2;

  if (!isTidValid(tid)) {
    curTask->tf.r0 = -1;
    taskYield();
    return;
  }
  TaskDescriptor *sender = &tasks[tid];
  if (sender->state != TaskDescriptor::State::kReplyBlocked) {
    curTask->tf.r0 = -2;
    taskYield();
    return;
  }

  sender->state = TaskDescriptor::State::kReady;
  readyQueues.enqueue(sender);

  char *senderBuf = (char *)sender->tf.r3;
  int senderBufLen = *(int *)sender->tf.r13;
  int copiedLen = msgCopy(reply, replyLen, senderBuf, senderBufLen);

  curTask->tf.r0 = copiedLen;
  sender->tf.r0 = copiedLen;

  taskYield();
}
