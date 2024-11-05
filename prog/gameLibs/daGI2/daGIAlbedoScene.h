// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint3.h>
#include <shaders/dag_computeShaders.h>
#include <EASTL/fixed_function.h>

class BBox3;

enum class UpdateAlbedoStatus
{
  NOTHING,
  RENDERED,
  CANCEL,
  NOT_READY
};

typedef eastl::fixed_function<64, UpdateAlbedoStatus(const BBox3 &box, float voxelSize, uintptr_t &handle)> dagi_albedo_rasterize_cb;

struct DaGIAlbedoScene
{
  static void fixup_settings(uint8_t &w, uint8_t &d, uint8_t &c);
  static uint16_t block_resolution();
  static uint16_t moving_threshold_blocks();
  static float clip_block_size(float voxel0Size, uint8_t clip);
  static float clip_voxel_size(float voxel0Size, uint8_t clip);
  void init(uint8_t w, uint8_t d, uint8_t c, float voxel0Size, float average_allocation = 0.5);

  bool update(const Point3 &pos, bool update_all, const dagi_albedo_rasterize_cb &cb);
  void debugRender();
  void debugRenderScreen();
  void updateFromGbuf();
  void setTemporalSpeedFromGbuf(float speed) { temporalSpeed = clamp(speed, 0.f, 1.f); } // 0 - off, 1 - max
  void setTemporalFromGbufStable(bool on) { temporalStable = on; } // if on, there is no randomization inside each voxel
  void fixBlocks();
  void afterReset();

protected:
  void close();
  void setUAV(uint32_t);
  void resetUAV(uint32_t);
  void validateClipmap(const char *n);
  float getInternalBlockSizeClip(uint32_t clip) const;
  bool updateClip(const dagi_albedo_rasterize_cb &cb, uint32_t clip_no, const IPoint3 &lt, float newBlockSize);
  void setClipVars(int clip_no) const;
  void initHistory(); // called after reset

  IPoint3 getNewClipLT(uint32_t clip, const Point3 &world_pos) const;

  PostFxRenderer dagi_albedo_scene_debug;
  PostFxRenderer dagi_world_scene_albedo_scene_debug;
  eastl::unique_ptr<ComputeShaderElement> albedo_scene_reset_cs;
  UniqueTexHolder dagi_albedo_atlas;
  UniqueBufHolder dagi_albedo_indirection__free_indices_list;
  UniqueBuf dagi_albedo_indirect_args;
  eastl::unique_ptr<ComputeShaderElement> dagi_clear_albedo_freelist_cs, dagi_clear_albedo_texture_cs, dagi_fix_empty_alpha_cs;
  eastl::unique_ptr<ComputeShaderElement> dagi_albedo_toroidal_movement_cs, dagi_albedo_after_toroidal_movement_create_indirect_cs,
    dagi_albedo_allocate_after_toroidal_movement_cs, dagi_albedo_allocate_after_toroidal_movement_pass2_cs,
    dagi_albedo_fix_insufficient_cs;
  eastl::unique_ptr<ComputeShaderElement> dagi_albedo_scene_from_gbuf_cs;
  uint16_t clipW = 0, clipD = 0;
  uint32_t atlasW = 0, atlasD = 0;
  float voxel0Size = 0;
  uint32_t gbuf_update_frame = 0, fix_update_frame = 0;
  bool validHistory = false;
  bool temporalStable = true;
  float temporalSpeed = 0.125;
  struct Clip
  {
    IPoint3 lt = {-100000, -100000, 100000};
    float internalBlockSize = 0;
  };
  dag::Vector<Clip> clipmap;
};
