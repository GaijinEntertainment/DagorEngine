// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// defines full draw/dispatch ready state that can be applied to execution state

#include "util/tracked_state.h"
#include "compute_state.h"
#include "graphics_state2.h"

namespace drv3d_vulkan
{

struct PipelineStateStorage
{
  FrontComputeState compute;
  FrontGraphicsState graphics;

  void reset() {}
  void dumpLog() const { debug("PipelineStateStorage end"); }

  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};
class PipelineState : public TrackedState<PipelineStateStorage, FrontComputeState, FrontGraphicsState>
{
  StateFieldResourceBinds *stageResources[STAGE_MAX_ACTIVE];

  template <typename T>
  bool handleObjectRemovalInResBinds(T object)
  {
    bool ret = false;
    for (uint32_t i = 0; i < STAGE_MAX_ACTIVE; ++i)
      ret |= stageResources[i]->handleObjectRemoval(object);
    return ret;
  }

  template <typename T>
  bool isReferencedByResBinds(T object) const
  {
    for (uint32_t i = 0; i < STAGE_MAX_ACTIVE; ++i)
      if (stageResources[i]->isReferenced(object))
        return true;
    return false;
  }

  void fillResBindFields();

public:
  template <typename T>
  bool handleObjectRemoval(T object);

  template <typename T>
  bool isReferenced(T object) const;

  bool processBufferDiscard(const BufferRef &old_buffer, const BufferRef &new_ref, uint32_t buf_flags);

  PipelineState();
  PipelineState(const PipelineState &from);

  StateFieldResourceBinds &getStageResourceBinds(ShaderStage stage) { return *stageResources[stage]; }
  void markResourceBindDirty(ShaderStage stage);
};

} // namespace drv3d_vulkan
