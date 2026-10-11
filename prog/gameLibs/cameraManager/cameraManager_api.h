// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix.h>
#include <EASTL/functional.h>
#include <EASTL/string.h>

namespace camera_manager
{

struct CameraState
{
  TMatrix itm = TMatrix::IDENT;
  float fovDeg = 90.f;
};

struct GameHooks
{
  eastl::function<bool(CameraState &)> getCamera;
  eastl::function<void(const CameraState &)> setCamera;
  eastl::function<eastl::string()> getMissionName;
  eastl::function<void(const char *screenshot_name)> requestScreenshot;
};

#if DAGOR_DBGLEVEL > 0

void init(const GameHooks &hooks);
void shutdown();
bool is_capture_pending();
bool on_screenshot_saved(const char *path);

#else

inline void init(const GameHooks &) {}
inline void shutdown() {}
inline bool is_capture_pending() { return false; }
inline bool on_screenshot_saved(const char *) { return false; }

#endif

} // namespace camera_manager
