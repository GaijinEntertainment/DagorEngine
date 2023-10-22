#include <3d/dag_drv3d.h>

#include "acceleration_structure_desc.h"
#include "buffers.h"

namespace acceleration_structure_descriptors::detail
{
API_AVAILABLE(ios(15.0), macos(11.0))
void processCommonGeometryFlags(MTLAccelerationStructureGeometryDescriptor *desc, RaytraceGeometryDescription::Flags flags)
{
  desc.intersectionFunctionTableOffset = 0; // not implemented

  desc.opaque = ((flags & RaytraceGeometryDescription::Flags::IS_OPAQUE) != RaytraceGeometryDescription::Flags::NONE);

  // Note that it is opposite to NO_DUBLICATE
  desc.allowDuplicateIntersectionFunctionInvocation = !((flags & RaytraceGeometryDescription::Flags::NO_DUPLICATE_ANY_HIT_INVOCATION) != RaytraceGeometryDescription::Flags::NONE);
}

API_AVAILABLE(ios(15.0), macos(11.0))
MTLAccelerationStructureGeometryDescriptor *getTriangleGeometryDesc(const RaytraceGeometryDescription::TrianglesInfo &triangles)
{
  MTLAccelerationStructureTriangleGeometryDescriptor *desc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];

  desc.vertexBuffer = ((drv3d_metal::Buffer*)triangles.vertexBuffer)->getBuffer();
  desc.vertexStride = triangles.vertexStride;

  if (@available(macOS 13.0, iOS 16.0, *))
  {
    switch (triangles.vertexFormat) {
      case VSDT_FLOAT1: desc.vertexFormat = MTLAttributeFormatFloat;
      case VSDT_FLOAT2: desc.vertexFormat = MTLAttributeFormatFloat2;
      case VSDT_FLOAT3: desc.vertexFormat = MTLAttributeFormatFloat3;
      case VSDT_FLOAT4: desc.vertexFormat = MTLAttributeFormatFloat4;
      default: desc.vertexFormat = MTLAttributeFormatFloat3;
    }
  }

  desc.indexBuffer = ((drv3d_metal::Buffer*)triangles.indexBuffer)->getBuffer();
  desc.indexType = MTLIndexTypeUInt32; // RaytraceGeometryDescription doesn't have anything resembling this
  desc.indexBufferOffset = triangles.indexOffset * sizeof(uint32_t);

  desc.triangleCount = triangles.indexCount / 3;

  processCommonGeometryFlags(desc, triangles.flags);

  return desc;
}

API_AVAILABLE(ios(15.0), macos(11.0))
MTLAccelerationStructureGeometryDescriptor *getAABBGeometryDesc(const RaytraceGeometryDescription::AABBsInfo &aabbs)
{
  MTLAccelerationStructureBoundingBoxGeometryDescriptor *desc = [MTLAccelerationStructureBoundingBoxGeometryDescriptor descriptor];
  desc.boundingBoxBuffer = ((drv3d_metal::Buffer*)aabbs.buffer)->getBuffer();
  desc.boundingBoxCount = aabbs.count;

  processCommonGeometryFlags(desc, aabbs.flags);

  return desc;
}

API_AVAILABLE(ios(15.0), macos(11.0))
MTLAccelerationStructureUsage getASUsage(RaytraceBuildFlags create_flags, RaytraceBuildFlags build_flags, bool update)
{
  MTLAccelerationStructureUsage result = MTLAccelerationStructureUsageNone;

  const bool isUpdatable = (create_flags & RaytraceBuildFlags::ALLOW_UPDATE) == RaytraceBuildFlags::ALLOW_UPDATE;
  if (update && !isUpdatable)
    logerr("Trying to update unupdatable acceleration structure");

  if (isUpdatable)
    result |= MTLAccelerationStructureUsageRefit;

  if ((build_flags & RaytraceBuildFlags::FAST_BUILD) == RaytraceBuildFlags::FAST_BUILD)
    result |= MTLAccelerationStructureUsagePreferFastBuild;

  return result;
}
} // acceleration_structure_descriptors::detail


namespace acceleration_structure_descriptors
{
API_AVAILABLE(ios(15.0), macos(11.0))
MTLInstanceAccelerationStructureDescriptor *getTLASDescriptor(Sbuffer *instance_descriptor_buffer, uint32_t instance_count, RaytraceBuildFlags struct_flags, RaytraceBuildFlags build_flags, bool update)
{
  // Create the instance acceleration structure that contains all instances in the scene.
  // RaytraceBuildFlags::FAST_TRACE is basically default
  MTLInstanceAccelerationStructureDescriptor *accDesc = [MTLInstanceAccelerationStructureDescriptor descriptor];

  accDesc.usage = detail::getASUsage(struct_flags, build_flags, update);

  if (@available(iOS 15, macOS 12.0, *))
  {
    accDesc.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeDefault;
  }

  accDesc.instanceCount = instance_count;
  accDesc.instanceDescriptorBuffer = ((drv3d_metal::Buffer*)instance_descriptor_buffer)->getBuffer();
  return accDesc;
}

API_AVAILABLE(ios(15.0), macos(11.0))
MTLPrimitiveAccelerationStructureDescriptor *getBLASDescriptor(RaytraceGeometryDescription *desc, uint32_t count, RaytraceBuildFlags struct_flags, RaytraceBuildFlags build_flags, bool update)
{
  NSMutableArray *geometryDescs  = [[NSMutableArray alloc] init];
  for (uint32_t i = 0; i < count; i++)
  {
    MTLAccelerationStructureGeometryDescriptor *geometryDesc = [desc, i]() {
      switch(desc[i].type)
      {
        case RaytraceGeometryDescription::Type::TRIANGLES:
          return detail::getTriangleGeometryDesc(desc[i].data.triangles);
        case RaytraceGeometryDescription::Type::AABBS:
          return detail::getAABBGeometryDesc(desc[i].data.aabbs);
      }
    }();
    [geometryDescs addObject: geometryDesc];
  }

  // RaytraceBuildFlags::FAST_TRACE is basically default
  MTLPrimitiveAccelerationStructureDescriptor *accDesc = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
  accDesc.geometryDescriptors = geometryDescs;
  [geometryDescs release];

  accDesc.usage = detail::getASUsage(struct_flags, build_flags, update);

  return accDesc;
}
} // namespace acceleration_structure_descriptors
