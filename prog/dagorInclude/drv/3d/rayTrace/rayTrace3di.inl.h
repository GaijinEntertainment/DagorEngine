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
void (*build_bottom_acceleration_structure)(RaytraceBottomAccelerationStructure *as,
  const raytrace::BottomAccelerationStructureBuildInfo &basbi);
void (*build_bottom_acceleration_structures)(::raytrace::BatchedBottomAccelerationStructureBuildInfo *as_array, uint32_t as_count);
void (*build_top_acceleration_structure)(RaytraceTopAccelerationStructure *as,
  const ::raytrace::TopAccelerationStructureBuildInfo &tasbi);
void (*build_top_acceleration_structures)(::raytrace::BatchedTopAccelerationStructureBuildInfo *as_array, uint32_t as_count);
void (*write_raytrace_index_entries_to_memory)(uint32_t count, const RaytraceGeometryInstanceDescription *desc, void *ptr);
uint64_t (*get_raytrace_acceleration_structure_size)(RaytraceAnyAccelerationStructure as);
RaytraceAccelerationStructureGpuHandle (*get_raytrace_acceleration_structure_gpu_handle)(RaytraceAnyAccelerationStructure as);
void (*copy_raytrace_acceleration_structure)(RaytraceAnyAccelerationStructure dst, RaytraceAnyAccelerationStructure src, bool compact);

struct
{
  bool (*check_vertex_format_support_for_acceleration_structure_build)(uint32_t format);
  raytrace::Pipeline (*create_pipeline)(const raytrace::PipelineCreateInfo &pci);
  raytrace::Pipeline (*expand_pipeline)(const raytrace::Pipeline &pipeline, const raytrace::PipelineExpandInfo &pei);
  void (*destroy_pipeline)(raytrace::Pipeline &p);
  raytrace::ShaderBindingTableBufferProperties (*get_shader_binding_table_buffer_properties)(
    const raytrace::ShaderBindingTableDefinition &sbtci, const raytrace::Pipeline &pipeline);
  void (*dispatch)(const raytrace::ResourceBindingTable &rbt, const raytrace::Pipeline &pipeline,
    const raytrace::RayDispatchParameters &rdp, GpuPipeline gpu_pipeline);
  void (*dispatch_indirect)(const raytrace::ResourceBindingTable &rbt, const raytrace::Pipeline &pipeline,
    const raytrace::RayDispatchIndirectParameters &rdip, GpuPipeline gpu_pipeline);
  void (*dispatch_indirect_count)(const raytrace::ResourceBindingTable &rbt, const raytrace::Pipeline &pipeline,
    const raytrace::RayDispatchIndirectCountParameters &rdicp, GpuPipeline gpu_pipeline);
} raytrace;
#endif
