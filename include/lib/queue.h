#ifndef LIB_QUEUE_H_
#define LIB_QUEUE_H_

#include "lib/assert.h"

template <typename T, int cap>
class Queue {
  T data[cap];
  int head, sz;

 public:
  Queue() : head{0}, sz{0} {}

  bool enqueue(T val) {
    if (sz >= cap) {
      return false;
    }
    data[(head + sz) % cap] = val;
    ++sz;
    return true;
  }

  T dequeue() {
    assert(sz > 0);
    T val = data[head];
    head = (head + 1) % cap;
    --sz;
    return val;
  }

  int size() const { return sz; }
};

#endif  // LIB_QUEUE_H_
