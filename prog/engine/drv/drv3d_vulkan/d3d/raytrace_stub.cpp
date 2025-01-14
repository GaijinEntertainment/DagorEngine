// Copyright (C) Gaijin Games KFT.  All rights reserved.

// raytrace features implementation
#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include "vulkan_api.h"
#include <drv/3d/dag_driver.h>

#define DEF_DESCRIPTOR_RAYTRACE

const bool isRaytraceAcclerationStructure(VkDescriptorType) { return false; }

#ifdef D3D_HAS_RAY_TRACING

RaytraceBottomAccelerationStructure *d3d::create_raytrace_bottom_acceleration_structure(RaytraceGeometryDescription *, uint32_t,
  RaytraceBuildFlags, uint32_t &, uint32_t *)
{
  return nullptr;
}

RaytraceBottomAccelerationStructure *d3d::create_raytrace_bottom_acceleration_structure(uint32_t) { return nullptr; }

void d3d::delete_raytrace_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *) {}

RaytraceTopAccelerationStructure *d3d::create_raytrace_top_acceleration_structure(uint32_t, RaytraceBuildFlags, uint32_t &, uint32_t *)
{
  return nullptr;
}

void d3d::delete_raytrace_top_acceleration_structure(RaytraceTopAccelerationStructure *) {}

void d3d::set_top_acceleration_structure(ShaderStage, uint32_t, RaytraceTopAccelerationStructure *) {}

void d3d::build_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *,
  const ::raytrace::BottomAccelerationStructureBuildInfo &)
{}

void d3d::build_bottom_acceleration_structures(::raytrace::BatchedBottomAccelerationStructureBuildInfo *, uint32_t) {}

void d3d::build_top_acceleration_structure(RaytraceTopAccelerationStructure *, const ::raytrace::TopAccelerationStructureBuildInfo &)
{}
void d3d::write_raytrace_index_entries_to_memory(uint32_t, const RaytraceGeometryInstanceDescription *, void *) {}
uint64_t d3d::get_raytrace_acceleration_structure_size(RaytraceAnyAccelerationStructure) { return 0; }
RaytraceAccelerationStructureGpuHandle d3d::get_raytrace_acceleration_structure_gpu_handle(RaytraceAnyAccelerationStructure)
{
  return {};
}

bool d3d::raytrace::check_vertex_format_support_for_acceleration_structure_build(uint32_t) { return false; }

::raytrace::Pipeline d3d::raytrace::create_pipeline(const ::raytrace::PipelineCreateInfo &) { return ::raytrace::InvalidPipeline; }
::raytrace::Pipeline d3d::raytrace::expand_pipeline(const ::raytrace::Pipeline &, const ::raytrace::PipelineExpandInfo &)
{
  return ::raytrace::InvalidPipeline;
}
void d3d::raytrace::destroy_pipeline(::raytrace::Pipeline &) {}
::raytrace::ShaderBindingTableBufferProperties d3d::raytrace::get_shader_binding_table_buffer_properties(
  const ::raytrace::ShaderBindingTableDefinition &, const ::raytrace::Pipeline &)
{
  return {};
}

void d3d::raytrace::dispatch(const ::raytrace::ResourceBindingTable &, const ::raytrace::Pipeline &,
  const ::raytrace::RayDispatchParameters &, GpuPipeline)
{}

void d3d::raytrace::dispatch_indirect(const ::raytrace::ResourceBindingTable &, const ::raytrace::Pipeline &,
  const ::raytrace::RayDispatchIndirectParameters &, GpuPipeline)
{}

void d3d::raytrace::dispatch_indirect_count(const ::raytrace::ResourceBindingTable &, const ::raytrace::Pipeline &,
  const ::raytrace::RayDispatchIndirectCountParameters &, GpuPipeline)
{}

#endif // D3D_HAS_RAY_TRACING
