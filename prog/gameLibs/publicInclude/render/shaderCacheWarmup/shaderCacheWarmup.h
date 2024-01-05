//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_tabFwd.h>

#include <utility>

class BaseTexture;
class String;
namespace shadercache
{
struct WarmupParams
{
  dag::Vector<eastl::pair<const char *, unsigned>> invalidVars;
  BaseTexture *colorTarget = nullptr;
  BaseTexture *depthTarget = nullptr;
};

void warmup_shaders(const Tab<const char *> &graphics_shader_names, const Tab<const char *> &compute_shader_names,
  const WarmupParams &params, const bool is_loading_thread);

void warmup_shaders_from_settings(const bool is_loading_thread);
void warmup_shaders_from_settings(const WarmupParams &params, const bool is_loading_thread);
} // namespace shadercache
