// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_driver.h>

#include "drv_log_defs.h"
#include "acceleration_structure_desc.h"
#include "render.h"

namespace acceleration_structure_descriptors::detail
{
void processCommonGeometryFlags(MTLAccelerationStructureGeometryDescriptor *desc, RaytraceGeometryDescription::Flags flags)
{
  desc.intersectionFunctionTableOffset = 0; // not implemented

  desc.opaque = ((flags & RaytraceGeometryDescription::Flags::IS_OPAQUE) != RaytraceGeometryDescription::Flags::NONE);

  // Note that it is opposite to NO_DUBLICATE
  desc.allowDuplicateIntersectionFunctionInvocation = !((flags & RaytraceGeometryDescription::Flags::NO_DUPLICATE_ANY_HIT_INVOCATION) != RaytraceGeometryDescription::Flags::NONE);
}

MTLAccelerationStructureGeometryDescriptor *getTriangleGeometryDesc(const RaytraceGeometryDescription::TrianglesInfo &triangles, eastl::vector<drv3d_metal::Buffer *, framemem_allocator> *resources)
{
  MTLAccelerationStructureTriangleGeometryDescriptor *desc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];

  auto vBuffer = reinterpret_cast<drv3d_metal::Buffer*>(triangles.vertexBuffer);
  desc.vertexBuffer = vBuffer->getBuffer();
  desc.vertexStride = triangles.vertexStride;
  desc.vertexBufferOffset = triangles.vertexOffset * triangles.vertexStride + triangles.vertexOffsetExtraBytes;

  G_ASSERT(desc.vertexBuffer);
  G_ASSERT(desc.vertexStride > 0);

  if (resources)
    resources->push_back(vBuffer);

  if (@available(macOS 13.0, iOS 16.0, *))
  {
    switch (triangles.vertexFormat)
    {
      case VSDT_FLOAT1: desc.vertexFormat = MTLAttributeFormatFloat; break;
      case VSDT_FLOAT2: desc.vertexFormat = MTLAttributeFormatFloat2; break;
      case VSDT_FLOAT3: desc.vertexFormat = MTLAttributeFormatFloat3; break;
      case VSDT_FLOAT4: desc.vertexFormat = MTLAttributeFormatFloat4; break;
      case VSDT_HALF2: desc.vertexFormat = MTLAttributeFormatHalf2; break;
      case VSDT_HALF4: desc.vertexFormat = MTLAttributeFormatHalf4; break;
      case VSDT_SHORT2N: desc.vertexFormat = MTLAttributeFormatShort2Normalized; break;
      case VSDT_SHORT4N: desc.vertexFormat = MTLAttributeFormatShort4Normalized; break;
      case VSDT_USHORT2N: desc.vertexFormat = MTLAttributeFormatUShort2Normalized; break;
      case VSDT_USHORT4N: desc.vertexFormat = MTLAttributeFormatUShort4Normalized; break;
      default:
        G_ASSERTF(0, "Unknown vertex format %d", triangles.vertexFormat);
    }

    if (triangles.transformBuffer)
    {
      auto tBuffer = reinterpret_cast<drv3d_metal::Buffer*>(triangles.transformBuffer);

      if (resources)
        resources->push_back(tBuffer);

      desc.transformationMatrixBuffer = tBuffer->getBuffer();
      desc.transformationMatrixBufferOffset = triangles.transformOffset*sizeof(MTLPackedFloat4x3) + tBuffer->getDynamicOffset();
    }
  }

  if (triangles.indexBuffer)
  {
    auto iBuffer = reinterpret_cast<drv3d_metal::Buffer*>(triangles.indexBuffer);
    const bool isUInt32 = (iBuffer->bufFlags & SBCF_INDEX32);

    G_ASSERT(iBuffer->getDynamicOffset() == 0);
    G_ASSERT(iBuffer->getSize() >= triangles.indexCount * (isUInt32 ? sizeof(uint32_t) : sizeof(uint16_t)));

    if (resources)
      resources->push_back(iBuffer);

    desc.indexBuffer = iBuffer->getBuffer();
    desc.indexType = isUInt32 ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16;
    desc.indexBufferOffset = triangles.indexOffset * (isUInt32 ? sizeof(uint32_t) : sizeof(uint16_t));

    desc.triangleCount = triangles.indexCount / 3;
  }
  else
  {
    const int vertexCount = (vBuffer->getSize() - desc.vertexBufferOffset) / triangles.vertexStride;
    desc.triangleCount = vertexCount / 3;

    // "the vertex buffer provides 3 vertices for each triangle"
    // https://developer.apple.com/documentation/metal/mtlaccelerationstructuretrianglegeometrydescriptor/3553875-trianglecount
    G_ASSERTF((vertexCount % 3) == 0, "Vertex count for blas without index buffer must be a multiple of 3");
  }

  processCommonGeometryFlags(desc, triangles.flags);

  return desc;
}

MTLAccelerationStructureGeometryDescriptor *getAABBGeometryDesc(const RaytraceGeometryDescription::AABBsInfo &aabbs, eastl::vector<drv3d_metal::Buffer *, framemem_allocator> *resources)
{
  drv3d_metal::Buffer *buffer = (drv3d_metal::Buffer*)aabbs.buffer;
  MTLAccelerationStructureBoundingBoxGeometryDescriptor *desc = [MTLAccelerationStructureBoundingBoxGeometryDescriptor descriptor];
  desc.boundingBoxBuffer = buffer->getBuffer();
  desc.boundingBoxCount = aabbs.count;

  if (resources)
    resources->push_back(buffer);

  // "The stride must be at least 24 bytes"
  // https://developer.apple.com/documentation/metal/mtlaccelerationstructureboundingboxgeometrydescriptor/3553863-boundingboxstride?language=objc
  G_ASSERTF(aabbs.stride >= 24, "The bounding box stride must be at least 24 bytes");
  desc.boundingBoxStride = aabbs.stride;
  desc.boundingBoxBufferOffset = aabbs.offset;

  processCommonGeometryFlags(desc, aabbs.flags);

  return desc;
}

MTLAccelerationStructureUsage getASUsage(RaytraceBuildFlags build_flags)
{
  MTLAccelerationStructureUsage result = MTLAccelerationStructureUsageNone;

  if ((build_flags & RaytraceBuildFlags::ALLOW_UPDATE) == RaytraceBuildFlags::ALLOW_UPDATE)
    result |= MTLAccelerationStructureUsageRefit;

  if ((build_flags & RaytraceBuildFlags::FAST_BUILD) == RaytraceBuildFlags::FAST_BUILD)
    result |= MTLAccelerationStructureUsagePreferFastBuild;

  return result;
}
} // acceleration_structure_descriptors::detail


namespace acceleration_structure_descriptors
{
MTLInstanceAccelerationStructureDescriptor *getTLASDescriptor(uint32_t instance_count, RaytraceBuildFlags flags)
{
  // Create the instance acceleration structure that contains all instances in the scene.
  // RaytraceBuildFlags::FAST_TRACE is basically default
  MTLInstanceAccelerationStructureDescriptor *accDesc = [MTLInstanceAccelerationStructureDescriptor descriptor];

  accDesc.usage = detail::getASUsage(flags);

  if (@available(iOS 15, macOS 12.0, *))
  {
    accDesc.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeUserID;
    accDesc.instanceDescriptorStride = sizeof(MTLAccelerationStructureUserIDInstanceDescriptor);
  }

  accDesc.instanceCount = instance_count;
  return accDesc;
}

MTLPrimitiveAccelerationStructureDescriptor *getBLASDescriptor(const RaytraceGeometryDescription *desc, uint32_t count, RaytraceBuildFlags flags, eastl::vector<drv3d_metal::Buffer *, framemem_allocator> *resources)
{
  NSMutableArray *geometryDescs  = [[NSMutableArray alloc] init];
  for (uint32_t i = 0; i < count; i++)
  {
    G_ASSERT(desc[i].type == RaytraceGeometryDescription::Type::TRIANGLES || desc[i].type == RaytraceGeometryDescription::Type::AABBS);
    MTLAccelerationStructureGeometryDescriptor *geometryDesc = desc[i].type == RaytraceGeometryDescription::Type::TRIANGLES ?
      detail::getTriangleGeometryDesc(desc[i].data.triangles, resources) : detail::getAABBGeometryDesc(desc[i].data.aabbs, resources);
    [geometryDescs addObject: geometryDesc];
  }

  // RaytraceBuildFlags::FAST_TRACE is basically default
  MTLPrimitiveAccelerationStructureDescriptor *accDesc = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
  accDesc.geometryDescriptors = geometryDescs;
  [geometryDescs release];

  accDesc.usage = detail::getASUsage(flags);

  return accDesc;
}
} // namespace acceleration_structure_descriptors
