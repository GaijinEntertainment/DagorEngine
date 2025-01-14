//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_inttypes.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_resource.h>
#include <drv/3d/dag_d3dResource.h>

class Sbuffer;
class BaseTexture;

/**
 * @brief Definition of ResourceHeapCreateFlags type.
 * See ResourceHeapCreateFlag for valid values.
 */
using ResourceHeapCreateFlags = uint32_t;

/**
 * @brief Enumeration representing different resource activation actions.
 */
enum class ResourceActivationAction : unsigned
{
  REWRITE_AS_COPY_DESTINATION, ///< Rewrite the resource as a copy destination.
  REWRITE_AS_UAV,              ///< Rewrite the resource as an unordered access view.
  REWRITE_AS_RTV_DSV,          ///< Rewrite the resource as a render target or depth-stencil view.
  CLEAR_F_AS_UAV,              ///< Clear the resource as a floating-point unordered access view.
  CLEAR_I_AS_UAV,              ///< Clear the resource as an integer unordered access view.
  CLEAR_AS_RTV_DSV,            ///< Clear the resource as a render target or depth-stencil view.
  DISCARD_AS_UAV,              ///< Discard the resource as an unordered access view.
  DISCARD_AS_RTV_DSV,          ///< Discard the resource as a render target or depth-stencil view.
};

/**
 * @brief Basic resource description that contains common fields for all GPU resource types.
 */
struct BasicResourceDescription
{
  uint32_t cFlags;                     ///< The resource creation flags.
  ResourceActivationAction activation; ///< The resource activation action.
  ResourceClearValue clearValue;       ///< The clear value.
};

/**
 * @brief Buffer resource description that contains fields specific to buffer resources.
 */
struct BufferResourceDescription : BasicResourceDescription
{
  uint32_t elementSizeInBytes; ///< The size of a single element in bytes.
  uint32_t elementCount;       ///< The number of elements in the buffer.
  uint32_t viewFormat;         ///< The view format of the buffer.
};

/**
 * @brief Basic texture resource description that contains fields common to all texture resources.
 */
struct BasicTextureResourceDescription : BasicResourceDescription
{
  uint32_t mipLevels; ///< The number of mip levels.
};

/**
 * @brief Texture resource description that contains fields specific to 2D textures.
 */
struct TextureResourceDescription : BasicTextureResourceDescription
{
  uint32_t width;  ///< The width of the texture.
  uint32_t height; ///< The height of the texture.
};

/**
 * @brief Volume texture resource description that contains fields specific to volume textures.
 */
struct VolTextureResourceDescription : TextureResourceDescription
{
  uint32_t depth; ///< The depth of the volume texture.
};

/**
 * @brief Array texture resource description that contains fields specific to array textures.
 */
struct ArrayTextureResourceDescription : TextureResourceDescription
{
  uint32_t arrayLayers; ///< The number of array layers.
};

/**
 * @brief Cube texture resource description that contains fields specific to cube textures.
 */
struct CubeTextureResourceDescription : BasicTextureResourceDescription
{
  uint32_t extent; ///< The extent of the cube texture.
};

/**
 * @brief Array cube texture resource description that contains fields specific to array cube textures.
 */
struct ArrayCubeTextureResourceDescription : CubeTextureResourceDescription
{
  uint32_t cubes; ///< The number of cubes in the array cube texture.
};

/**
 * @brief A common type for a resource description that can be used to describe any type of GPU resource.
 */
struct ResourceDescription
{
  uint32_t resType; ///< The type of the resource. One of the RES3D_* constants.
  union
  {
    BasicResourceDescription asBasicRes;                   ///< The basic resource description.
    BufferResourceDescription asBufferRes;                 ///< The buffer resource description.
    BasicTextureResourceDescription asBasicTexRes;         ///< The basic texture resource description.
    TextureResourceDescription asTexRes;                   ///< The 2D texture resource description.
    VolTextureResourceDescription asVolTexRes;             ///< The volume texture resource description.
    ArrayTextureResourceDescription asArrayTexRes;         ///< The array texture resource description.
    CubeTextureResourceDescription asCubeTexRes;           ///< The cube texture resource description.
    ArrayCubeTextureResourceDescription asArrayCubeTexRes; ///< The array cube texture resource description.
  };

  ResourceDescription() = default;                                       ///< Default constructor.
  ResourceDescription(const ResourceDescription &) = default;            ///< Copy constructor.
  ResourceDescription &operator=(const ResourceDescription &) = default; ///< Copy assignment operator.

  ResourceDescription(const BufferResourceDescription &buf) :
    resType{RES3D_SBUF}, asBufferRes{buf} {} ///< Constructor for buffer resources.

  ResourceDescription(const TextureResourceDescription &tex) : resType{RES3D_TEX}, asTexRes{tex} {} ///< Constructor for 2D textures.

  ResourceDescription(const VolTextureResourceDescription &tex) :
    resType{RES3D_VOLTEX}, asVolTexRes{tex} {} ///< Constructor for volume textures.

  ResourceDescription(const ArrayTextureResourceDescription &tex) :
    resType{RES3D_ARRTEX}, asArrayTexRes{tex} {} ///< Constructor for array textures.

  ResourceDescription(const CubeTextureResourceDescription &tex) :
    resType{RES3D_CUBETEX}, asCubeTexRes{tex} {} ///< Constructor for cube textures.

  ResourceDescription(const ArrayCubeTextureResourceDescription &tex) :
    resType{RES3D_CUBEARRTEX}, asArrayCubeTexRes{tex} {} ///< Constructor for array cube textures.

  /**
   * @brief Equality operator for resource descriptions.
   *
   * The operator compares all necessary fields of the resource descriptions for equality.
   *
   * @param r The resource description to compare with.
   * @return True if the resource descriptions are equal, false otherwise.
   */
  bool operator==(const ResourceDescription &r) const
  {
#define FIELD_MATCHES(field) (this->field == r.field)
    if (!FIELD_MATCHES(resType) || !FIELD_MATCHES(asBasicRes.cFlags))
      return false;
    if (resType == RES3D_SBUF)
      return FIELD_MATCHES(asBufferRes.elementCount) && FIELD_MATCHES(asBufferRes.elementSizeInBytes) &&
             FIELD_MATCHES(asBufferRes.viewFormat);
    if (!FIELD_MATCHES(asBasicTexRes.mipLevels))
      return false;
    if (resType == RES3D_CUBETEX || resType == RES3D_CUBEARRTEX)
    {
      if (!FIELD_MATCHES(asCubeTexRes.extent))
        return false;
      if (resType == RES3D_CUBEARRTEX)
        return FIELD_MATCHES(asArrayCubeTexRes.cubes);
    }
    if (!FIELD_MATCHES(asTexRes.width) || !FIELD_MATCHES(asTexRes.height))
      return false;
    if (resType == RES3D_VOLTEX)
      return FIELD_MATCHES(asVolTexRes.depth);
    if (resType == RES3D_ARRTEX)
      return FIELD_MATCHES(asArrayTexRes.arrayLayers);
    return true;
#undef FIELD_MATCHES
  }

  using HashT = size_t; ///< The type of the hash value.

  /**
   * @brief Computes the hash value of the resource description.
   *
   * @return The hash value of the resource description.
   */
  HashT hash() const
  {
    HashT hashValue = resType;
    switch (resType)
    {
      case RES3D_SBUF:
        return hashPack(hashValue, asBufferRes.cFlags, asBufferRes.elementCount, asBufferRes.elementSizeInBytes,
          asBufferRes.viewFormat);
      case RES3D_TEX:
        return hashPack(hashValue, eastl::to_underlying(asTexRes.activation), asTexRes.cFlags, asTexRes.mipLevels, asTexRes.height,
          asTexRes.width, asTexRes.clearValue.asUint[0], asTexRes.clearValue.asUint[1], asTexRes.clearValue.asUint[2],
          asTexRes.clearValue.asUint[3]);
      case RES3D_VOLTEX:
        return hashPack(hashValue, eastl::to_underlying(asVolTexRes.activation), asVolTexRes.cFlags, asVolTexRes.mipLevels,
          asVolTexRes.height, asVolTexRes.width, asVolTexRes.depth, asVolTexRes.clearValue.asUint[0], asVolTexRes.clearValue.asUint[1],
          asVolTexRes.clearValue.asUint[2], asVolTexRes.clearValue.asUint[3]);
      case RES3D_ARRTEX:
        return hashPack(hashValue, eastl::to_underlying(asArrayTexRes.activation), asArrayTexRes.arrayLayers, asArrayTexRes.cFlags,
          asArrayTexRes.height, asArrayTexRes.mipLevels, asArrayTexRes.height, asArrayTexRes.width, asArrayTexRes.clearValue.asUint[0],
          asArrayTexRes.clearValue.asUint[1], asArrayTexRes.clearValue.asUint[2], asArrayTexRes.clearValue.asUint[3]);
      case RES3D_CUBETEX:
        return hashPack(hashValue, eastl::to_underlying(asCubeTexRes.activation), asCubeTexRes.cFlags, asCubeTexRes.extent,
          asCubeTexRes.mipLevels, asCubeTexRes.clearValue.asUint[0], asCubeTexRes.clearValue.asUint[1],
          asCubeTexRes.clearValue.asUint[2], asCubeTexRes.clearValue.asUint[3]);
      case RES3D_CUBEARRTEX:
        return hashPack(hashValue, eastl::to_underlying(asArrayCubeTexRes.activation), asArrayCubeTexRes.cFlags,
          asArrayCubeTexRes.mipLevels, asArrayCubeTexRes.cubes, asArrayCubeTexRes.extent, asArrayCubeTexRes.clearValue.asUint[0],
          asArrayCubeTexRes.clearValue.asUint[1], asArrayCubeTexRes.clearValue.asUint[2], asArrayCubeTexRes.clearValue.asUint[3]);
    }
    return hashValue;
  }

  String toDebugString() const
  {
    switch (resType)
    {
      case RES3D_SBUF:
        return String(0,
          "Buffer { .cFlags = %u, .activation = %u, .clearValue = (%u, %u, %u, %u),"
          " .elementSizeInBytes = %u, .elementCount = %u, .viewFormat = %u }",
          asBufferRes.cFlags, eastl::to_underlying(asBufferRes.activation), asBufferRes.clearValue.asUint[0],
          asBufferRes.clearValue.asUint[1], asBufferRes.clearValue.asUint[2], asBufferRes.clearValue.asUint[3],
          asBufferRes.elementSizeInBytes, asBufferRes.elementCount, asBufferRes.viewFormat);
      case RES3D_TEX:
        return String(0,
          "Texture { .cFlags = %u, .activation = %u, .clearValue = (%u, %u, %u, %u),"
          " .mipLevels = %u, .width = %u, .height = %u }",
          asTexRes.cFlags, eastl::to_underlying(asTexRes.activation), asTexRes.clearValue.asUint[0], asTexRes.clearValue.asUint[1],
          asTexRes.clearValue.asUint[2], asTexRes.clearValue.asUint[3], asTexRes.mipLevels, asTexRes.width, asTexRes.height);
      case RES3D_VOLTEX:
        return String(0,
          "VolumeTexture { .cFlags = %u, .activation = %u, .clearValue = (%u, %u, %u, %u),"
          " .mipLevels = %u, .width = %u, .height = %u, .depth = %u }",
          asVolTexRes.cFlags, eastl::to_underlying(asVolTexRes.activation), asVolTexRes.clearValue.asUint[0],
          asVolTexRes.clearValue.asUint[1], asVolTexRes.clearValue.asUint[2], asVolTexRes.clearValue.asUint[3], asVolTexRes.mipLevels,
          asVolTexRes.width, asVolTexRes.height, asVolTexRes.depth);
      case RES3D_ARRTEX:
        return String(0,
          "ArrayTexture { .cFlags = %u, .activation = %u, .clearValue = (%u, %u, %u, %u),"
          " .mipLevels = %u, .width = %u, .height = %u, .arrayLayers = %u }",
          asArrayTexRes.cFlags, eastl::to_underlying(asArrayTexRes.activation), asArrayTexRes.clearValue.asUint[0],
          asArrayTexRes.clearValue.asUint[1], asArrayTexRes.clearValue.asUint[2], asArrayTexRes.clearValue.asUint[3],
          asArrayTexRes.mipLevels, asArrayTexRes.width, asArrayTexRes.height, asArrayTexRes.arrayLayers);
      case RES3D_CUBETEX:
        return String(0,
          "CubeTexture { .cFlags = %u, .activation = %u, .clearValue = (%u, %u, %u, %u),"
          " .mipLevels = %u, .extent = %u }",
          asCubeTexRes.cFlags, eastl::to_underlying(asCubeTexRes.activation), asCubeTexRes.clearValue.asUint[0],
          asCubeTexRes.clearValue.asUint[1], asCubeTexRes.clearValue.asUint[2], asCubeTexRes.clearValue.asUint[3],
          asCubeTexRes.mipLevels, asCubeTexRes.extent);
      case RES3D_CUBEARRTEX:
        return String(0,
          "ArrayCubeTexture { .cFlags = %u, .activation = %u, .clearValue = (%u, %u, %u, %u),"
          " .mipLevels = %u, .extent = %u, .cubes = %u }",
          asArrayCubeTexRes.cFlags, eastl::to_underlying(asArrayCubeTexRes.activation), asArrayCubeTexRes.clearValue.asUint[0],
          asArrayCubeTexRes.clearValue.asUint[1], asArrayCubeTexRes.clearValue.asUint[2], asArrayCubeTexRes.clearValue.asUint[3],
          asArrayCubeTexRes.mipLevels, asArrayCubeTexRes.extent, asArrayCubeTexRes.cubes);
      default: break;
    }
    return String("INVALID");
  }

private:
  /**
   * @brief Hashes an empty tuple of values.
   *
   * @return HashT the hash value of the tuple.
   */
  inline HashT hashPack() const { return 0; }
  /**
   * @brief Hashes a tuple of values.
   *
   * @tparam T type of the first value.
   * @tparam Ts types of the other values.
   * @param first the first value.
   * @param other the other values.
   * @return HashT the hash value of the tuple.
   */
  template <typename T, typename... Ts>
  inline HashT hashPack(const T &first, const Ts &...other) const
  {
    HashT hashVal = hashPack(other...);
    hashVal ^= first + 0x9e3779b9 + (hashVal << 6) + (hashVal >> 2);
    return hashVal;
  }
};

namespace eastl
{
/**
 * @brief Hash function specialization for ResourceDescription.
 */
template <>
class hash<ResourceDescription>
{
public:
  /**
   * @brief Computes the hash value of a resource description.
   *
   * @param desc The resource description.
   * @return The hash value of the resource description.
   */
  size_t operator()(const ResourceDescription &desc) const { return desc.hash(); }
};
} // namespace eastl

struct ResourceHeapGroup;

/**
 * @brief Properties of a resource heap group.
 * A heap group is a type of memory with specific properties.
 *
 * @note Heap groups can **not** be substitutes for different groups
 * with equal properties, as some devices have different memory types
 * with the same public properties, but with purpose bound properties
 * that are identified by a heap group.
 * For example, older NVIDIA hardware can not put render target
 * and non render target textures into the same memory region, they
 * have to be specific for render targets and non render targets,
 * this is exposed as different heap groups, but they have the same
 * public properties, as they are all reside on GPU local memory.
 */
struct ResourceHeapGroupProperties
{
  union
  {
    uint32_t flags; ///< Flags of the resource heap group represented as a single dword.
    struct
    {
      /// If true, the CPU can access this memory directly.
      /// On consoles this is usually true for all heap groups, on PC only
      /// for system memory heap groups.
      bool isCPUVisible : 1;
      /// If true, the GPU can access this memory directly without going
      /// over a bus like PCIE.
      /// On consoles this is usually true for all heap groups, on PC only
      /// for memory dedicated to the GPU.
      bool isGPULocal : 1;
      /// Special on chip memory, like ESRAM of the XB1.
      bool isOnChip : 1;
    };
  };
  /// The maximum size of a resource that can be placed into a heap of this group.
  uint64_t maxResourceSize;
  /// the maximum size of a individual heap, this is usually limited by the
  /// amount that is installed in the system. Drivers may impose other limitations.
  uint64_t maxHeapSize;
  /// This is a hint for the user to try to aim for this heap size for best performance.
  /// Larger heaps until maxHeapSize are still possible, but larger heaps than optimalMaxHeapSize
  /// may yield worse performance, as the runtime may has to use sub-optimal memory sources
  /// to satisfy the allocation request.
  /// A value of 0 indicates that there is no optimal size and any size is expected to
  /// perform similarly.
  ///
  /// For example on DX12 on Windows the optimal size is 64 MiBytes, suggested by MS
  /// representatives, as windows may not be able to provide heaps in the requested
  /// memory source.
  uint64_t optimalMaxHeapSize;
};

/// Properties of a resource allocation.
struct ResourceAllocationProperties
{
  size_t sizeInBytes;           ///< The size of the resource allocation in bytes.
  size_t offsetAlignment;       ///< The alignment of the offset in bytes.
  ResourceHeapGroup *heapGroup; ///< The resource heap group.
};

/// A set of flags that steer the behavior of the driver during creation of resource heaps.
enum ResourceHeapCreateFlag
{
  /// By default the drivers are allowed to use already reserved memory of internal heaps, to source the needed memory.
  /// Drivers are also allowed to allocate larger memory heaps and use the excess memory for their internal resource
  /// and memory management.
  RHCF_NONE = 0,
  /// Resource heaps created with this flag, will use their own dedicate memory heap to supply the memory for resources.
  /// When this flag is not used to create a resource heap, the driver is allowed to source the need memory from existing
  /// driver managed heaps, or create a larger underlying memory heap and use the excess memory for its internal resource
  /// and memory management.
  /// This flag should be used only when really necessary, as it restricts the drivers option to use already reserved
  /// memory for this heap and increase the memory pressure on the system.
  ///
  /// @note That on certain system configurations, for example GDK with predefined memory heap allocations, this flag
  /// may be ignored by the driver, as it would otherwise not be able to satisfy heap creation request.
  RHCF_REQUIRES_DEDICATED_HEAP = 1u << 0,
};

/// A resource heap, is a memory heap for d3d resources, like textures and buffers.
struct ResourceHeap;

namespace d3d
{
/** \defgroup HeapD3D
 * @{
 */

/**
 * @brief Retrieves the resource allocation properties for a given resource description.
 *
 * @param desc The resource description. Resource descriptions that would describe a resource with one or more of its dimensions being
 * 0 will result in an error.
 * @return The resource allocation properties. On error sizeInBytes field of the returned value will be 0.
 */
ResourceAllocationProperties get_resource_allocation_properties(const ResourceDescription &desc);

/**
 * @brief Creates a resource heap with the specified size and flags.
 *
 * @param heap_group The resource heap group.
 * @param size The size of the resource heap in bytes. A value of 0 is invalid and results in undefined behavior.
 * @param flags The flags for creating the resource heap.
 * @return A pointer to the created resource heap. May be nullptr on error, like out of memory or invalid
 * inputs.
 */
ResourceHeap *create_resource_heap(ResourceHeapGroup *heap_group, size_t size, ResourceHeapCreateFlags flags);

/**
 * @brief Destroys a resource heap.
 *
 * @param heap The resource heap to destroy.
 */
void destroy_resource_heap(ResourceHeap *heap);

/**
 * @brief Places a buffer in a resource heap at the specified offset.
 *
 * The heap group of heap has to match the heap group of alloc_info
 * that was returned by get_resource_allocation_properties.
 *
 * @param heap The resource heap.
 * @param desc The resource description.
 * @param offset The offset in the resource heap in bytes.
 * @param alloc_info The resource allocation properties.
 * @param name The name of the buffer.
 * @return A pointer to the placed buffer. May be nullptr on error, like invalid inputs.
 */
Sbuffer *place_buffer_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name);

/**
 * @brief Places a texture in a resource heap at the specified offset.
 *
 * The heap group of heap has to match the heap group of alloc_info
 * that was returned by get_resource_allocation_properties.
 *
 * @param heap The resource heap.
 * @param desc The resource description.
 * @param offset The offset in the resource heap in bytes.
 * @param alloc_info The resource allocation properties.
 * @param name The name of the texture.
 * @return A pointer to the placed texture. May be nullptr on error, like invalid inputs.
 */
BaseTexture *place_texture_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name);

/**
 * @brief Retrieves the resource heap group properties for a given resource heap group.
 *
 * @note Different groups may return identical values, this does not mean that the heap
 * group is identical or can be substituted for each another. Heap groups represent purpose
 * bound memory that may have device specific properties, that limit the use of a heap
 * group for certain resource types.
 *
 * @param heap_group The resource heap group.
 * @return The resource heap group properties.
 */
ResourceHeapGroupProperties get_resource_heap_group_properties(ResourceHeapGroup *heap_group);

/**
 * @brief Activates a buffer with the specified action and optional clear value.
 *
 * Only activated placed buffer could be safely used. Using the buffer before being activated results in undefined behavior.
 *
 * @param buf The buffer to activate.
 * @param action The activation action.
 * @param value The clear value (optional).
 * @param gpu_pipeline The GPU pipeline to use (optional).
 * @warning gpu_pipeline parameter doesn't work currently.
 */
void activate_buffer(Sbuffer *buf, ResourceActivationAction action, const ResourceClearValue &value = {},
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

/**
 * @brief Activates a texture with the specified action and optional clear value.
 *
 * Only activated placed textures could be safely used. Using the texture before being activated results in undefined behavior.
 *
 * @param tex The texture to activate.
 * @param action The activation action.
 * @param value The clear value (optional).
 * @param gpu_pipeline The GPU pipeline to use (optional).
 * @warning gpu_pipeline parameter doesn't work currently.
 */
void activate_texture(BaseTexture *tex, ResourceActivationAction action, const ResourceClearValue &value = {},
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

/**
 * @brief Deactivates a buffer with the specified GPU pipeline.
 *
 * The method call is necessary to have a correct state of the resources on a heap. Using the buffer after being deactivated results in
 * undefined behavior.
 *
 * @param buf The placed buffer to deactivate.
 * @param gpu_pipeline The GPU pipeline to use (optional).
 * @warning gpu_pipeline parameter doesn't work currently.
 */
void deactivate_buffer(Sbuffer *buf, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

/**
 * @brief Deactivates a texture with the specified GPU pipeline.
 *
 * The method call is necessary to have a correct state of the resources on a heap. Using the texture after being deactivated results
 * in undefined behavior.
 *
 * @param tex The placed texture to deactivate.
 * @param gpu_pipeline The GPU pipeline to use (optional).
 * @warning gpu_pipeline parameter doesn't work currently.
 */
void deactivate_texture(BaseTexture *tex, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

/** @}*/
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline ResourceAllocationProperties get_resource_allocation_properties(const ResourceDescription &desc)
{
  return d3di.get_resource_allocation_properties(desc);
}
inline ResourceHeap *create_resource_heap(ResourceHeapGroup *heap_group, size_t size, ResourceHeapCreateFlags flags)
{
  return d3di.create_resource_heap(heap_group, size, flags);
}
inline void destroy_resource_heap(ResourceHeap *heap) { d3di.destroy_resource_heap(heap); }
inline Sbuffer *place_buffer_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  return d3di.place_buffer_in_resource_heap(heap, desc, offset, alloc_info, name);
}
inline BaseTexture *place_texture_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  return d3di.place_texture_in_resource_heap(heap, desc, offset, alloc_info, name);
}
inline ResourceHeapGroupProperties get_resource_heap_group_properties(ResourceHeapGroup *heap_group)
{
  return d3di.get_resource_heap_group_properties(heap_group);
}
inline void activate_buffer(Sbuffer *buf, ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline gpu_pipeline)
{
  d3di.activate_buffer(buf, action, value, gpu_pipeline);
}
inline void activate_texture(BaseTexture *tex, ResourceActivationAction action, const ResourceClearValue &value,
  GpuPipeline gpu_pipeline)
{
  d3di.activate_texture(tex, action, value, gpu_pipeline);
}
inline void deactivate_buffer(Sbuffer *buf, GpuPipeline gpu_pipeline) { d3di.deactivate_buffer(buf, gpu_pipeline); }
inline void deactivate_texture(BaseTexture *tex, GpuPipeline gpu_pipeline) { d3di.deactivate_texture(tex, gpu_pipeline); }
} // namespace d3d
#endif
