// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "frontend/validityInfo.h"

namespace dabfg
{

struct InternalRegistry;
struct DependencyData;
class NameResolver;
struct ResourceData;

class RegistryValidator
{
public:
  RegistryValidator(const InternalRegistry &reg, const DependencyData &depDataCalc, const NameResolver &nameRes) :
    registry{reg}, depData{depDataCalc}, nameResolver{nameRes}
  {}

  void validateRegistry();

  ValidityInfo validityInfo;

private:
  const InternalRegistry &registry;
  const DependencyData &depData;
  const NameResolver &nameResolver;

  bool validateResource(ResNameId resId) const;
  bool validateNode(NodeNameId resId) const;
  void validateLifetimes(ValidityInfo &validity) const;

  const ResourceData &resourceDataFor(ResNameId resId) const;
};

} // namespace dabfg
