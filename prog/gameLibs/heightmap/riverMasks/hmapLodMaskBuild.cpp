// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmapLodMaskBuild.h"
#include <math/dag_adjpow2.h>
#include <math/dag_bits.h>
#include <string.h>

void hmap_sdf_build_lod_masks(const uint32_t *path, int w, int patch_dim_bits, LodsRiverMasks &out)
{
  int cell_bits = patch_dim_bits + 1; // patch_dim * 2
  int pw = w >> cell_bits;
  if (pw < 1)
  {
    out.finestLod = -1;
    out.masks.clear();
    return;
  }

  int finest_level = get_bigger_log2(pw);

  out.finestLod = (int8_t)finest_level;

  // Allocate flat bit array for all levels 0..finest_level
  uint32_t total_bits = LodsRiverMasks::get_level_ofs(finest_level + 1);
  uint32_t total_words = (total_bits + 31) / 32;
  out.masks.resize(total_words);
  memset(out.masks.data(), 0, total_words * sizeof(uint32_t));
  uint32_t *mdata = out.masks.data();

  // Build finest level by scanning critical path pixels
  int cell = 1 << cell_bits;
  uint32_t finest_ofs = LodsRiverMasks::get_level_ofs(finest_level);
  const uint32_t *cpdata = path;
  int cp_words = (w * w + 31) / 32;
  for (int wi = 0; wi < cp_words; wi++)
  {
    uint32_t word = cpdata[wi];
    while (word)
    {
      int bit = __bsf(word);
      word &= word - 1;
      int idx = wi * 32 + bit;
      if (idx >= w * w)
        break;
      int x = idx % w, y = idx / w;
      int px = x >> cell_bits, py = y >> cell_bits;
      if (px >= pw)
        px = pw - 1;
      if (py >= pw)
        py = pw - 1;
      uint32_t bidx = finest_ofs + py * pw + px;
      mdata[bidx >> 5] |= 1u << (bidx & 31);
    }
  }

  // Build coarser levels by OR-ing 2x2 blocks from the finer level
  for (int L = finest_level - 1; L >= 0; L--)
  {
    uint32_t ofs = LodsRiverMasks::get_level_ofs(L);
    uint32_t child_ofs = LodsRiverMasks::get_level_ofs(L + 1);
    int side = 1 << L;
    int child_side = 1 << (L + 1);
    for (int py = 0; py < side; py++)
      for (int px = 0; px < side; px++)
      {
        int sx = px * 2, sy = py * 2;
        uint32_t c00 = child_ofs + sy * child_side + sx;
        uint32_t c10 = c00 + 1;
        uint32_t c01 = c00 + child_side;
        uint32_t c11 = c01 + 1;
        if (((mdata[c00 >> 5] >> (c00 & 31)) | (mdata[c10 >> 5] >> (c10 & 31)) | (mdata[c01 >> 5] >> (c01 & 31)) |
              (mdata[c11 >> 5] >> (c11 & 31))) &
            1)
        {
          uint32_t bidx = ofs + py * side + px;
          mdata[bidx >> 5] |= 1u << (bidx & 31);
        }
      }
  }
}
