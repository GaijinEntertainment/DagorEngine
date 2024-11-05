#if D3D_HAS_RAY_TRACING
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
PROGRAM d3d::create_raytrace_program(const int *, uint32_t, const RaytraceShaderGroup *, uint32_t, uint32_t) { return -1; }
int d3d::create_raytrace_shader(RaytraceShaderType, const uint32_t *, uint32_t) { return -1; }
void d3d::delete_raytrace_shader(int) {}
void d3d::trace_rays(Sbuffer *, uint32_t, Sbuffer *, uint32_t, uint32_t, Sbuffer *, uint32_t, uint32_t, Sbuffer *, uint32_t, uint32_t,
  uint32_t, uint32_t, uint32_t)
{}
void d3d::build_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *,
  const ::raytrace::BottomAccelerationStructureBuildInfo &)
{}
void d3d::build_bottom_acceleration_structures(::raytrace::BatchedBottomAccelerationStructureBuildInfo *, uint32_t) {}
void d3d::build_top_acceleration_structure(RaytraceTopAccelerationStructure *, const ::raytrace::TopAccelerationStructureBuildInfo &)
{}
void d3d::build_top_acceleration_structures(::raytrace::BatchedTopAccelerationStructureBuildInfo *, uint32_t) {}
void d3d::copy_raytrace_shader_handle_to_memory(PROGRAM, uint32_t, uint32_t, uint32_t, Sbuffer *, uint32_t) {}
void d3d::write_raytrace_index_entries_to_memory(uint32_t, const RaytraceGeometryInstanceDescription *, void *) {}
uint64_t d3d::get_raytrace_acceleration_structure_size(RaytraceAnyAccelerationStructure) { return 0; }
RaytraceAccelerationStructureGpuHandle d3d::get_raytrace_acceleration_structure_gpu_handle(RaytraceAnyAccelerationStructure)
{
  return {0};
}
void d3d::copy_raytrace_acceleration_structure(RaytraceAnyAccelerationStructure, RaytraceAnyAccelerationStructure, bool) {}
bool d3d::raytrace::check_vertex_format_support_for_acceleration_structure_build(uint32_t) { return false; }
#endif