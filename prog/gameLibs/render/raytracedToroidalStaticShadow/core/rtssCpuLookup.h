// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_hlsl_floatx.h>
#include "../shaders/rtss_common.hlsli"
#include "rtssTransforms.h"
#include <math/integer/dag_IPoint4.h>
#include <generic/dag_span.h>
#include <EASTL/algorithm.h>
#include <math.h>

struct RtssCpuTables
{
  dag::ConstSpan<RtssLevel> levels;
  dag::ConstSpan<RtssSlot> slots;
  IPoint4 paramsI;
  Point4 params, origin;
  float centerY = 0, texel0 = 0;
};

// the level the shader lookup reads for pos_rel, or -1, with no below-horizon check; first_level < 0 selects it as the shader does,
// and then fade gets the weight of lit in the seam band of the coarsest level
inline int rtss_cpu_lookup(const RtssCpuTables &t, const Point3 &pos_rel, const Point3 &normal, float normal_bias_texels, float dither,
  int first_level = -1, float *fade = nullptr)
{
  const int levelCount = t.paramsI.x, W = t.paramsI.y;
  const float4 *rows0 = &t.levels[0].xform[int(t.params.z) * 3];
  const Point3 fromCenter = pos_rel - Point3(0, t.centerY, 0);
  const Point3 dc = rtss_rows_to_ls(rows0, fromCenter, 0);
  const float m = fmaxf(fmaxf(fmaxf(fabsf(dc.x), fabsf(dc.y) * t.origin.w), fabsf(dc.z)) * t.params.x, 1e-6f);
  int level = first_level;
  if (fade)
    *fade = 0.f;
  if (level < 0)
  {
    level = eastl::max(0, int(ceilf(log2f(m))));
    const float band = fminf(fmaxf((m * exp2f(-float(level)) - 1.f) * t.params.w + 1.f, 0.f), 1.f);
    const int last = levelCount - 1;
    if (fade && level >= last)
    {
      const Point3 dl = rtss_rows_to_ls(&t.levels[last].xform[(t.levels[last].flags & RTSS_LEVEL_NEWEST_BIT) * 3], fromCenter, 0);
      const float ml = fmaxf(fmaxf(fabsf(dl.x), fabsf(dl.y)), fabsf(dl.z)) * t.params.x * exp2f(-float(last));
      *fade = fminf(fmaxf((ml - 1.f) * t.params.w + 1.f, 0.f), 1.f);
    }
    level = level < last && dither < band ? level + 1 : eastl::min(level, last);
  }
  const float NoL = fminf(fmaxf(-(normal.x * rows0[2].x + normal.y * rows0[2].y + normal.z * rows0[2].z), 0.f), 1.f);
  const Point3 pos = pos_rel + normal * (normal_bias_texels * t.texel0 * fmaxf(m, 1.f) * (1 - NoL));
  for (; level < levelCount; ++level)
  {
    const RtssLevel &l = t.levels[level];
    uint32_t bit = l.flags & RTSS_LEVEL_NEWEST_BIT;
    for (uint32_t i = 0; i < ((l.flags & RTSS_LEVEL_SWEEPING) ? 2u : 1u); ++i, bit ^= 1)
    {
      const Point3 ls = rtss_rows_to_ls(&l.xform[bit * 3], pos);
      const int px = l.pageOrigin.x + int(floorf(ls.x * l.invPage)), py = l.pageOrigin.y + int(floorf(ls.y * l.invPage));
      const RtssSlot &s = t.slots[level * W * W + rtss_slot_index(int2(px, py), W)];
      if ((s.flags & RTSS_SLOT_VALID) && s.page == rtss_pack_page(int2(px, py)) && rtss_slot_bit(s.flags) == bit &&
          ls.z >= s.minDepth && ls.z <= s.maxDepth)
        return level;
    }
  }
  return -1;
}
