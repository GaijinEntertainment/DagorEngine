// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_frustum.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <generic/dag_carray.h>

#include <render/depthAOAboveRenderer.h>

static constexpr int DEPTH_AROUND_TEX_SIZE = 1024;
static constexpr int DEPTH_AROUND_DISTANCE_SECOND_CASCADE = 320;
static constexpr int DEPTH_AROUND_DISTANCE = 80;
static constexpr int DEPTH_AROUND_EXTRA_CASCADE_MUL = DEPTH_AROUND_DISTANCE_SECOND_CASCADE / DEPTH_AROUND_DISTANCE;

class WorldRenderer;

class DepthAOAboveContext
{
public:
  DepthAOAboveContext(int tex_size, float depth_around_distance, bool render_transparent = false);
  ~DepthAOAboveContext();

  bool prepare(const Point3 &view_pos, float scene_min_z, float scene_max_z); // Return true if some tp jobs were added
  void render(WorldRenderer &wr, const TMatrix &view_itm);
  void invalidateAO(bool force) { renderer.invalidateAO(force); }
  void invalidateAO(const BBox3 &box) { renderer.invalidateAO(box); }
  // We only need refreshed regions of the cascade that was recently updated
  const Tab<BBox2> &getRefreshedRegions() const
  {
    int lastRenderedCascade = (cascadeToUpdate + 1) % DepthAOAboveRenderer::MAX_DEPTH_ABOVE_CASCADES;
    return renderer.getRefreshedRegionsForCascade(lastRenderedCascade);
  }
  void waitCullJobs();
  bool isValidAtAnyQuality() const { return renderer.isValidAtAnyQuality(); }

private:
  static const int g_max_visibility_jobs = 8;

  struct AsyncVisiblityJob : public cpujobs::IJob
  {
    Frustum cullingFrustum;
    TMatrix4_vec4 cullTm;
    Point3 viewPos;
    struct RiGenVisibility *visibility = nullptr;

    const char *getJobName(bool &) const override { return "AsyncVisiblityJob"; }
    void doJob() override;
  };

  DepthAOAboveRenderer renderer;
  carray<AsyncVisiblityJob, g_max_visibility_jobs> cullJobs;
  int cascadeToUpdate = 0;

  friend class RenderDepthAOCB;
};
