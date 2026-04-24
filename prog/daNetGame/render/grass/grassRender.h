// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/gpuGrass.h>
#include <EASTL/optional.h>

struct CameraParams;

void render_grass_prepass(const GrassView view);
void render_grass_visibility_pass(const GrassView view);
void resolve_grass_visibility(const GrassView view);
void render_grass(const GrassView view);
void init_grass(const DataBlock *grass_settings_from_level);
void erase_grass(const Point3 &world_pos, float radius);
void grass_invalidate();
void grass_invalidate(const dag::ConstSpan<BBox3> &boxes);
void grass_prepare(const CameraParams &main_view,
  const CameraParams &prev_main_view,
  const eastl::optional<CameraParams> &camcam_view,
  const eastl::optional<CameraParams> &prev_camcam_view);
void grass_prepare_per_camera(const CameraParams &main_view);
void grass_prepare_per_view(const CameraParams &view, const CameraParams &prev_view, const bool is_main_view);
