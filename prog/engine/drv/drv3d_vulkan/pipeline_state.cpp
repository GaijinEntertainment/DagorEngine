// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline_state.h"
#include "globals.h"
#include "pipeline/manager.h"
#include "memory_heap_resource.h"

namespace drv3d_vulkan
{

// specializations for handleObjectRemoval

template <>
bool PipelineState::handleObjectRemoval(Buffer *object)
{
  using VBField = StateFieldGraphicsVertexBuffers;
  using VBBind = StateFieldGraphicsVertexBufferBind;
  bool ret = false;

  if (object->isFrameMemReferencedAtRemoval())
    ret |= true;

  VBBind *vbinds = &get<VBField, VBBind, FrontGraphicsState>();
  for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
    if (vbinds[i].bRef.buffer == object)
    {
      set<VBField, uint32_t, FrontGraphicsState>(i);
      ret |= true;
    }

  if (get<StateFieldGraphicsIndexBuffer, BufferRef, FrontGraphicsState>().buffer == object)
  {
    makeFieldDirty<StateFieldGraphicsIndexBuffer, FrontGraphicsState>();
    ret |= true;
  }

  ret |= handleObjectRemovalInResBinds(object);

  if (ret && object->mayAlias())
    object->releaseHeapEarly(object->getDebugName());

  return ret;
}

template <>
bool PipelineState::isReferenced(Image *object) const
{
  const FrontFramebufferState &fb = getRO<FrontFramebufferState, FrontFramebufferState, FrontGraphicsState>();
  const FrontRenderPassState &rp = getRO<FrontRenderPassState, FrontRenderPassState, FrontGraphicsState>();
  return isReferencedByResBinds(object) || fb.isReferenced(object) || rp.isReferenced(object);
}

template <>
bool PipelineState::handleObjectRemoval(Image *object)
{
  FrontFramebufferState &fb = get<FrontFramebufferState, FrontFramebufferState, FrontGraphicsState>();
  FrontRenderPassState &rp = get<FrontRenderPassState, FrontRenderPassState, FrontGraphicsState>();
  bool ret = fb.handleObjectRemoval(object) || rp.handleObjectRemoval(object) || handleObjectRemovalInResBinds(object);
  if (ret && object->mayAlias())
    object->releaseHeapEarly(object->getDebugName());
  return ret;
}

template <>
bool PipelineState::handleObjectRemoval(ProgramID object)
{
  // unfortunately it can't be verified at use-place
  // so just fill with empty programs (so it can be noticed) and hope for the best
  bool ret = false;
  int progType = get_program_type(object);
  if (progType == program_type_graphics)
  {
    if (get<StateFieldGraphicsProgram, ProgramID, FrontGraphicsState>() == object)
    {
      // D3D_ERROR("vulkan: removing active graphics program %u", object.get());
      set<StateFieldGraphicsProgram, ProgramID, FrontGraphicsState>(ProgramID::Null());
      ret |= true;
    }

    if (Globals::pipelines.get<VariatedGraphicsPipeline>(object).pendingCompilation())
      ret |= true;
  }

  if (progType == program_type_compute)
  {
    if (get<StateFieldComputeProgram, ProgramID, FrontComputeState>() == object)
    {
      // D3D_ERROR("vulkan: removing active compute program %u", object.get());
      set<StateFieldComputeProgram, ProgramID, FrontComputeState>(ProgramID::Null());
      ret |= true;
    }

    if (Globals::pipelines.get<ComputePipeline>(object).pendingCompilation())
      ret |= true;
  }

  return ret;
}

template <>
bool PipelineState::handleObjectRemoval(RenderPassResource *object)
{
  if (object->isPipelineCompileReferenced())
    return true;
  FrontRenderPassState &rp = get<FrontRenderPassState, FrontRenderPassState, FrontGraphicsState>();
  return rp.handleObjectRemoval(object);
}

template <>
bool PipelineState::handleObjectRemoval(SamplerResource *object)
{
  return handleObjectRemovalInResBinds(object);
}

// specializations for isReferenced

template <>
bool PipelineState::isReferenced(Buffer *object) const
{
  using VBField = StateFieldGraphicsVertexBuffers;
  using VBBind = StateFieldGraphicsVertexBufferBind;
  const VBBind *vbinds = &getRO<VBField, VBBind, FrontGraphicsState>();

  if (object->isFrameMemReferencedAtRemoval())
    return true;

  for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
    if (vbinds[i].bRef.buffer == object)
      return true;

  if (getRO<StateFieldGraphicsIndexBuffer, BufferRef, FrontGraphicsState>().buffer == object)
    return true;

  return isReferencedByResBinds(object);
}

template <>
bool PipelineState::isReferenced(ProgramID object) const
{
  int progType = get_program_type(object);
  if (progType == program_type_graphics)
  {
    if (getRO<StateFieldGraphicsProgram, ProgramID, FrontGraphicsState>() == object)
      return true;
    if (Globals::pipelines.get<VariatedGraphicsPipeline>(object).pendingCompilation())
      return true;
  }

  if (progType == program_type_compute)
  {
    if (getRO<StateFieldComputeProgram, ProgramID, FrontComputeState>() == object)
      return true;

    if (Globals::pipelines.get<ComputePipeline>(object).pendingCompilation())
      return true;
  }

  return false;
}

template <>
bool PipelineState::isReferenced(RenderPassResource *object) const
{
  if (object->isPipelineCompileReferenced())
    return true;
  const FrontRenderPassState &rp = getRO<FrontRenderPassState, FrontRenderPassState, FrontGraphicsState>();
  return rp.isReferenced(object);
}

template <>
bool PipelineState::isReferenced(SamplerResource *object) const
{
  return isReferencedByResBinds(object);
}

template <>
bool PipelineState::isReferenced(MemoryHeapResource *object) const
{
  return object->hasPlacedResources();
}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void PipelineStateStorage::makeDirty()
{
  compute.makeDirty();
  graphics.makeDirty();
}

void PipelineStateStorage::clearDirty()
{
  compute.clearDirty();
  graphics.clearDirty();
}


bool PipelineState::processBufferDiscard(const BufferRef &old_buffer, const BufferRef &new_ref, uint32_t buf_flags)
{
  bool ret = false;
  // update object in VB set
  if (SBCF_BIND_VERTEX & buf_flags)
  {
    using VBField = StateFieldGraphicsVertexBuffers;
    using VBBind = StateFieldGraphicsVertexBufferBind;
    VBBind *vbinds = &get<VBField, VBBind, FrontGraphicsState>();

    for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
    {
      if (vbinds[i].bRef == old_buffer)
      {
        const VBBind updatedBind{new_ref, vbinds[i].offset};
        set<VBField, VBBind::Indexed, FrontGraphicsState>({i, updatedBind});
        ret |= true;
      }
    }
  }

  // update object in IB set
  if (get<StateFieldGraphicsIndexBuffer, BufferRef, FrontGraphicsState>() == old_buffer)
  {
    set<StateFieldGraphicsIndexBuffer, BufferRef, FrontGraphicsState>(new_ref);
    ret |= true;
  }

  for (uint32_t i = 0; i < STAGE_MAX_ACTIVE; ++i)
    ret |= stageResources[i]->replaceResource(old_buffer, new_ref, buf_flags);

  return ret;
}

void PipelineState::fillResBindFields()
{
  // fill up per stage res binds array
  // so we are able to access res binds via array, while keeping tracked state logic intact
  // note that field must be nested, in order for this to work properly
  stageResources[STAGE_CS] = &get<StateFieldStageResourceBinds<STAGE_CS>, StateFieldResourceBinds, FrontComputeState>();
  stageResources[STAGE_PS] = &get<StateFieldStageResourceBinds<STAGE_PS>, StateFieldResourceBinds, FrontGraphicsState>();
  stageResources[STAGE_VS] = &get<StateFieldStageResourceBinds<STAGE_VS>, StateFieldResourceBinds, FrontGraphicsState>();
}

PipelineState::PipelineState() { fillResBindFields(); }

PipelineState::PipelineState(const PipelineState &from) : TrackedState(from) { fillResBindFields(); }

void PipelineState::markResourceBindDirty(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_CS: makeFieldDirty<StateFieldStageResourceBinds<STAGE_CS>, FrontComputeState>(); break;
    case STAGE_PS: makeFieldDirty<StateFieldStageResourceBinds<STAGE_PS>, FrontGraphicsState>(); break;
    case STAGE_VS: makeFieldDirty<StateFieldStageResourceBinds<STAGE_VS>, FrontGraphicsState>(); break;
    default: G_ASSERTF(0, "vulkan: unknown stage %u to mark it dirty", stage); break;
  }
}
