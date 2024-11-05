// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline_state_pending_references.h"
#include "globals.h"
#include "pipeline/manager.h"
#include "backend.h"
#include "timelines.h"

namespace drv3d_vulkan
{

template <>
void PipelineStatePendingReferenceList::cleanupObj(Image *obj)
{
  FrameInfo &frame = Backend::gpuJob.get();
  frame.cleanups.enqueueFromBackend<Image::CLEANUP_DESTROY>(*obj);
}

template <>
void PipelineStatePendingReferenceList::cleanupObj(Buffer *obj)
{
  FrameInfo &frame = Backend::gpuJob.get();
  frame.cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*obj);
}

template <>
void PipelineStatePendingReferenceList::cleanupObj(ProgramID obj)
{
  Globals::pipelines.prepareRemoval(obj);
  Globals::shaderProgramDatabase.reuseId(obj);
}

template <>
void PipelineStatePendingReferenceList::cleanupObj(RenderPassResource *obj)
{
  FrameInfo &frame = Backend::gpuJob.get();
  frame.cleanups.enqueueFromBackend<RenderPassResource::CLEANUP_DESTROY>(*obj);
}

template <>
void PipelineStatePendingReferenceList::cleanupObj(MemoryHeapResource *obj)
{
  FrameInfo &frame = Backend::gpuJob.get();
  frame.cleanups.enqueueFromBackend<MemoryHeapResource::CLEANUP_DESTROY>(*obj);
}

template <>
eastl::vector<Image *> &PipelineStatePendingReferenceList::getArray()
{
  return images;
}

template <>
eastl::vector<Buffer *> &PipelineStatePendingReferenceList::getArray()
{
  return buffers;
}

template <>
eastl::vector<ProgramID> &PipelineStatePendingReferenceList::getArray()
{
  return progs;
}

template <>
eastl::vector<RenderPassResource *> &PipelineStatePendingReferenceList::getArray()
{
  return renderPasses;
}

template <>
eastl::vector<SamplerResource *> &PipelineStatePendingReferenceList::getArray()
{
  return samplers;
}

template <>
eastl::vector<MemoryHeapResource *> &PipelineStatePendingReferenceList::getArray()
{
  return memHeaps;
}

} // namespace drv3d_vulkan
