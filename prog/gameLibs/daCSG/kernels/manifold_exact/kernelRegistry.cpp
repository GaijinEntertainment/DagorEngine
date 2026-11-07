// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/exact.h"
#include "kernelManifold/manifold.h"
#include <gameLibs/daCSG/kernel_api.h>
#include <EASTL/iterator.h>
#include <string.h>

namespace dacsg
{
#ifndef DACSG_EXACT
#define DACSG_EXACT 1
#endif

// A kernel in another object needs a reference from here, or a static lib leaves it out of the exe
static dag::Vector<IKernel *> &registry()
{
  static dag::Vector<IKernel *> kernels = {
    manifold_kernel_instance(),
#if DACSG_EXACT
    exact_kernel_instance(),
#endif
  };
  return kernels;
}

bool is_library_kernel(const char *name) { return name && (strcmp(name, "manifold") == 0 || strcmp(name, "exact") == 0); }
bool is_exact_kernel(const IKernel *k) { return strcmp(k->name(), "exact") == 0; }

IKernel *other_kernel(const IKernel *k)
{
  return strcmp(k->name(), "manifold") == 0 ? find_kernel("exact") : is_exact_kernel(k) ? find_kernel("manifold") : nullptr;
}

void register_kernel(IKernel *kernel)
{
  for (IKernel *&k : registry())
    if (strcmp(k->name(), kernel->name()) == 0)
    {
      k = kernel;
      return;
    }
  registry().push_back(kernel);
}

class ConcatKernel final : public IPairwiseKernel
{
public:
  const char *name() const override { return "concat"; }
  bool boolean(BoolTree::Kind op, const Solid &, const Solid &, const KernelParams &, Solid &, KernelStats &,
    eastl::string &err) override
  {
    static const char *const names[] = {"union", "subtract", "intersect", "concat"};
    static_assert(eastl::size(names) == size_t(BoolTree::Kind::Concat) + 1);
    err.sprintf("kernel not available: %s", names[int(op)]);
    return false;
  }
};

static ConcatKernel concat_kernel;

IKernel *find_kernel(const char *name)
{
  for (IKernel *k : registry())
    if (name && strcmp(k->name(), name) == 0)
      return k;
  return name && strcmp(name, concat_kernel.name()) == 0 ? &concat_kernel : nullptr;
}

dag::Vector<const char *> kernel_names()
{
  dag::Vector<const char *> names;
  for (IKernel *k : registry())
    names.push_back(k->name());
  return names;
}
} // namespace dacsg
