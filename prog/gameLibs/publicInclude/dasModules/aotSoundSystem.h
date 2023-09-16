//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/banks.h>
#include <soundSystem/debug.h>
#include <soundSystem/delayed.h>
#include <soundSystem/vars.h>

namespace soundsystem_bind_dascript
{
inline bool have_sound() { return sndsys::is_inited(); }
inline void sound_debug(const char *message) { sndsys::debug_trace_info("%s", message); }
inline bool get_enable_debug_draw() { return sndsys::get_enable_debug_draw(); }
inline Point3 get_listener_pos() { return sndsys::get_3d_listener_pos(); }
inline void update_listener(float delta_time, const TMatrix &listener_tm) { sndsys::update_listener(delta_time, listener_tm); }
inline void reset_3d_listener() { sndsys::reset_3d_listener(); }
inline bool sound_banks_is_preset_loaded(const char *preset_name) { return sndsys::banks::is_loaded(preset_name); }

inline void sound_enable_distant_delay(bool enable) { sndsys::delayed::enable_distant_delay(enable); }
inline void sound_release_delayed_events() { sndsys::delayed::release_delayed_events(); }

inline void sound_override_time_speed(float time_speed) { sndsys::override_time_speed(time_speed); }

inline void sound_banks_enable_preset(const char *name, bool enable) { sndsys::banks::enable(name ? name : "", enable); }

inline void sound_banks_enable_preset_starting_with(const char *name, bool enable)
{
  sndsys::banks::enable_starting_with(name ? name : "", enable);
}

inline bool sound_banks_is_preset_enabled(const char *name) { return sndsys::banks::is_enabled(name); }
inline void sound_debug_enum_events() { sndsys::debug_enum_events(); }
} // namespace soundsystem_bind_dascript
