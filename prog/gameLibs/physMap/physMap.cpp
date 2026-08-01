// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <physMap/physMap.h>
#include <physMap/physMapCompactDecals.h>

#include <util/dag_treeBitmap.h>
#include <scene/dag_physMat.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_mathUtils.h>
#include <vecmath/dag_vecMath.h>
#include <string.h>

void PhysMap::clear()
{
  del_it(parent);
  del_it(compactDecals);
  clear_and_shrink(decals);
}

int PhysMap::getMaterialAt(const Point2 &pos) const
{
  if (!parent)
    return PHYSMAT_INVALID;
  if (pos.x < worldOffset.x || pos.x > worldOffset.x + scale * size || pos.y < worldOffset.y || pos.y > worldOffset.y + scale * size)
    return PHYSMAT_INVALID;

  uint16_t posX = clamp<uint16_t>(uint16_t((pos.x - worldOffset.x) * invScale), 0, size - 1);
  uint16_t posY = clamp<uint16_t>(uint16_t((pos.y - worldOffset.y) * invScale), 0, size - 1);
  uint8_t idx = parent->get(posX, posY, size);
  // a corrupt/out-of-version bitmap byte beyond the table maps to PHYSMAT_INVALID,
  // matching the fine remap path in fillMaterialsRegion instead of reading OOB
  return idx < materials.size() ? materials[idx] : PHYSMAT_INVALID;
}


void PhysMap::fillMaterialsRegion(const BBox2 &box, dag::Span<uint8_t> map, const int width, const int height) const
{
  TIME_PROFILE(physmat__fillMaterialsRegion);
  Point2 boxWidth = box.width();
  Point2 step = Point2(safediv(boxWidth.x, width), safediv(boxWidth.y, height));

  if (step.x > scale && step.y > scale)
  {
    // plain algorithm as we just step over grid cells here
    for (int y = 0; y < height; ++y)
      for (int x = 0; x < width; ++x)
      {
        const int matId = getMaterialAt(box.lim[0] + Point2((x + 0.5f) * step.x, (y + 0.5f) * step.y));
        G_ASSERT(matId < UCHAR_MAX);
        map[y * width + x] = matId;
      }
    return;
  }

  // We can query less cells if cell in the requested grid is smaller than the scale, thus we can save us a lot of
  // requests to the materials and bitmap
  // Note that there's no clamp here, so region can start from negative numbers and grow outside of
  // size-1 from the top, this is needed, so we can fill incoming array with PHYSMAT_INVALID in those
  // cells which lies outside of the bounds

  // XY left-top part of the region
  int left = floorf((box.lim[0][0] - worldOffset[0] + step[0] * 0.5f) * invScale);
  int top = floorf((box.lim[0][1] - worldOffset[1] + step[1] * 0.5f) * invScale);
  // XY right-bottom part of the region
  int right = floorf((box.lim[1][0] - worldOffset[0] - step[0] * 0.5f) * invScale);
  int bottom = floorf((box.lim[1][1] - worldOffset[1] - step[1] * 0.5f) * invScale);

  Tab<uint8_t> matCache(framemem_ptr());
  const int cacheWidth = max(right - left, bottom - top) + 1;
  matCache.resize(cacheWidth * cacheWidth);
  mem_set_ff(matCache);
  mem_set_ff(map);

  parent->gatherPixels(IPoint4(left, top, left + cacheWidth, top + cacheWidth), make_span(matCache), cacheWidth, size);
  Point2 invStep = Point2(safeinv(step.x), safeinv(step.y));

  // branchless per-pixel remap; 0xff and out-of-table indices stay PHYSMAT_INVALID
  uint8_t remapLut[256];
  memset(remapLut, 0xff, sizeof(remapLut));
  for (int i = 0; i < materials.size() && i < 255; ++i)
    remapLut[i] = uint8_t(materials[i]);
#if _TARGET_SIMD_SSE >= 3 || _TARGET_SIMD_NEON // on SSE2 v_perm_i8 is scalar emulation, slower than the byte-LUT loop
  if (materials.size() <= 16)
  {
    // valid indices fit one dynamic byte shuffle; table lanes at
    // [materials.size(), 16) hold 0xff and every byte outside [0, 15]
    // (0xff included) is forced back to 0xff, so the result is
    // byte-identical to the LUT loop for all 256 input values
    vec4i tbl = v_ldui((const int *)remapLut); //-V1032 v_ldui is an unaligned load; the byte buffer needs no alignment
    vec4i hiNibble = v_splatsi(int(0xF0F0F0F0u));
    vec4i ones = v_splatsi(-1);
    uint8_t *p = matCache.data();
    int n = matCache.size(), i = 0;
    for (; i + 16 <= n; i += 16)
    {
      vec4i v = v_ldui((const int *)(p + i));
      vec4i valid = v_cmp_eqi8(v_andi(v, hiNibble), v_zeroi());
      v_stui(p + i, v_ori(v_perm_i8(tbl, v), v_andnoti(valid, ones)));
    }
    for (; i < n; ++i)
      p[i] = remapLut[p[i]];
  }
  else
#endif
    for (uint8_t &matIdx : matCache)
      matIdx = remapLut[matIdx];

  auto getMat = [&](int rx, int ry) { return matCache[(ry - top) * cacheWidth + (rx - left)]; };

  const Point2 stepOffset = hadamard_product_2d(worldOffset - box.lim[0], invStep) - Point2(0.5f, 0.5f);
  const Point2 scalePerStep = scale * invStep;
  for (int ry = max(top, 0); ry <= min(bottom, size - 1); ++ry)
  {
    const float baseY = ry * scalePerStep.y + stepOffset.y;
    const int beginY = max(int(ceilf(baseY)), 0);
    const int endY = min(int(ceilf(baseY + scalePerStep.y)), height);
    for (int rx = max(left, 0); rx <= min(right, size - 1); ++rx)
    {
      int matId = getMat(rx, ry);
      G_ASSERT(matId < UCHAR_MAX);
      int dx = 1;
      for (; rx + dx <= right && getMat(rx + dx, ry) == matId; ++dx)
        ; // nop, just find right-most pixel of the same matid value
      // iterate over real y/x of the requested grid
      // It's an inverse of the function which maps x -> gridx position
      const float baseX = rx * scalePerStep.x + stepOffset.x;
      const int beginX = max(int(ceilf(baseX)), 0);
      const int endX = min(int(ceilf(baseX + dx * scalePerStep.x)), width);
      for (int y = beginY; y < endY; ++y)
        memset(map.data() + (y * width + beginX), matId, endX - beginX);
      rx += dx - 1; // do an 'overstep'
    }
  }
}
