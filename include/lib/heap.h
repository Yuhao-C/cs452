#ifndef LIB_HEAP_H_
#define LIB_HEAP_H_

#include "lib/assert.h"

template <typename T, int cap>
class MinHeap {
  T heap[cap];
  int sz = 0;

  int parent(int i) { return (i - 1) / 2; }

  int left(int i) { return (2 * i + 1); }

  int right(int i) { return (2 * i + 2); }

  void swap(int i, int j) {
    T temp = heap[i];
    heap[i] = heap[j];
    heap[j] = temp;
  }

 public:
  MinHeap() {}

  void insert(T node) {
    assert(sz < cap);
    ++sz;
    int i = sz - 1;
    heap[i] = node;

    while (i != 0 && heap[i] < heap[parent(i)]) {
      swap(i, parent(i));
      i = parent(i);
    }
  }

  const T *peekMin() { return sz > 0 ? &heap[0] : nullptr; }

  void deleteMin() {
    if (sz == 0) {
      return;
    }
    if (sz == 1) {
      --sz;
      return;
    }

    heap[0] = heap[sz - 1];
    --sz;

    int i = 0;
    while (true) {
      int l = left(i);
      int r = right(i);
      int smaller = i;
      if (l < sz && heap[l] < heap[i]) {
        smaller = l;
      }
      if (r < sz && heap[r] < heap[smaller]) {
        smaller = r;
      }
      if (smaller == i) {
        break;
      }
      swap(smaller, i);
    }
  }

  bool has(const T &node) {
    for (int i = 0; i < sz; ++i) {
      if (node == heap[i]) {
        return true;
      }
    }
    return false;
  }

  int size() const { return sz; }
};

#endif  // LIB_HASHTABLE_H_