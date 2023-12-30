#pragma once

#include <frontend/internalRegistry.h>
#include <frontend/nameResolver.h>
#include <common/graphDumper.h>
#include <EASTL/fixed_vector.h>

#include <frontend/dependencyData.h>


namespace dabfg
{

class DependencyDataCalculator
{
public:
  DependencyDataCalculator(InternalRegistry &reg, const NameResolver &nameRes) : registry{reg}, nameResolver{nameRes} {}

  DependencyData depData;

  void recalculate();

private:
  InternalRegistry &registry;
  const NameResolver &nameResolver;

  void recalculateResourceLifetimes();
  void resolveRenaming();
  void updateRenamedResourceProperties();
};

} // namespace dabfg
