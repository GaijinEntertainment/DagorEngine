// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/hash_map.h>
#include <generic/dag_relocatableFixedVector.h>
#include <shaders/dag_rendInstRes.h>

namespace dagdp
{

static constexpr uint32_t ESTIMATED_PROCESSED_MESHES_PER_FRAME = 8;
using AreasIndices = dag::RelocatableFixedVector<uint32_t, ESTIMATED_PROCESSED_MESHES_PER_FRAME>;

class RiexProcessor
{
public:
  struct ResourceState
  {
    AreasIndices areasIndices;
  };

private:
  eastl::hash_map<RenderableInstanceResource *, ResourceState> resources;
  RenderableInstanceResource *nextToProcess = nullptr;

public:
  const ResourceState *ask(RenderableInstanceResource *res);
  RenderableInstanceResource *current() const { return nextToProcess; }
  void resetCurrent();
  void markCurrentAsProcessed(AreasIndices &&areasIndices);

  ~RiexProcessor()
  {
    for (const auto &item : resources)
      item.first->delRef();
  }
};

} // namespace dagdp
