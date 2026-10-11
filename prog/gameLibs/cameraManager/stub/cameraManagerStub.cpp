// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <cameraManager/cameraManager_api.h>

#if DAGOR_DBGLEVEL > 0

namespace camera_manager
{
void init(const GameHooks &) {}
void shutdown() {}
bool is_capture_pending() { return false; }
bool on_screenshot_saved(const char *) { return false; }
} // namespace camera_manager

#endif
