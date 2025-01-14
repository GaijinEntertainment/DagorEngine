//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasManagedTab.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/strHash.h>
#include <soundSystem/vars.h>
#include <soundSystem/events.h>
#include <soundSystem/visualLabels.h>
#include <soundSystem/eventInstanceStealing.h>
#include <soundSystem/occlusion.h>
#include <ecs/sound/soundComponent.h>
#include <ecs/sound/soundGroup.h>

MAKE_TYPE_FACTORY(SoundEvent, SoundEvent);
MAKE_TYPE_FACTORY(SoundEventHandle, sndsys::EventHandle);
MAKE_TYPE_FACTORY(SoundVarId, SoundVarId);
MAKE_TYPE_FACTORY(SoundEventGroup, SoundEventGroup);
MAKE_TYPE_FACTORY(VisualLabel, sndsys::VisualLabel);
MAKE_TYPE_FACTORY(SoundEventsPreload, SoundEventsPreload);

DAS_BIND_VECTOR(VisualLabels, sndsys::VisualLabels, sndsys::VisualLabel, " ::sndsys::VisualLabels")

namespace das
{
template <>
struct cast<sndsys::EventHandle>
{
  static __forceinline sndsys::EventHandle to(vec4f x)
  {
    return sndsys::EventHandle(sndsys::sound_handle_t(v_extract_xi(v_cast_vec4i(x))));
  }

  static __forceinline vec4f from(const sndsys::EventHandle &x) { return v_cast_vec4f(v_splatsi(sndsys::sound_handle_t(x))); }
};
template <>
struct WrapType<sndsys::EventHandle>
{
  enum
  {
    value = false
  };
  typedef sndsys::sound_handle_t type;
  typedef sndsys::sound_handle_t rettype;
};

template <>
struct cast<SoundVarId>
{
  static __forceinline SoundVarId to(vec4f x) { return SoundVarId(sndsys::var_id_t(v_extract_xi64(v_cast_vec4i(x)))); }

  static __forceinline vec4f from(const SoundVarId &x) { return v_cast_vec4f(v_splatsi64(sndsys::var_id_t(x))); }
};

template <>
struct WrapType<SoundVarId>
{
  enum
  {
    value = false
  };
  typedef sndsys::var_id_t type;
  typedef sndsys::var_id_t rettype;
};

template <>
struct SimPolicy<sndsys::EventHandle>
{
  static __forceinline auto to(vec4f a) { return das::cast<sndsys::EventHandle>::to(a); }
  static __forceinline bool Equ(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) == to(b); }
  static __forceinline bool NotEqu(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) != to(b); }
  static __forceinline bool BoolNot(vec4f a, Context &, LineInfo *) { return !to(a); }
  // and for the AOT, since we don't have vec4f c-tor
  static __forceinline bool Equ(sndsys::EventHandle a, sndsys::EventHandle b, Context &, LineInfo *) { return a == b; }
  static __forceinline bool NotEqu(sndsys::EventHandle a, sndsys::EventHandle b, Context &, LineInfo *) { return a != b; }
  static __forceinline bool BoolNot(sndsys::EventHandle a, Context &, LineInfo *) { return !a; }
};

template <>
struct SimPolicy<SoundVarId>
{
  static __forceinline auto to(vec4f a) { return das::cast<SoundVarId>::to(a); }
  static __forceinline bool Equ(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) == to(b); }
  static __forceinline bool NotEqu(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) != to(b); }
  static __forceinline bool BoolNot(vec4f a, Context &, LineInfo *) { return !to(a); }
  // and for the AOT, since we don't have vec4f c-tor
  static __forceinline bool Equ(SoundVarId a, SoundVarId b, Context &, LineInfo *) { return a == b; }
  static __forceinline bool NotEqu(SoundVarId a, SoundVarId b, Context &, LineInfo *) { return a != b; }
  static __forceinline bool BoolNot(SoundVarId a, Context &, LineInfo *) { return !a; }
};
}; // namespace das

namespace soundevent_bind_dascript
{
inline void add_sound(SoundEventGroup &group, sndsys::event_id_t id, sndsys::EventHandle handle)
{
  ::add_sound(group, id, {}, handle);
}
inline void __add_sound(SoundEventGroup &group, sndsys::event_id_t id, const SoundEvent &sound_event)
{
  ::add_sound(group, id, {}, sound_event.handle);
}
inline void add_sound_with_pos(SoundEventGroup &group, sndsys::event_id_t id, Point3 pos, sndsys::EventHandle handle,
  int max_instances)
{
  ::add_sound(group, id, pos, handle, max_instances);
}
inline void __add_sound_with_pos(SoundEventGroup &group, sndsys::event_id_t id, Point3 pos, const SoundEvent &sound_event,
  int max_instances)
{
  ::add_sound(group, id, pos, sound_event.handle, max_instances);
}

inline void remove_sound(SoundEventGroup &group, sndsys::EventHandle handle) { ::remove_sound(group, handle); }
inline void __remove_sound(SoundEventGroup &group, const SoundEvent &sound_event) { ::remove_sound(group, sound_event.handle); }
inline void release_all_sounds(SoundEventGroup &group) { ::release_all_sounds(group); }
inline void abandon_all_sounds(SoundEventGroup &group) { ::abandon_all_sounds(group); }
inline void reject_sound(SoundEventGroup &group, sndsys::event_id_t id) { ::reject_sound(group, id, false); }
inline void abandon_sound(SoundEventGroup &group, sndsys::event_id_t id) { ::abandon_sound(group, id); }
inline void reject_sound_with_stop(SoundEventGroup &group, sndsys::event_id_t id, bool stop) { ::reject_sound(group, id, stop); }
inline void release_sound(SoundEventGroup &group, sndsys::event_id_t id) { ::release_sound(group, id); }
inline sndsys::EventHandle get_sound(const SoundEventGroup &group, sndsys::event_id_t id) { return ::get_sound(group, id); }
inline bool has_sound(const SoundEventGroup &group, sndsys::event_id_t id) { return ::has_sound(group, id); }
inline bool has_sound_with_name_path(const SoundEventGroup &group, const char *name, const char *path)
{
  return ::has_sound(group, name, path);
}

inline int get_num_sounds_with_id(const SoundEventGroup &group, sndsys::event_id_t id) { return ::get_num_sounds(group, id); }
inline int get_num_sounds(const SoundEventGroup &group) { return ::get_num_sounds(group); }
inline int get_max_capacity(const SoundEventGroup &group) { return ::get_max_capacity(group); }

inline void update_sounds(SoundEventGroup &group) { ::update_sounds(group); }

inline void __reset(SoundEvent &sound_event, sndsys::EventHandle handle) { sound_event.reset(handle); }

inline void __move(SoundEvent &target, SoundEvent &source)
{
  target.reset(source.handle);
  source.handle = sndsys::EventHandle{};
}

inline bool is_valid_event_handle_value(const sndsys::EventHandle &handle) { return (bool)handle; }
inline bool is_valid_event_value(const SoundEvent &sound_event) { return (bool)sound_event.handle; }
inline sndsys::EventHandle invalid_sound_event_handle() { return {}; }

inline sndsys::EventHandle play_with_name(const char *name) { return sndsys::play(name); }
inline sndsys::EventHandle play_with_name_pos(const char *name, Point3 pos) { return sndsys::play(name, nullptr, pos); }
inline sndsys::EventHandle play_with_name_path_pos_far(const char *name, const char *path, Point3 pos, bool allow_far_oneshots)
{
  return sndsys::play(name, path, pos, allow_far_oneshots);
}
inline sndsys::EventHandle play_with_name_path_pos(const char *name, const char *path, Point3 pos)
{
  return sndsys::play(name, path, pos);
}
inline sndsys::EventHandle play_with_name_pos_vol(const char *name, Point3 pos, float volume)
{
  return sndsys::play(name, nullptr, pos, volume);
}
inline sndsys::EventHandle delayed_play_with_name_path_pos(const char *name, const char *path, Point3 pos, float delay)
{
  return sndsys::delayed_play(name, path, pos, delay);
}

inline void play_sound_with_name(SoundEvent &sound_event, const char *name) { sound_event.reset(sndsys::play(name)); }
inline void play_sound_with_name_pos(SoundEvent &sound_event, const char *name, Point3 pos)
{
  sound_event.reset(sndsys::play(name, nullptr, pos));
}
inline void play_sound_with_name_path_pos(SoundEvent &sound_event, const char *name, const char *path, Point3 pos)
{
  sound_event.reset(sndsys::play(name, path, pos));
}
inline void play_sound_with_name_pos_vol(SoundEvent &sound_event, const char *name, Point3 pos, float volume)
{
  sound_event.reset(sndsys::play(name, nullptr, pos, volume));
}
inline void play_sound_with_name_pos_delayed(SoundEvent &sound_event, const char *name, Point3 pos, float delay)
{
  sound_event.reset(sndsys::delayed_play(name, nullptr, pos, delay));
}

inline void oneshot_with_name_pos(const char *name, Point3 pos) { sndsys::play_one_shot(name, pos); }
inline void oneshot_with_name_pos_far(const char *name, Point3 pos, bool allow_far_oneshots)
{
  sndsys::play_one_shot(name, pos, allow_far_oneshots);
}
inline void oneshot_with_name(const char *name) { sndsys::play_one_shot(name); }
inline void delayed_oneshot_with_name_pos(const char *name, Point3 pos, float delay)
{
  sndsys::delayed_oneshot(name, nullptr, pos, delay);
}
inline void delayed_oneshot_with_name(const char *name, float delay) { sndsys::delayed_oneshot(name, nullptr, delay); }

inline bool should_play(Point3 pos) { return sndsys::should_play(pos); }
inline bool should_play_ex(Point3 pos, float dist_threshold) { return sndsys::should_play(pos, dist_threshold); }

inline bool is_oneshot(sndsys::EventHandle handle) { return sndsys::is_one_shot(handle); }
inline bool __is_oneshot(const SoundEvent &sound_event) { return sndsys::is_one_shot(sound_event.handle); }

inline bool is_delayable(sndsys::EventHandle handle) { return sndsys::is_delayable(handle); }
inline bool __is_delayable(const SoundEvent &sound_event) { return sndsys::is_delayable(sound_event.handle); }

inline bool has_occlusion(sndsys::EventHandle handle) { return sndsys::has_occlusion(handle); }
inline bool __has_occlusion(const SoundEvent &sound_event) { return sndsys::has_occlusion(sound_event.handle); }

inline bool is_playing(sndsys::EventHandle handle) { return sndsys::is_playing(handle); }
inline bool __is_playing(const SoundEvent &sound_event) { return sndsys::is_playing(sound_event.handle); }
inline bool is_valid_event(sndsys::EventHandle handle) { return sndsys::is_valid_handle(handle); }
inline bool __is_valid_event(const SoundEvent &sound_event) { return sndsys::is_valid_handle(sound_event.handle); }
inline bool is_valid_event_instance(sndsys::EventHandle handle) { return sndsys::is_valid_event_instance(handle); }
inline bool __is_valid_event_instance(const SoundEvent &sound_event) { return sndsys::is_valid_event_instance(sound_event.handle); }

inline int get_num_event_instances(sndsys::EventHandle handle) { return sndsys::get_num_event_instances(handle); }
inline int __get_num_event_instances(const SoundEvent &sound_event) { return sndsys::get_num_event_instances(sound_event.handle); }
inline int get_num_event_instances_with_name_path(const char *name, const char *path)
{
  return sndsys::get_num_event_instances(name, path);
}

inline bool has(const char *name, const char *path) { return sndsys::has_event(name, path); }

inline void set_pos(sndsys::EventHandle handle, Point3 pos) { sndsys::set_3d_attr(handle, pos); }
inline void __set_pos(const SoundEvent &sound_event, Point3 pos) { sndsys::set_3d_attr(sound_event.handle, pos); }
inline void __set_pos_and_ori(SoundEvent &sound_event, const Point3 &pos, const Point3 &vel, const Point3 &forward, const Point3 &up)
{
  sndsys::set_3d_attr(sound_event.handle, pos, vel, forward, up);
}


inline void set_var(sndsys::EventHandle handle, const char *var_name, float value) { sndsys::set_var(handle, var_name, value); }
inline void __set_var(const SoundEvent &sound_event, const char *var_name, float value)
{
  sndsys::set_var(sound_event.handle, var_name, value);
}

inline void set_var_optional(sndsys::EventHandle handle, const char *var_name, float value)
{
  sndsys::set_var_optional(handle, var_name, value);
}
inline void __set_var_optional(const SoundEvent &sound_event, const char *var_name, float value)
{
  sndsys::set_var_optional(sound_event.handle, var_name, value);
}

inline void set_var_global(const char *var_name, float value) { sndsys::set_var_global(var_name, value); }
inline void set_var_global_with_id(SoundVarId var_id, float value) { sndsys::set_var_global(var_id, value); }
inline SoundVarId invalid_sound_var_id() { return {}; }
inline SoundVarId get_var_id_global(const char *var_name) { return sndsys::get_var_id_global(var_name); }

inline void set_volume(sndsys::EventHandle handle, float vol) { sndsys::set_volume(handle, vol); }
inline void __set_volume(const SoundEvent &sound_event, float vol) { sndsys::set_volume(sound_event.handle, vol); }

inline void set_pitch(sndsys::EventHandle handle, float pitch) { sndsys::set_pitch(handle, pitch); }
inline void __set_pitch(const SoundEvent &sound_event, float pitch) { sndsys::set_pitch(sound_event.handle, pitch); }

inline void set_timeline_position(sndsys::EventHandle handle, int position) { sndsys::set_timeline_position(handle, position); }
inline void __set_timeline_position(const SoundEvent &sound_event, int position)
{
  sndsys::set_timeline_position(sound_event.handle, position);
}

inline int get_timeline_position(sndsys::EventHandle handle) { return sndsys::get_timeline_position(handle); }
inline int __get_timeline_position(const SoundEvent &sound_event) { return sndsys::get_timeline_position(sound_event.handle); }

inline int get_length_by_path(const char *name)
{
  int len = 0;
  sndsys::get_length(name, len);
  return len;
}

inline const char *get_sample_loading_state_by_path(const char *path) { return sndsys::get_sample_loading_state(path); }
inline bool load_sample_data_by_path(const char *path) { return sndsys::load_sample_data(path); }
inline bool unload_sample_data_by_path(const char *path) { return sndsys::unload_sample_data(path); }

inline const char *get_sample_loading_state(sndsys::EventHandle handle) { return sndsys::get_sample_loading_state(handle); }
inline bool load_sample_data(sndsys::EventHandle handle) { return sndsys::load_sample_data(handle); }
inline bool unload_sample_data(sndsys::EventHandle handle) { return sndsys::unload_sample_data(handle); }

inline void release_immediate(sndsys::EventHandle &handle) { sndsys::release_immediate(handle); }
inline void __release_immediate(SoundEvent &sound_event) { sndsys::release_immediate(sound_event.handle); }

inline void release(sndsys::EventHandle &handle) { sndsys::release(handle); }
inline void __release(SoundEvent &sound_event) { sndsys::release(sound_event.handle); }

inline void abandon_immediate(sndsys::EventHandle &handle) { sndsys::abandon_immediate(handle); }
inline void __abandon_immediate(SoundEvent &sound_event) { sndsys::abandon_immediate(sound_event.handle); }

inline void abandon(sndsys::EventHandle &handle) { sndsys::abandon(handle); }
inline void __abandon(SoundEvent &sound_event) { sndsys::abandon(sound_event.handle); }

inline void abandon_with_delay(sndsys::EventHandle &handle, float delay) { sndsys::abandon_delayed(handle, delay); }
inline void __abandon_with_delay(SoundEvent &sound_event, float delay) { sndsys::abandon_delayed(sound_event.handle, delay); }

inline bool keyoff(sndsys::EventHandle handle) { return sndsys::keyoff(handle); }
inline bool __keyoff(const SoundEvent &sound_event) { return sndsys::keyoff(sound_event.handle); }

inline bool is_3d(sndsys::EventHandle handle) { return sndsys::is_3d(handle); }
inline bool __is_3d(const SoundEvent &sound_event) { return sndsys::is_3d(sound_event.handle); }

inline float get_max_distance(sndsys::EventHandle handle) { return sndsys::get_max_distance(handle); }
inline float __get_max_distance(const SoundEvent &sound_event) { return sndsys::get_max_distance(sound_event.handle); }

inline float get_max_distance_by_name(const char *name) { return sndsys::get_max_distance(name); }

inline void set_occlusion_group(sndsys::EventHandle handle, sndsys::occlusion::group_id_t group_id)
{
  sndsys::occlusion::set_event_group(handle, group_id);
}

inline void set_occlusion_group_eid(sndsys::EventHandle handle, ecs::EntityId eid)
{
  sndsys::occlusion::set_event_group(handle, sndsys::occlusion::group_id_t(eid));
}

inline void set_occlusion_pos(sndsys::occlusion::group_id_t group_id, Point3 pos) { sndsys::occlusion::set_group_pos(group_id, pos); }
inline void set_occlusion_pos_eid(ecs::EntityId eid, Point3 pos)
{
  sndsys::occlusion::set_group_pos(sndsys::occlusion::group_id_t(eid), pos);
}

inline bool is_occlusion_inited() { return sndsys::occlusion::is_inited(); }

inline bool das_query_visual_labels(const das::TBlock<void, const das::TTemporary<const das::TArray<sndsys::VisualLabel>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  sndsys::VisualLabels labels = sndsys::query_visual_labels();

  das::Array arr;
  arr.data = (char *)labels.data();
  arr.size = uint32_t(labels.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return true;
}

inline int create_event_instance_stealing_group(const char *group_name, int target_instances, float fade_in_out_speed)
{
  return sndsys::create_event_instance_stealing_group(group_name, target_instances, fade_in_out_speed);
}

inline void update_event_instance_stealing(sndsys::EventHandle handle, int group_id, float dt)
{
  sndsys::update_event_instance_stealing(handle, group_id, dt);
}

inline void sound_events_load(SoundEventsPreload &sound_events_preload, const das::TArray<char *> &paths)
{
  sound_events_preload.load(make_span((const char **)paths.data, paths.size));
}

inline void sound_events_unload(SoundEventsPreload &sound_events_preload) { sound_events_preload.unload(); }

} // namespace soundevent_bind_dascript
