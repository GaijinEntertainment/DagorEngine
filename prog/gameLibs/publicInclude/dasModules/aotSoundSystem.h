//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <memory/dag_framemem.h>
#include <dasModules/dasModulesCommon.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/banks.h>
#include <soundSystem/debug.h>
#include <soundSystem/delayed.h>
#include <soundSystem/vars.h>
#include <rendInst/rendInstGen.h>
#include <EASTL/vector.h>

namespace soundsystem_bind_dascript
{
inline bool have_sound() { return sndsys::is_inited(); }
inline void sound_debug(const char *message) { sndsys::debug_trace_info("%s", message); }
inline Point3 get_listener_pos() { return sndsys::get_3d_listener_pos(); }
inline Point3 get_listener_up() { return sndsys::get_3d_listener_up(); }
inline void sound_update_listener(float delta_time, const TMatrix &listener_tm) { sndsys::update_listener(delta_time, listener_tm); }
inline void sound_reset_3d_listener() { sndsys::reset_3d_listener(); }
inline bool sound_banks_is_preset_loaded(const char *preset_name) { return preset_name && sndsys::banks::is_loaded(preset_name); }
inline bool sound_banks_is_master_preset_loaded() { return sndsys::banks::is_master_loaded(); }

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

inline void sound_init_ri_occluders(const das::TArray<char *> &_ri_names, const das::TArray<char *> &_ri_types,
  const das::TArray<char *> &_occluder_types, const das::TArray<float> &_occluder_values)
{
  const auto names = make_span_const((const char **)_ri_names.data, _ri_names.size);
  const auto types = make_span_const((const char **)_ri_types.data, _ri_types.size);
  const auto occluderTypes = make_span_const((const char **)_occluder_types.data, _occluder_types.size);
  const auto occluderValues = make_span_const((const float *)_occluder_values.data, _occluder_values.size);

  G_ASSERT_RETURN(names.size() == types.size(), );
  G_ASSERT_RETURN(occluderTypes.size() == occluderValues.size(), );

  eastl::vector<eastl::pair<const char *, const char *>, framemem_allocator> riNameToOccluderType;
  eastl::vector<eastl::pair<const char *, float>, framemem_allocator> occluders;

  riNameToOccluderType.reserve(names.size());
  occluders.reserve(occluderTypes.size());

  for (eastl_size_t i = 0; i < names.size(); ++i)
    riNameToOccluderType.emplace_back(names[i], types[i]);

  for (eastl_size_t i = 0; i < occluderTypes.size(); ++i)
    occluders.emplace_back(occluderTypes[i], occluderValues[i]);

  rendinst::initRiSoundOccluders(make_span_const(riNameToOccluderType.begin(), riNameToOccluderType.size()),
    make_span_const(occluders.begin(), occluders.size()));
}

inline float sound_debug_get_sound_occlusion(const char *ri_name, float def_value)
{
  return rendinst::debugGetSoundOcclusion(ri_name, def_value);
}

} // namespace soundsystem_bind_dascript
