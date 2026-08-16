//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/lights/omniLightsManager.h>
#include <render/lights/spotLightsManager.h>
#include <generic/dag_tab.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_Point3.h>
#include <shaders/dag_computeShaders.h>

class Sbuffer;

class LightsSorter
{
public:
  LightsSorter(OmniLightsManager &omni_lights, SpotLightsManager &spot_lights);
  void sortOmniLightsCPU(Tab<uint16_t> &visible_ids, vec4f cur_view_pos);
  void sortSpotLightsCPU(Tab<uint16_t> &visible_ids, vec4f cur_view_pos);
  // zfar must be > 0. It does not cull or clip lights beyond it - it only normalizes the
  // shader's internal half-precision distance encoding (dist/zfar, see sort_omni/spot_lights_cs
  // in lights_partition.dshl), so pick something close to the actual range of the lights being
  // sorted rather than the camera's full draw distance: if zfar ends up more than about 2^24
  // (~16.8 million) times the farthest sorted light's distance, every real key underflows to
  // zero and the sort silently stops reordering lights, in release builds too - only a debug
  // assert (zfar > 0) guards against misuse.
  void sortOmniLightsGPU(Sbuffer *buf, Sbuffer *count_buf, const Point3 &view_pos, float zfar);
  void sortSpotLightsGPU(Sbuffer *buf, Sbuffer *count_buf, const Point3 &view_pos, float zfar);

private:
  OmniLightsManager *omniLights;
  SpotLightsManager *spotLights;
  ComputeShader sortOmniCS;
  ComputeShader sortSpotCS;
};
