//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
inline Point3 get_listener_pos() { return sndsys::get_3d_listener_pos(); }
inline void sound_update_listener(float delta_time, const TMatrix &listener_tm) { sndsys::update_listener(delta_time, listener_tm); }
inline void sound_reset_3d_listener() { sndsys::reset_3d_listener(); }
inline bool sound_banks_is_preset_loaded(const char *preset_name) { return preset_name && sndsys::banks::is_loaded(preset_name); }

inline void sound_enable_distant_delay(bool enable) { sndsys::delayed::enable_distant_delay(enable); }
inline void sound_release_delayed_events() { sndsys::delayed::release_delayed_events(); }

inline void sound_override_time_speed(float time_speed) { sndsys::override_time_speed(time_speed); }

inline void sound_banks_enable_preset(const char *name, bool enable) { sndsys::banks::enable(name ? name : "", enable); }

inline void sound_banks_enable_preset_starting_with(const char *name, bool enable)
{
  sndsys::banks::enable_starting_with(name ? name : "", enable);
}

inline bool sound_banks_is_preset_enabled(const char *name) { return name && sndsys::banks::is_enabled(name); }
inline bool sound_banks_is_preset_exist(const char *name) { return name && sndsys::banks::is_exist(name); }

inline void sound_debug_enum_events() { sndsys::debug_enum_events(); }

inline void sound_debug_enum_events_in_bank(const char *bank_name, const das::TBlock<void, const char *> &block, das::Context *context,
  das::LineInfoArg *at)
{
  sndsys::debug_enum_events(bank_name, [&](const char *event_name) {
    vec4f arg = das::cast<const char *const>::from(event_name);
    context->invoke(block, &arg, nullptr, at);
  });
}

} // namespace soundsystem_bind_dascript
