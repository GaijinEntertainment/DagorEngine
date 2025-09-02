//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/utility.h>
#include <debug/dag_textMarks.h>
#include <math/dag_e3dColor.h>
#include <generic/dag_tab.h>

namespace gbuffer_debug_color_legends
{
static const Tab<eastl::pair<const char *, E3DCOLOR>> mipLegend = {
  {"MIP LEVEL: 0", E3DCOLOR_MAKE(255, 255, 255, 255)},
  {"MIP LEVEL: 1", E3DCOLOR_MAKE(255, 0, 0, 255)},
  {"MIP LEVEL: 2", E3DCOLOR_MAKE(255, 127, 0, 255)},
  {"MIP LEVEL: 3", E3DCOLOR_MAKE(255, 255, 0, 255)},
  {"MIP LEVEL: 4", E3DCOLOR_MAKE(51, 255, 0, 255)},
  {"MIP LEVEL: 5", E3DCOLOR_MAKE(0, 127, 0, 255)},
  {"MIP LEVEL: 6", E3DCOLOR_MAKE(0, 127, 127, 255)},
  {"MIP LEVEL: 7", E3DCOLOR_MAKE(0, 255, 255, 255)},
  {"MIP LEVEL: 8", E3DCOLOR_MAKE(0, 0, 255, 255)},
  {"MIP LEVEL: 9", E3DCOLOR_MAKE(0, 0, 127, 255)},
  {"MIP LEVEL: 10", E3DCOLOR_MAKE(127, 0, 127, 255)},
  {"MIP LEVEL: 10+", E3DCOLOR_MAKE(255, 0, 255, 255)},
};

static const Tab<eastl::pair<const char *, E3DCOLOR>> texelDensityLegend = {{"Limit: texDensity<0.1", E3DCOLOR_MAKE(255, 0, 255, 255)},
  {"texDensity: 0.1", E3DCOLOR_MAKE(0, 0, 255, 255)}, {"texDensity: 1", E3DCOLOR_MAKE(255, 255, 255, 255)},
  {"texDensity: 9.5", E3DCOLOR_MAKE(255, 0, 0, 255)}, {"Limit: texDensity>9.5", E3DCOLOR_MAKE(255, 255, 0, 255)}};

static const Tab<eastl::pair<const char *, E3DCOLOR>> debugMeshLegend = {
  {"LOD/drawElements:", E3DCOLOR_MAKE(0, 0, 0, 1)},
  {"LEVEL: 0", E3DCOLOR_MAKE(255, 255, 255, 255)},
  {"LEVEL: 1", E3DCOLOR_MAKE(255, 0, 0, 255)},
  {"LEVEL: 2", E3DCOLOR_MAKE(255, 255, 0, 255)},
  {"LEVEL: 3", E3DCOLOR_MAKE(0, 255, 0, 255)},
  {"LEVEL: 4", E3DCOLOR_MAKE(0, 255, 255, 255)},
  {"LEVEL: 5", E3DCOLOR_MAKE(0, 0, 255, 255)},
  {"LEVEL: 6", E3DCOLOR_MAKE(255, 0, 255, 255)},
};

inline void renderColorLegend(const Tab<eastl::pair<const char *, E3DCOLOR>> &values)
{
  for (int i = 0; i < values.size(); i++)
  {
    const eastl::pair<const char *, E3DCOLOR> &entry = values[i];
    add_debug_text_mark(100, 50, entry.first, -1, i * 2, entry.second);
  }
}
}; // namespace gbuffer_debug_color_legends
