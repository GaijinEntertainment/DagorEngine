// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// generalized delayed cleanup impl

#include <dag/dag_vector.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h> // for D3D_HAS_RAY_TRACING

#include "driver.h"
#include "buffer_resource.h"
#include "image_resource.h"
#include "sampler_resource.h"
#include "memory_heap_resource.h"
#include "async_completion_state.h"
#if D3D_HAS_RAY_TRACING
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
  template <typename T, int Tag>
  struct DelayedCleanup
  {
    T *ptr;
    static int getTag() { return Tag; }
    void frontendCleanup() { ptr->template onDelayedCleanupFrontend<Tag>(); }
    void backendCleanup() { ptr->template onDelayedCleanupBackend<Tag>(); }
    void backendFinish() { ptr->template onDelayedCleanupFinish<Tag>(); }
  };

  Tab<DelayedCleanup<Buffer, Buffer::CLEANUP_DESTROY>> bufferDestructions;
  Tab<DelayedCleanup<Image, Image::CLEANUP_DESTROY>> imageDestructions;
#if D3D_HAS_RAY_TRACING
  Tab<DelayedCleanup<RaytraceAccelerationStructure, RaytraceAccelerationStructure::CLEANUP_DESTROY_TOP>> rtASTopDestructions;
  Tab<DelayedCleanup<RaytraceAccelerationStructure, RaytraceAccelerationStructure::CLEANUP_DESTROY_BOTTOM>> rtASBottomDestructions;
#endif
  Tab<DelayedCleanup<Image, Image::CLEANUP_DELAYED_DESTROY>> imageDelayedDestructions;
  Tab<DelayedCleanup<VariatedGraphicsPipeline, VariatedGraphicsPipeline::CLEANUP_DESTROY>> variatedGrPipelineDestructions;
  Tab<DelayedCleanup<ComputePipeline, ComputePipeline::CLEANUP_DESTROY>> computePipelineDestructions;
#if D3D_HAS_RAY_TRACING
  Tab<DelayedCleanup<RaytracePipeline, RaytracePipeline::CLEANUP_DESTROY>> raytracePipelineDestructions;
#endif
  Tab<DelayedCleanup<RenderPassResource, RenderPassResource::CLEANUP_DESTROY>> renderPassDestructions;
  Tab<DelayedCleanup<MemoryHeapResource, MemoryHeapResource::CLEANUP_DESTROY>> heapDestructions;

  template <typename T>
  T getQueue();

  template <typename Cb>
  void enumerateAll(Cb cb) const
  {
    cb(bufferDestructions);
    cb(imageDestructions);
#if D3D_HAS_RAY_TRACING
    cb(rtASTopDestructions);
    cb(rtASBottomDestructions);
#endif
    cb(imageDelayedDestructions);
    cb(variatedGrPipelineDestructions);
    cb(computePipelineDestructions);
#if D3D_HAS_RAY_TRACING
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
#if D3D_HAS_RAY_TRACING
    cb(rtASTopDestructions);
    cb(rtASBottomDestructions);
#endif
    cb(imageDelayedDestructions);
    cb(variatedGrPipelineDestructions);
    cb(computePipelineDestructions);
#if D3D_HAS_RAY_TRACING
    cb(raytracePipelineDestructions);
#endif
    cb(renderPassDestructions);
    cb(heapDestructions);
  }

#if DAGOR_DBGLEVEL > 0
  std::atomic<bool> referencedByGpuWork{false};
#endif

public:
  void checkValid();

  template <int Tag, typename T>
  void enqueue(T &obj)
  {
    DelayedCleanup<T, Tag> item{&obj};
    item.frontendCleanup();
    getQueue<Tab<DelayedCleanup<T, Tag>> &>().push_back(item);
  }

  template <int Tag, typename T>
  void enqueueFromBackend(T &obj)
  {
    DelayedCleanup<T, Tag> item{&obj};
    getQueue<Tab<DelayedCleanup<T, Tag>> &>().push_back(item);
  }

  // shortcut for tag 0 aka CLEANUP_DESTROY
  template <typename T>
  void enqueue(T &obj)
  {
    enqueue<0, T>(obj);
  }

  template <typename T>
  void enqueueFromBackend(T &obj)
  {
    enqueueFromBackend<0, T>(obj);
  }

  void backendAfterReplayCleanup();
  void backendAfterGPUCleanup();
  void backendAfterFrameSubmitCleanup();

  void dumpData(FaultReportDump &dump) const;
  size_t capacity() const;
};

} // namespace drv3d_vulkan
