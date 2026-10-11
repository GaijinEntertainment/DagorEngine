//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daFracture/core/meshSlicing.h>


namespace frx
{

struct ShatterMaterialProfileImGuiState
{
  ShatterMaterialProfile profile;

  PlaneShatterFlavorSettings planeSettings;
  RoughShatterFlavorSettings roughSettings;
  int flavorIndex = 0;

  void setProfile(const ShatterMaterialProfile &p)
  {
    profile = p;
    flavorIndex = int(p.flavor.index());
    if (const auto *pl = eastl::get_if<PlaneShatterFlavorSettings>(&p.flavor))
      planeSettings = *pl;
    else if (const auto *rg = eastl::get_if<RoughShatterFlavorSettings>(&p.flavor))
      roughSettings = *rg;
  }
};

bool imgui_draw_shatter_material_profile(ShatterMaterialProfileImGuiState &state);

} // namespace frx
