// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/integer/dag_IPoint3.h>
#include <generic/dag_carray.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>

struct SkyVisibility
{
  void init(uint32_t w, uint32_t h, uint32_t clips, float probeSize0, uint32_t frames_to_update_clip, bool repl_to_irradiance);
  void updatePos(const Point3 &at, bool update_all = false);
  void setHasSimulatedBounceCascade(uint32_t count) { simulatedBounceCascadesCount = min(count, clipmap.size() - 1); }
  float get_probe_size(uint32_t i) const { return (1 << i) * probeSize0; }
  float horizontalSize() const { return get_probe_size(clipmap.size() - 1) * clipW; }
  // debug
  void drawDebug(int debug_type = 0);
  void setReplicateToIrradiance(bool);
  void setTracingDistances(float max_trace_dist_in_meters, float upscale_intersect_trace_in_probes, float air_trace_dist_scale)
  {
    maxTraceDist = max_trace_dist_in_meters;
    intersectTraceDistProbes = upscale_intersect_trace_in_probes;
    airTraceDistScale = min(1.f, air_trace_dist_scale);
  }
  uint32_t getRequiredTemporalBufferSize() const;
  void setTemporalBuffer(const ManagedBuf &buf);
  void afterReset() { resetClipmap(clipmap.size()); }
  ~SkyVisibility();

protected:
  void resetClipmap(uint32_t clips);
  void initClipmap(uint32_t w, uint32_t h, uint32_t clips);
  void initTemporal(uint32_t frames_to_update_clip);
  void initVars();
  void setTraceDist(uint32_t clip_no);

  void initHistory(); // called after reset
  void updateTemporal(bool recalc_probes);
  bool updateClip(uint32_t clip_no, const IPoint3 &lt, float newProbeSize, bool updateLast);
  void setClipVars(int clip_no) const;
  IPoint3 getNewClipLT(uint32_t clip, const Point3 &world_pos) const;

  UniqueTexHolder dagi_sky_visibility_sph;
  UniqueTexHolder dagi_sky_visibility_age;
  Sbuffer *dagi_sky_visibility_selected_probes = nullptr;
  UniqueBufHolder dagi_sky_visibility_probabilities;
  UniqueBufHolder dagi_sky_vis_indirect_buffer;

  eastl::unique_ptr<ComputeShaderElement> dagi_sky_visibility_calc_temporal_cs, dagi_sky_visibility_toroidal_movement_interpolate_cs,
    dagi_sky_visibility_toroidal_movement_trace_cs, dagi_sky_visibility_select_temporal_cs, dagi_sky_visibility_create_indirect_cs,
    dagi_sky_visibility_clear_temporal_cs, dagi_sky_visibility_toroidal_movement_spatial_filter_split_cs,
    dagi_sky_visibility_toroidal_movement_spatial_filter_split_apply_cs;
  uint32_t clipW = 1, clipD = 1;
  struct Clip
  {
    IPoint3 lt = {-100000, -100000, 100000};
    float probeSize = 0;
  };
  dag::Vector<uint32_t> temporalFrames;
  dag::Vector<Clip> clipmap;
  uint32_t totalFramesPerClipToUpdate = ~0u, temporalProbesBufferSize = 0, selectedProbesBufferSize = 0;

  bool validHistory = false, supportUnorderedLoad = true, replicateToIrradiance = false;
  float probeSize0 = 0;
  float maxTraceDist = 64.f, intersectTraceDistProbes = 6.f, airTraceDistScale = 0.75f;
  uint32_t simulatedBounceCascadesCount = 0;
  uint32_t temporalFrame = 0;
  DynamicShaderHelper drawDebugAllProbes;
  void setClipmapVars();
};
