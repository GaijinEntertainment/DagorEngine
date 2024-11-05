//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_sampler.h>

class BaseTexture;
class D3dResource;

namespace d3d
{
/**
 * \brief Registers the sampler portion of the given texture into the global bindless sampler table.
 *
 * Textures with identical samplers may yield identical return values.
 * Default sampler (mip linear, filter linear, address wrap, anisotropic = 1, mip bias = 0, transparent black border)
 * is available at zero slot by default.
 *
 * \param texture Pointer to the BaseTexture object.
 * \return Index in the global sampler table representing this sampler.
 */
uint32_t register_bindless_sampler(BaseTexture *texture);

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
 * \brief Allocates a persistent bindless slot range of the given 'resource_type' resource type.
 *
 * \param resource_type The type of resource to allocate the slot range for. Must be one of RES3D_ enum values.
 * \param count The number of slots to allocate. Must be larger than 0.
 * \return The first slot index into the bindless heap of the requested range.
 */
uint32_t allocate_bindless_resource_range(uint32_t resource_type, uint32_t count);

/**
 * \brief Resizes a previously allocated bindless slot range.
 *
 * It can shrink and enlarge a slot range. The contents of all slots of the old range are migrated to the new range,
 * so only new entries have to be updated.
 *
 * \param resource_type The type of resource to resize the slot range for. Must be one of RES3D_ enum values.
 * \param index The index of the slot range to resize. Must be in a previously allocated bindless range,
 *              or any value if 'current_count' is 0.
 * \param current_count The current count of slots in the range. Must be within a previously allocated bindless slot range
 *                      or 0, when 0 then it behaves like 'allocate_bindless_resource_range'.
 * \param new_count The new count of slots in the range. Can be larger or smaller than 'current_count',
 *                  shrinks or enlarges the slot range accordingly.
 * \return The first slot of the new range.
 */
uint32_t resize_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t current_count, uint32_t new_count);

/**
 * \brief Frees a previously allocated slot range.
 *
 * This can also be used to shrink ranges, similarly to 'resize_bindless_resource_range'.
 *
 * \param resource_type The type of resource to free the slot range for. Must be one of RES3D_ enum values.
 * \param index The index of the slot range to free. Must be in a previously allocated bindless range,
 *              or any value if 'count' is 0.
 * \param count The number of slots to free. Must not exceed the previously allocated bindless slot range.
 *              Can be 0 which will be a no-op.
 */
void free_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t count);

/**
 * \brief Updates a given bindless slot with the reference to 'res'.
 *
 * The slot has to be allocated previously with the corresponding allocation methods with 'resource_type'
 * matching the restype of 'res'.
 *
 * \param index The index of the bindless slot to update. Must be in a previously allocated bindless range.
 * \param res Pointer to the D3dResource object.
 */
void update_bindless_resource(uint32_t index, D3dResource *res);

/**
 * \brief Updates one or more bindless slots with a "null" resource of the given type.
 *
 * Shader access to those slots will read all zeros and writes will be discarded.
 *
 * \param resource_type The type of resource to update with null. Must be one of RES3D_ enum values.
 * \param index The index of the bindless slot to update. Must be in a previously allocated bindless range.
 * \param count The number of slots to update. Must not exceed the previously allocated bindless slot range.
 */
void update_bindless_resources_to_null(uint32_t resource_type, uint32_t index, uint32_t count);

} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline uint32_t register_bindless_sampler(BaseTexture *texture) { return d3di.register_bindless_sampler(texture); }

inline uint32_t register_bindless_sampler(SamplerHandle sampler) { return d3di.register_bindless_sampler(sampler); }

inline uint32_t allocate_bindless_resource_range(uint32_t resource_type, uint32_t count)
{
  return d3di.allocate_bindless_resource_range(resource_type, count);
}

inline uint32_t resize_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t current_count, uint32_t new_count)
{
  return d3di.resize_bindless_resource_range(resource_type, index, current_count, new_count);
}

inline void free_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t count)
{
  d3di.free_bindless_resource_range(resource_type, index, count);
}

inline void update_bindless_resource(uint32_t index, D3dResource *res) { d3di.update_bindless_resource(index, res); }

inline void update_bindless_resources_to_null(uint32_t resource_type, uint32_t index, uint32_t count)
{
  d3di.update_bindless_resources_to_null(resource_type, index, count);
}
} // namespace d3d
#endif
