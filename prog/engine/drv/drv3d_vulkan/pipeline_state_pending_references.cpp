#include "device.h"
#include "pipeline_state_pending_references.h"

namespace drv3d_vulkan
{

template <>
void PipelineStatePendingReferenceList::cleanupObj(Image *obj)
{
  FrameInfo &frame = get_device().getContext().getBackend().contextState.frame.get();
  frame.cleanups.enqueueFromBackend<Image::CLEANUP_DESTROY>(*obj);
}

template <>
void PipelineStatePendingReferenceList::cleanupObj(Buffer *obj)
{
  FrameInfo &frame = get_device().getContext().getBackend().contextState.frame.get();
  frame.cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*obj);
}

template <>
void PipelineStatePendingReferenceList::cleanupObj(ProgramID obj)
{
  get_device().pipeMan.prepareRemoval(obj);
  get_shader_program_database().reuseId(obj);
}

template <>
void PipelineStatePendingReferenceList::cleanupObj(RenderPassResource *obj)
{
  FrameInfo &frame = get_device().getContext().getBackend().contextState.frame.get();
  frame.cleanups.enqueueFromBackend<RenderPassResource::CLEANUP_DESTROY>(*obj);
}

template <>
void PipelineStatePendingReferenceList::cleanupObj(SamplerResource *obj)
{
  FrameInfo &frame = get_device().getContext().getBackend().contextState.frame.get();
  frame.cleanups.enqueueFromBackend<SamplerResource::CLEANUP_DESTROY>(*obj);
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

} // namespace drv3d_vulkan
