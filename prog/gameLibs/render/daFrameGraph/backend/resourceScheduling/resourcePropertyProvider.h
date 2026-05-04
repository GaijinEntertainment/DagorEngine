// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>

struct ResourceDescription;
struct ResourceAllocationProperties;
struct ResourceHeapGroupProperties;
struct ResourceHeapGroup;

namespace dafg
{

class IResourcePropertyProvider
{
public:
  virtual ResourceAllocationProperties getResourceAllocationProperties(const ResourceDescription &desc,
    bool force_not_on_chip = false) = 0;
  virtual ResourceHeapGroupProperties getResourceHeapGroupProperties(ResourceHeapGroup *group) = 0;

protected:
  ~IResourcePropertyProvider() = default;
};

} // namespace dafg
