// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_carray.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <render/voxelClip.h>

struct ShaderVariableInfo;

struct RadianceGrid
{
  void init(uint8_t w, uint8_t h, uint8_t clips, float probeSize0, uint8_t additional_iclips, float irradiance_detail,
    uint32_t batch_count, float temporal_speed);
  static void fixup_settings(uint8_t &w, uint8_t &d, uint8_t &clips, uint8_t &additional_iclips, float &irradianceProbeDetail);
  void updatePos(const Point3 &at, bool update_all = false);
  // debug
  void drawDebug(int debug_type = 0);
  void drawDebugIrradiance(int debug_type = 0);
  void afterReset();
  void resetHistoryAge();
  void invalidateFrustum(const Frustum &invalid_frustum, const BBox3 &update_box);
  ~RadianceGrid();

  float get_probe_size(uint32_t i) const { return radiance.get_probe_size(i); }
  float get_irradiance_probe_size(uint32_t i) const
  {
    return (irradianceType == IrradianceType::DIRECT ? radiance : irradiance).get_probe_size(i);
  }
  int get_irradiance_clips_count() const { return (irradianceType == IrradianceType::DIRECT ? radiance : irradiance).get_clips(); }
  int get_irradiance_clip_w() const { return (irradianceType == IrradianceType::DIRECT ? radiance : irradiance).clipW; }
  float horizontalSize() const { return get_probe_size(radiance.clipmap.size() - 1) * radiance.clipW; }

  bool isDetailedIrradiance() const { return irradianceType == IrradianceType::DETAILED; }
  void updateTemporal();

protected:
  void initClipmap(uint32_t w, uint32_t h, uint32_t clips, uint32_t oct_res, float probeSize0);
  void initClipmapIrradiance(uint32_t w, uint32_t h, uint32_t clips, float probeSize0);
  void createIrradiance();
  void initTemporal(uint32_t batch_count, float temporal_speed);
  void initVars();

  void initHistory(); // called after reset
  bool updateClip(uint32_t clip_no, const Point3 &world_pos);
  bool updateClipIrradiance(uint32_t clip_no, const Point3 &world_pos, bool last_clip);

  UniqueTexHolder dagi_radiance_grid, dagi_radiance_grid_dist;
  UniqueBufHolder dagi_radiance_grid_selected_probes;
  UniqueTexHolder dagi_radiance_grid_probes_age;
  UniqueBuf dagi_rad_grid_indirect_buffer;

  eastl::unique_ptr<ComputeShaderElement> dagi_radiance_grid_calc_temporal_cs, dagi_radiance_grid_toroidal_movement_cs,
    dagi_radiance_grid_select_temporal_cs, dagi_radiance_grid_create_indirect_cs, dagi_radiance_grid_clear_temporal_cs;

  UniqueTexHolder dagi_irradiance_grid_sph0, dagi_irradiance_grid_sph1;
  UniqueTexHolder dagi_irradiance_grid_probes_age; // only if detailed irradiance
  eastl::unique_ptr<ComputeShaderElement> dagi_radiance_grid_calc_temporal_irradiance_cs,
    dagi_radiance_grid_toroidal_movement_irradiance_cs, dagi_radiance_grid_invalidate_cs;

  eastl::unique_ptr<ComputeShaderElement> dagi_irradiance_grid_toroidal_movement_interpolate_cs,
    dagi_irradiance_grid_toroidal_movement_trace_cs, dagi_irradiance_grid_select_temporal_cs, dagi_irradiance_grid_calc_temporal_cs,
    dagi_irradiance_grid_spatial_filter_cs, dagi_irradiance_grid_invalidate_cs;

  bool validHistory = false;
  enum class IrradianceType
  {
    DIRECT = 0,
    DETAILED = 1
  } irradianceType = IrradianceType::DIRECT;
  struct Clipmap
  {
    uint32_t clipW = 1, clipD = 1;
    dag::Vector<uint32_t> temporalFrames;
    dag::Vector<VoxelClip> clipmap;

    uint32_t batchCount = 0, temporalProbesBufferSize = 0;
    float temporalSpeed = 0;
    float probeSize0 = 0;
    uint32_t temporalFrame = 0;
    int get_clips() const { return clipmap.size(); }
    float get_probe_size(uint32_t i) const { return (1 << i) * probeSize0; }
    void resetHistory(uint32_t clips);
    void invalidateFrustum(const BBox3 &update_box, const ComputeShaderElement *invalidate_cs,
      const ShaderVariableInfo &update_lt_coord_var_id, const ShaderVariableInfo &update_sz_coord_var_id);
    struct Temporal
    {
      uint32_t clip = 0, batchSize = 0;
    };
    Temporal updateTemporal(const uint32_t tempOfs);
  } radiance, irradiance;
  uint32_t octRes = 1, selectedProbesDwords = 0;
  bool lastUpdatedAnyIrradianceClip = false, lastUpdatedAnyRadianceClip = false;

  DynamicShaderHelper drawDebugAllProbes, drawDebugAllProbesIrradiance;
  static void clearPosition();
  static void clearRadiancePosition();
  static void clearIrradiancePosition();
  void updatePosIrradiance(const Point3 &at, bool update_all);
  void updateTemporalIrradiance();
  void updateTemporalRadiance();
  void createIndirect();
  void selectProbes(ComputeShaderElement &cs, uint32_t batchSize);
};
