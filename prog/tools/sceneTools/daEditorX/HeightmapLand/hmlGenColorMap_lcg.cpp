// Copyright (C) Gaijin Games KFT.  All rights reserved.

// LandColorGenData out-of-line implementations. Lives in its own TU because
// the bodies are independent of the local ScriptParamImage / ScriptParamMask
// / PostScriptParamLandLayer classes (those stay file-scoped in
// hmlGenColorMap.cpp); pulling them out keeps that big file's compile cost
// from carrying these whenever it changes.

#include "hmlGenColorMap.h"
#include "blockedDetTexMap.h"

#include <de3_hmapStorage.h>
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>
#include <math/dag_e3dColor.h>
#include <math/dag_mathBase.h>
#include <util/dag_globDef.h>
#include <EASTL/algorithm.h>
#include <math.h>


void LandColorGenData::initCurvatureFilter()
{
  // FSZ x FSZ kernel: weight = smoothstep(distance) / (distance * gridCellSize)
  // for off-center taps; center holds -sum(neighbors). Then normalize by the
  // sum of the smoothstep coefficients so the filter sums-of-positives to 1
  // regardless of FSZ. Uses gridCellSize, so this must be called after that
  // field is populated for the current pass.
  double wtSum = 0, kSum = 0;
  for (int fy = 0; fy < FSZ; ++fy)
    for (int fx = 0; fx < FSZ; ++fx)
    {
      if (fx == FC && fy == FC)
        continue;

      int dx = fx - FC;
      int dy = fy - FC;
      real d = sqrtf(dx * dx + dy * dy);

      real k = (FC + 1 - d) / (FC + 1);
      if (k < 0)
        k = 0;

      k = (3 - 2 * k) * k * k;

      curvWt[fy][fx] = k / (d * gridCellSize);
      wtSum += curvWt[fy][fx];
      kSum += k;
    }

  curvWt[FC][FC] = -wtSum;

  if (kSum != 0)
    for (int fy = 0; fy < FSZ; ++fy)
      for (int fx = 0; fx < FSZ; ++fx)
        curvWt[fy][fx] /= kSum;
}

float LandColorGenData::sampleHeight(int x, int y) const
{
  if (detDiv && insideDetRectC(x * detDiv, y * detDiv))
  {
    x *= detDiv;
    y *= detDiv;
    int x0 = max(x - detDiv / 2, detRectC[0].x), y0 = max(y - detDiv / 2, detRectC[0].y);
    int x1 = min(x + detDiv / 2, detRectC[1].x), y1 = min(y + detDiv / 2, detRectC[1].y);
    float h = 0;
    for (y = y0; y <= y1; y++)
      for (x = x0; x <= x1; x++)
        h += hmapDet->getFinalData(x, y);
    return h / (x1 - x0 + 1) / (y1 - y0 + 1);
  }
  return hmap->getFinalData(x, y);
}

float LandColorGenData::getCurvature()
{
  if (curvatureReady)
    return curvature;

  double curv = 0;
  for (int fy = 0; fy < FSZ; ++fy)
    for (int fx = 0; fx < FSZ; ++fx)
      curv += sampleHeight(fx + curv_dx, fy + curv_dy) * curvWt[fy][fx];

  curvatureReady = true;
  return curvature = curv;
}

unsigned LandColorGenData::makeColor(int r, int g, int b, int t)
{
  if (r < 0)
    r = 0;
  else if (r > 255)
    r = 255;
  if (g < 0)
    g = 0;
  else if (g > 255)
    g = 255;
  if (b < 0)
    b = 0;
  else if (b > 255)
    b = 255;

  return E3DCOLOR(r, g, b, t);
}

unsigned LandColorGenData::blendColors(unsigned u1, unsigned u2, float t)
{
  if (t <= 0)
    return u1;
  if (t >= 1)
    return u2;

  E3DCOLOR c1 = u1, c2 = u2;

  return E3DCOLOR(real2int(c1.r * (1 - t) + c2.r * t), real2int(c1.g * (1 - t) + c2.g * t), real2int(c1.b * (1 - t) + c2.b * t),
    (t <= 0.5f ? c1.a : c2.a));
}

void LandColorGenData::blendDetTex(unsigned idx, float t)
{
  if (t <= 0)
    return;

  if (!blendTex.size() || t > 1.0f)
    t = 1.0f;

  int id = -1;
  for (int i = 0; i < blendTex.size(); i++)
    if (blendTex[i].idx != idx)
    {
      blendTex[i].w *= (1.0f - t);
    }
    else
    {
      blendTex[i].w = blendTex[i].w * (1.0f - t) + t;
      id = i;
    }

  if (id < 0)
  {
    WeighedTexIdx &w = blendTex.push_back();
    w.idx = idx;
    w.w = t;
  }
}

void LandColorGenData::applyBlendDetTex(int x, int y, int def_det_tex, hmap_storage::BlockedDetTexMap *detMap)
{
  float sum = 0;
  if (!blendTex.size())
  {
    if (!detMap)
      return;
    WeighedTexIdx &w = blendTex.push_back();
    w.idx = def_det_tex;
    sum = w.w = 1.0f;
  }
  else
  {
    // Sum positive contributions and locate the dominant entry. The dominant
    // index is what the landClsMap detLayer records as this pixel's
    // representative texture (hmlGenColorMap.cpp reads blendTex[0].idx
    // after this call), so we swap the heaviest entry into slot 0.
    // w<=0 entries are dropped silently in the quantize loop below (they
    // would also quantize to 0 anyway).
    int maxI = 0;
    float maxW = 0;
    for (int i = 0, ie = blendTex.size(); i < ie; ++i)
      if (blendTex[i].w > 0)
      {
        sum += blendTex[i].w;
        if (blendTex[i].w > maxW)
        {
          maxW = blendTex[i].w;
          maxI = i;
        }
      }
    if (sum <= 0)
      return; // nothing positive to emit; pixel keeps whatever was there
    if (maxI != 0)
      eastl::swap(blendTex[0], blendTex[maxI]);
  }

  // prepareDetTexMaps() was already called at the top of generateLandColors,
  // so detMap is non-null and correctly sized for the current heightmap.
  if (!detMap)
    return;

  // Quantize weights into uint8 (0..255) and stream every nonzero (idx, wt)
  // pair into the block. setAt is order-independent and getPackedAtLocal
  // does its own descending top-8 at read time, so we don't need to sort or
  // pre-truncate here -- just compact in the same loop that quantizes,
  // skipping w<=0 (negative/dropped blends) and qw==0 (sub-1/255 weights
  // that would only pollute the block's slot table).
  dag::RelocatableFixedVector<uint8_t, 31, true, framemem_allocator, uint8_t, false> wIdxBuf;
  const int n = blendTex.size();
  wIdxBuf.resize(n << 1);
  auto wtBuf = wIdxBuf.data(), idxBuf = wIdxBuf.data() + n;
  float wSum255 = 255.0f / sum;
  int nCnt = 0;
  for (int i = 0; i < n; i++)
  {
    if (blendTex[i].w <= 0)
      continue;
    uint8_t qw = uint8_t(floorf(blendTex[i].w * wSum255));
    if (!qw)
      continue;
    unsigned tx = blendTex[i].idx;
    idxBuf[nCnt] = uint8_t(tx & 0xFF);
    wtBuf[nCnt] = qw;
    lcActiveMask |= 1ULL << (tx < 63 ? tx : 63);
    ++nCnt;
  }
  detMap->setAt(x, y, idxBuf, wtBuf, nCnt);
}

unsigned LandColorGenData::mulColors(unsigned u1, unsigned u2)
{
  E3DCOLOR c1 = u1, c2 = u2;

  return E3DCOLOR((c1.r * c2.r + 127) / 255, (c1.g * c2.g + 127) / 255, (c1.b * c2.b + 127) / 255, c1.a);
}

unsigned LandColorGenData::mulColors2x(unsigned u1, unsigned u2)
{
  E3DCOLOR c1 = u1, c2 = u2;

  int r = (c1.r * c2.r * 2 + 127) / 255;
  int g = (c1.g * c2.g * 2 + 127) / 255;
  int b = (c1.b * c2.b * 2 + 127) / 255;

  if (r > 255)
    r = 255;
  if (g > 255)
    g = 255;
  if (b > 255)
    b = 255;

  return E3DCOLOR(r, g, b, c1.a);
}

float LandColorGenData::calcBlendK(float val0, float val1, float v)
{
  float d = val1 - val0;
  if (!float_nonzero(d))
    return v < val0 ? 0.f : 1.f;

  float t = (v - val0) / d;
  if (t <= 0)
    return 0;
  if (t >= 1)
    return 1;
  return t;
}
