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

class ClusteredLightsSorter
{
public:
  ClusteredLightsSorter(OmniLightsManager &omni_lights, SpotLightsManager &spot_lights);
  void sortOmniLightsCPU(Tab<uint16_t> &visible_ids, vec4f cur_view_pos);
  void sortSpotLightsCPU(Tab<uint16_t> &visible_ids, vec4f cur_view_pos);
  void sortOmniLightsGPU(Sbuffer *buf, const Point3 &view_pos, int light_count);
  void sortSpotLightsGPU(Sbuffer *buf, const Point3 &view_pos, int light_count);

private:
  OmniLightsManager *omniLights;
  SpotLightsManager *spotLights;
  ComputeShader sortOmniCS;
  ComputeShader sortSpotCS;
};
