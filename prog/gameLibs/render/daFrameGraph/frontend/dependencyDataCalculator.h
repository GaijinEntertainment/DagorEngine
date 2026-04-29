// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <frontend/internalRegistry.h>
#include <frontend/nameResolver.h>
#include <common/graphDumper.h>
#include <EASTL/fixed_vector.h>

#include <frontend/dependencyData.h>


namespace dafg
{

class DependencyDataCalculator
{
public:
  DependencyDataCalculator(InternalRegistry &reg, const NameResolver &nameRes) : registry{reg}, nameResolver{nameRes} {}

  DependencyData depData;


  using NodesChanged = IdIndexedFlags<NodeNameId, framemem_allocator>;
  using ResourcesChanged = IdIndexedFlags<ResNameId, framemem_allocator>;

  ResourcesChanged recalculate(const NodesChanged &node_changed);

private:
  InternalRegistry &registry;
  const NameResolver &nameResolver;

  DependencyData depDataClone;

  ResourcesChanged recalculateResourceLifetimes(const NodesChanged &node_changed);
  void resolveRenaming();
};

} // namespace dafg
