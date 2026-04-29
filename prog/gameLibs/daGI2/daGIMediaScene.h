// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/integer/dag_IPoint3.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <render/voxelClip.h>

struct DaGIMediaScene
{
  void init(uint32_t w, uint32_t h, uint32_t clips, float voxel0);

  void setTemporalSpeedFromGbuf(float speed) { temporalSpeed = clamp(speed, 0.f, 1.f); } // 0 - off, 1 - max
  void updateFromGbuf();
  void updatePos(const Point3 &pos, bool update_all = false);
  void rbNone();
  void rbFinish();
  void debugRender();
  void afterReset();
  void resetHistoryAge();
  void invalidateBox(const BBox3 &box);
  bool isValid() const;

protected:
  void initHistory();
  void initVars();
  bool updateClip(uint32_t clip_no, const Point3 &world_pos);
  float get_voxel_size(uint32_t i) const { return (1 << i) * voxelSize0; }
  void setClipVars(int clip_no, float voxel_size) const;

  PostFxRenderer dagi_media_scene_debug, dagi_media_scene_trace_debug;

  dag::Vector<VoxelClip> clipmap;

  eastl::unique_ptr<ComputeShaderElement> dagi_media_scene_reset_cs, dagi_media_scene_from_gbuf_cs, dagi_media_toroidal_movement_cs;
  float temporalSpeed = 0.125;
  UniqueTexHolder dagi_media_scene;
  uint32_t gbuf_update_frame = 0;
  uint16_t clipW = 0, clipD = 0, fullAtlasResD = 0;
  float voxelSize0 = 0;
  bool cleared = false;
};
