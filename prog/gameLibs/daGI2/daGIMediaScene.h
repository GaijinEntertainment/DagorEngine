// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/integer/dag_IPoint3.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>

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

protected:
  void initHistory();
  void initVars();
  bool updateClip(uint32_t clip_no, const IPoint3 &lt, float newProbeSize);
  float get_voxel_size(uint32_t i) const { return (1 << i) * voxelSize0; }
  IPoint3 getNewClipLT(uint32_t clip, const Point3 &world_pos) const
  {
    return ipoint3(floor(Point3::xzy(world_pos) / get_voxel_size(clip) + 0.5)) - IPoint3(clipW, clipW, clipD) / 2;
  }
  void setClipVars(int clip_no) const;

  PostFxRenderer dagi_media_scene_debug, dagi_media_scene_trace_debug;
  struct Clip
  {
    IPoint3 lt = {-100000, -100000, 100000};
    float voxelSize = 0;
  };
  dag::Vector<Clip> clipmap;

  eastl::unique_ptr<ComputeShaderElement> dagi_media_scene_reset_cs, dagi_media_scene_from_gbuf_cs, dagi_media_toroidal_movement_cs;
  float temporalSpeed = 0.125;
  UniqueTexHolder dagi_media_scene;
  uint32_t gbuf_update_frame = 0;
  uint16_t clipW = 0, clipD = 0, fullAtlasResD = 0;
  float voxelSize0 = 0;
  bool cleared = false;
};
