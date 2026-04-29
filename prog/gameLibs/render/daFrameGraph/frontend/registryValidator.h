// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "frontend/validityInfo.h"
#include <render/daFrameGraph/history.h>
#include <id/idIndexedMapping.h>
#include <render/daFrameGraph/detail/resNameId.h>

namespace dafg
{

struct InternalRegistry;
struct DependencyData;
class NameResolver;
struct CreatedResourceData;

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

  void buildCreatedResourceDataCache();

  bool validateResource(ResNameId resId) const;
  bool validateNode(NodeNameId resId) const;
  void validateLifetimes(ValidityInfo &validity) const;

  // Precomputed ResNameId -> CreatedResourceData* mapping, built once per validateRegistry call.
  // nullptr means the resource has no created data.
  IdIndexedMapping<ResNameId, const CreatedResourceData *> createdResourceDataCache;
};

} // namespace dafg
