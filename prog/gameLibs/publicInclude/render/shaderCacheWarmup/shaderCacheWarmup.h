//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_tabFwd.h>
#include <drv/3d/dag_texFlags.h>

#include <utility>

class BaseTexture;
class String;
namespace shadercache
{
struct WarmupParams
{
  dag::Vector<eastl::pair<const char *, unsigned>> invalidVars;
  dag::Vector<const char *> shadersToSkip;
  int colorTargetCount = 0;
  int colorTargetFormat = TEXFMT_DEFAULT;
  int depthTargetFormat = 0;
};

// NOTE: requires GPU acquire!
// returns true if warmup is in progress
bool get_warmup_status(uint32_t &completed, uint32_t &total);
// Draws on the current render target a progress bar displaying the progress of 'completed' towards 'total'.
// The 'completed' value must be equal or smaller than the 'total' value.
// The value of 'total' must be greater than zero.
// On invalid inputs for 'completed' or 'total' an assert may be raised and nothing will be drawn.
// This may alter the currently set d3d view.
// Render/GPU mutex has to be held (eg GPU acquire).
void draw_warmup_status(uint32_t completed, uint32_t total);

void warmup_shaders(const Tab<const char *> &graphics_shader_names, const Tab<const char *> &compute_shader_names,
  const WarmupParams &params, const bool is_loading_thread);

void warmup_shaders_from_settings(const bool is_loading_thread);
void warmup_shaders_from_settings(const WarmupParams &params, const bool is_loading_thread, const bool backgroundWarmup);
} // namespace shadercache
