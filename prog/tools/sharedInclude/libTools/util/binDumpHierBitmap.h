//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/util/makeBindump.h>
#include <debug/dag_debug.h>

namespace mkbindump
{
template <class Bitmap>
inline void build_ro_hier_bitmap(BinDumpSaveCB &cwr, const Bitmap &bm, int l1_npt, int l2_npt)
{
  int L1_SZ = 1 << l1_npt;
  int L2_SZ = 1 << (l1_npt + l2_npt);
  int tszx = (bm.getW() + L2_SZ - 1) / L2_SZ, tszy = (bm.getH() + L2_SZ - 1) / L2_SZ;
  Tab<unsigned> l2_ofs(tmpmem);
  Tab<unsigned> bits(tmpmem);
  Tab<unsigned> l1_cache(tmpmem);

  l2_ofs.resize((L2_SZ / L1_SZ) * (L2_SZ / L1_SZ));
  bits.resize((L1_SZ * L1_SZ + 31) / 32);

  cwr.writeInt32e(l1_npt);
  cwr.writeInt32e(l2_npt);
  cwr.beginBlock();
  cwr.setOrigin();
  cwr.writeInt32e(tszx);
  cwr.writeInt32e(tszy);
  cwr.writePtr64e(cwr.tell() + cwr.PTR_SZ);
  cwr.writeZeroes(sizeof(uint32_t) * tszx * tszy);
  cwr.align16();

  int l1_cnt[4] = {0, 0, 0, 0}, l2_cnt[4] = {0, 0, 0, 0};
  for (int y = 0, l2 = 0; y < bm.getH(); y += L2_SZ)
    for (int x = 0; x < bm.getW(); x += L2_SZ, l2++)
    {
      int l2_ones_cnt = 0;
      int pos = cwr.tell();
      mem_set_0(l2_ofs);

      for (int dy = 0, l1 = 0; dy < L2_SZ; dy += L1_SZ)
        for (int dx = 0; dx < L2_SZ; dx += L1_SZ, l1++)
        {
          int l1_ones_cnt = 0;
          mem_set_0(bits);

          for (int dy2 = 0; dy2 < L1_SZ; dy2++)
            for (int dx2 = 0; dx2 < L1_SZ; dx2++)
              if (bm.get(x + dx + dx2, y + dy + dy2))
              {
                l1_ones_cnt++;
                int idx = (dy2 << l1_npt) + dx2;
                bits[idx >> 5] |= (1 << (idx & 0x1F));
              }
          if (l1_ones_cnt == 0)
            l1_cnt[0]++;
          else if (l1_ones_cnt == L1_SZ * L1_SZ)
          {
            l1_cnt[1]++;
            l2_ofs[l1] = 1;
            l2_ones_cnt++;
          }
          else
          {
            l1_cnt[2]++;
            if (cwr.tell() == pos)
              cwr.writeZeroes(data_size(l2_ofs));
            for (int k = 0; k < l1_cache.size(); k += 1 + bits.size())
              if (mem_eq(bits, &l1_cache[k + 1]))
              {
                l2_ofs[l1] = l1_cache[k];
                l1_cnt[3]++;
                goto next_l1_block;
              }

            l2_ofs[l1] = cwr.tell();
            cwr.write32ex(bits.data(), data_size(bits));
            l1_cache.push_back(l2_ofs[l1]);
            append_items(l1_cache, bits.size(), bits.data());
          }
        next_l1_block:;
        }

      if (l2_ones_cnt == (L2_SZ / L1_SZ) * (L2_SZ / L1_SZ))
      {
        l2_cnt[1]++;
        cwr.writeInt32eAt(1, cwr.PTR_SZ + 8 + l2 * 4);
      }
      else if (cwr.tell() == pos)
        l2_cnt[0]++;
      else
      {
        l2_cnt[2]++;
        int cpos = cwr.tell();
        cwr.writeInt32eAt(pos, cwr.PTR_SZ + 8 + l2 * 4);
        cwr.seekto(pos);
        cwr.write32ex(l2_ofs.data(), data_size(l2_ofs));
        cwr.seekto(cpos);
      }
    }
  debug("%dx%d -> L1: %d/%d/%d/%d  L2: %d/%d/%d/%d -> %d", bm.getW(), bm.getH(), l1_cnt[0], l1_cnt[1], l1_cnt[2], l1_cnt[3], l2_cnt[0],
    l2_cnt[1], l2_cnt[2], l2_cnt[3], cwr.tell());

  cwr.popOrigin();
  cwr.endBlock();
}
} // namespace mkbindump
