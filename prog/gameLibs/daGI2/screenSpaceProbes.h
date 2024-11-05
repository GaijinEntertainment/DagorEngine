// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// #include <frustumCulling/frustumPlanes.h>
#include "screenProbes/screenprobes_indirect.hlsli"
#include <generic/dag_carray.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include "giFrameInfo.h"
#include <math/integer/dag_IPoint3.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_resizableTex.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_DynamicShaderHelper.h>

struct ScreenSpaceProbes
{
  void init(int tile_size, int sw, int sh, int max_sw, int max_sh, int radiance_oct_res, bool need_distance, float temporality);
  // todo: irradianceRes can be get from shader var
  void initial_probes_placement();
  void additional_probes_placement();
  void rearrange_probes_placement();
  void probes_placement();
  void clear_probes();
  void create_indirect();
  void calc_probe_pos();
  void trace_probe_radiance(float quality, bool angle_filtering);
  void calc_probe_radiance(float quality, bool angle_filtering);
  void calc_probe_irradiance();
  DaGIFrameInfo prevFrameInfo;

  void calcProbesPlacement(const DPoint3 &world_view_pos, const TMatrix &viewItm, const TMatrix4 &proj, float zn, float zf);
  void relightProbes(float quality, bool angle_filtering);
  void initDebug();
  void drawDebugRadianceProbes(int debug_type = 0);

  void drawDebugTileClassificator();
  void drawSequence(uint32_t c);
  ~ScreenSpaceProbes();
  void afterReset();

protected:
  void filter_probe_radiance(const ComputeShaderElement &e, BaseTexture *dest);
  DynamicShaderHelper drawDebugAllProbes, drawJitteredPoints;
  PostFxRenderer tileClassificatorDebug;

  ResizableTex screenspace_irradiance, current_radiance, current_radiance_distance;
  carray<ResizableTex, 2> screenspaceRadiance;

  carray<UniqueBuf, 2> screenspaceProbePos;
  UniqueBuf screenprobes_indirect_buffer;
  UniqueBufHolder screenspace_probes_list;
  UniqueBufHolder linked_list_additional_screenspace_probes;
  UniqueBufHolder tileClassificator;
  eastl::unique_ptr<ComputeShaderElement> calc_screenspace_radiance_cs, calc_screenspace_selected_radiance_cs,
    calc_screenspace_irradiance_cs, calc_screenspace_irradiance_sph3_cs, filter_screenspace_radiance_cs,
    temporal_filter_screenspace_radiance_cs, temporal_only_filter_screenspace_radiance_cs;
  eastl::unique_ptr<ComputeShaderElement> clear_additional_screenspace_probes_count_cs, initial_probes_placement_cs,
    additional_probes_placement_cs, rearrange_additional_probes_placement_cs;
  eastl::unique_ptr<ComputeShaderElement> screenspace_probes_create_dispatch_indirect_cs;
  float additionalPlanesAllocation = 0.5; // over allocation, we probably need no more than 25%

  int radianceRes = 8;
  struct ScreenRes
  {
    int sw = 0, sh = 0, w = 0, h = 0, atlasW = 0, atlasH = 0, tileSize = 0;
    uint32_t screenProbes = 0, totalProbes = 0, additionalProbes = 0;
    bool operator==(const ScreenRes &b) const
    {
      return sw == b.sw && sh == b.sh && tileSize == b.tileSize && atlasW == b.atlasW && atlasH == b.atlasH;
    }
    bool operator!=(const ScreenRes &b) const { return !(*this == b); }
  } allocated, current, prev, next;

  static ScreenRes calc(int tile_size, int sw, int sh, float additional);
  void setCurrentScreenRes(const ScreenRes &);
  void setPrevScreenRes(const ScreenRes &sr);
  void ensureRadianceRes(int cf, const ScreenRes &sr);
  void initMaxResolution(int tile_size, int max_sw, int max_sh, int radiance_res, bool need_distance, float temporality);
  void initProbesList(uint32_t total, bool history);


  int irradianceRes = 6;
  uint32_t frame = 0, radianceFrame = 0;
  bool useRGBEFormat = false, supportRGBE_UAV = false;
  float currentTemporality = 1.f;
  bool validHistory = false;
};
