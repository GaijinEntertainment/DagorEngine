// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/solid_api.h>
#include <dag/dag_vector.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <util/dag_stdint.h>
#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <string.h>

namespace dacsg
{
// the number backend of the exact kernel, spec.md 7.1
enum class Backend : uint8_t
{
  Int64Grid, // the vertices snap to one integer grid, gridBits wide
  Shewchuk,  // the vertices stay as given, predicates on floating-point expansions
};

// The threads of one kernel call, its caller's included: a parallel loop leases the spare ones while it runs, so the
// loops of booleans that run at once share them
class ThreadBudget
{
public:
  explicit ThreadBudget(uint32_t threads) : spare(threads > 1 ? threads - 1 : 0) {}
  uint32_t take(uint32_t want)
  {
    uint32_t have = spare.load();
    while (have && !spare.compare_exchange_weak(have, have - eastl::min(have, want)))
      ;
    return eastl::min(have, want);
  }
  void give(uint32_t n) { spare.fetch_add(n); }

private:
  dag::AtomicInteger<uint32_t> spare;
};

// Up to want workers for one loop, the caller and threads of the budget, given back at the end of the scope; all of
// want with no budget
class WorkerLease
{
public:
  WorkerLease(ThreadBudget *b, uint32_t want) : budget(b), extra(want <= 1 ? 0 : b ? b->take(want - 1) : want - 1) {}
  EA_NON_COPYABLE(WorkerLease)
  ~WorkerLease()
  {
    if (budget && extra)
      budget->give(extra);
  }
  uint32_t workers() const { return 1 + extra; }

private:
  ThreadBudget *budget;
  uint32_t extra;
};

struct KernelParams
{
  double tolerance = 0;
  ThreadBudget *budget = nullptr; // the threads of the whole call, valid for the call only; nullptr: a kernel takes threads alone
  int threads = 0;                // 0: pool size
  uint32_t maxArrangementTris = 1000000; // about 2 KB each, spec.md 13: the 2 GB target
  int gridBits = 40;                     // 1..40: the exact predicates' integer widths assume at most 40; Shewchuk ignores it
  Backend backend = Backend::Int64Grid;
};

struct KernelCounter
{
  const char *name; // static storage
  int64_t value;
};

// One thread at a time writes it: a pool phase counts per worker and adds the counts after the phase
struct KernelStats
{
  dag::Vector<KernelCounter> counters;

  void add(const char *name, int64_t value)
  {
    for (KernelCounter &c : counters)
      if (strcmp(c.name, name) == 0)
      {
        c.value += value;
        return;
      }
    counters.push_back(KernelCounter{name, value});
  }
};

struct BoolTree
{
  enum class Kind : uint8_t
  {
    Union,
    Subtract,
    Intersect,
    Concat,
    Leaf,
  };
  struct Node
  {
    Kind kind = Kind::Leaf;
    uint32_t child[2] = {0, 0};
    uint32_t leaf = 0;
  };

  dag::Vector<Node> nodes; // children first, the root last; a node feeds one parent, which may name it in both slots
  dag::Vector<Solid> leaves;
};

// BoolTree's contract, the same refusal for every kernel: children first, each feeding one parent; leaves that exist
bool bool_tree_valid(const BoolTree &tree, eastl::string &err);

struct IKernel
{
  virtual ~IKernel() {}
  virtual const char *name() const = 0; // static storage, it outlives the kernel
  // false: err holds the reason, which execute reports
  virtual bool evaluate(const BoolTree &tree, const KernelParams &p, Solid &out, KernelStats &stats, eastl::string &err) = 0;
};

struct IPairwiseKernel : IKernel
{
  bool evaluate(const BoolTree &tree, const KernelParams &p, Solid &out, KernelStats &stats, eastl::string &err) override;
  // true: two boolean() calls may run at once
  virtual bool concurrentBooleans() const { return false; }
  // op is Union, Subtract or Intersect, never Concat or Leaf; false: err holds the reason, which evaluate passes on
  virtual bool boolean(BoolTree::Kind op, const Solid &a, const Solid &b, const KernelParams &p, Solid &out, KernelStats &stats,
    eastl::string &err) = 0;
};

// Not owned, and the registry keeps the pointer: a registered kernel outlives every execute and every call below
void register_kernel(IKernel *kernel);
// Also finds "concat", the internal kernel of Concat-only trees, which kernel_names() leaves out
IKernel *find_kernel(const char *name);
dag::Vector<const char *> kernel_names();
// "manifold" or "exact", built or not
bool is_library_kernel(const char *name);
bool is_exact_kernel(const IKernel *k);
// the other library kernel, the fallback of one that refused; null for another kernel or one not built
IKernel *other_kernel(const IKernel *k);
} // namespace dacsg
