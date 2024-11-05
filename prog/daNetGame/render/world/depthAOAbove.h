// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_frustum.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <generic/dag_carray.h>

#include <render/depthAOAboveRenderer.h>

#define DEPTH_AROUND_TEX_SIZE 1024
#define DEPTH_AROUND_DISTANCE 320

class WorldRenderer;

class DepthAOAboveContext
{
public:
  DepthAOAboveContext(int tex_size, float depth_around_distance, bool render_transparent = false);
  ~DepthAOAboveContext();

  bool prepare(const Point3 &view_pos, float scene_min_z, float scene_max_z); // Return true if some tp jobs were added
  void render(WorldRenderer &wr, const TMatrix &view_itm);
  inline void invalidateAO(bool force) { renderer.invalidateAO(force); }
  inline void invalidateAO(const BBox3 &box) { renderer.invalidateAO(box); }

private:
  static const int g_max_visibility_jobs = 8;

  struct AsyncVisiblityJob : public cpujobs::IJob
  {
    Frustum cullingFrustum;
    TMatrix4_vec4 cullTm;
    Point3 viewPos;
    struct RiGenVisibility *visibility = nullptr;

    void doJob() override;
  };

  DepthAOAboveRenderer renderer;
  carray<AsyncVisiblityJob, g_max_visibility_jobs> cullJobs;

  friend class RenderDepthAOCB;
};
