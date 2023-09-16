//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include "shadowSplit.h"

struct Split
{
  // INPUT
  inline void init(unsigned x2, unsigned y2, unsigned width2, unsigned height2, float splitz, float zbias, float warp_dist,
    bool is_align, bool is_warp)
  {
    x = x2;
    y = y2;
    width = width2;
    height = height2;
    splitZ = splitz;
    ZBias = zbias;
    warpDist = warp_dist;
    isAlign = is_align;
    isWarp = is_warp;
  }

  unsigned x, y;          // split top-left corner at texture space
  unsigned width, height; // split width\height at texture space

  float splitZ;   // split Z range [split[N-1].splitZ .. split[N].splitZ]
                  // at camera post projection space
                  //     split1          split2
                  // 0 |------------|---------------|---------| 1
                  //                ^               ^
                  //         split1.splitZ     split2.splitZ
  float ZBias;    // world space Z bias for this split
  float warpDist; // warping coefficient - singular point(center of projection)
                  // (-warpDist, 0,0,1) at pre-projective light space
  bool isAlign;   // is post projective texel alignment used
  bool isWarp;    // is warping used
};


class XCSM
{
private:
  static constexpr int MAX_SPLIT_NUM = 4;
  Split splitSet[MAX_SPLIT_NUM];
  ComputedShadowSplit splitOut[MAX_SPLIT_NUM];
  int splitNum;
  unsigned smapWidth, smapHeight; // total shadow map width\height

public:
  XCSM();

  ~XCSM() { close(); }

  // total shadow map width\height and count of shadow map splits
  void init(int width2, int height2, int count, float *zbias = NULL);
  void close();

  // i - split index [0..splitNum-1]
  inline void setSplit(int i, const Split &split) { splitSet[i] = split; }
  inline const ComputedShadowSplit &getSplitOut(int i) const { return splitOut[i]; }
  inline const ComputedShadowSplit *getSplitsOut() const { return splitOut; }
  inline const Split &getSplit(int i) { return splitSet[i]; }
  inline TMatrix4 getSplitProj(int i) const { return splitOut[i].getProj(); }
  inline TMatrix4 getSplitTexTM(int i) const { return splitOut[i].getTexTM(); }
  inline void setSplitZ(int i, float z) { splitSet[i].splitZ = z; }
  inline void setSplitZbias(int i, float zbias) { splitSet[i].ZBias = zbias; }

  inline void setSplitNum(int split_num) { splitNum = split_num; }
  inline int getSplitNum() const { return splitNum; }

  void precomputeSplit(int i);
  static void computeFocusTM(const BBox3 &box, TMatrix4 &tm);
  // void processObjects(RenderShadowObject* objects, int objects_count);
  void prepare(float wk, float hk, float znear, float zfar, const TMatrix4 &view, const Point3 &sun_dir, float shadow_dist);
  // objects, objects_count - objects (or objects forest). Can be NULL,0 - then function should work for whole frustum
  // ground - optional bounding box for ground (shadow receiver, but not caster)
  // wk, hk, znear,zfar - perspective projection parameters
  // view - view matrix
  // sun_dir - direction of the sun
  // shadow_dist - maximum shadow distance
};
