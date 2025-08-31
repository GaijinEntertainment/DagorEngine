//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct ResourceHeap;
class BaseTexture;

/**
 * @brief Describes a mapping between a tile in a texture and a memory area in a heap.
 */
struct TileMapping
{
  size_t texX;           ///< The tile coordinates in tiles, not pixels!
  size_t texY;           ///< The tile coordinates in tiles, not pixels!
  size_t texZ;           ///< The tile coordinates in tiles, not pixels!
  size_t texSubresource; ///< The index of the subresource.
  size_t heapTileIndex;  ///< The index of the tile in the heap. Not bytes, but tile index!
  size_t heapTileSpan;   ///< The number of tiles to map. Zero is invalid, and if it is not one, an array of tiles will be
                         ///< mapped, to the specified location.
                         ///< Example usage for this is packed mip tails.
                         ///< - Map to subresource TextureTilingInfo::numUnpackedMips at 0, 0, 0.
  ///< - Use a span of TextureTilingInfo::numTilesNeededForPackedMips so the whole mip tail can be mapped at once.
  ///< - From TileMapping::heapTileIndex the given number of tiles will be mapped to the packed mip tail.
  bool isPacked; ///< Whether this mapping is for packed mips or not. If true, the mapping is for packed mips.
};


/**
 * @brief Structure representing the properties of a tiled resource.
 *
 * NOTE: even if numPackedMips is zero, numTilesNeededForPackedMips may be greater than zero, which is a special case,
 * and numTilesNeededForPackedMips tiles still need to be assigned at numUnpackedMips.
 */
struct TextureTilingInfo
{
  size_t totalNumberOfTiles;          ///< Total number of tiles in the resource.
  size_t numUnpackedMips;             ///< Number of unpacked mips in the resource.
  size_t numPackedMips;               ///< Number of packed mips in the resource.
  size_t numTilesNeededForPackedMips; ///< Number of tiles needed to store the packed mips.
  size_t firstPackedTileIndex;        ///< Index of the first tile storing packed mips.
  size_t tileWidthInPixels;           ///< Width of each tile in pixels.
  size_t tileHeightInPixels;          ///< Height of each tile in pixels.
  size_t tileDepthInPixels;           ///< Depth of each tile in pixels.
  size_t tileMemorySize;              ///< Size of each tile in bytes.

  size_t subresourceWidthInTiles;   ///< Width of the subresource in tiles.
  size_t subresourceHeightInTiles;  ///< Height of the subresource in tiles.
  size_t subresourceDepthInTiles;   ///< Depth of the subresource in tiles.
  size_t subresourceStartTileIndex; ///< Index of the first tile of the subresource.
};


namespace d3d
{
/**
 * @brief Maps a memory area of the heap to the specified xyz location of the texture.
 *
 * Use heap == nullptr to remove the link between a tile and the mapped heap portion.
 *
 * @param tex The texture to map.
 * @param heap The heap to map.
 * @param mapping The mapping to apply.
 * @param mapping_count The number of mappings in the mapping array.
 */
void map_tile_to_resource(BaseTexture *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count);

/**
 * @brief Retrieves the tiling information of a texture.
 *
 * @param tex The texture to query.
 * @param subresource The index of the subresource.
 * @return The tiling information of the texture.
 */
TextureTilingInfo get_texture_tiling_info(BaseTexture *tex, size_t subresource);
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline void map_tile_to_resource(BaseTexture *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count)
{
  return d3di.map_tile_to_resource(tex, heap, mapping, mapping_count);
}
inline TextureTilingInfo get_texture_tiling_info(BaseTexture *tex, size_t subresource)
{
  return d3di.get_texture_tiling_info(tex, subresource);
}
} // namespace d3d
#endif
