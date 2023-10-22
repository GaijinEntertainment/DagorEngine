#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)

#define DEF_DESCRIPTOR_RAYTRACE                                                                    \
  DEF_DESCRIPTOR("raytrace", VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_IMAGE_VIEW_TYPE_1D, \
    spirv::T_ACCELERATION_STRUCTURE_OFFSET, false),

const bool isRaytraceAcclerationStructure(VkDescriptorType descType)
{
  return descType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
}

RaytraceBottomAccelerationStructure *d3d::create_raytrace_bottom_acceleration_structure(RaytraceGeometryDescription *desc,
  uint32_t count, RaytraceBuildFlags flags)
{
  return (RaytraceBottomAccelerationStructure *)api_state.device.createRaytraceAccelerationStructure(desc, count, flags);
}

void d3d::delete_raytrace_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *as)
{
  api_state.device.getContext().deleteRaytraceBottomAccelerationStructure(as);
}

RaytraceTopAccelerationStructure *d3d::create_raytrace_top_acceleration_structure(uint32_t elements, RaytraceBuildFlags flags)
{
  return (RaytraceTopAccelerationStructure *)api_state.device.createRaytraceAccelerationStructure(elements, flags);
}

void d3d::delete_raytrace_top_acceleration_structure(RaytraceTopAccelerationStructure *as)
{
  api_state.device.getContext().deleteRaytraceTopAccelerationStructure(as);
}

void d3d::set_top_acceleration_structure(ShaderStage stage, uint32_t index, RaytraceTopAccelerationStructure *as)
{
  PipelineState &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  auto &resBinds = pipeState.getStageResourceBinds((ShaderStage)stage);
  TRegister tReg((RaytraceAccelerationStructure *)as);
  if (resBinds.set<StateFieldTRegisterSet, StateFieldTRegister::Indexed>({index, tReg}))
    pipeState.markResourceBindDirty((ShaderStage)stage);
}

PROGRAM d3d::create_raytrace_program(const int *shaders, uint32_t shader_count, const RaytraceShaderGroup *shader_groups,
  uint32_t shader_group_count, uint32_t max_recursion_depth)
{
  G_UNUSED(shaders);
  G_UNUSED(shader_count);
  G_UNUSED(shader_groups);
  G_UNUSED(shader_group_count);
  G_UNUSED(max_recursion_depth);
  G_ASSERTF(false, "d3d::create_raytrace_program called on API without support");
  return -1;
}

void d3d::trace_rays(Sbuffer *ray_gen_table, uint32_t ray_gen_offset, Sbuffer *miss_table, uint32_t miss_offset, uint32_t miss_stride,
  Sbuffer *hit_table, uint32_t hit_offset, uint32_t hit_stride, Sbuffer *callable_table, uint32_t callable_offset,
  uint32_t callable_stride, uint32_t width, uint32_t height, uint32_t depth)
{
  G_UNUSED(ray_gen_table);
  G_UNUSED(ray_gen_offset);
  G_UNUSED(miss_table);
  G_UNUSED(miss_offset);
  G_UNUSED(miss_stride);
  G_UNUSED(hit_table);
  G_UNUSED(hit_offset);
  G_UNUSED(hit_stride);
  G_UNUSED(callable_table);
  G_UNUSED(callable_offset);
  G_UNUSED(callable_stride);
  G_UNUSED(width);
  G_UNUSED(height);
  G_UNUSED(depth);
  G_ASSERTF(false, "d3d::trace_rays called on API without support");
}

void d3d::build_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *as, RaytraceGeometryDescription *desc,
  uint32_t count, RaytraceBuildFlags flags, bool update)
{
  api_state.device.getContext().raytraceBuildBottomAccelerationStructure(as, desc, count, flags, update,
    api_state.device.getRaytraceScratchBuffer());
}

void d3d::build_top_acceleration_structure(RaytraceTopAccelerationStructure *as, Sbuffer *index_buffer, uint32_t index_count,
  RaytraceBuildFlags flags, bool update)
{
  auto vkBuf = (GenericBufferInterface *)index_buffer;
  auto deviceBuffer = vkBuf->getDeviceBuffer();
  api_state.device.getContext().raytraceBuildTopAccelerationStructure(as, deviceBuffer, index_count, flags, update,
    api_state.device.getRaytraceScratchBuffer());
}

void d3d::copy_raytrace_shader_handle_to_memory(PROGRAM prog, uint32_t first_group, uint32_t group_count, uint32_t size,
  Sbuffer *buffer, uint32_t offset)
{
  G_UNUSED(prog);
  G_UNUSED(first_group);
  G_UNUSED(group_count);
  G_UNUSED(size);
  G_UNUSED(buffer);
  G_UNUSED(offset);
  G_ASSERTF(false, "d3d::copy_raytrace_shader_handle_to_memory called on API without support");
}

void d3d::write_raytrace_index_entries_to_memory(uint32_t count, const RaytraceGeometryInstanceDescription *desc, void *ptr)
{
  G_STATIC_ASSERT(sizeof(VkAccelerationStructureInstanceKHR) == sizeof(RaytraceGeometryInstanceDescription));
  // just copy everything over, they are bit compatible (except for the struct ptr)
  memcpy(ptr, desc, count * sizeof(VkAccelerationStructureInstanceKHR));
  // now fixup the struct ptr
  auto tptr = reinterpret_cast<VkAccelerationStructureInstanceKHR *>(ptr);
  for (uint32_t i = 0; i < count; ++i)
    tptr[i].accelerationStructureReference =
      ((RaytraceAccelerationStructure *)desc[i].accelerationStructure)->getDeviceAddress(api_state.device.getVkDevice());
}

int d3d::create_raytrace_shader(RaytraceShaderType type, const uint32_t *data, uint32_t data_size)
{
  G_UNUSED(type);
  G_UNUSED(data);
  G_UNUSED(data_size);
  G_ASSERTF(false, "d3d::create_raytrace_shader called on API without support");
  return -1;
}

void d3d::delete_raytrace_shader(int shader)
{
  G_UNUSED(shader);
  G_ASSERTF(false, "d3d::delete_raytrace_shader called on API without support");
}

#else
#include "stub/interface.inc.cpp"
#endif
