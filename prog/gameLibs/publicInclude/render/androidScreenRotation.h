//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace android_screen_rotation
{
#if _TARGET_ANDROID
void init();
void shutdown();
void onFrameEnd();
#else
inline static void init() {}
inline static void shutdown() {}
inline static void onFrameEnd() {}
#endif
} // namespace android_screen_rotation
