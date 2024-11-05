// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "cloudsRendererData.h"

class CloudsRenderer
{
public:
  void init();

  void setCloudsOfs(const DPoint2 &xz) { cloudsOfs += xz; }
  DPoint2 getCloudsOfs() const { return cloudsOfs; }

  void setCloudsOffsetVars(float current_clouds_offset, float world_size);
  void setCloudsOffsetVars(CloudsRendererData &data, float world_size) { setCloudsOffsetVars(data.currentCloudsOffset, world_size); }
  void render(CloudsRendererData &data, const TextureIDPair &depth, const TextureIDPair &prev_depth, const Point2 &wind_change_ofs,
    float worldSize, const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 *world_pos = nullptr);

  void renderFull(CloudsRendererData &data, const TextureIDPair &downsampled_depth, TEXTUREID target_depth,
    const Point4 &target_depth_transform, const TMatrix &view_tm, const TMatrix4 &proj_tm);

private:
  static void set_program(ShaderElement *oe, ShaderElement *ne);
  void renderDirect(CloudsRendererData &data);

  int getNotLesserDepthLevel(CloudsRendererData &data, int &depth_levels, Texture *depth);
  void setCloudsOriginOffset(float world_size);

  void renderTiledDist(CloudsRendererData &data);
  Point2 currentCloudsOffsetCalc(float world_size) const
  {
    G_ASSERT_RETURN(world_size != 0.f, point2(cloudsOfs));
    return point2(cloudsOfs - floor(cloudsOfs / world_size) * world_size);
  }

  struct DispatchGroups2D
  {
    int x, y, z, w;
  };
  DispatchGroups2D set_dispatch_groups(int w, int h, int w_sz, int h_sz, bool lowres_close_clouds);

  PostFxRenderer clouds2_direct;
  PostFxRenderer clouds2_temporal_ps, clouds2_close_temporal_ps;
  PostFxRenderer clouds2_taa_ps, clouds2_taa_ps_has_empty, clouds2_taa_ps_no_empty;
  PostFxRenderer clouds2_apply, clouds2_apply_has_empty, clouds2_apply_no_empty;
  PostFxRenderer clouds2_apply_blur_ps;
  eastl::unique_ptr<ComputeShaderElement> clouds2_apply_blur_cs;

  eastl::unique_ptr<ComputeShaderElement> clouds_create_indirect;

  eastl::unique_ptr<ComputeShaderElement> clouds2_temporal_cs, clouds2_close_temporal_cs;
  eastl::unique_ptr<ComputeShaderElement> clouds2_taa_cs, clouds2_taa_cs_has_empty, clouds2_taa_cs_no_empty;
  eastl::unique_ptr<ComputeShaderElement> clouds_tile_dist_prepare_cs, clouds_tile_dist_min_cs;
  PostFxRenderer clouds_tile_dist_prepare_ps, clouds_tile_dist_min_ps;

  eastl::unique_ptr<ComputeShaderElement> clouds_close_layer_outside, clouds_close_layer_clear;
  eastl::unique_ptr<ComputeShaderElement> clouds_tile_dist_count_cs;

  DPoint2 cloudsOfs = {0, 0}, currentCloudsOfs = {0, 0};
};