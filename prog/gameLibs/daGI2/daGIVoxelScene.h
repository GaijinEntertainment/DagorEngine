// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>

struct DaGIVoxelScene
{
  enum class Aniso
  {
    Off,
    On
  };
  enum class Radiance
  {
    Colored,
    Luma
  };
  void init(uint32_t sdfW, uint32_t sdfH, float scale, uint32_t first_sdf_clip, uint32_t clips, Aniso aniso, Radiance rad);
  ~DaGIVoxelScene();

  void setTemporalSpeedFromGbuf(float speed) { temporalSpeed = clamp(speed, 0.f, 1.f); } // 0 - off, 1 - max
  void setTemporalFromGbufStable(bool on) { temporalStable = on; } // if on, there is no randomization inside each voxel
  void afterReset() { cleared = false; }
  void updateFromGbuf();
  void update();
  void rbNone();
  void rbFinish();
  void debugRender();
  uint32_t getSdfClips() const { return clips + firstSdfClip; }
  PostFxRenderer dagi_lit_scene_voxels_debug;
  PostFxRenderer dagi_world_scene_voxel_scene_debug;
  eastl::unique_ptr<ComputeShaderElement> dagi_voxel_scene_reset_cs, dagi_voxel_lit_scene_from_gbuf_cs;
  float temporalSpeed = 0.125;
  UniqueTexHolder dagi_lit_voxel_scene, dagi_lit_voxel_scene_alpha;
  uint32_t gbuf_update_frame = 0;
  uint16_t resW = 0, resD = 0, clips = 0, fullAtlasResD = 0;
  uint8_t firstSdfClip = 0;
  bool anisotropy = false;
  bool lumaOnly = false;
  bool temporalStable = false;
  bool cleared = false;
};
