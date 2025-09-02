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
  frame.cleanups.enqueue(*obj);
}

template <>
void PipelineStatePendingReferenceList::cleanupObj(Buffer *obj)
{
  FrameInfo &frame = Backend::gpuJob.get();
  frame.cleanups.enqueue(*obj);
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
  frame.cleanups.enqueue(*obj);
}

template <>
void PipelineStatePendingReferenceList::cleanupObj(MemoryHeapResource *obj)
{
  FrameInfo &frame = Backend::gpuJob.get();
  frame.cleanups.enqueue(*obj);
}

#if VULKAN_HAS_RAYTRACING
template <>
void PipelineStatePendingReferenceList::cleanupObj(RaytraceAccelerationStructure *obj)
{
  FrameInfo &frame = Backend::gpuJob.get();
  frame.cleanups.enqueue<CleanupTag::DESTROY_TOP>(*obj);
}
#endif

template <>
dag::Vector<Image *> &PipelineStatePendingReferenceList::getArray()
{
  return images;
}

template <>
dag::Vector<Buffer *> &PipelineStatePendingReferenceList::getArray()
{
  return buffers;
}

template <>
dag::Vector<ProgramID> &PipelineStatePendingReferenceList::getArray()
{
  return progs;
}

template <>
dag::Vector<RenderPassResource *> &PipelineStatePendingReferenceList::getArray()
{
  return renderPasses;
}

template <>
dag::Vector<SamplerResource *> &PipelineStatePendingReferenceList::getArray()
{
  return samplers;
}

template <>
dag::Vector<MemoryHeapResource *> &PipelineStatePendingReferenceList::getArray()
{
  return memHeaps;
}

#if VULKAN_HAS_RAYTRACING
template <>
dag::Vector<RaytraceAccelerationStructure *> &PipelineStatePendingReferenceList::getArray()
{
  return accelerationStructures;
}
#endif

} // namespace drv3d_vulkan
