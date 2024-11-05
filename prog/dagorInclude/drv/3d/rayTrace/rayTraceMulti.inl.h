#pragma once
#if D3D_HAS_RAY_TRACING
// raytrace interface ->
static inline RaytraceBottomAccelerationStructure *create_raytrace_bottom_acceleration_structure(RaytraceGeometryDescription *desc,
  uint32_t count, RaytraceBuildFlags flags, uint32_t &build_scratch_size_in_bytes, uint32_t *update_scratch_size_in_bytes)
{
  return d3di.create_raytrace_bottom_acceleration_structure_0(desc, count, flags, build_scratch_size_in_bytes,
    update_scratch_size_in_bytes);
}
static inline RaytraceBottomAccelerationStructure *create_raytrace_bottom_acceleration_structure(uint32_t size)
{
  return d3di.create_raytrace_bottom_acceleration_structure_1(size);
}
static inline void delete_raytrace_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *as)
{
  return d3di.delete_raytrace_bottom_acceleration_structure(as);
}
static inline RaytraceTopAccelerationStructure *create_raytrace_top_acceleration_structure(uint32_t elements, RaytraceBuildFlags flags,
  uint32_t &build_scratch_size_in_bytes, uint32_t *update_scratch_size_in_bytes)
{
  return d3di.create_raytrace_top_acceleration_structure(elements, flags, build_scratch_size_in_bytes, update_scratch_size_in_bytes);
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
static inline void build_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *as,
  const ::raytrace::BottomAccelerationStructureBuildInfo &basbi)
{
  d3di.build_bottom_acceleration_structure(as, basbi);
}
static inline void build_bottom_acceleration_structures(::raytrace::BatchedBottomAccelerationStructureBuildInfo *as_array,
  uint32_t as_count)
{
  d3di.build_bottom_acceleration_structures(as_array, as_count);
}
static inline void build_top_acceleration_structure(RaytraceTopAccelerationStructure *as,
  const ::raytrace::TopAccelerationStructureBuildInfo &tasbi)
{
  d3di.build_top_acceleration_structure(as, tasbi);
}
static inline void build_top_acceleration_structures(::raytrace::BatchedTopAccelerationStructureBuildInfo *as_array, uint32_t as_count)
{
  d3di.build_top_acceleration_structures(as_array, as_count);
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
static inline uint64_t get_raytrace_acceleration_structure_size(RaytraceAnyAccelerationStructure as)
{
  return d3di.get_raytrace_acceleration_structure_size(as);
}
static inline RaytraceAccelerationStructureGpuHandle get_raytrace_acceleration_structure_gpu_handle(
  RaytraceAnyAccelerationStructure as)
{
  return d3di.get_raytrace_acceleration_structure_gpu_handle(as);
}
static inline void copy_raytrace_acceleration_structure(RaytraceAnyAccelerationStructure dst, RaytraceAnyAccelerationStructure src,
  bool compact = false)
{
  return d3di.copy_raytrace_acceleration_structure(dst, src, compact);
}

namespace raytrace
{
static inline bool check_vertex_format_support_for_acceleration_structure_build(uint32_t format)
{
  return d3di.raytrace.check_vertex_format_support_for_acceleration_structure_build(format);
}
} // namespace raytrace
// <- raytrace interface
#endif