//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_sampler.h>
#include <generic/dag_span.h>
#include <generic/dag_stridedSpan.h>

class BaseTexture;
class D3dResource;
enum class D3DResourceType : uint8_t;

namespace bindless
{
/// DX12 has a overall limit of 2048 samplers on a single GPU heap, this is the upper limit that is guaranteed available.
/// Those GPU heaps also need to have space for dynamically placed descriptors, for samplers that do not use bindless.
/// So this value should be much less than 2048.
/// Usually there are less than 20 unique samplers used, so this value is kept fairly small at 1u << 8u and leaves plenty
/// room for other descriptors.
/// With those small limits, values can be bit packed for less indexing storage needed.
inline constexpr uint32_t MAX_SAMPLER_INDEX_BITS = 8u;
inline constexpr uint32_t MAX_SAMPLER_INDEX_COUNT = 1u << MAX_SAMPLER_INDEX_BITS;
inline constexpr uint32_t SAMPLER_INDEX_MASK = MAX_SAMPLER_INDEX_COUNT - 1;

/// DX12 has a limit of 1000000 and similarly as with samplers, we need some space for descriptors that are not bindless.
/// Right now the limit is somewhat arbitrary chosen to be 65K, we probably need far less than that, but this gives us
/// lot of bindless room to work with.
/// This limit also allows bit packing to be used, to reduce memory usage. A sampler and texture bindless index can be
/// packed into one 32 bit int with some extra bits for extra info.
inline constexpr uint32_t MAX_RESOURCE_INDEX_BITS = 16u;
inline constexpr uint32_t MAX_RESOURCE_INDEX_COUNT = 1u << MAX_RESOURCE_INDEX_BITS;
inline constexpr uint32_t RESOURCE_INDEX_MASK = MAX_RESOURCE_INDEX_COUNT - 1;
} // namespace bindless

namespace d3d
{

/**
 * \brief Registers a sampler into the global bindless sampler table.
 *
 * Multiple register calls with the same sampler may yield identical return values.
 * Default sampler (mip linear, filter linear, address wrap, anisotropic = 1, mip bias = 0, transparent black border)
 * is available at zero slot by default.
 *
 * \param sampler Sampler that should be added.
 * \return Index in the global sampler table representing this sampler.
 */
uint32_t register_bindless_sampler(SamplerHandle sampler);

/**
 * \brief Allocates a persistent bindless slot range of the given 'type' resource type.
 *
 * \param type The type of resource to allocate the slot range for.
 * \param count The number of slots to allocate. Must be larger than 0.
 * \return The first slot index into the bindless heap of the requested range.
 */
uint32_t allocate_bindless_resource_range(D3DResourceType type, uint32_t count);

/**
 * \brief Resizes a previously allocated bindless slot range.
 *
 * It can shrink and enlarge a slot range. The contents of all slots of the old range are migrated to the new range,
 * so only new entries have to be updated.
 *
 * \Note This function is considered deprecated, causes more problems when used and fosters bad use cases.
 *
 * \param type The type of resource to resize the slot range for.
 * \param index The index of the slot range to resize. Must be in a previously allocated bindless range,
 *              or any value if 'current_count' is 0.
 * \param current_count The current count of slots in the range. Must be within a previously allocated bindless slot range
 *                      or 0, when 0 then it behaves like 'allocate_bindless_resource_range'.
 * \param new_count The new count of slots in the range. Can be larger or smaller than 'current_count',
 *                  shrinks or enlarges the slot range accordingly.
 * \return The first slot of the new range.
 */
uint32_t resize_bindless_resource_range(D3DResourceType type, uint32_t index, uint32_t current_count, uint32_t new_count);

/**
 * \brief Frees a previously allocated slot range.
 *
 * This can also be used to shrink ranges, similarly to 'resize_bindless_resource_range'.
 *
 * \param type The type of resource to free the slot range for.
 * \param index The index of the slot range to free. Must be in a previously allocated bindless range,
 *              or any value if 'count' is 0.
 * \param count The number of slots to free. Must not exceed the previously allocated bindless slot range.
 *              Can be 0 which will be a no-op.
 */
void free_bindless_resource_range(D3DResourceType type, uint32_t index, uint32_t count);

/**
 * \brief Updates a given bindless slot range with the references from 'resources' span.
 *
 * The slots have to be allocated previously with the corresponding allocation methods with 'type'
 * matching the getType() of 'res'. Null pointer are also acceptable and they will empty their corresponding slots.
 *
 * \param type The type of resource range to update with res.
 * \param index The start index of the bindless slot to update. Must be in a previously allocated bindless range.
 * \param resources Array of D3dResource objects (including null pointers).
 */
void update_bindless_resource_range(D3DResourceType type, uint32_t index, const dag::ConstSpan<D3dResource *> &resources);

/**
 * \brief Updates a given bindless slot with the reference to 'res'.
 *
 * The slot has to be allocated previously with the corresponding allocation methods with 'type'
 * matching the getType() of 'res'.
 *
 * \note We guarantee that draw calls and dispatch calls will see the result of the last update in the current frame
 * (before flush or update_screen) after the update call returns. The descriptor value after intermediate update calls
 * is undefined behavior and differs between platforms.
 *
 * \param index The index of the bindless slot to update. Must be in a previously allocated bindless range.
 * \param type The type of resource range to update with res.
 * \param res Pointer to the D3dResource object.
 * \return true if actual update is occurred, or false if internal cache was used
 */
bool update_bindless_resource(D3DResourceType type, uint32_t index, D3dResource *res);

/**
 * \brief Updates one or more bindless slots with a "null" resource of the given type.
 *
 * Shader access to those slots will read all zeros and writes will be discarded.
 *
 * \note We guarantee that draw calls and dispatch calls will see the result of the last update in the current frame
 * (before flush or update_screen) after the update call returns. The descriptor value after intermediate update calls
 * is undefined behavior and differs between platforms.
 *
 * \param type The type of resource to update with null.
 * \param index The index of the bindless slot to update. Must be in a previously allocated bindless range.
 * \param count The number of slots to update. Must not exceed the previously allocated bindless slot range.
 */
void update_bindless_resources_to_null(D3DResourceType type, uint32_t index, uint32_t count);

/**
 * Basically optimized version of
 * auto id = allocate_bindless_resource_range(type, 1);
 * update_bindless_resource(type, id, res);
 * Note that the slot has to be freed with free_bindless_resource_range(type, id, 1);
 *
 * \param type The type of resource to allocate the slot for.
 * \param res Pointer to the D3dResource object.
 * \return The slot where the object is stored.
 */
uint32_t add_bindless_resource(D3DResourceType type, D3dResource *res);

/**
 * This is equivalent to, but potentially more efficient than:
 * for (uint32_t i = 0; i < res_count; ++i)
 *   ids[i] = add_bindless_resource(types[i], resources[i])
 * Validation:
 * - ids.size() != 0
 * - ids.stride() != 0
 * - types.size() == ids.size()
 * - resources.size() == ids.size()
 *
 * 'resources' may contain nullptrs, then the corresponding type in 'types' will be used to determine the null resource for that slot.
 * Note that you can pass 0 as stride bytes to strided spans to back an arbitrary count with a single object.
 * eg for types one could initialize it with: StridedSpanConstConstruct{.base_ptr=&typeValue, .stride=0, .count=<count i need>}
 * Same works for resources, when all allocations should use the same resource (eg nullptr), Important to **not** pass nullptr to
 * .base_ptr or the dereference will crash, it has to point to a valid address, that stores that nullptr.
 */
void add_bindless_resources(dag::StridedConstSpan<D3DResourceType> types, dag::StridedConstSpan<D3dResource *> resources,
  dag::StridedSpan<uint32_t> ids);

} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{

inline uint32_t register_bindless_sampler(SamplerHandle sampler) { return d3di.register_bindless_sampler(sampler); }

inline uint32_t allocate_bindless_resource_range(D3DResourceType type, uint32_t count)
{
  return d3di.allocate_bindless_resource_range(type, count);
}

inline uint32_t resize_bindless_resource_range(D3DResourceType type, uint32_t index, uint32_t current_count, uint32_t new_count)
{
  return d3di.resize_bindless_resource_range(type, index, current_count, new_count);
}

inline void free_bindless_resource_range(D3DResourceType type, uint32_t index, uint32_t count)
{
  d3di.free_bindless_resource_range(type, index, count);
}

inline void update_bindless_resource_range(D3DResourceType type, uint32_t index, const dag::ConstSpan<D3dResource *> &resources)
{
  d3di.update_bindless_resource_range(type, index, resources);
}

inline bool update_bindless_resource(D3DResourceType type, uint32_t index, D3dResource *res)
{
  return d3di.update_bindless_resource(type, index, res);
}

inline void update_bindless_resources_to_null(D3DResourceType type, uint32_t index, uint32_t count)
{
  d3di.update_bindless_resources_to_null(type, index, count);
}

inline uint32_t add_bindless_resource(D3DResourceType type, D3dResource *res) { return d3di.add_bindless_resource(type, res); }

inline void add_bindless_resources(dag::StridedConstSpan<D3DResourceType> types, dag::StridedConstSpan<D3dResource *> resources,
  dag::StridedSpan<uint32_t> ids)
{
  d3di.add_bindless_resources(types, resources, ids);
}
} // namespace d3d
#endif
