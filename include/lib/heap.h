#ifndef LIB_HEAP_H_
#define LIB_HEAP_H_

template <typename T, int cap>
class MinHeap {
  T heap[cap];
  int size = 0;

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
    assert(size < cap);
    ++size;
    int i = size - 1;
    heap[i] = node;

    while (i != 0 && heap[i] < heap[parent(i)]) {
      swap(i, parent(i));
      i = parent(i);
    }
  }

  const T *peekMin() { return size > 0 ? &heap[0] : nullptr; }

  void deleteMin() {
    if (size == 0) {
      return;
    }
    if (size == 1) {
      --size;
      return;
    }

    heap[0] = heap[size - 1];
    --size;

    int i = 0;
    while (true) {
      int l = left(i);
      int r = right(i);
      int smaller = i;
      if (l < size && heap[l] < heap[i]) {
        smaller = l;
      }
      if (r < size && heap[r] < heap[smaller]) {
        smaller = r;
      }
      if (smaller == i) {
        break;
      }
      swap(smaller, i);
    }
  }
};

#endif  // LIB_HASHTABLE_H_