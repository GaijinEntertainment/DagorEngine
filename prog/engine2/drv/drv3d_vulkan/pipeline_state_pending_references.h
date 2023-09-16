// storage/managment of pending references
#pragma once
#include "driver.h"
#include "image_resource.h"
#include "buffer_resource.h"
#include "render_pass_resource.h"

namespace drv3d_vulkan
{

class PipelineStatePendingReferenceList
{
  eastl::vector<Image *> images;
  eastl::vector<Buffer *> buffers;
  eastl::vector<ProgramID> progs;
  eastl::vector<RenderPassResource *> renderPasses;

  template <typename T>
  void cleanupNonUsed(const PipelineState &state)
  {
    eastl::vector<T> &ref = getArray<T>();
    if (ref.empty())
      return;

    eastl::vector<T> copy = ref;
    ref.clear();
    for (T i : copy)
      if (state.isReferenced(i))
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
  }

  void cleanupAllNonUsed(const PipelineState &state)
  {
    cleanupNonUsed<Image *>(state);
    cleanupNonUsed<Buffer *>(state);
    cleanupNonUsed<ProgramID>(state);
    cleanupNonUsed<RenderPassResource *>(state);
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
