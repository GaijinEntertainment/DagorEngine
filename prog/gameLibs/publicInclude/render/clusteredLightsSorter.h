//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/omniLightsManager.h>
#include <render/spotLightsManager.h>
#include <generic/dag_tab.h>
#include <vecmath/dag_vecMathDecl.h>

class ClusteredLightsSorter
{
public:
  ClusteredLightsSorter(OmniLightsManager &omni_lights, SpotLightsManager &spot_lights);
  void sortOmniLightsCPU(Tab<uint16_t> &visible_ids, vec4f cur_view_pos);
  void sortSpotLightsCPU(Tab<uint16_t> &visible_ids, vec4f cur_view_pos);

private:
  OmniLightsManager *omniLights;
  SpotLightsManager *spotLights;
};
