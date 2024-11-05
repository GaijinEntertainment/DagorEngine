// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint3.h>

class RingCPUBufferLock;

struct RadianceCache
{
  static void fixup_clipmap_settings(uint16_t &xz_res, uint16_t &y_res, uint8_t &clips);
  void initAtlas(int probes_h_min, int probes_h_max); // if probes_h_min != probes_h_max - wil auto adjust atlas
  void initClipmap(uint16_t w, uint16_t d, uint8_t clips, float probe_size_0);
  bool updateClipMapPosition(const Point3 &world_pos, bool updateAll);
  // debug
  void drawDebug(int debug_type = 0);
  ~RadianceCache();
  RadianceCache();
  bool hasReadBack() const { return bool(readBackData); }
  void initReadBack();
  void closeReadBack();
  bool getLatestFreeAmount(uint32_t &f) const
  {
    f = latestFreeAmount;
    return hasReadBack();
  }

  // mark needed radiance probes by: start_mark_new_probes, mark*, mark*, finish_marking
  void start_marking();
  void mark_new_probes(ComputeShaderElement &cs, int threads_w, int threads_h, int threads_z); // can be called with different shaders
  void finish_marking();
  // example
  void markProbesSample(int screen_w, int screen_h)
  {
    start_marking();
    mark_new_screen_probes(screen_w, screen_h);
    finish_marking();
  }
  void calcAllProbes();
  void mark_new_screen_probes(int spw, int sph);

  // update_all_in_frames - temporality how many frames to update all active probes
  // frames_to_keep_probe - new probe will be kept at least that amount of frames
  // old_probes_update_freq - will skip each N-1 frames for non-active probes. Non-active probes will be updated in each
  // updateInFrames*updateOldProbeFreq frames
  void setTemporalParams(uint32_t update_all_in_frames, uint32_t frames_to_keep_probe, uint32_t old_probes_update_freq); // default are
                                                                                                                         // 16, 4, 4
protected:
  void resetReadBack();
  enum class MarkProcessState
  {
    Started,
    Marked,
    Finished
  } state = MarkProcessState::Finished;
  void initAtlasInternal(int probes_w, int probes_h, int probe_oct_size);
  int validateCache(const char *step);
  float getProbeSizeClip(uint32_t clip) const { return probeSize0 * (1 << clip); }
  IPoint3 getNewClipCenter(uint32_t clip, const Point3 &world_pos) const
  {
    return ipoint3(floor(Point3::xzy(world_pos) / getProbeSizeClip(clip) + 0.5));
  }
  IPoint3 getNewClipLT(uint32_t clip, const Point3 &world_pos) const
  {
    return getNewClipCenter(clip, world_pos) - IPoint3(clipW, clipW, clipD) / 2;
  }

  bool updateClip(uint32_t clip_no);
  void set_clip_vars(int clip_no) const;
  void initVars();
  uint32_t updateInFrames = 16, // temporality how many frames to update all active probes
    probeFramesKeep = 4,        // new probe will be kept at least that amount of frames
    updateOldProbeFreq = 4;     // will skip each N-1 frames for non-active probes. Non-active probes will be updated in each
                                // updateInFrames*updateOldProbeFreq frames
  // the cache
  UniqueTexHolder current_radiance_cache, current_radiance_cache_hit_distance;
  UniqueBufHolder radiance_cache_positions; // current positions of radiance cache, (currently in absolute values)
  UniqueBufHolder radiance_cache_age;       // current age of radiance cache probes (latest frame it was used in)
  UniqueBufHolder radiance_cache_selected_temporal_probes;

  // clipmap
  UniqueBufHolder radiance_cache_indirection_clipmap; // positions of radiance cache
  UniqueBuf used_radiance_cache_mask;
  UniqueBufHolder new_radiance_cache_probes_needed, free_radiance_cache_indices_list;

  UniqueBuf radiance_cache_indirect_buffer;
  eastl::unique_ptr<ComputeShaderElement> get_fake_radiance_cache_res_cs;

  eastl::unique_ptr<ComputeShaderElement> radiance_cache_free_unused_after_movement_cs, radiance_cache_toroidal_movement_cs,
    clear_radiance_cache_clipmap_indirection_cs;
  eastl::unique_ptr<ComputeShaderElement> clear_radiance_cache_positions_cs, mark_used_radiance_cache_probes_cs,
    free_unused_radiance_cache_probes_cs, allocate_radiance_cache_probes_cs, radiance_cache_clear_new_cs;

  eastl::unique_ptr<ComputeShaderElement> calc_radiance_cache_new_cs, calc_temporal_radiance_cache_cs,
    select_temporal_radiance_cache_cs;
  eastl::unique_ptr<ComputeShaderElement> filter_temporal_radiance_cache_cs;
  eastl::unique_ptr<ComputeShaderElement> radiance_cache_create_dispatch_indirect_cs;
  eastl::unique_ptr<RingCPUBufferLock> readBackData;
  // irradiance
  UniqueTexHolder radiance_cache_irradiance;
  eastl::unique_ptr<ComputeShaderElement> rd_calc_irradiance_cache_new_cs, rd_calc_irradiance_cache_temporal_cs;

  void initIrradiance();
  void calcIrradiance();

  uint32_t exhaustedCacheFramesCnt = 0, freeCacheFramesCnt = 0;
  uint32_t latestFreeAmount = 0;
  bool readBackCounter(uint32_t &freeAmount);
  void adjustAtlasSize();
  uint32_t totalProbesCount = 0;
  int probeSize = 0;
  int atlasSizeInProbesW = 0, atlasSizeInProbesH = 0;
  int temporalProbesPerFrameCount = 0;
  int minAtlasSizeInProbes = 256, maxAtlasSizeInProbes = 256 * 256;
  int atlasSizeInTexelsW = 0, atlasSizeInTexelsH = 0;
  bool validHistory = false;

  uint32_t clipW = 0, clipD = 0;
  float probeSize0 = 0;
  struct Clip
  {
    IPoint3 prevLt = {0, 0, 0}; // yz are swizzled to worldPos
    float prevProbeSize = 0;

    IPoint3 lt = {0, 0, 0}; // yz are swizzled to worldPos
    float probeSize = 0;
  };
  dag::Vector<Clip> clipmap;
  uint32_t frame = 0;
  bool supportUnorderedLoad = false;
  bool filterWithDataRace = false; // filtering with data race is not stable, so switch it off

  DynamicShaderHelper drawDebugAllProbes, drawDebugIndirection;
  void initDebug();
};
