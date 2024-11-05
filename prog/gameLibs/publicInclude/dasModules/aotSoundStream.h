//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <dasModules/dasModulesCommon.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/streams.h>
#include <ecs/sound/soundComponent.h>

MAKE_TYPE_FACTORY(SoundStream, SoundStream);

using SoundStreamState = sndsys::StreamState;

DAS_BIND_ENUM_CAST(SoundStreamState)

namespace soundstream_bind_dascript
{
inline void init(SoundStream &sound_stream, const char *src, Point2 min_max_dist, const char *bus)
{
  sound_stream.reset(sndsys::init_stream(src, min_max_dist, bus));
}
inline void open(const SoundStream &sound_stream) { sndsys::open(sound_stream.handle); }
inline void release(SoundStream &sound_stream) { sound_stream.reset(); }
inline void set_fader(const SoundStream &sound_stream, Point2 min_max_fader) { sndsys::set_fader(sound_stream.handle, min_max_fader); }
inline void set_pos(const SoundStream &sound_stream, Point3 pos) { sndsys::set_pos(sound_stream.handle, pos); }
inline void set_volume(const SoundStream &sound_stream, float vol) { sndsys::set_volume(sound_stream.handle, vol); }
inline sndsys::StreamState get_state(const SoundStream &sound_stream) { return sndsys::get_state(sound_stream.handle); }
inline bool is_valid(const SoundStream &sound_stream) { return sndsys::is_valid_handle(sound_stream.handle); }
} // namespace soundstream_bind_dascript
