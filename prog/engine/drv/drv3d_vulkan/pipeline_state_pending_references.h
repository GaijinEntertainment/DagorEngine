// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// storage/managment of pending references

#include "driver.h"
#include "image_resource.h"
#include "buffer_resource.h"
#include "render_pass_resource.h"
#include "sampler_resource.h"
#include "pipeline_state.h"
#include "frame_info.h"
#include "memory_heap_resource.h"

namespace drv3d_vulkan
{

class PipelineStatePendingReferenceList
{
  eastl::vector<Image *> images;
  eastl::vector<Buffer *> buffers;
  eastl::vector<ProgramID> progs;
  eastl::vector<RenderPassResource *> renderPasses;
  eastl::vector<SamplerResource *> samplers;
  eastl::vector<MemoryHeapResource *> memHeaps;

  template <typename T>
  void cleanupNonUsed(const PipelineState &front_state, const PipelineState &back_state)
  {
    eastl::vector<T> &ref = getArray<T>();
    if (ref.empty())
      return;

    eastl::vector<T> copy = ref;
    ref.clear();
    for (T i : copy)
      if (front_state.isReferenced(i) || back_state.isReferenced(i))
        ref.push_back(i);
      else
        cleanupObj<T>(i);
  }

  template <typename T>
  void shutdownArray()
  {
    eastl::vector<T> &ref = getArray<T>();
    if (ref.empty())
      return;

    for (T i : ref)
      cleanupObj<T>(i);
    ref.clear();
  }

  template <typename T>
  void cleanupObj(T obj);

public:
  template <typename T>
  eastl::vector<T> &getArray();

  void shutdown()
  {
    shutdownArray<Image *>();
    shutdownArray<Buffer *>();
    shutdownArray<ProgramID>();
    shutdownArray<RenderPassResource *>();
    shutdownArray<MemoryHeapResource *>();
  }

  void cleanupAllNonUsed(const PipelineState &front_state, const PipelineState &back_state)
  {
    cleanupNonUsed<Image *>(front_state, back_state);
    cleanupNonUsed<Buffer *>(front_state, back_state);
    cleanupNonUsed<ProgramID>(front_state, back_state);
    cleanupNonUsed<RenderPassResource *>(front_state, back_state);
    cleanupNonUsed<MemoryHeapResource *>(front_state, back_state);
  }

  template <typename T>
  bool removeWithReferenceCheck(T object, PipelineState &state)
  {
    if (state.handleObjectRemoval(object))
    {
      getArray<T>().push_back(object);
      return false;
    }
    return true;
  }

  // for objects that are surely referenced
  template <typename T>
  void removeReferenced(T object)
  {
    getArray<T>().push_back(object);
  }
};

} // namespace drv3d_vulkan
