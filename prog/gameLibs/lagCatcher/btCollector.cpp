#include "btCollector.h"
#include <new>
#include <string.h>
#include <stdint.h>
#include <execinfo.h>
#include <stdlib.h>

#define BASE_GEN (1 << 30)

static uintptr_t hash_bt(void **stack, int depth)
{
  uintptr_t h = 0;
  for (int i = 0; i < depth; i++)
  {
    uintptr_t slot = (uintptr_t)stack[i];
    h = (h << 8) | (h >> (8 * (sizeof(h) - 1)));
    h += (slot * 31) + (slot * 7) + (slot * 3);
  }
  return h;
}

BTCollector::BTCollector(int mem_budget) : numEvicted(0), count(0), lost(0), curGen(BASE_GEN), oldCount(0)
{
  totalBuckets = ((mem_budget * 100) / 66) / sizeof(buckets[0]); // 2/3
  totalEvicted = ((mem_budget * 100) / 33) / sizeof(evicted[0]); // 1/3

  buckets = new Bucket[totalBuckets];
  evicted = new Entry[totalEvicted];

  memset(buckets, 0, sizeof(buckets[0]) * totalBuckets);
}

BTCollector::~BTCollector()
{
  delete[] buckets;
  delete[] evicted;
}

void BTCollector::nextGen()
{
  if (oldCount != count) // only if was something collected on prev cycle
  {
    // decrement generation for proper sorting
    // (we want ascending generation but descending events count sorting)
    --curGen;
    oldCount = count;
  }
}

void BTCollector::clear()
{
  count = numEvicted = lost = oldCount = 0;
  curGen = BASE_GEN;
  memset(buckets, 0, sizeof(buckets[0]) * totalBuckets);
}

void BTCollector::push(void **stack, int depth)
{
  if (depth <= 0)
    return;
  else if (depth > MAX_STACK_DEPTH)
    depth = MAX_STACK_DEPTH;

  count++;

  uintptr_t hash = hash_bt(stack, depth);

  // check if stack already exists in table
  bool done = false;
  Bucket &buck = buckets[hash % totalBuckets];
  for (int a = 0; a < ASSOCIATIVITY; ++a)
  {
    Entry &ent = buck.entries[a];
    if (ent.depth == depth && memcmp(stack, ent.stack, sizeof(void *) * depth) == 0)
    {
      ent.count++;
      done = true;
    }
  }
  if (!done)
  {
    // evict entry with smallest number of hits
    Entry *e = &buck.entries[0];
    for (int a = 1; a < ASSOCIATIVITY; ++a)
      if (buck.entries[a].count < e->count)
        e = &buck.entries[a];
    if (e->count > 0) // was occupied?
    {
      if (numEvicted < totalEvicted)
        memcpy(&evicted[numEvicted++], e, sizeof(evicted[0]));
      else
        lost++;
    }
    e->count = 1;
    e->generation = curGen;
    e->depth = depth;
    memcpy(e->stack, stack, sizeof(void *) * depth);
  }
}

/* static */
int BTCollector::entry_cmp(const void *a, const void *b)
{
  BTCollector::Entry *ea = (BTCollector::Entry *)a;
  BTCollector::Entry *eb = (BTCollector::Entry *)b;
  if (eb->sortKey > ea->sortKey)
    return 1;
  else if (eb->sortKey < ea->sortKey)
    return -1;
  else
    return 0;
}

int BTCollector::flush(FILE *f)
{
  if (isEmpty())
    return 0;

#define WRITE_ENT(e)                                                                            \
  {                                                                                             \
    fprintf(f, "\nCollected %d times, generation %d:\n", (e).count, BASE_GEN - (e).generation); \
    fflush(f);                                                                                  \
    backtrace_symbols_fd((e).stack, (e).depth, fd);                                             \
    flushed++;                                                                                  \
  }

  qsort(buckets, totalBuckets * ASSOCIATIVITY, sizeof(buckets[0].entries[0]), &entry_cmp);
  qsort(evicted, numEvicted, sizeof(evicted[0]), &entry_cmp);

  int flushed = 0;
  fprintf(f, "Collected %d backtraces (%d lost):\n", count, lost);
  int fd = fileno(f);
  for (int b = 0; b < totalBuckets; ++b)
    for (int a = 0; a < ASSOCIATIVITY; ++a)
      if (buckets[b].entries[a].count > 0)
        WRITE_ENT(buckets[b].entries[a])
  for (int i = 0; i < numEvicted; ++i)
    WRITE_ENT(evicted[i])

#undef WRITE_ENT

  return flushed;
}
