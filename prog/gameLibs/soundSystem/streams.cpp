// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h> // SNPRINTF
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_critSec.h>
#include <fmod_studio.hpp>
#include <soundSystem/soundSystem.h>
#include <soundSystem/streams.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/debug.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <memory/dag_framemem.h>
#include "internal/fmodCompatibility.h"
#include "internal/framememString.h"
#include "internal/streams.h"
#include "internal/pool.h"
#include "internal/debug.h"

namespace sndsys
{
using namespace fmodapi;
namespace streams
{
static WinCritSec g_pool_cs;
#define SNDSYS_POOL_BLOCK WinAutoLock poolLock(g_pool_cs);

struct Stream
{
  FMOD::Sound *sound = nullptr;
  FMOD::Channel *channel = nullptr;
  StreamState state = StreamState::CLOSED;
  SimpleString url;
  SimpleString bus;
  Point3 pos = Point3(0, 0, 0);
  Point2 minMaxDist = Point2(0, 0);
  float volume = 1.f;
  float fader = 1.f;
  Point2 fadeNearFarTime = {};
};

using StreamsPoolType = sndsys::Pool<Stream, sound_handle_t, 128, POOLTYPE_STREAM, POOLTYPE_TYPES_COUNT>;
G_STATIC_ASSERT(INVALID_SOUND_HANDLE == StreamsPoolType::invalid_value);
static StreamsPoolType all_streams;
static Tab<FMOD::Sound *> sounds_to_release;

static const char *stream_state_names[] = {"Error", "Closed", "Opened", "Connecting", "Buffering", "Stopped", "Paused", "Playing"};

static inline const char *get_state_name(StreamState state)
{
  G_STATIC_ASSERT(countof(stream_state_names) == int(StreamState::NUM_STATES));
  return stream_state_names[int(state)];
}

#define STREAM_VERIFY_RETURN(result) \
  if (FMOD_OK != (result))           \
  {                                  \
    close_stream(stream, true);      \
    return;                          \
  }

static inline bool is_3d(const Stream &stream) { return stream.minMaxDist.y > 0; }

static float g_virtual_vol_limit = 0.f;
static inline bool apply_volume(Stream &stream)
{
  if (!stream.channel)
    return false;

  float volume = 0.f;

  if (stream.volume >= g_virtual_vol_limit)
  {
    /* there is a problem with silent or out-of-range channel: it turns virtual and need to reconnect on restore.
     * that takes some time and causes not good transitions.
     * trying to add smooth fadein clamping on-bottom to keep channel non-virtual.
     * also smooth transitions on switching channels are nice.
     */
    float lowestNonVirtualVol = g_virtual_vol_limit;
    if (is_3d(stream))
    {
      const float dist = length(get_3d_listener_pos() - stream.pos);
      const float t = cvt(dist, stream.minMaxDist.x, stream.minMaxDist.y, 1.f, 0.f);
      lowestNonVirtualVol = t > VERY_SMALL_NUMBER ? g_virtual_vol_limit / t : 1.f;
    }
    volume = min(max(lowestNonVirtualVol + 1e-3f, stream.volume * stream.fader), 1.f);
  }
  return stream.channel->setVolume(volume) == FMOD_OK;
}

static inline void init_fader(Stream &stream) { stream.fader = stream.fadeNearFarTime.y >= VERY_SMALL_NUMBER ? 0.f : 1.f; }
static inline bool update_fader(Stream &stream, float dt)
{
  if (stream.fader < 1.f)
  {
    float t = stream.fadeNearFarTime.x;
    if (is_3d(stream))
    {
      const float dist = length(get_3d_listener_pos() - stream.pos);
      t = cvt(dist, stream.minMaxDist.x, stream.minMaxDist.y, stream.fadeNearFarTime.x, stream.fadeNearFarTime.y);
    }
    stream.fader = t > 0.01f ? min(stream.fader + dt / t, 1.f) : 1.f;
    return true;
  }
  return false;
}

static inline void set_state(Stream &stream, StreamState state)
{
  if (stream.state == state)
    return;
  stream.state = state;
  debug_trace_log("Stream (%s) state is now %s", stream.url.c_str(), get_state_name(state));
}

static inline void open_stream(Stream &stream)
{
  G_ASSERT(stream.state == StreamState::CLOSED || stream.state == StreamState::ERROR);

  FMOD_CREATESOUNDEXINFO exinfo = {};
  exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
  // increase the default file chunk size to handle seeking inside large playlist files that may be over 2kb
  exinfo.filebuffersize = 16 << 10;
  exinfo.suggestedsoundtype = FMOD_SOUND_TYPE_MPEG;

  uint32_t flags = FMOD_CREATESTREAM | FMOD_NONBLOCKING;
  if (is_3d(stream))
    flags |= FMOD_3D | FMOD_3D_LINEARROLLOFF;

  G_ASSERT(!stream.sound);
  FMOD_RESULT createStreamResult = get_system()->createSound(stream.url, flags, &exinfo, &stream.sound);
  if (FMOD_OK == createStreamResult)
    set_state(stream, StreamState::OPENED);
  else
  {
    set_state(stream, StreamState::ERROR);
    stream.sound = nullptr;
  }
}

static inline bool try_release_sound(FMOD::Sound *sound)
{
  if (sound)
  {
    FMOD_OPENSTATE openstate = FMOD_OPENSTATE_READY;
    FMOD_RESULT result = sound->getOpenState(&openstate, 0, 0, 0);
    if (FMOD_OK == result && openstate != FMOD_OPENSTATE_READY)
      return false;
    sound->release();
  }
  return true;
}

static inline void close_stream(Stream &stream, bool error = false)
{
  if (stream.channel)
    stream.channel->stop();
  if (find_value_idx(sounds_to_release, stream.sound) < 0)
  {
    if (!try_release_sound(stream.sound))
      sounds_to_release.push_back(stream.sound);
  }
  stream.channel = nullptr;
  stream.sound = nullptr;
  set_state(stream, error ? StreamState::ERROR : StreamState::CLOSED);
}

static inline void release_stream(StreamHandle stream_handle, Stream *stream)
{
  G_ASSERT(stream);
  close_stream(*stream);
  all_streams.reject((sound_handle_t)stream_handle);
}

static inline void update_stream(Stream &stream, float dt)
{
  FMOD_OPENSTATE openstate = FMOD_OPENSTATE_READY;
  bool isStarving = false;
  STREAM_VERIFY_RETURN(stream.sound->getOpenState(&openstate, nullptr, &isStarving, nullptr));
  if (FMOD_OPENSTATE_ERROR == openstate)
  {
    debug_trace_err("Stream (%s) sound open state result is FMOD_OPENSTATE_ERROR, stream closed", stream.url.c_str());
    close_stream(stream, true);
    return;
  }

  FrameStr newUrl;
  float newFrequency = 0.f;
  bool setNewUrl = false;
  bool setNewFrequency = false;

  FMOD_TAG tag = {};
  for (; stream.sound->getTag(0, -1, &tag) == FMOD_OK;)
  {
    if (tag.type == FMOD_TAGTYPE_PLAYLIST)
    {
      if (strcmp(tag.name, "FILE") == 0)
      {
        if (tag.datatype == FMOD_TAGDATATYPE_STRING)
        {
          newUrl = (const char *)tag.data;
          setNewUrl = true;
        }
        else
        {
          debug_trace_warn("unexpected datatype in 'FILE' tag");
          close_stream(stream, true);
          return;
        }
      }
    }
    else if (tag.type == FMOD_TAGTYPE_FMOD)
    {
      if (strcmp(tag.name, "Sample Rate Change") == 0)
      { // when a song changes, the sample rate may also change, so compensate here
        if (tag.datatype == FMOD_TAGDATATYPE_FLOAT)
        {
          newFrequency = *((float *)tag.data);
          setNewFrequency = true;
        }
        else
        {
          debug_trace_warn("unexpected datatype in 'Sample Rate Change' tag");
          close_stream(stream, true);
          return;
        }
      }
    }
    tag = {};
  }

  if (setNewFrequency)
  {
    if (newFrequency <= 0.f || check_nan(newFrequency))
    {
      debug_trace_err("Stream (%s) new frequency value(%f) is not valid, stream closed", stream.url.c_str(), newFrequency);
      close_stream(stream, true);
      return;
    }
    debug_trace_warn("Stream (%s) frequency was changed to %f", stream.url.c_str(), newFrequency);
    if (stream.channel)
      stream.channel->setFrequency(newFrequency);
  }

  if (setNewUrl)
  {
    if (newUrl.empty())
    {
      debug_trace_err("Stream (%s) new url is empty, stream closed", stream.url.c_str());
      close_stream(stream, true);
      return;
    }
    if (newUrl == stream.url)
      debug_trace_warn("Stream (%s) new url is the same as current", stream.url.c_str());
    else
    {
      debug_trace_warn("Stream (%s) url was changed to %s", stream.url.c_str(), newUrl.c_str());
      close_stream(stream);
      stream.url = newUrl.c_str();
      open_stream(stream);
      return;
    }
  }

  bool isPaused = false;
  bool isPlaying = false;

  if (stream.channel)
  {
    STREAM_VERIFY_RETURN(stream.channel->getPaused(&isPaused));
    STREAM_VERIFY_RETURN(stream.channel->isPlaying(&isPlaying));
    //// silence the stream until have sufficient data for smooth playback
    // this feature is not good because sound becomes virtual and have FMOD_ERR_FILE_COULDNOTSEEK
    // STREAM_VERIFY_RETURN(stream.channel->setMute(isStarving));
  }
  else if (openstate == FMOD_OPENSTATE_READY)
  {
    FMOD::ChannelGroup *channelGroup = nullptr;
    FMOD::Studio::Bus *bus = get_bus(stream.bus);
    if (bus)
      get_channel_group_by_bus(bus, channelGroup);

    if (!channelGroup)
      debug_trace_err("Could not get channel group from bus: %s", stream.bus.c_str());

    // this may fail if the stream isn't ready yet, so don't check the error code
    get_system()->playSound(stream.sound, channelGroup, true, &stream.channel);
    if (stream.channel)
    {
      init_fader(stream);
      apply_volume(stream);
      if (is_3d(stream))
      {
        STREAM_VERIFY_RETURN(stream.channel->set3DMinMaxDistance(stream.minMaxDist.x, stream.minMaxDist.y));
        STREAM_VERIFY_RETURN(stream.channel->set3DAttributes(&as_fmod_vector(stream.pos), nullptr));
      }
      stream.channel->setPaused(false);
    }
  }

  if (isPlaying)
  {
    if (update_fader(stream, dt))
      apply_volume(stream);
  }

  if (openstate == FMOD_OPENSTATE_BUFFERING)
    set_state(stream, StreamState::BUFFERING);
  else if (openstate == FMOD_OPENSTATE_CONNECTING)
    set_state(stream, StreamState::CONNECTING);
  else if (isPaused)
    set_state(stream, StreamState::PAUSED);
  else if (isPlaying)
    set_state(stream, StreamState::PLAYING);
}

void init(const DataBlock &blk, float virtual_vol_limit, FMOD::System *system)
{
  SNDSYS_IS_MAIN_THREAD;
  SNDSYS_POOL_BLOCK;
  const int defBufferSizeKb = 64;
  const int bufferSizeKb = blk.getInt("streamBufferSizeKb", defBufferSizeKb);
  SOUND_VERIFY(system->setStreamBufferSize(bufferSizeKb * 1024, FMOD_TIMEUNIT_RAWBYTES));
  g_virtual_vol_limit = virtual_vol_limit;
}

void close()
{
  SNDSYS_IS_MAIN_THREAD;
  SNDSYS_POOL_BLOCK;
  all_streams.enumerate([&](Stream &stream, sound_handle_t handle) {
#if DAGOR_DBGLEVEL > 0
    debug_trace_warn("Stream handle %lld was not released: \"%s\"", int64_t(handle), stream.url.c_str());
#else
    G_UNREFERENCED(handle);
#endif
    close_stream(stream);
  });
  all_streams.close();

  for (auto sound : sounds_to_release)
  {
    enum
    {
      WAIT_MSEC = 50,
      WAIT_MAX = 10000
    };
    for (int j = 0;; j += WAIT_MSEC)
    {
      sleep_msec(WAIT_MSEC);
      SOUND_VERIFY(get_studio_system()->update());
      if (try_release_sound(sound))
        break;
      G_ASSERTF(j < WAIT_MAX, "Waiting too long for a sound stream to release");
      if (j >= WAIT_MAX)
        break;
    }
  }
  clear_and_shrink(sounds_to_release);
}

void update(float dt)
{
  SNDSYS_POOL_BLOCK;
  all_streams.enumerate([dt](auto &stream, sound_handle_t) {
    if (stream.state >= StreamState::OPENED)
      update_stream(stream, dt);
  });

  if (!sounds_to_release.size())
    return;
  int newCount = 0;
  for (int i = 0; i < sounds_to_release.size(); ++i)
  {
    if (!try_release_sound(sounds_to_release[i]))
      sounds_to_release[newCount++] = sounds_to_release[i];
  }
  erase_items(sounds_to_release, newCount, sounds_to_release.size() - newCount);
}

void debug_get_info(uint32_t &num_handles, uint32_t &to_release_count)
{
  SNDSYS_POOL_BLOCK;
  num_handles = all_streams.get_used();
  to_release_count = sounds_to_release.size();
}

void debug_enum(eastl::function<void(const char *info, const Point3 &pos, bool is_3d)> fun)
{
  SNDSYS_POOL_BLOCK;
  eastl::basic_string<char, framemem_allocator> info;
  all_streams.enumerate([&info, &fun](const Stream &stream, sound_handle_t) {
    G_STATIC_ASSERT(countof(stream_state_names) == int(StreamState::NUM_STATES));
    info = stream_state_names[int(stream.state)];
    info += ":\n";
    info += stream.url;
    fun(info.c_str(), stream.pos, is_3d(stream));
  });
}
} // namespace streams

using namespace streams;

StreamHandle init_stream(const char *url, const Point2 &min_max_distance, const char *bus)
{
  SNDSYS_IF_NOT_INITED_RETURN_({});
  SNDSYS_POOL_BLOCK;
  G_ASSERT_RETURN(url, {});
  sound_handle_t sh = INVALID_SOUND_HANDLE;
  Stream *stream = all_streams.emplace(sh);
  if (!stream)
    return {};
  stream->url = url;
  stream->minMaxDist = min_max_distance;
  stream->bus = bus;
  return StreamHandle(sh);
}

void release(StreamHandle &stream_handle)
{
  SNDSYS_POOL_BLOCK;
  Stream *stream = all_streams.get((sound_handle_t)stream_handle);
  if (stream)
    release_stream(stream_handle, stream);
  stream_handle.reset();
}

void open(StreamHandle stream_handle)
{
  SNDSYS_IF_NOT_INITED_RETURN;
  SNDSYS_POOL_BLOCK;
  Stream *stream = all_streams.get((sound_handle_t)stream_handle);
  if (stream)
    open_stream(*stream);
}

void close(StreamHandle stream_handle)
{
  SNDSYS_POOL_BLOCK;
  Stream *stream = all_streams.get((sound_handle_t)stream_handle);
  if (stream)
    close_stream(*stream);
}

void set_pos(StreamHandle stream_handle, const Point3 &pos)
{
  SNDSYS_IF_NOT_INITED_RETURN;
  SNDSYS_POOL_BLOCK;
  Stream *stream = all_streams.get((sound_handle_t)stream_handle);
  if (!stream)
    return;
  stream->pos = pos;
  if (!stream->channel)
    return;
  if (FMOD_OK != stream->channel->set3DAttributes(&as_fmod_vector(pos), nullptr))
    close_stream(*stream, true);
}

bool set_paused(StreamHandle stream_handle, bool pause)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  SNDSYS_POOL_BLOCK;
  Stream *stream = all_streams.get((sound_handle_t)stream_handle);
  if (!stream)
    return false;
  return stream->channel->setPaused(pause) == FMOD_OK;
}

bool set_volume(StreamHandle stream_handle, float volume)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  SNDSYS_POOL_BLOCK;
  if (Stream *stream = all_streams.get((sound_handle_t)stream_handle))
  {
    stream->volume = volume;
    return apply_volume(*stream);
  }
  return false;
}

void set_fader(StreamHandle stream_handle, const Point2 &fade_near_far_time)
{
  SNDSYS_IF_NOT_INITED_RETURN;
  SNDSYS_POOL_BLOCK;
  if (Stream *stream = all_streams.get((sound_handle_t)stream_handle))
    stream->fadeNearFarTime = fade_near_far_time;
}

StreamState get_state(StreamHandle stream_handle)
{
  SNDSYS_IF_NOT_INITED_RETURN_(StreamState::ERROR);
  SNDSYS_POOL_BLOCK;
  Stream *stream = all_streams.get((sound_handle_t)stream_handle);
  return stream ? stream->state : StreamState::ERROR;
}

const char *get_state_name(StreamHandle stream_handle) { return get_state_name(get_state(stream_handle)); }

bool is_valid_handle(StreamHandle stream_handle)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  SNDSYS_POOL_BLOCK;
  return all_streams.get((sound_handle_t)stream_handle) != nullptr;
}
} // namespace sndsys
