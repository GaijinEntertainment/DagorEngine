#include <3d/dag_drv3d.h>

#if APPLE_RT_SUPPORTED

#include "render.h"
#include "acceleration_structure.h"
#include "acceleration_structure_desc.h"

void d3d::set_top_acceleration_structure(ShaderStage stage, uint32_t index, RaytraceTopAccelerationStructure *topAccStruct)
{
  drv3d_metal::render.scheduleTLASSet(topAccStruct, stage, index);
}

void d3d::delete_raytrace_top_acceleration_structure(RaytraceTopAccelerationStructure *topAccStruct)
{
  drv3d_metal::render.deleteTLAS(topAccStruct);
}

void d3d::delete_raytrace_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *botAccStruct)
{
  drv3d_metal::render.deleteBLAS(botAccStruct);
};

RaytraceBottomAccelerationStructure *d3d::create_raytrace_bottom_acceleration_structure(RaytraceGeometryDescription *desc, uint32_t count,
  RaytraceBuildFlags flags)
{
  G_UNUSED(desc);
  G_UNUSED(count);
  return drv3d_metal::render.createBottomAccelerationStructure(flags);
}

void d3d::build_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *as, RaytraceGeometryDescription *desc,
  uint32_t count, RaytraceBuildFlags flags, bool update)
{
  MTLPrimitiveAccelerationStructureDescriptor *accDesc = acceleration_structure_descriptors::getBLASDescriptor(desc, count, as->createFlags, flags, update);
  drv3d_metal::render.scheduleBLASBuild(as, accDesc, update);
}

void d3d::write_raytrace_index_entries_to_memory(uint32_t count, const RaytraceGeometryInstanceDescription *desc, void *ptr)
{
  auto instanceDescriptors = reinterpret_cast<MTLAccelerationStructureInstanceDescriptor *>(ptr);

  for (int i = 0; i < count; i++)
  {
    instanceDescriptors[i].accelerationStructureIndex = desc[i].accelerationStructure->index;

    instanceDescriptors[i].options = desc[i].flags;

    instanceDescriptors[i].intersectionFunctionTableOffset = 0;
    instanceDescriptors[i].mask = desc[i].mask;

    for (int row = 0; row < 3; row++)
    {
      for (int column = 0; column < 4; column++)
      {
        instanceDescriptors[i].transformationMatrix.columns[column][row] = desc[i].transform[4*row + column];
      }
    }
  }
}

RaytraceTopAccelerationStructure *d3d::create_raytrace_top_acceleration_structure(uint32_t elements, RaytraceBuildFlags flags)
{
  G_UNUSED(elements);
  return drv3d_metal::render.createTopAccelerationStructure(flags);
}

void d3d::build_top_acceleration_structure(RaytraceTopAccelerationStructure *as, Sbuffer *instance_descriptor_buffer, uint32_t instance_count,
  RaytraceBuildFlags flags, bool update)
{
  MTLInstanceAccelerationStructureDescriptor *accDesc = acceleration_structure_descriptors::getTLASDescriptor(instance_descriptor_buffer, instance_count, as->createFlags, flags, update);
  drv3d_metal::render.scheduleTLASBuild(as, accDesc, update);
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

#include "stub/interface.inc.mm"

#endif //RT_SUPPORTED
