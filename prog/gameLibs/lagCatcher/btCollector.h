// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdio.h> // FILE
#include <stdint.h>

class BTCollector // Backtrace collector
{
public:
  static constexpr int MAX_STACK_DEPTH = 62;

  BTCollector(int mem_budget);
  ~BTCollector();

  void push(void **stack, int depth);
  bool isEmpty() const { return count == 0; }
  void clear();
  int flush(FILE *f);
  void nextGen();

private:
  static constexpr int ASSOCIATIVITY = 4;

  struct Entry
  {
    union
    {
      struct
      {
        int count;      // low 32 bits
        int generation; // high 32 bits
      };
      int64_t sortKey;
    };
    int depth;
    void *stack[MAX_STACK_DEPTH];
  };

  struct Bucket
  {
    Entry entries[ASSOCIATIVITY];
  };

  Bucket *buckets;
  int totalBuckets;

  Entry *evicted;
  int totalEvicted;
  int numEvicted;

  int count; // samples seen (including lost ones)
  int lost;  // samples lost (no space left)

  int oldCount;
  int curGen; // current generation of samples

  static int entry_cmp(const void *a, const void *b);
};
