#ifndef LIB_HASHTABLE_H_
#define LIB_HASHTABLE_H_

#include "lib/string.h"

int hash(const String &s);
int hash(int n);

template <typename K, typename V, int cap, int bucketSize>
class HashTable {
  struct Entry {
    K key;
    V value;
    Entry *next;
  };

  Entry nodes[cap];
  Entry *bucket[bucketSize];

  int size = 0;

 public:
  HashTable() {
    for (int i = 0; i < bucketSize; ++i) {
      bucket[i] = nullptr;
    }
  }

  void put(K key, V value) {
    assert(size < cap);
    int b = hash(key) % bucketSize;
    Entry *cur = bucket[b];
    Entry *last = nullptr;
    while (cur) {
      if (cur->key == key) {
        cur->value = value;
        return;
      }
      last = cur;
      cur = cur->next;
    }
    nodes[size].key = key;
    nodes[size].value = value;
    nodes[size].next = nullptr;
    ++size;
    if (last) {
      last->next = &nodes[size - 1];
    } else {
      bucket[b] = &nodes[size - 1];
    }
  }

  const V *get(K key) {
    int b = hash(key) % bucketSize;
    Entry *cur = bucket[b];
    while (cur) {
      if (cur->key == key) {
        return &cur->value;
      }
      cur = cur->next;
    }
    return nullptr;
  }
};

#endif  // LIB_HASHTABLE_H_