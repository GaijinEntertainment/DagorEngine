#pragma once
#if D3D_HAS_RAY_TRACING
// raytrace interface ->
static inline RaytraceBottomAccelerationStructure *create_raytrace_bottom_acceleration_structure(RaytraceGeometryDescription *desc,
  uint32_t count, RaytraceBuildFlags flags)
{
  return d3di.create_raytrace_bottom_acceleration_structure(desc, count, flags);
}
static inline void delete_raytrace_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *as)
{
  return d3di.delete_raytrace_bottom_acceleration_structure(as);
}
static inline RaytraceTopAccelerationStructure *create_raytrace_top_acceleration_structure(uint32_t elements, RaytraceBuildFlags flags)
{
  return d3di.create_raytrace_top_acceleration_structure(elements, flags);
}
static inline void delete_raytrace_top_acceleration_structure(RaytraceTopAccelerationStructure *as)
{
  d3di.delete_raytrace_top_acceleration_structure(as);
}
static inline void set_top_acceleration_structure(ShaderStage stage, uint32_t index, RaytraceTopAccelerationStructure *as)
{
  d3di.set_top_acceleration_structure(stage, index, as);
}
static inline PROGRAM create_raytrace_program(const int *shaders, uint32_t shader_count, const RaytraceShaderGroup *shader_groups,
  uint32_t shader_group_count, uint32_t max_recursion_depth)
{
  return d3di.create_raytrace_program(shaders, shader_count, shader_groups, shader_group_count, max_recursion_depth);
}
static inline int create_raytrace_shader(RaytraceShaderType stage, const uint32_t *data, uint32_t data_size)
{
  return d3di.create_raytrace_shader(stage, data, data_size);
}
static inline void delete_raytrace_shader(int shader) { d3di.delete_raytrace_shader(shader); }
static inline void trace_rays(Sbuffer *ray_gen_table, uint32_t ray_gen_offset, Sbuffer *miss_table, uint32_t miss_offset,
  uint32_t miss_stride, Sbuffer *hit_table, uint32_t hit_offset, uint32_t hit_stride, Sbuffer *callable_table,
  uint32_t callable_offset, uint32_t callable_stride, uint32_t width, uint32_t height, uint32_t depth)
{
  d3di.trace_rays(ray_gen_table, ray_gen_offset, miss_table, miss_offset, miss_stride, hit_table, hit_offset, hit_stride,
    callable_table, callable_offset, callable_stride, width, height, depth);
}
static inline void build_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *as, RaytraceGeometryDescription *desc,
  uint32_t count, RaytraceBuildFlags flags, bool update)
{
  d3di.build_bottom_acceleration_structure(as, desc, count, flags, update);
}
static inline void build_top_acceleration_structure(RaytraceTopAccelerationStructure *as, Sbuffer *index_buffer, uint32_t index_count,
  RaytraceBuildFlags flags, bool update)
{
  d3di.build_top_acceleration_structure(as, index_buffer, index_count, flags, update);
}
static inline void copy_raytrace_shader_handle_to_memory(PROGRAM prog, uint32_t first_group, uint32_t group_count, uint32_t size,
  Sbuffer *buffer, uint32_t offset)
{
  d3di.copy_raytrace_shader_handle_to_memory(prog, first_group, group_count, size, buffer, offset);
}
static inline void write_raytrace_index_entries_to_memory(uint32_t count, const RaytraceGeometryInstanceDescription *desc, void *ptr)
{
  d3di.write_raytrace_index_entries_to_memory(count, desc, ptr);
}
// <- raytrace interface
#endif