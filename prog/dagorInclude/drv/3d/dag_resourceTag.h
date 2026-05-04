//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_consts.h>
#include <generic/dag_functionRef.h>
#include <EASTL/variant.h>

/// Filter set, each type set to false will be skipped by the operation.
struct ResourceTypeFilter
{
  /// Should buffers be included
  bool includeDeviceMemoryBuffers : 1 = false;
  /// Should buffers that use host memory be included
  bool includeHostMemoryBuffers : 1 = false;
  /// Should texture be included
  bool includeTextures : 1 = false;
  /// Should ray trace bottom acceleration structures be included
  bool includeRayTraceBottomStructures : 1 = false;
  /// Should ray trace top acceleration structures be included
  bool includeRayTraceTopStructures : 1 = false;
  /// Should ray trace other acceleration structures be included
  /// Such structures are:
  /// - Opacity Micro Maps
  bool includeRayTraceOtherStructures : 1 = false;
  /// Should ray trace structure pools be included
  bool includeRayTraceStructurePools : 1 = false;
  /// Should user-created resource heaps be included
  bool includeResourceHeaps : 1 = false;
  /// Memory where it is not know for which kind of resource it is being used.
  /// This is typical for memory usage by third party libraries like DLSS, XESS and such.
  bool includeUnspecifiedMemory : 1 = false;

  inline bool includes_any() const
  {
    return includeDeviceMemoryBuffers || includeHostMemoryBuffers || includeTextures || includeRayTraceBottomStructures ||
           includeRayTraceTopStructures || includeRayTraceOtherStructures || includeRayTraceStructurePools || includeResourceHeaps ||
           includeUnspecifiedMemory;
  }
};

enum class TaggedResourceType
{
  /// Undefined resource type, should not be used!
  Undefined,
  /// A buffer backed by device (eg GPU) memory.
  DeviceBuffer,
  /// A buffer backed by host (eg System) memory.
  HostBuffer,
  /// A texture
  Texture,
  /// A TLAS
  TopLevelAccelerationStructure,
  /// A BLAS
  BottomLevelAccelerationStructure,
  /// A RT structure that is neither TLAS or BLAS
  OtherAccelerationStructure,
  /// A RT structure pool holding any type of RT structure.
  /// Individual structure in this pool are not visited.
  AccelerationStructurePool,
  /// A user-created resource heap (see d3d::create_resource_heap).
  ResourceHeap,
  /// Memory where it is not know for which kind of resource it is being used.
  /// This is typical for memory usage by third party libraries like DLSS, XESS and such.
  /// NOTE: Has to be **always** the last entry in the enum!
  UnspecifiedMemory,
};

/// Information of a tagged resource.
struct TaggedResourceInfo
{
  /// Type of the resource, undefined should be ignored.
  TaggedResourceType type = TaggedResourceType::Undefined;
  /// Resource name, may be null, name string may become invalid after return.
  const char *name = nullptr;
  /// The tag, may be null meaning no tag.
  ResourceTagType tag = nullptr;
  /// Bytes of memory occupied by the resource.
  uint32_t sizeInBytes = 0;
  /// Some resource types have address alignment restrictions, this is reported here, a value of 0 or 1 indicate not alignment
  /// requirement.
  uint32_t addressAlignment = 0;
  /// Indicates if the buffer was internally allocated from a bigger buffer.
  /// Will be false for textures and external memory type.
  bool isSubAllocated : 1 = false;
  /// Some APIs do not report exact memory usage values (like DLLSG).
  bool isAproximation : 1 = false;
};

/// Visitor type passed to visit_tagged_resources.
using ResourceVisitor = dag::FunctionRef<void(const TaggedResourceInfo &res) const>;

namespace d3d
{
/// Driver will invoke 'visitor' for each resource that matches the filters in 'filter' plus there additional rules:
/// - Aliasing resources are excluded
/// - Resources placed in user resource heaps are excluded (the heaps themselves can be included via includeResourceHeaps)
/// - RT structures of RT structure pools are not visited as the pools are
/// NOTE: This may negatively impact overall driver performance at it may acquires multiple internal mutexes for the duration.
void visit_tagged_resources(const ResourceTypeFilter &filter, const ResourceVisitor &visitor);
} // namespace d3d


#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline void visit_tagged_resources(const ResourceTypeFilter &filter, const ResourceVisitor &visitor)
{
  d3di.visit_tagged_resources(filter, visitor);
}
} // namespace d3d
#endif
