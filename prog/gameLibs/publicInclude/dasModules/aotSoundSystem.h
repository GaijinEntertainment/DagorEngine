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
#include <soundSystem/geometry.h>

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

// geometry
inline int sound_add_geometry(int max_polygons, int max_vertices) { return sndsys::add_geometry(max_polygons, max_vertices); }

inline void sound_remove_geometry(int geometry_id) { sndsys::remove_geometry(geometry_id); }

inline void sound_remove_all_geometry() { sndsys::remove_all_geometry(); }

inline void sound_add_polygons(int geometry_id, const das::TArray<Point3> &vertices, int num_verts_per_poly, float direct_occlusion,
  float reverb_occlusion, bool doublesided)
{
  sndsys::add_polygons(geometry_id, make_span_const((const Point3 *)vertices.data, vertices.size), num_verts_per_poly,
    direct_occlusion, reverb_occlusion, doublesided);
}

inline void sound_add_polygon(int geometry_id, const Point3 &a, const Point3 &b, const Point3 &c, float direct_occlusion,
  float reverb_occlusion, bool doublesided)
{
  sndsys::add_polygon(geometry_id, a, b, c, direct_occlusion, reverb_occlusion, doublesided);
}

inline void sound_set_geometry_position(int geometry_id, const Point3 &position)
{
  sndsys::set_geometry_position(geometry_id, position);
}

inline Point3 sound_get_geometry_position(int geometry_id) { return sndsys::get_geometry_position(geometry_id); }

inline void sound_enum_geometry(const das::TBlock<void, int /*id*/> &block, das::Context *context, das::LineInfoArg *at)
{
  const int count = sndsys::get_geometry_count();
  for (int idx = 0; idx < count; ++idx)
  {
    const int id = sndsys::get_geometry_id(idx);
    vec4f arg = das::cast<int>::from(id);
    context->invoke(block, &arg, nullptr, at);
  }
}

// debug
inline void sound_enum_geometry_faces(int geometry_id,
  const das::TBlock<void, const das::TTemporary<const das::TArray<Point3>>> &block, das::Context *context, das::LineInfoArg *at)
{
  const auto *faces = sndsys::get_geometry_faces(geometry_id);

  das::Array arr;
  arr.data = (char *)(faces ? faces->data() : nullptr);
  arr.size = uint32_t(faces ? faces->size() : 0);
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void sound_save_geometry_to_file(const char *filename) { sndsys::save_geometry_to_file(filename); }
inline bool sound_load_geometry_from_file(const char *filename) { return sndsys::load_geometry_from_file(filename); }

inline Point2 sound_get_geometry_occlusion(const Point3 &source, const Point3 &listener)
{
  return sndsys::get_geometry_occlusion(source, listener);
}

} // namespace soundsystem_bind_dascript
