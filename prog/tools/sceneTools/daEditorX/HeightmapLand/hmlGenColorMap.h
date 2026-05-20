// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_mathBase.h>
#include <math/integer/dag_IBBox2.h>
#include <memory/dag_mem.h>
#include <stdint.h>

namespace hmap_storage
{
class BlockedDetTexMap;
}
class HeightMapStorage;


// Per-pass land-color generator scratch + setup state.
//
// Was originally a `namespace LandColorGenData { static ... }` of file-scoped
// globals in hmlGenColorMap.cpp. Pulled into a real struct so:
//   1. one instance lives on the stack at the top of generateLandColors
//      (no more file-scope state surviving between passes),
//   2. the per-pass curvWt initialization can be its own method, and
//   3. the per-pixel inner loop can fan out across threadpool workers using
//      one struct per thread.
//
// Layout intent (ordered as fields below):
//   [setup]   filled once before per-pixel work; remains read-only after.
//   [scratch] mutated per pixel; must be per-thread when parallelized
//             (each parallel_for worker uses its own LandColorGenData and
//             passes it down to the per-pixel ScriptParam* methods by
//             reference).
//   [output]  written from per-pixel code; reduced (OR'd) across threads.
//
// Cross-class consumers (ScriptParamImage / ScriptParamMask /
// PostScriptParamLandLayer::calcWeight, plus the brush-image getters) read
// the scratch fields via an explicit `const LandColorGenData &` parameter --
// no thread-local indirection.
struct LandColorGenData
{
  enum : int
  {
    FSZ = 7, // curvature filter kernel side
    FC = 3   // kernel center index (FSZ / 2)
  };

  struct WeighedTexIdx
  {
    float w;
    int idx;
    static int cmp(const WeighedTexIdx *a, const WeighedTexIdx *b)
    {
      float d = b->w - a->w;
      return d > 0 ? 1 : (d < 0 ? -1 : 0);
    }
  };

  // ----- setup state (initialized once per pass before per-pixel work) -----
  HeightMapStorage *hmap = nullptr;
  HeightMapStorage *hmapDet = nullptr;
  int detDiv = 0;
  IBBox2 detRectC;
  real gridCellSize = 0;
  Point2 heightMapOffset = Point2(0, 0);
  int heightMapSizeX = 0, heightMapSizeZ = 0;
  real curvWt[FSZ][FSZ] = {};

  // ----- per-pixel scratch (per-thread when parallelized) -----
  Point3 normal = Point3(0, 0, 0);
  real height = 0;
  real curvature = 0;
  real heightMapX = 0, heightMapZ = 0;
  int curv_dx = 0, curv_dy = 0;
  bool curvatureReady = false;
  Tab<WeighedTexIdx> blendTex;

  // ----- output (mergeable across threads) -----
  // Bit i set when slot i emitted a non-zero quantized weight in the just-
  // completed pass (idx clamped to bit 63 as overflow). Drives both the
  // lmDump-staleness rebuild trigger and the post-pass invalid/inactive
  // classification.
  uint64_t lcActiveMask = 0;

  LandColorGenData() : blendTex(inimem) {}

  // Build the FSZ x FSZ Laplacian-of-Gaussian-like curvature filter, then
  // normalize so center = -sum(neighbors) and sum-of-positive-coeffs is 1.
  // Uses gridCellSize, so call after gridCellSize has been set.
  void initCurvatureFilter();

  bool insideDetRectC(int x, int y) const
  {
    return x >= detRectC[0].x && x < detRectC[1].x && y >= detRectC[0].y && y < detRectC[1].y;
  }
  float sampleHeight(int x, int y) const;
  float getCurvature();
  void resetBlendDetTex() { blendTex.clear(); }
  void blendDetTex(unsigned idx, float t);
  // detMap may be null; in that case the call is a no-op (matches the legacy
  // behavior when HmapLandPlugin::self->getDetTexMap() returned null).
  // Touches lcActiveMask, so each thread must call against its own
  // LandColorGenData copy when running in parallel.
  void applyBlendDetTex(int x, int y, int def_det_tex, hmap_storage::BlockedDetTexMap *detMap);

  // Pure utility helpers (kept as static methods for parity with the old
  // namespace functions; some are unreferenced today but preserved as part
  // of the original API surface).
  static unsigned makeColor(int r, int g, int b, int t);
  static unsigned setType(unsigned u, int t)
  {
    E3DCOLOR c = u;
    c.a = t;
    return c;
  }
  static int getType(unsigned u) { return E3DCOLOR(u).a; }
  static unsigned blendColors(unsigned u1, unsigned u2, float t);
  static unsigned mulColors(unsigned u1, unsigned u2);
  static unsigned mulColors2x(unsigned u1, unsigned u2);
  static unsigned dissolveOver(unsigned u1, unsigned u2, float t, float mask)
  {
    if (t <= 0)
      return u1;
    if (t >= 1)
      return u2;
    return t >= mask ? u2 : u1;
  }
  static float calcBlendK(float val0, float val1, float v);
  static float smoothStep(float t)
  {
    if (t <= 0)
      return 0;
    if (t >= 1)
      return 1;
    return (3 - 2 * t) * t * t;
  }
};
