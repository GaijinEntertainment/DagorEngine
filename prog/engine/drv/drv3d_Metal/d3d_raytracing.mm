// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_driver.h>

#include "render.h"
#include "drv_assert_defs.h"
#include "acceleration_structure_desc.h"

uint64_t d3d::get_raytrace_acceleration_structure_size(RaytraceAnyAccelerationStructure as)
{
  RaytraceAccelerationStructure *as_struct = as.top ? reinterpret_cast<RaytraceAccelerationStructure *>(as.top) :
    reinterpret_cast<RaytraceAccelerationStructure *>(as.bottom);
  if (as_struct)
  {
    G_ASSERT(as_struct->acceleration_struct);
    return as_struct->acceleration_struct.size;
  }
  return 0;
}

static uint32_t getASSizeData(MTLAccelerationStructureDescriptor *desc, uint32_t &build_scratch_size_in_bytes,
  uint32_t *update_scratch_size_in_bytes)
{
  MTLAccelerationStructureSizes sizes = [drv3d_metal::render.device accelerationStructureSizesWithDescriptor:desc];
  build_scratch_size_in_bytes = sizes.buildScratchBufferSize;

  if (update_scratch_size_in_bytes)
  {
    *update_scratch_size_in_bytes = sizes.refitScratchBufferSize;
  }

  return sizes.accelerationStructureSize;
}

RaytraceBottomAccelerationStructure *d3d::create_raytrace_bottom_acceleration_structure(RaytraceGeometryDescription *desc, uint32_t count,
  RaytraceBuildFlags flags, uint32_t &build_scratch_size_in_bytes, uint32_t *update_scratch_size_in_bytes)
{
  MTLPrimitiveAccelerationStructureDescriptor *accDesc = acceleration_structure_descriptors::getBLASDescriptor(desc, count, flags);
  uint32_t size = getASSizeData(accDesc, build_scratch_size_in_bytes, update_scratch_size_in_bytes);

  return (RaytraceBottomAccelerationStructure *)drv3d_metal::render.createAccelerationStructure(flags, size, true);
}

RaytraceBottomAccelerationStructure *d3d::create_raytrace_bottom_acceleration_structure(uint32_t size)
{
  G_UNUSED(size);
  D3D_CONTRACT_ASSERT_FAIL("Creating arbitrary sized BLAS-es is not yet supported on Metal.");
  return nullptr;
}

void d3d::delete_raytrace_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *botAccStruct)
{
  D3D_CONTRACT_ASSERT(botAccStruct->index >= 0);
  drv3d_metal::render.deleteAccelerationStructure(botAccStruct);
};

void d3d::build_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *as,
  const ::raytrace::BottomAccelerationStructureBuildInfo &basbi)
{
  D3D_CONTRACT_ASSERTF(!basbi.compactedSizeOutputBuffer, "Calculating compacted_size is not yet implemented for Metal.");
  drv3d_metal::render.buildBLAS(as, basbi);
}

void d3d::build_bottom_acceleration_structures(::raytrace::BatchedBottomAccelerationStructureBuildInfo* as_array, uint32_t as_count)
{
  for (uint32_t ix = 0; ix < as_count; ++ix)
  {
    D3D_CONTRACT_ASSERTF_RETURN(as_array[ix].basbi.scratchSpaceBuffer, , "This API requires providing a scatch buffer for the AS builds");
  }

  for (uint32_t ix = 0; ix < as_count; ++ix)
    build_bottom_acceleration_structure(as_array[ix].as, as_array[ix].basbi);
}

void d3d::write_raytrace_index_entries_to_memory(uint32_t count, const RaytraceGeometryInstanceDescription *desc, void *ptr)
{
  if (@available(iOS 15, macOS 12.0, *))
  {
    auto instanceDescriptors = reinterpret_cast<MTLAccelerationStructureUserIDInstanceDescriptor *>(ptr);

    for (int i = 0; i < count; i++)
    {
      instanceDescriptors[i].accelerationStructureIndex = desc[i].accelerationStructure->index;

      instanceDescriptors[i].options = desc[i].flags;

      instanceDescriptors[i].intersectionFunctionTableOffset = 0;
      instanceDescriptors[i].mask = desc[i].mask;

      instanceDescriptors[i].userID = desc[i].instanceId;

      for (int row = 0; row < 3; row++)
      {
        for (int column = 0; column < 4; column++)
        {
          instanceDescriptors[i].transformationMatrix.columns[column][row] = desc[i].transform[4*row + column];
        }
      }
    }
  }
}

RaytraceTopAccelerationStructure *d3d::create_raytrace_top_acceleration_structure(uint32_t elements, RaytraceBuildFlags flags,
  uint32_t &build_scratch_size_in_bytes, uint32_t *update_scratch_size_in_bytes)
{
  MTLInstanceAccelerationStructureDescriptor *accDesc = acceleration_structure_descriptors::getTLASDescriptor(elements, flags);
  uint32_t size = getASSizeData(accDesc, build_scratch_size_in_bytes, update_scratch_size_in_bytes);
  return (RaytraceTopAccelerationStructure *)drv3d_metal::render.createAccelerationStructure(flags, size, false);
}

void d3d::set_top_acceleration_structure(ShaderStage stage, uint32_t index, RaytraceTopAccelerationStructure *topAccStruct)
{
  drv3d_metal::render.setTLAS(topAccStruct, stage, index);
}

void d3d::delete_raytrace_top_acceleration_structure(RaytraceTopAccelerationStructure *topAccStruct)
{
  D3D_CONTRACT_ASSERT(topAccStruct->index == -1);
  drv3d_metal::render.deleteAccelerationStructure(topAccStruct);
}

void d3d::build_top_acceleration_structure(RaytraceTopAccelerationStructure *as, const ::raytrace::TopAccelerationStructureBuildInfo &tasbi)
{
  drv3d_metal::render.buildTLAS(as, tasbi);
}

void d3d::build_top_acceleration_structures(::raytrace::BatchedTopAccelerationStructureBuildInfo *as_array, uint32_t as_count)
{
  for (uint32_t ix = 0; ix < as_count; ++ix)
  {
    D3D_CONTRACT_ASSERTF_RETURN(as_array[ix].tasbi.scratchSpaceBuffer, , "This API requires providing a scatch buffer for the AS builds");
  }

  for (uint32_t ix = 0; ix < as_count; ++ix)
    build_top_acceleration_structure(as_array[ix].as, as_array[ix].tasbi);
}

RaytraceAccelerationStructureGpuHandle d3d::get_raytrace_acceleration_structure_gpu_handle(RaytraceAnyAccelerationStructure as)
{
  RaytraceAccelerationStructure *as_struct = as.top ? reinterpret_cast<RaytraceAccelerationStructure *>(as.top) :
    reinterpret_cast<RaytraceAccelerationStructure *>(as.bottom);
  if (as_struct)
    return {as_struct->index == -1 ? 0ull : as_struct->index};
  return {0};
}

void d3d::copy_raytrace_acceleration_structure(RaytraceAnyAccelerationStructure dst, RaytraceAnyAccelerationStructure src, bool compact)
{
  D3D_CONTRACT_ASSERT_RETURN((dst.bottom && src.bottom) || (dst.top && src.top), );
  // D3D_CONTRACT_ASSERT_RETURN(!compact || (dst.bottom && src.bottom), );
  D3D_CONTRACT_ASSERT_RETURN(!compact, );

  auto rsrc = src.top ? reinterpret_cast<RaytraceAccelerationStructure *>(src.top)
                      : reinterpret_cast<RaytraceAccelerationStructure *>(src.bottom);
  auto rdst = dst.top ? reinterpret_cast<RaytraceAccelerationStructure *>(dst.top)
                      : reinterpret_cast<RaytraceAccelerationStructure *>(dst.bottom);

  D3D_CONTRACT_ASSERT(rsrc);
  D3D_CONTRACT_ASSERT(rdst);
  D3D_CONTRACT_ASSERT(rsrc->acceleration_struct);
  D3D_CONTRACT_ASSERT(rdst->acceleration_struct);
  drv3d_metal::render.copyAccelerationStruct(rdst, rsrc);
}

bool d3d::raytrace::check_vertex_format_support_for_acceleration_structure_build(uint32_t format)
{
  switch (format)
  {
    // currently only implements translation of those types
    case VSDT_FLOAT1:
    case VSDT_FLOAT2:
    case VSDT_FLOAT3:
    case VSDT_FLOAT4:
      return true;
    case VSDT_HALF2:
    case VSDT_HALF4:
    case VSDT_SHORT2N:
    case VSDT_SHORT4N:
    case VSDT_USHORT4N:
    case VSDT_USHORT2N:
    case VSDT_DEC3N:
    case VSDT_E3DCOLOR:
    case VSDT_UBYTE4:
    case VSDT_SHORT2:
    case VSDT_SHORT4:
    case VSDT_UDEC3:
    case VSDT_INT1:
    case VSDT_INT2:
    case VSDT_INT3:
    case VSDT_INT4:
    case VSDT_UINT1:
    case VSDT_UINT2:
    case VSDT_UINT3:
    case VSDT_UINT4:
    default: return false;
  }
}

::raytrace::AccelerationStructurePool d3d::raytrace::create_acceleration_structure_pool(
  const ::raytrace::AccelerationStructurePoolCreateInfo &)
{
  return ::raytrace::InvalidAccelerationStructurePool;
}

void d3d::raytrace::destroy_acceleration_structure_pool(::raytrace::AccelerationStructurePool pool)
{
  G_UNUSED(pool);
  D3D_CONTRACT_ASSERT(::raytrace::InvalidAccelerationStructurePool == pool);
}

RaytraceAccelerationStructureGpuHandle d3d::raytrace::get_pool_base_address(::raytrace::AccelerationStructurePool) { return {0}; }

namespace
{
::raytrace::AnyAccelerationStructure create_as(const ::raytrace::BottomAccelerationStructurePlacementInfo &info)
{
  // TODO: Flags of the AS are never used so we can pass none flags without any effects
  return (RaytraceBottomAccelerationStructure *)drv3d_metal::render.createAccelerationStructure(RaytraceBuildFlags::NONE,
    info.sizeInBytes, true);
}

::raytrace::AnyAccelerationStructure create_as(const ::raytrace::TopAccelerationStructurePlacementInfo &info)
{
  // TODO: Flags of the AS are never used so we can pass none flags without any effects
  return (RaytraceTopAccelerationStructure *)drv3d_metal::render.createAccelerationStructure(RaytraceBuildFlags::NONE,
    info.sizeInBytes, false);
}

::raytrace::AccelerationStructureSizes calculate_as_sizes(const ::raytrace::TopAccelerationStructureSizeCalculcationInfo &info)
{
  ::raytrace::AccelerationStructureSizes result;

  if (@available(macOS 11.0, iOS 15.0, *))
  {
    MTLInstanceAccelerationStructureDescriptor *accDesc =
      acceleration_structure_descriptors::getTLASDescriptor(info.elementCount, info.flags);
    result.structureSizeInBytes = getASSizeData(accDesc, result.buildScratchBufferSizeInBytes, &result.updateScratchBufferSizeInBytes);
  }

  return result;
}

::raytrace::AccelerationStructureSizes calculate_as_sizes(const ::raytrace::BottomAccelerationStructureSizeCalculcationInfo &info)
{
  ::raytrace::AccelerationStructureSizes result;

  if (@available(macOS 11.0, iOS 15.0, *))
  {
    MTLPrimitiveAccelerationStructureDescriptor *accDesc =
      acceleration_structure_descriptors::getBLASDescriptor(info.geometryDesc.data(), info.geometryDesc.size(), info.flags);
    result.structureSizeInBytes = getASSizeData(accDesc, result.buildScratchBufferSizeInBytes, &result.updateScratchBufferSizeInBytes);
  }

  return result;
}
} // namespace

::raytrace::AccelerationStructureSizes d3d::raytrace::calculate_acceleration_structure_sizes(
  const ::raytrace::AccelerationStructureSizeCalculcationInfo &info)
{
  return eastl::visit([](auto &i) { return calculate_as_sizes(i); }, info);
}

::raytrace::AnyAccelerationStructure d3d::raytrace::create_acceleration_structure(::raytrace::AccelerationStructurePool pool,
  const ::raytrace::AccelerationStructurePlacementInfo &placement_info)
{
  D3D_CONTRACT_ASSERT(::raytrace::InvalidAccelerationStructurePool == pool);
  if (::raytrace::InvalidAccelerationStructurePool == pool)
  {
    return eastl::visit([](auto &info) { return create_as(info); }, placement_info);
  }
  return {};
}

void d3d::raytrace::destroy_acceleration_structure(::raytrace::AccelerationStructurePool pool,
  ::raytrace::AnyAccelerationStructure structure)
{
  D3D_CONTRACT_ASSERT(::raytrace::InvalidAccelerationStructurePool == pool);
  if (::raytrace::InvalidAccelerationStructurePool == pool)
  {
    if (structure.top)
    {
      D3D_CONTRACT_ASSERT(structure.top->index == -1);
      drv3d_metal::render.deleteAccelerationStructure(structure.top);
    }
    else if (structure.bottom)
    {
      D3D_CONTRACT_ASSERT(structure.bottom->index >= 0);
      drv3d_metal::render.deleteAccelerationStructure(structure.bottom);
    }
  }
}

void d3d::raytrace::build_acceleration_structure(::raytrace::AccelerationStructureBuildParameters build_params,
  ::raytrace::AccelerationStructureBuildMode)
{
  if (!build_params.bottomBuilds.empty())
  {
    build_bottom_acceleration_structures(build_params.bottomBuilds.data(), build_params.bottomBuilds.size());
  }
  if (!build_params.topBuilds.empty())
  {
    build_top_acceleration_structures(build_params.topBuilds.data(), build_params.topBuilds.size());
  }
}

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
