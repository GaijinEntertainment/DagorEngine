// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// generalized delayed cleanup impl

#include <dag/dag_vector.h>

#include "cleanup_queue_tags.h"
#include "driver.h"
#include "buffer_resource.h"
#include "image_resource.h"
#include "sampler_resource.h"
#include "memory_heap_resource.h"
#include "async_completion_state.h"
#if VULKAN_HAS_RAYTRACING
#include "raytrace_as_resource.h"
#endif
#include "render_pass_resource.h"
#include "pipeline.h"
#include "util/fault_report.h"

namespace drv3d_vulkan
{

class Buffer;
class Image;
class RenderPassResource;
class RaytraceAccelerationStructure;
class VariatedGraphicsPipeline;
class ComputePipeline;
class RaytracePipeline;

class CleanupQueue
{
  template <typename T, CleanupTag Tag>
  struct DelayedCleanup
  {
    T *ptr;
    static int getTag() { return (int)Tag; }
    void backendCleanup() { ptr->template onDelayedCleanupBackend<Tag>(); }
    void backendFinish() { ptr->template onDelayedCleanupFinish<Tag>(); }
  };

  Tab<DelayedCleanup<Buffer, CleanupTag::DESTROY>> bufferDestructions;
  Tab<DelayedCleanup<Image, CleanupTag::DESTROY>> imageDestructions;
#if VULKAN_HAS_RAYTRACING
  Tab<DelayedCleanup<RaytraceAccelerationStructure, CleanupTag::DESTROY_TOP>> rtASTopDestructions;
  Tab<DelayedCleanup<RaytraceAccelerationStructure, CleanupTag::DESTROY_BOTTOM>> rtASBottomDestructions;
#endif
  Tab<DelayedCleanup<VariatedGraphicsPipeline, CleanupTag::DESTROY>> variatedGrPipelineDestructions;
  Tab<DelayedCleanup<ComputePipeline, CleanupTag::DESTROY>> computePipelineDestructions;
#if VULKAN_HAS_RAYTRACING
  Tab<DelayedCleanup<RaytracePipeline, CleanupTag::DESTROY>> raytracePipelineDestructions;
#endif
  Tab<DelayedCleanup<RenderPassResource, CleanupTag::DESTROY>> renderPassDestructions;
  Tab<DelayedCleanup<MemoryHeapResource, CleanupTag::DESTROY>> heapDestructions;

  template <typename T>
  T getQueue();

  template <typename Cb>
  void enumerateAll(Cb cb) const
  {
    cb(bufferDestructions);
    cb(imageDestructions);
#if VULKAN_HAS_RAYTRACING
    cb(rtASTopDestructions);
    cb(rtASBottomDestructions);
#endif
    cb(variatedGrPipelineDestructions);
    cb(computePipelineDestructions);
#if VULKAN_HAS_RAYTRACING
    cb(raytracePipelineDestructions);
#endif
    cb(renderPassDestructions);
    cb(heapDestructions);
  }

  template <typename Cb>
  void visitAll(Cb cb)
  {
    cb(bufferDestructions);
    cb(imageDestructions);
#if VULKAN_HAS_RAYTRACING
    cb(rtASTopDestructions);
    cb(rtASBottomDestructions);
#endif
    cb(variatedGrPipelineDestructions);
    cb(computePipelineDestructions);
#if VULKAN_HAS_RAYTRACING
    cb(raytracePipelineDestructions);
#endif
    cb(renderPassDestructions);
    cb(heapDestructions);
  }

public:
  template <CleanupTag Tag, typename T>
  void enqueue(T &obj)
  {
    DelayedCleanup<T, Tag> item{&obj};
    getQueue<Tab<DelayedCleanup<T, Tag>> &>().push_back(item);
  }

  // shortcut for CleanupTag::DESTROY
  template <typename T>
  void enqueue(T &obj)
  {
    enqueue<CleanupTag::DESTROY, T>(obj);
  }

  void backendAfterGPUCleanup();
  void backendAfterFrameSubmitCleanup();

  void dumpData(FaultReportDump &dump) const;
  size_t capacity() const;
};

} // namespace drv3d_vulkan
