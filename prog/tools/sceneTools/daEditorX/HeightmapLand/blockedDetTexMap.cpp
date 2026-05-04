// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blockedDetTexMap.h"

#include <string.h>

namespace hmap_storage
{

void BlockedDetTexMap::reset(int w, int h)
{
  mapSizeX = w;
  mapSizeY = h;
  blocksX = (w + DETTEX_BLOCK_W - 1) / DETTEX_BLOCK_W;
  blocksY = (h + DETTEX_BLOCK_W - 1) / DETTEX_BLOCK_W;
  blocks.clear();
  blocks.resize(size_t(blocksX) * size_t(blocksY));
}

void BlockedDetTexMap::setAt(int x, int y, const uint8_t *in_idx, const uint8_t *in_wt, int count)
{
  if (x < 0 || x >= mapSizeX || y < 0 || y >= mapSizeY)
    return;
  const int bx = x >> DETTEX_BLOCK_SHIFT;
  const int by = y >> DETTEX_BLOCK_SHIFT;
  const int lx = x & DETTEX_BLOCK_MASK;
  const int ly = y & DETTEX_BLOCK_MASK;
  const int localOfs = ly * DETTEX_BLOCK_W + lx;
  DetTexBlock &b = blocks[by * blocksX + bx];

  // Make sure every incoming idx is registered in the block's shared table.
  // Typical count is 1..4; blockDetTexIdx size stays under 8 for most blocks,
  // so the linear search here is a handful of comparisons.
  for (int i = 0; i < count; i++)
  {
    const uint8_t idx = in_idx[i];
    int slot = -1;
    for (int s = 0, se = (int)b.blockDetTexIdx.size(); s < se; s++)
      if (b.blockDetTexIdx[s] == idx)
      {
        slot = s;
        break;
      }
    if (slot < 0)
    {
      b.blockDetTexIdx.push_back(idx);
      // Grow the planar weights buffer by one zero-filled plane so the new
      // slot has zero weight everywhere until we write it below.
      const size_t oldSize = b.weights.size();
      b.weights.resize(oldSize + DETTEX_BLOCK_PIXELS);
      memset(b.weights.data() + oldSize, 0, DETTEX_BLOCK_PIXELS);
    }
  }

  // Fully replace this pixel: zero every slot first, then write the requested
  // (idx, wt) pairs. This matches the legacy uint64-pack semantics where each
  // applyBlendDetTex call fully replaces the pixel's blend list.
  const int nSlots = (int)b.blockDetTexIdx.size();
  for (int s = 0; s < nSlots; s++)
    b.weights[s * DETTEX_BLOCK_PIXELS + localOfs] = 0;

  for (int i = 0; i < count; i++)
  {
    const uint8_t idx = in_idx[i];
    for (int s = 0; s < nSlots; s++)
      if (b.blockDetTexIdx[s] == idx)
      {
        b.weights[s * DETTEX_BLOCK_PIXELS + localOfs] = in_wt[i];
        break;
      }
  }
}

void BlockedDetTexMap::getPackedAtLocal(const DetTexBlock &b, int local_x, int local_y, uint64_t &out_idx, uint64_t &out_wt)
{
  out_idx = 0;
  out_wt = 0;
  const int nSlots = (int)b.blockDetTexIdx.size();
  if (nSlots == 0)
    return;
  const int localOfs = local_y * DETTEX_BLOCK_W + local_x;

  // Online top-8 selection over the full slot set. The old code had an
  // unsigned-shift UB when blendTex.size() > 8 (shifts past 56 in a uint64);
  // the new layout caps to top-8 here, at read time. Maintaining a
  // descending-sorted top-8 as we scan is O(nSlots * 8) and visits every
  // slot, so high-weight entries never disappear even when a block's slot
  // table grows past 8.
  uint8_t idxArr[8];
  uint8_t wtArr[8];
  int n = 0;
  for (int s = 0; s < nSlots; s++)
  {
    const uint8_t w = b.weights[s * DETTEX_BLOCK_PIXELS + localOfs];
    if (w == 0)
      continue;
    const uint8_t idx = b.blockDetTexIdx[s];
    // When the buffer isn't full yet, always insert. Once full, a candidate
    // only gets in if it beats the current minimum (wtArr[7], which sits at
    // the end of the descending-sorted prefix).
    if (n == 8 && w <= wtArr[7])
      continue;
    int j = n < 8 ? n : 7;
    while (j > 0 && wtArr[j - 1] < w)
    {
      wtArr[j] = wtArr[j - 1];
      idxArr[j] = idxArr[j - 1];
      j--;
    }
    wtArr[j] = w;
    idxArr[j] = idx;
    if (n < 8)
      n++;
  }
  for (int i = 0; i < n; i++)
  {
    out_idx |= uint64_t(idxArr[i]) << (i * 8);
    out_wt |= uint64_t(wtArr[i]) << (i * 8);
  }
}

void BlockedDetTexMap::getPackedAt(int x, int y, uint64_t &out_idx, uint64_t &out_wt) const
{
  out_idx = 0;
  out_wt = 0;
  if (x < 0 || x >= mapSizeX || y < 0 || y >= mapSizeY)
    return;
  const int bx = x >> DETTEX_BLOCK_SHIFT;
  const int by = y >> DETTEX_BLOCK_SHIFT;
  const int lx = x & DETTEX_BLOCK_MASK;
  const int ly = y & DETTEX_BLOCK_MASK;
  const DetTexBlock &b = blocks[by * blocksX + bx];
  getPackedAtLocal(b, lx, ly, out_idx, out_wt);
}

} // namespace hmap_storage
