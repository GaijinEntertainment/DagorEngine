//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <shaders/dag_overrideStateId.h>
#include <3d/dag_resPtr.h>

class LandMeshManager;
class LandMeshRenderer;
class Point2;
class Point3;
class BBox3;
struct ToroidalHelper;

struct LandMeshData
{
  LandMeshManager *lmeshMgr;
  LandMeshRenderer *lmeshRenderer;
  shaders::OverrideStateId flipCullStateId;
  void (*start_render)();
  void (*decals_cb)(const BBox3 &landPart);
  int global_frame_id;
  int texture_size;
  uint32_t texflags; // TEXCF_SYSTEXCOPY|TEXCF_LOADONCE
  bool use_dxt;
  float texelSize;
  LandMeshData() :
    lmeshMgr(NULL),
    lmeshRenderer(NULL),
    start_render(NULL),
    decals_cb(NULL),
    texflags(0),
    use_dxt(false),
    global_frame_id(-1),
    texture_size(1024),
    texelSize(0)
  {}
};

void apply_last_clip_anisotropy(d3d::SamplerInfo &last_clip_sampler);
void preload_textures_for_last_clip();
void prepare_fixed_clip(UniqueTexHolder &last_clip, d3d::SamplerInfo &last_clip_sampler, LandMeshData &data, bool update_game_screen,
  const Point3 &view_pos);
void render_last_clip_in_box(const BBox3 &land_box_part, const Point2 &half_texel, const Point3 &view_pos, LandMeshData &data);
void render_last_clip_in_box_tor(const BBox3 &land_box_part, const Point2 &half_texel, const Point3 &view_pos, LandMeshData &data,
  Point2 &tor_offsets, ToroidalHelper &tor_helper);
