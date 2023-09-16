//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if DAGOR_DBGLEVEL > 0
namespace fpslog
{
void start_log(const char *app_name);
void stop_log();

void set_min_fps_threshold(float fps);

void set_location_name(const char *name);
void flush_log();
} // namespace fpslog
#else
namespace fpslog
{
inline void start_log(const char *) {}
inline void stop_log() {}

inline void set_min_fps_threshold(float) {}

inline void set_location_name(const char *) {}
inline void flush_log() {}
} // namespace fpslog
#endif
