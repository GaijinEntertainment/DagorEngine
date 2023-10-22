//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_occlusionTest.h>
#include <3d/dag_stereoIndex.h>

class BaseTexture;
typedef BaseTexture Texture;
class MaskedOcclusionCulling;

enum CockpitReprojectionMode
{
  COCKPIT_NO_REPROJECT,            // Cockpit pixels will be leaved as is without reprojection.
  COCKPIT_REPROJECT_OUT_OF_SCREEN, // Cockpit pixels will be reprojected out of screen (i. e. "discarded").
  COCKPIT_REPROJECT_ANIMATED       // Cockpit pixels will be reprojected using separate matrix (for animated cockpit).
};

class OcclusionSystem : protected OcclusionTest<OCCLUSION_W, OCCLUSION_H> // Inherit for empty base class optimization
{
protected:
  enum
  {
    WIDTH = OCCLUSION_W,
    HEIGHT = OCCLUSION_H
  };

public:
  virtual ~OcclusionSystem() {}
  static OcclusionSystem *create();
  virtual void init() = 0;
  virtual void close() = 0;
  virtual void reset() = 0;
  virtual bool hasGPUFrame() const = 0;
  virtual void readGPUframe() = 0; // can be run from any thread, but will read from gpu texture (gpu readback)
  virtual void clear() = 0;        // if there is neither gpu frame, nor cpu frame, we need to have cleared
  // cockpit_distance is linear distance to max cockpit len. Everything close than this dist will not be reprojected and would be used
  // as is
  //  cockpit_anim = currentTransform * previousTransform^-1
  virtual void prepareGPUDepthFrame(vec3f pos, mat44f_cref view, mat44f_cref proj, mat44f_cref viewProj, float cockpit_distance,
    CockpitReprojectionMode cockpit_mode, const mat44f &cockpit_anim) = 0; // performs reprojection of latest GPU frame
  virtual void buildMips() = 0;
  virtual void prepareDebug() = 0;
  virtual void prepareNextFrame(vec3f viewPos, mat44f_cref view, mat44f_cref proj, mat44f_cref viewProj, float zn, float zf,
    Texture *mipped_depth, Texture *depth = 0, StereoIndex stereo_index = StereoIndex::Mono) = 0;
  virtual void setReprojectionUseCameraTranslatedSpace(bool enabled) = 0;
  virtual bool getReprojectionUseCameraTranslatedSpace() const = 0;

  static constexpr int DEFAULT_MAX_TEST_MIP = 3;
  // the bigger mip number, the coarser check, which matters only on close distance.
  // if dip is cheap, use bigger (4..5), if it is very expensive - use lower (0..2)
  // 3 provides reasonable average quality
  VECTORCALL static __forceinline int testVisibility(vec3f bmin, vec3f bmax, vec3f threshold, mat44f_cref clip,
    int mip = DEFAULT_MAX_TEST_MIP)
  {
    return OcclusionTest<WIDTH, HEIGHT>::testVisibility(bmin, bmax, threshold, clip, mip);
  }

  virtual void initSWRasterization() = 0;
  virtual void startRasterization(float zn) = 0;
  virtual void rasterizeBoxes(mat44f_cref viewproj, const bbox3f *box, const mat43f *mat, int cnt) = 0;
  virtual void rasterizeBoxes(mat44f_cref viewproj, const bbox3f *box, const mat44f *mat, int cnt) = 0;
  virtual void rasterizeQuads(mat44f_cref viewproj, const vec4f *verts, int cnt) = 0; // 4*cnt verts is required
  virtual void rasterizeMesh(mat44f_cref viewproj, vec3f bmin, vec3f bmax, const vec4f *verts, int vert_cnt,
    const unsigned short *faces, int tri_count) = 0;

  virtual void combineWithSWRasterization() = 0;
  virtual void prepareDebugSWRasterization() = 0;
  virtual void mergeOcclusionsZmin(MaskedOcclusionCulling **another_occl, uint32_t occl_count, uint32_t first_tile,
    uint32_t last_tile) = 0;
  virtual void getMaskedResolution(uint32_t &width, uint32_t &height) const = 0;

  enum
  {
    CULL_FRUSTUM = OcclusionTest<WIDTH, HEIGHT>::CULL_FRUSTUM,
    VISIBLE = OcclusionTest<WIDTH, HEIGHT>::VISIBLE,
    CULL_OCCLUSION = OcclusionTest<WIDTH, HEIGHT>::CULL_OCCLUSION
  };
};
