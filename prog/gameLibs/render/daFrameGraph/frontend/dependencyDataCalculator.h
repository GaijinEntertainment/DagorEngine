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
  DependencyDataCalculator(InternalRegistry &reg, const NameResolver &nameRes, FrontendRecompilationData &recompData) :
    registry{reg}, nameResolver{nameRes}, frontendRecompilationData{recompData}
  {}

  DependencyData depData;

  void recalculate();

private:
  InternalRegistry &registry;
  const NameResolver &nameResolver;
  FrontendRecompilationData &frontendRecompilationData;

  void recalculateResourceLifetimes();
  void resolveRenaming();
  void updateRenamedResourceProperties();
};

} // namespace dafg
