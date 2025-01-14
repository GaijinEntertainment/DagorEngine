//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <soundSystem/handle.h>
#include <generic/dag_span.h>
#include <util/dag_index16.h>

class TMatrix4;

namespace FMOD
{
namespace Studio
{
class EventInstance;
}
class ChannelGroup;
} // namespace FMOD

namespace sndsys
{
bool has_event(const char *name, const char *path = nullptr);
int get_num_event_instances(const char *name, const char *path = nullptr);
int get_num_event_instances(EventHandle event_handle);

enum InitEventFlags
{
  IEF_EXCLUDE_NON_ONESHOT = (1 << 0),
  IEF_EXCLUDE_FAR_ONESHOT = (1 << 1),
  IEF_DEFAULT = IEF_EXCLUDE_FAR_ONESHOT,
  IEF_ONESHOT = IEF_EXCLUDE_NON_ONESHOT | IEF_EXCLUDE_FAR_ONESHOT,
};

typedef uint32_t ieff_t;
EventHandle init_event(const char *name, const char *path, ieff_t flags, const Point3 *position = nullptr);

__forceinline EventHandle init_event(const char *name, const Point3 &position)
{
  return init_event(name, nullptr, IEF_DEFAULT, &position);
}

__forceinline EventHandle init_event(const char *name, const char *path, const Point3 &position)
{
  return init_event(name, path, IEF_DEFAULT, &position);
}

__forceinline EventHandle init_event(const char *name, const char *path) { return init_event(name, path, IEF_DEFAULT); }

__forceinline EventHandle init_event(const char *name) { return init_event(name, nullptr, IEF_DEFAULT); }

EventHandle init_event(const FMODGUID &event_id, const Point3 *position = nullptr);
bool has_event(const FMODGUID &event_id);

bool get_event_id(const char *name, const char *path, bool is_snapshot, FMODGUID &event_id, bool debug_trace = true);
bool preload(const FMODGUID &event_id);
bool unload(const FMODGUID &event_id, bool is_strict = true);
void release_immediate(EventHandle &event_handle, bool is_stop = true);
void release_delayed(EventHandle &event_handle, float delay = 0.f);
void release(EventHandle &event_handle, float delay = 0.f, bool is_stop = true);

void release_all_instances(const char *path);

bool play_one_shot(const char *name, const char *path, const Point3 *position, ieff_t flags, float delay);

__forceinline bool play_one_shot(const char *name, const char *path, const Point3 *position)
{
  return play_one_shot(name, path, position, IEF_ONESHOT, 0.f);
}

__forceinline bool play_one_shot(const char *name) { return play_one_shot(name, nullptr, nullptr); }

__forceinline bool play_one_shot(const char *name, const char *path) { return play_one_shot(name, path, nullptr); }

__forceinline bool play_one_shot(const char *name, const Point3 &position) { return play_one_shot(name, nullptr, &position); }

__forceinline bool play_one_shot(const char *name, const Point3 &position, bool allow_far_oneshots)
{
  return play_one_shot(name, nullptr, &position, IEF_ONESHOT & ~(allow_far_oneshots ? sndsys::IEF_EXCLUDE_FAR_ONESHOT : 0), 0.f);
}

static const float def_dist_threshold = 40.f;
bool should_play(const Point3 &pos, float dist_threshold = def_dist_threshold);

// invalidates handle, oneshot event will play until the end, non-oneshot event with no cue will release immediately
void abandon_immediate(EventHandle &event_handle);
void abandon_delayed(EventHandle &event_handle, float delay = 0.f);
void abandon(EventHandle &event_handle, float delay = 0.f);

void start_immediate(EventHandle event_handle);
void start_delayed(EventHandle event_handle, float delay = 0.f);
void start(EventHandle event_handle, float delay = 0.f);
void start(EventHandle event_handle, const Point3 &pos, float delay = 0.f);

// stop sound with keyoff of fadeout
void stop_immediate(EventHandle event_handle, bool allow_fadeout = false, bool try_keyoff = false);
void stop_delayed(EventHandle event_handle, bool allow_fadeout = false, bool try_keyoff = false, float delay = 0.f);
void stop(EventHandle event_handle, bool allow_fadeout = false, bool try_keyoff = false, float delay = 0.f);

// allow the timeline cursor to move past sustain points
bool keyoff(EventHandle event_handle);

void pause(EventHandle event_handle, bool pause = true);
bool get_paused(EventHandle event_handle);

enum PlayBackState
{
  STATE_UNUSED = 0,
  STATE_LOADING,
  STATE_READY,
  STATE_STARTED
};

bool get_playback_state(EventHandle event_handle, PlayBackState &state);
bool get_property(const FMODGUID &event_id, int prop_id, int &value);
float get_property(EventHandle event_handle, const char *name, float def_val = -1.f);
float get_audibility(const FMOD::Studio::EventInstance &eventInstance);

bool is_playing(EventHandle event_handle);
bool is_stopping(EventHandle event_handle);
bool is_virtual(EventHandle event_handle);
const char *debug_event_state(EventHandle event_handle);

bool is_valid_handle(EventHandle event_handle);
bool is_valid_event_instance(EventHandle event_handle);

bool is_one_shot(EventHandle event_handle);
bool is_delayable(EventHandle event_handle);
bool has_occlusion(EventHandle event_handle);

bool is_one_shot(const FMODGUID &event_id);
bool is_snapshot(const FMODGUID &event_id);

void set_volume(EventHandle event_handle, float volume);
void set_pitch(EventHandle event_handle, float pitch);

void set_timeline_position(EventHandle event_handle, int position);
int get_timeline_position(EventHandle event_handle);

void set_3d_attr(EventHandle event_handle, const Point3 &pos);
void set_3d_attr(EventHandle event_handle, const Point3 &pos, const Point3 &vel, const Point3 &forward, const Point3 &up);
void set_3d_attr(EventHandle event_handle, const TMatrix4 &tm, const Point3 &vel);
void set_3d_attr(EventHandle event_handle, const TMatrix &tm, const Point3 &vel);
bool get_3d_attr(EventHandle event_handle, TMatrix &tm, Point3 &vel);
bool get_3d_attr(EventHandle event_handle, Point3 &pos);

__forceinline void set_pos(EventHandle event_handle, const Point3 &pos) { set_3d_attr(event_handle, pos); }

bool is_3d(EventHandle event_handle);
bool are_of_the_same_desc(EventHandle first_handle, EventHandle second_handle);

float get_max_distance(EventHandle event_handle);
float get_max_distance(const char *name);
float get_max_distance(const FMODGUID &event_id);

bool has_sustain_point(EventHandle event_handle);
void set_occlusion(EventHandle event_handle, float direct_occlusion, float reverb_occlusion);

void set_node_id(EventHandle event_handle, dag::Index16 node_id);
dag::Index16 get_node_id(EventHandle event_handle);

void get_info(size_t &used_handles, size_t &total_handles);

bool get_length(EventHandle event_handle, int &length);
bool get_length(const FMODGUID &event_id, int &length);
bool get_length(const char *name, int &length);

const char *get_sample_loading_state(const char *path);
bool load_sample_data(const char *path);
bool unload_sample_data(const char *path);

const char *get_sample_loading_state(EventHandle event_handle);
bool load_sample_data(EventHandle event_handle);
bool unload_sample_data(EventHandle event_handle);

const char *get_sample_loading_state(const FMODGUID &event_id);
bool load_sample_data(const FMODGUID &event_id);
bool unload_sample_data(const FMODGUID &event_id);

__forceinline EventHandle play(const char *name)
{
  EventHandle handle = init_event(name);
  start(handle);
  return handle;
}
__forceinline EventHandle play(const char *name, const char *path, const Point3 &pos)
{
  EventHandle handle = init_event(name, path, pos);
  start(handle, pos);
  return handle;
}

__forceinline EventHandle play(const char *name, const char *path, const Point3 &pos, bool allow_far_oneshots)
{
  const ieff_t flags = IEF_DEFAULT & ~(allow_far_oneshots ? IEF_EXCLUDE_FAR_ONESHOT : 0);
  EventHandle handle = init_event(name, path, flags, &pos);
  start(handle, pos);
  return handle;
}
__forceinline EventHandle play(const char *name, const char *path, const Point3 &pos, float volume)
{
  EventHandle handle = init_event(name, path, pos);
  set_volume(handle, volume);
  start(handle, pos);
  return handle;
}
__forceinline EventHandle delayed_play(const char *name, const char *path, const Point3 &pos, float delay)
{
  EventHandle handle = init_event(name, path, pos);
  start(handle, pos, delay);
  return handle;
}

__forceinline void delayed_oneshot(const char *name, const char *path, const Point3 &pos, float delay)
{
  play_one_shot(name, path, &pos, IEF_ONESHOT, delay);
}
__forceinline void delayed_oneshot(const char *name, const char *path, float delay)
{
  play_one_shot(name, path, nullptr, IEF_ONESHOT, delay);
}

} // namespace sndsys
