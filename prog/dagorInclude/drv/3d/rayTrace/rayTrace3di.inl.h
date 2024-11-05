#pragma once
#if D3D_HAS_RAY_TRACING
RaytraceBottomAccelerationStructure *(*create_raytrace_bottom_acceleration_structure_0)(RaytraceGeometryDescription *desc,
  uint32_t count, RaytraceBuildFlags flags, uint32_t &build_scratch_size_in_bytes, uint32_t *update_scratch_size_in_bytes);
RaytraceBottomAccelerationStructure *(*create_raytrace_bottom_acceleration_structure_1)(uint32_t size);
void (*delete_raytrace_bottom_acceleration_structure)(RaytraceBottomAccelerationStructure *as);
RaytraceTopAccelerationStructure *(*create_raytrace_top_acceleration_structure)(uint32_t elements, RaytraceBuildFlags flags,
  uint32_t &build_scratch_size_in_bytes, uint32_t *update_scratch_size_in_bytes);
void (*delete_raytrace_top_acceleration_structure)(RaytraceTopAccelerationStructure *as);
void (*set_top_acceleration_structure)(ShaderStage stage, uint32_t index, RaytraceTopAccelerationStructure *as);
PROGRAM(*create_raytrace_program)
(const int *shaders, uint32_t shader_count, const RaytraceShaderGroup *shader_groups, uint32_t shader_group_count,
  uint32_t max_recursion_depth);
int (*create_raytrace_shader)(RaytraceShaderType stage, const uint32_t *data, uint32_t data_size);
void (*delete_raytrace_shader)(int shader);
void (*trace_rays)(Sbuffer *ray_gen_table, uint32_t ray_gen_offset, Sbuffer *miss_table, uint32_t miss_offset, uint32_t miss_stride,
  Sbuffer *hit_table, uint32_t hit_offset, uint32_t hit_stride, Sbuffer *callable_table, uint32_t callable_offset,
  uint32_t callable_stride, uint32_t width, uint32_t height, uint32_t depth);
void (*build_bottom_acceleration_structure)(RaytraceBottomAccelerationStructure *as,
  const raytrace::BottomAccelerationStructureBuildInfo &basbi);
void (*build_bottom_acceleration_structures)(::raytrace::BatchedBottomAccelerationStructureBuildInfo *as_array, uint32_t as_count);
void (*build_top_acceleration_structure)(RaytraceTopAccelerationStructure *as,
  const ::raytrace::TopAccelerationStructureBuildInfo &tasbi);
void (*build_top_acceleration_structures)(::raytrace::BatchedTopAccelerationStructureBuildInfo *as_array, uint32_t as_count);
void (*copy_raytrace_shader_handle_to_memory)(PROGRAM prog, uint32_t first_group, uint32_t group_count, uint32_t size, Sbuffer *buffer,
  uint32_t offset);
void (*write_raytrace_index_entries_to_memory)(uint32_t count, const RaytraceGeometryInstanceDescription *desc, void *ptr);
uint64_t (*get_raytrace_acceleration_structure_size)(RaytraceAnyAccelerationStructure as);
RaytraceAccelerationStructureGpuHandle (*get_raytrace_acceleration_structure_gpu_handle)(RaytraceAnyAccelerationStructure as);
void (*copy_raytrace_acceleration_structure)(RaytraceAnyAccelerationStructure dst, RaytraceAnyAccelerationStructure src, bool compact);

struct
{
  bool (*check_vertex_format_support_for_acceleration_structure_build)(uint32_t format);
} raytrace;
#endif
