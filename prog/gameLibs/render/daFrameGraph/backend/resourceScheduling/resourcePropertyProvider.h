// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/numeric_limits.h>
#include <3d/dag_resPtr.h>

struct ResourceDescription;
struct ResourceAllocationProperties;
struct ResourceHeapGroupProperties;
struct ResourceHeapGroup;

namespace dafg
{

inline constexpr uint64_t UNKNOWN_MEMORY_BUDGET = eastl::numeric_limits<uint64_t>::max();

class IResourcePropertyProvider
{
public:
  virtual ResourceAllocationProperties getResourceAllocationProperties(const ResourceDescription &desc,
    bool force_not_on_chip = false) = 0;
  virtual ResourceHeapGroupProperties getResourceHeapGroupProperties(ResourceHeapGroup *group) = 0;
  virtual bool doHeapsCountObjects() const = 0;
  virtual bool doesResizeKeepContents() const = 0;
  virtual uint64_t getFreeMemoryBudget() const = 0;

protected:
  ~IResourcePropertyProvider() = default;
};

} // namespace dafg
