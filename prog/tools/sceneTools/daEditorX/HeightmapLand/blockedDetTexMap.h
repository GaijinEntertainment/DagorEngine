// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_relocatableFixedVector.h>
#include <dag/dag_vector.h>
#include <dag/dag_relocatable.h>

namespace hmap_storage
{

// 64x64 pixel block. Each block holds the small set of detail-texture indices
// that appear anywhere inside it (up to 8 inplace, heap spill allowed), plus
// a planar weight array: the first 4096 bytes of `weights` give the per-pixel
// weight for blockDetTexIdx[0], the next 4096 bytes for blockDetTexIdx[1],
// and so on. An empty block (no detail textures used) stores zero bytes.
static constexpr int DETTEX_BLOCK_SHIFT = 6;
static constexpr int DETTEX_BLOCK_W = 1 << DETTEX_BLOCK_SHIFT; // 64
static_assert((DETTEX_BLOCK_W & (DETTEX_BLOCK_W - 1)) == 0, "block width must be power of two");
static constexpr int DETTEX_BLOCK_MASK = DETTEX_BLOCK_W - 1;
static constexpr int DETTEX_BLOCK_PIXELS = DETTEX_BLOCK_W * DETTEX_BLOCK_W; // 4096

struct DetTexBlock
{
  dag::RelocatableFixedVector<uint8_t, 8, true> blockDetTexIdx;
  dag::Vector<uint8_t> weights;
};

// Replaces the old MapStorage<uint64_t> detTexIdxMap + detTexWtMap pair. The
// 64-bit-packed "top 8 per pixel" layout over-allocated memory for regions
// that use 1-3 detail textures (majority of water / uniform terrain). The
// blocked layout stores each detail-tex index once per 64x64 region and only
// allocates per-pixel weight bytes for indices actually used in that region.
//
// Memory footprint vs the old 2 x uint64 pair (16 B/pixel uniform):
//   - Per-block overhead: ~56 B (RelocatableFixedVector header + dag::Vector
//     header), independent of whether the block is populated.
//   - Per-block weights: N * 4096 B, where N is the count of distinct
//     detail-tex indices present anywhere in that 64x64 block.
//   - Per-pixel marginal cost: 1 B per in-use detail-tex slot, versus 16 B
//     fixed in the old layout.
//
// Fully-populated 4096x4096 map (4096 blocks, 224 KB total block overhead):
//
//   avg N per block  |   1   |   2   |   4   |   8
//   weights data     | 16 MB | 32 MB | 64 MB | 128 MB
//   new total        | 16 MB | 32 MB | 64 MB | 128 MB
//   old uint64 pair  |              256 MB
//   savings          |  94%  |  87%  |  75%  |   50%
//
// Sparse usage is a bigger win: the old paged MapStorage allocated whole
// 2048x2048 subMaps (32 MB each for the pair) the moment any pixel in the
// subMap differed from default. A water-heavy 4kx4k level with only a
// handful of populated blocks drops from ~128-256 MB to under 5 MB.
//
// Typical levels (N=2..4 per block) land at ~75-87% reduction vs the old
// pair. Worst-case N=8+ blocks still match or beat 16 B/pixel because the
// block overhead amortizes across 4096 pixels.
class BlockedDetTexMap
{
public:
  BlockedDetTexMap() = default;
  BlockedDetTexMap(int w, int h) { reset(w, h); }

  void reset(int w, int h);

  int getMapSizeX() const { return mapSizeX; }
  int getMapSizeY() const { return mapSizeY; }

  // Fully replaces the pixel's blend list. Adds any new idx values to the
  // block's shared table (growing the planar weights buffer by one 4096-byte
  // zero-filled plane per new slot) and zeros every previous slot at (x,y)
  // before writing the new weights. Matches applyBlendDetTex semantics.
  void setAt(int x, int y, const uint8_t *in_idx, const uint8_t *in_wt, int count);

  // Packs the top-8 weighted (idx, wt) pairs at (x,y) into the legacy uint64
  // pair layout so existing readers (readLandDetailTexturePixel, TIF export,
  // getMostUsedDetTex-style loops) keep working with a single-line diff.
  void getPackedAt(int x, int y, uint64_t &out_idx, uint64_t &out_wt) const;

  // Block count getters used by block-by-block scans. Full-image loops
  // should iterate blocks and call getBlockAt / getPackedAtLocal rather
  // than calling getPackedAt per pixel -- per-pixel getPackedAt strides
  // across up to 8 * 4 KB pages per read, which is a throughput disaster
  // for 4kx4k images.
  int getBlocksX() const { return blocksX; }
  int getBlocksY() const { return blocksY; }

  // Returns nullptr when the block at (bx, by) has no detail-tex indices
  // at all (empty / all-default). Callers may skip the block entirely on
  // nullptr (every pixel's packed pair is 0, 0).
  const DetTexBlock *getBlockAt(int bx, int by) const
  {
    if (bx < 0 || bx >= blocksX || by < 0 || by >= blocksY)
      return nullptr;
    const DetTexBlock &b = blocks[size_t(by) * size_t(blocksX) + size_t(bx)];
    return b.blockDetTexIdx.empty() ? nullptr : &b;
  }

  // Block-local version of getPackedAt. (local_x, local_y) are coordinates
  // inside the 64x64 block, i.e. 0 <= local_x,y < DETTEX_BLOCK_W. Callers
  // that scan a whole image should fetch the block pointer once per block
  // (via getBlockAt) and call this inner function; the shared block pointer
  // dereferences then hit L1/L2 cache instead of striding.
  static void getPackedAtLocal(const DetTexBlock &b, int local_x, int local_y, uint64_t &out_idx, uint64_t &out_wt);

private:
  int mapSizeX = 0;
  int mapSizeY = 0;
  int blocksX = 0;
  int blocksY = 0;
  dag::Vector<DetTexBlock> blocks; // blocks[by * blocksX + bx]
};

} // namespace hmap_storage

DAG_DECLARE_RELOCATABLE(hmap_storage::DetTexBlock);
