//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/integer/dag_IBBox2.h>
#include <generic/dag_staticTab.h>
#include <3d/dag_resPtr.h>

// is_same = true means that we are continue to render same cascade
// needed_now is always within needed_for_cascade
typedef eastl::fixed_function<64, void(const BBox2 &needed_for_cascade, const BBox2 &needed_now, uint32_t cascade, bool is_same)>
  hmap_shadows_prepare_cb;

class BBox2;

class ComputeShaderElement;
class HeightmapShadows
{
public:
  // number of steps_to_update declares speed of convergence and spike in total calc
  void render(const Point3 &pos, const Point3 &from_sun_dir, const hmap_shadows_prepare_cb &cb, int steps_to_update = 2);
  // use_16bit_fmt use r16g16. It is enough to use 16 bit format, but there will be subtle quantization artifacts
  // but if 16 bit is used, setMinMax should be called/ht_min_max shoild be accurate
  void init(int w_, int cascades_cnt, float dist0, float scale_, const Point2 &ht_min_max = Point2(0, 1), bool use_16bit_fmt = true);
  void close();
  void setDist(float d);
  void setScale(float scale_);
  void setMinMax(const Point2 &ht_min_max);
  void invalidate();
  uint32_t cascadesCount() const { return cascades.size(); }
  uint32_t cascadeWidth() const { return w; }
  float cascadeDist(uint32_t cascade) const;       // returns min dist, without rotation, for current Dist/Scale
  float actualCascadeDist(uint32_t cascade) const; // returns min dist, without rotation, for rendered Dist/Scale
  BBox2 cascadeBox(uint32_t cascade) const;        // returns max world space box, with rotation
  BBox2 cascadeValidBox(uint32_t cascade) const;   // returns max world space box, with rotation
  Point2 getHtMinMax() const;
  HeightmapShadows();
  ~HeightmapShadows();

protected:
  struct UpdateCascade;
  uint32_t renderCascade(const UpdateCascade &c, int steps, const hmap_shadows_prepare_cb &cb);
  void updateCascades(const Point3 &pos, const Point3 &from_sun_dir, const hmap_shadows_prepare_cb &cb, int steps);
  void setVars();
  UniqueTexHolder heightmap_shadow;
  UniqueBuf heightmap_shadow_start;
  struct Cascade
  {
    IPoint2 alignedPos = {-1000000, 1000000};
    float dist0 = 11.f;
    float scale = 2;
    Point2 lastSunDirXZ = {0, 1};
    float fromSunDirYTan = 0; // tangence of sunDirY ... i.e from_sun_dir.y/length(from_sun_dir.xz)
    float maxPenumbra = 1.f;
    int16_t finRow = 0;  // we have finished final row here (due to ping-pong)
    int16_t lastRow = 0; // we have finished current ping-pong here
    IBBox2 valid;        // valid is where data is correct, not nesessarily actual
    bool isValid() const { return !valid.isEmpty(); }
  };
  struct UpdateCascade
  {
    Point2 from_sun_direction_xz = {0, 1};
    float fromSunDirYTan = 0;
    IPoint2 newPos, oldPos;
    uint32_t updatedZRows = 0;
    uint32_t cascade = ~0u;
  } moving;
  enum
  {
    MAX_POSSIBLE_CASCADES = 4
  };
  StaticTab<Cascade, MAX_POSSIBLE_CASCADES> cascades;
  eastl::unique_ptr<ComputeShaderElement> calc_heightmap_shadows_cs, start_heightmap_shadows_cs, copy_last_row_heightmap_shadows_cs;
  float dist0 = 256.f;
  int w = 1;
  float scale = 2;
  struct HtScaleOfs
  {
    float mul = 1, ofs = 0;
  } htScaleOfs;
  float htMin = 0, htMax = 1;
  uint32_t d3dResetCounter = 0;
  // can be configured
  float sunSizeTan = 0.00872686779f; // tanf(0.5*PI/180);
  float constBlurSize = 0.05f;
};