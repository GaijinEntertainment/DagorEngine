//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

enum class CloudsRenderFlags : char
{
  None = 0,
  SetCameraVars = 1,
  MainView = 1 << 2,
  RestartTAA = 1 << 3,
  Default = SetCameraVars | MainView
};

constexpr inline CloudsRenderFlags operator|(const CloudsRenderFlags a, const CloudsRenderFlags b)
{
  return static_cast<CloudsRenderFlags>((char)a | (char)b);
}

constexpr inline CloudsRenderFlags operator&(const CloudsRenderFlags a, const CloudsRenderFlags b)
{
  return static_cast<CloudsRenderFlags>((char)a & (char)b);
}

constexpr inline bool check_clouds_render_flag(const CloudsRenderFlags flgs, const CloudsRenderFlags flg_to_check)
{
  return (flgs & flg_to_check) != CloudsRenderFlags::None;
}
