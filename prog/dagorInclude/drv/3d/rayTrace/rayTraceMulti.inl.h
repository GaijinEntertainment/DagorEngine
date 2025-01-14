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

static inline ::raytrace::Pipeline create_pipeline(const ::raytrace::PipelineCreateInfo &pci)
{
  return d3di.raytrace.create_pipeline(pci);
}

static inline ::raytrace::Pipeline expand_pipeline(const ::raytrace::Pipeline &pipeline, const ::raytrace::PipelineExpandInfo &pei)
{
  return d3di.raytrace.expand_pipeline(pipeline, pei);
}

static inline void destroy_pipeline(::raytrace::Pipeline &p) { d3di.raytrace.destroy_pipeline(p); }

static inline ::raytrace::ShaderBindingTableBufferProperties get_shader_binding_table_buffer_properties(
  const ::raytrace::ShaderBindingTableDefinition &sbtci, const ::raytrace::Pipeline &pipeline)
{
  return d3di.raytrace.get_shader_binding_table_buffer_properties(sbtci, pipeline);
}

static inline void dispatch(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
  const ::raytrace::RayDispatchParameters &rdp, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS)
{
  d3di.raytrace.dispatch(rbt, pipeline, rdp, gpu_pipeline);
}

static inline void dispatch_indirect(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
  const ::raytrace::RayDispatchIndirectParameters &rdip, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS)
{
  d3di.raytrace.dispatch_indirect(rbt, pipeline, rdip, gpu_pipeline);
}
static inline void dispatch_indirect_count(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
  const ::raytrace::RayDispatchIndirectCountParameters &rdicp, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS)
{
  d3di.raytrace.dispatch_indirect_count(rbt, pipeline, rdicp, gpu_pipeline);
}
} // namespace raytrace
// <- raytrace interface
#endif