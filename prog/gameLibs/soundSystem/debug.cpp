// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dag/dag_vectorMap.h>
#include <EASTL/fixed_string.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/array.h>
#include <generic/dag_span.h>
#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_critSec.h>
#include <gui/dag_stdGuiRender.h>
#include <debug/dag_textMarks.h>
#include <fmod_studio.hpp>
#include <soundSystem/soundSystem.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/events.h>
#include <soundSystem/streams.h>
#include <soundSystem/strHash.h>
#include <soundSystem/debug.h>
#include <drv/3d/dag_lock.h>
#include <shaders/dag_shaderBlock.h>

#include "internal/fmodCompatibility.h"
#include "internal/framememString.h"
#include "internal/attributes.h"
#include "internal/delayed.h"
#include "internal/events.h"
#include "internal/streams.h"
#include "internal/soundSystem.h"
#include "internal/occlusion.h"
#include "internal/debug.h"

static WinCritSec g_debug_trace_cs;
#define SNDSYS_DEBUG_TRACE_BLOCK WinAutoLock debugTraceLock(g_debug_trace_cs);

namespace sndsys
{
using namespace fmodapi;

static const Point2 g_offset = {10, 70};
static const E3DCOLOR g_def_color = E3DCOLOR_MAKE(255, 255, 255, 255);
static const E3DCOLOR g_update_color = E3DCOLOR_MAKE(255, 200, 0, 255);
static const E3DCOLOR g_loading_color = E3DCOLOR_MAKE(255, 0, 255, 255);
static const E3DCOLOR g_shadow_color = E3DCOLOR_MAKE(0, 0, 60, 255);
static const E3DCOLOR g_empty_color = E3DCOLOR_MAKE(190, 190, 190, 255);
static const E3DCOLOR g_virtual_color = E3DCOLOR_MAKE(128, 128, 255, 255);
static const E3DCOLOR g_sustaining_color = E3DCOLOR_MAKE(255, 128, 255, 255);
static const E3DCOLOR g_stopped_color = E3DCOLOR_MAKE(255, 128, 0, 255);
static const E3DCOLOR g_stopping_color = E3DCOLOR_MAKE(100, 50, 50, 255);
static const E3DCOLOR g_invalid_color = E3DCOLOR_MAKE(255, 0, 0, 0);
static const E3DCOLOR g_snapshot_color = E3DCOLOR_MAKE(100, 0, 255, 255);
static const E3DCOLOR g_just_created_color = E3DCOLOR_MAKE(255, 245, 0, 255);
static bool g_draw_audibility = false;

enum class TraceLevel : int
{
  info = 0,
  warn,
  err,
  log,
};

struct DebugMessage
{
  eastl::fixed_string<char, 128> text;
  TraceLevel level = {};
  size_t count = 0;
};

using FrameString = eastl::fixed_string<char, 32, true, framemem_allocator>;

static eastl::array<FMOD::Studio::EventDescription *, 2048> events_list;
static eastl::array<FMOD::Studio::EventInstance *, 1024> instance_list;
static eastl::array<FMOD::Studio::Bank *, 256> banks_list;

static eastl::fixed_vector<const FMOD::Studio::EventDescription *, 16> g_3d_instances_in_zero;

static int g_first_message_id = 0;
static int g_num_messages = 0;
static int g_interligne = 0;
static int g_font_height = 0;
static int g_box_extent_x = 0;
static int g_box_extent_y = 0;
static int g_box_offset = 0;
static bool g_have_3d_stream_in_zero = false;

static constexpr TraceLevel def_log_level = TraceLevel::err;
static TraceLevel g_log_level = def_log_level;
static eastl::array<DebugMessage, 24> debug_messages;

static const E3DCOLOR g_background_none = E3DCOLOR(0, 0, 0, 0);
static const E3DCOLOR g_background_def = E3DCOLOR(0, 0, 0, 200);
static const E3DCOLOR g_background_err = E3DCOLOR(0xff, 0x40, 0, 0xff);

static inline int print(int x, int y, const E3DCOLOR &color, const char *text, int len = -1, int line_index = 0,
  E3DCOLOR background = g_background_none)
{
  if (text && *text)
  {
    if (background != g_background_none)
    {
      StdGuiRender::set_draw_str_attr(FFT_NONE, 0, 0, g_shadow_color, 24);
      StdGuiRender::set_color(background);
      const Point2 htsz = StdGuiRender::get_str_bbox(text, len).width();
      StdGuiRender::render_box(x - g_box_extent_x, y - g_box_extent_y + line_index * g_font_height + g_box_offset,
        x + htsz.x + g_box_extent_x, y + htsz.y + g_box_extent_y + line_index * g_font_height + g_box_offset);
    }
    else
    {
      StdGuiRender::set_draw_str_attr(FFT_SHADOW, 0, 0, g_shadow_color, 24);
      const int offset = max(1, g_font_height / 16);
      StdGuiRender::set_color(E3DCOLOR(0, 0, 0, 0xff));
      StdGuiRender::goto_xy(x - offset, y + offset);
      StdGuiRender::draw_str(text, len);
    }
    StdGuiRender::set_color(color);
    StdGuiRender::goto_xy(floorf(x), floorf(y + line_index * g_font_height));
    StdGuiRender::draw_str(text, len);
  }
  return g_interligne;
}

template <typename String>
static inline int print(int x, int y, const E3DCOLOR &color, const String &text, int len = -1, int line_index = 0,
  E3DCOLOR background = g_background_none)
{
  return print(x, y, color, text.c_str(), len, line_index, background);
}

template <typename String, bool Print = true>
static inline int print_multiline_impl(int x, int y, const E3DCOLOR &color, const String &text, E3DCOLOR background)
{
  G_UNREFERENCED(x);
  G_UNREFERENCED(y);
  G_UNREFERENCED(color);
  if (text.empty())
    return g_interligne;
  int height = 0;
  const char *ptr = text.begin();
  for (; ptr < text.end();)
  {
    const char *separator = strchr(ptr, '\n');
    if (!separator)
      break;
    if constexpr (Print)
      print(x, y + height, color, ptr, separator - ptr, 0, background);
    height += g_interligne;
    ptr = separator + 1;
  }
  if constexpr (Print)
    print(x, y + height, color, ptr, text.end() - ptr, 0, background);
  height += g_interligne;
  return height;
}

template <typename String>
static inline int print_multiline(int x, int y, const E3DCOLOR &color, const String &text, E3DCOLOR background = g_background_none)
{
  return print_multiline_impl(x, y, color, text, background);
}

template <typename String>
static inline int print_multiline_bottom_align(int x, int y, const E3DCOLOR &color, const String &text,
  E3DCOLOR background = g_background_none)
{
  const int height = print_multiline_impl<String, false>(x, y, color, text, background);
  print_multiline_impl(x, y - height, color, text, background);
  return height;
}

template <typename... Args>
static inline int print_format(int x, int y, const E3DCOLOR &color, const char *format, Args... args)
{
  FrameString text;
  text.sprintf(format ? format : "", args...);
  return print_multiline(x, y, color, text);
}

static inline bool is_playing(const FMOD::Studio::EventInstance &event_instance)
{
  FMOD_STUDIO_PLAYBACK_STATE playbackState = FMOD_STUDIO_PLAYBACK_STOPPED;
  event_instance.getPlaybackState(&playbackState);
  return playbackState == FMOD_STUDIO_PLAYBACK_PLAYING || playbackState == FMOD_STUDIO_PLAYBACK_STARTING ||
         playbackState == FMOD_STUDIO_PLAYBACK_SUSTAINING;
}
static inline bool is_stopping(const FMOD::Studio::EventInstance &event_instance)
{
  FMOD_STUDIO_PLAYBACK_STATE playbackState = FMOD_STUDIO_PLAYBACK_STOPPED;
  event_instance.getPlaybackState(&playbackState);
  return playbackState == FMOD_STUDIO_PLAYBACK_STOPPING;
}
static inline bool is_sustaining(const FMOD::Studio::EventInstance &event_instance)
{
  FMOD_STUDIO_PLAYBACK_STATE playbackState = FMOD_STUDIO_PLAYBACK_STOPPED;
  event_instance.getPlaybackState(&playbackState);
  return playbackState == FMOD_STUDIO_PLAYBACK_SUSTAINING;
}
static inline bool is_virtual(const FMOD::Studio::EventInstance &event_instance)
{
  bool isVirtual = false;
  event_instance.isVirtual(&isVirtual);
  return isVirtual;
}
static inline bool is_muted(const FMOD::Studio::EventInstance &event_instance)
{
  float volume = 0.f;
  event_instance.getVolume(&volume);
  return volume == 0.f;
}
static inline bool is_snapshot(const FMOD::Studio::EventDescription &event_description)
{
  bool snapshot = false;
  event_description.isSnapshot(&snapshot);
  return snapshot;
}
static inline bool is_loading(const FMOD::Studio::EventDescription &event_description)
{
  if (const char *sampleLoadingState = get_sample_loading_state(event_description))
    return strcmp(sampleLoadingState, "loading") == 0;
  return false;
}

static inline E3DCOLOR make_color(const FMOD::Studio::EventInstance &event_instance,
  const FMOD::Studio::EventDescription &event_description)
{
  return !event_instance.isValid()        ? g_invalid_color
         : is_sustaining(event_instance)  ? g_sustaining_color
         : is_stopping(event_instance)    ? g_stopping_color
         : !is_playing(event_instance)    ? g_stopped_color
         : is_virtual(event_instance)     ? g_virtual_color
         : is_snapshot(event_description) ? g_snapshot_color
                                          : g_def_color;
}

static class
{
  enum
  {
    capacity = 32,
    max_time = 16,
  };
  struct Desc
  {
    const FMOD::Studio::EventDescription *desc = nullptr;
    const FMOD::Studio::EventInstance *instance = nullptr;
    uint8_t numInstances = 0;
    uint8_t numPlaying = 0;
    uint8_t numVirtual = 0;
    uint8_t numMuted = 0;
    uint8_t numDuplicated = 0;
    uint8_t time = 0;
    bool odd = false;
  };
  eastl::fixed_vector<Desc, capacity> descs;
  bool odd = false;

public:
  void push(const FMOD::Studio::EventInstance &event_instance, const FMOD::Studio::EventDescription &event_description,
    int num_instances, int num_playing, int num_virtual, int num_muted)
  {
    Desc *desc = eastl::find_if(descs.begin(), descs.end(), [&](const auto &it) { return it.desc == &event_description; });
    if (desc != descs.end())
    {
      if (desc->odd == odd && desc->numDuplicated < 0xff)
        ++desc->numDuplicated;
      if (desc->numInstances < num_instances)
        desc->time = 0;
    }
    else
    {
      if (descs.size() >= capacity)
        return;
      desc = &descs.push_back();
      desc->desc = &event_description;
      desc->numDuplicated = 0;
      desc->time = 0;
    }
    desc->instance = &event_instance;
    desc->numInstances = min(num_instances, 0xff);
    desc->numPlaying = min(num_playing, 0xff);
    desc->numVirtual = min(num_virtual, 0xff);
    desc->numMuted = min(num_muted, 0xff);
    desc->odd = odd;
    if (desc->time < max_time)
      ++desc->time;
  }

  E3DCOLOR getColor(const Desc &desc, const FMOD::Studio::EventInstance &event_instance)
  {
    if (is_loading(*desc.desc))
      return g_loading_color;
    const float t = saturate(float(desc.time) / max_time);
    return e3dcolor_lerp(g_just_created_color, make_color(event_instance, *desc.desc), t < 0.6f ? 0.f : t);
  }

  E3DCOLOR getColor(const Desc &desc) { return getColor(desc, *desc.instance); }

  E3DCOLOR getColor(const FMOD::Studio::EventDescription &event_description, const FMOD::Studio::EventInstance &event_instance)
  {
    const Desc *desc = eastl::find_if(descs.begin(), descs.end(), [&](const auto &it) { return it.desc == &event_description; });
    if (desc != descs.end())
      return getColor(*desc, event_instance);
    return make_color(event_instance, event_description);
  }

  void advance()
  {
    auto pred = [&](const Desc &it) { return it.odd != odd; };
    descs.erase(eastl::remove_if(descs.begin(), descs.end(), pred), descs.end());
    odd = !odd;
  }

  const Desc *begin() const { return descs.begin(); }
  const Desc *end() const { return descs.end(); }

} g_instances;

static inline void get_name_audibility(const FMOD::Studio::EventInstance &event_instance,
  const FMOD::Studio::EventDescription &event_description, FrameString &text)
{
  text.sprintf("[%s]", get_debug_name(event_description).c_str());

  FMOD_STUDIO_PARAMETER_DESCRIPTION paramDesc;
  if (FMOD_OK == event_description.getParameterDescriptionByName("Distance", &paramDesc))
  {
    float value = 0;
    float finalValue = 0;
    event_instance.getParameterByID(paramDesc.id, &value, &finalValue);
    text.append_sprintf(" [%s %.2f]", paramDesc.name, finalValue);
  }

  const float audibility = get_audibility(event_instance);
  text.append_sprintf("  (%f)", audibility);
}

static inline void get_event_name(const FMOD::Studio::EventInstance &event_instance,
  const FMOD::Studio::EventDescription &event_description, FrameString &text)
{
  text.sprintf("[%s]", get_debug_name(event_description).c_str());

  int paramDescCount = 0;
  event_description.getParameterDescriptionCount(&paramDescCount);

  FMOD_STUDIO_PARAMETER_DESCRIPTION paramDesc;
  for (int i = 0; i < paramDescCount; ++i)
    if (FMOD_OK == event_description.getParameterDescriptionByIndex(i, &paramDesc))
    {
      float value = 0;
      float finalValue = 0;
      event_instance.getParameterByID(paramDesc.id, &value, &finalValue);
      text.append_sprintf(" [%s %.2f]", paramDesc.name, finalValue);
    }
  float volume = 0.f, finalVolume = 0.f;
  if (FMOD_OK == event_instance.getVolume(&volume, &finalVolume) && volume != 1.f)
    text.append_sprintf(" vol=%.2f", volume);
}

static inline void print_samples_instances(int offset)
{
  const auto samples = dbg_get_sample_data_refs();

  if (!samples.empty())
  {
    offset += print(g_offset.x, offset, g_def_color, "[PRELOADED EVENTS]");

    eastl::array<char, 512> eventName;
    if (!samples.empty())
    {
      static constexpr int maxSamples = 32;
      int count = 0;
      for (const auto &it : samples)
      {
        if (const FMOD::Studio::EventDescription *evtDesc = get_description(it.first))
        {
          int len = 0;
          evtDesc->getPath(eventName.begin(), eventName.count, &len);
          eventName[eventName.count - 1] = 0;
          offset += print_format(g_offset.x, offset, g_def_color, "(%d) %s", it.second, eventName.begin());
        }
        else
          offset += print_format(g_offset.x, offset, g_def_color, "(%d) %s", it.second, "???");
        ++count;
        if (count >= maxSamples)
        {
          offset += print_format(g_offset.x, offset, g_def_color, "%d more...", samples.size() - count);
          break;
        }
      }
    }
    offset += print(g_offset.x, offset, g_def_color, "");
  }

  offset += print(g_offset.x, offset, g_def_color, "[INSTANCES]");

  FrameString text;
  int numBanks = 0;
  SOUND_VERIFY(get_studio_system()->getBankList(banks_list.begin(), banks_list.count, &numBanks));
  G_ASSERT(numBanks <= banks_list.count);
  numBanks = min(numBanks, (int)banks_list.count);
  dag::VectorMap<int, int, eastl::less<int>, framemem_allocator> commonPos;
  const int bottom = StdGuiRender::get_viewport().rightBottom.y;

  const auto banksSlice = make_span(banks_list.begin(), numBanks);
  for (const FMOD::Studio::Bank *bank : banksSlice)
  {
    int numDescs = 0;
    if (FMOD_OK != bank->getEventList(events_list.begin(), events_list.count, &numDescs))
      continue;
    G_ASSERT(numDescs <= events_list.count);
    numDescs = min(numDescs, (int)events_list.count);

    const auto descsSlice = make_span(events_list.begin(), numDescs);
    for (const FMOD::Studio::EventDescription *evtDesc : descsSlice)
    {
      int numInstances = 0;
      if (FMOD_OK != evtDesc->getInstanceList(instance_list.begin(), instance_list.count, &numInstances))
        continue;
      G_ASSERT(numInstances <= instance_list.count);
      numInstances = min(numInstances, (int)instance_list.count);
      if (!numInstances)
        continue;

      bool is3d = false;
      evtDesc->is3D(&is3d);

      int numPlaying = 0, numVirtual = 0, numMuted = 0;
      const auto instancesSlice = make_span(instance_list.begin(), numInstances);
      for (const FMOD::Studio::EventInstance *instance : instancesSlice)
      {
        numPlaying += is_playing(*instance) ? 1 : 0;
        numVirtual += is_virtual(*instance) ? 1 : 0;
        numMuted += is_muted(*instance) ? 1 : 0;
      }

      const FMOD::Studio::EventInstance &firstInstance = *instance_list.front();
      g_instances.push(firstInstance, *evtDesc, numInstances, numPlaying, numVirtual, numMuted);

      for (const FMOD::Studio::EventInstance *instance : instancesSlice)
      {
        if (!is3d)
          continue;

        FMOD_3D_ATTRIBUTES attributes = {};
        if (FMOD_OK != instance->get3DAttributes(&attributes))
          continue;
        Point2 pointOnScreen = ZERO<Point2>();
        if (cvt_debug_text_pos(sndsys::as_point3(attributes.position), pointOnScreen.x, pointOnScreen.y))
        {
          int pos = pointOnScreen.x + 10000 * pointOnScreen.y;
          auto ins = commonPos.insert(eastl::make_pair(pos, 0));
          if (!ins.second) // i.e. not inserted
            ins.first->second++;
          const int lineIndex = int(ins.first->second);
          const E3DCOLOR color = g_instances.getColor(*evtDesc, *instance);
          if (g_draw_audibility)
            get_name_audibility(*instance, *evtDesc, text);
          else
            get_event_name(*instance, *evtDesc, text);
          print(int(pointOnScreen.x), int(pointOnScreen.y), color, text.c_str(), -1, lineIndex, g_background_def);
        }

        if (lengthSq(sndsys::as_point3(attributes.position)) == 0.f)
          if (eastl::find(g_3d_instances_in_zero.begin(), g_3d_instances_in_zero.end(), evtDesc) == g_3d_instances_in_zero.end())
          {
            g_3d_instances_in_zero.push_back(evtDesc);
            get_event_name(*instance, *evtDesc, text);
            debug_trace_warn("3d sound event \"%s\" plays in (0,0,0)", text.c_str());
          }
      }
    }
  }
  g_instances.advance();

  for (auto &it : g_instances)
  {
    const FMOD::Studio::EventInstance &instance = *it.instance;
    const FMOD::Studio::EventDescription &desc = *it.desc;

    if (g_draw_audibility)
      get_name_audibility(instance, desc, text);
    else
      get_event_name(instance, desc, text);

    if (it.numInstances > 1)
      text.append_sprintf(" (%d)", it.numInstances);

    char fmodPath[DAGOR_MAX_PATH];
    int fmodPathSize = 0;
    EventAttributes attributes;
    if (FMOD_OK != desc.getPath(fmodPath, countof(fmodPath), &fmodPathSize) ||
        !(attributes = find_event_attributes(fmodPath, fmodPathSize - 1)))
    {
      FMODGUID id;
      G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
      if (desc.getID((FMOD_GUID *)&id) != FMOD_OK || !(attributes = find_event_attributes(id)))
        text += " (missing attributes)";
    }

    if (attributes.isOneshot())
      text += " (oneshot)";

    if (attributes.hasSustainPoint())
      text += " (sustainPt)";

    text += attributes.is3d() ? " (3D)" : " (2D)";

    if (is_loading(desc))
      text += " (--LOADING--)";

    if (it.numInstances > 1)
    {
      if (it.numVirtual)
        text.append_sprintf(" (%d virtual)", it.numVirtual);
      if (it.numMuted)
        text.append_sprintf(" (%d muted)", it.numMuted);
      if (it.numPlaying < it.numInstances)
        text.append_sprintf(" (%d stopped)", it.numInstances - it.numPlaying);
    }

    if (const char *sampleLoadingState = get_sample_loading_state(desc))
      if (strcmp(sampleLoadingState, "loaded") != 0)
        text.append_sprintf(" (%s)", sampleLoadingState);

    if (it.numDuplicated > 1)
      text.append_sprintf(" (--DUPLICATED IN BANKS (%d)--) ", it.numDuplicated);

    offset += print(g_offset.x, offset, g_instances.getColor(it), text, -1, 0, g_background_def);

    if (offset >= bottom)
      break;
  }
}

static inline int get_channels_x()
{
  constexpr int rightOffset = 400;
  return StdGuiRender::get_viewport().rightBottom.x - rightOffset;
}

static inline void print_channels(int max_y)
{
  int numChannelsPlaying = 0;
  SOUND_VERIFY(get_system()->getChannelsPlaying(&numChannelsPlaying));

  int numPlayingChannels = 0, numVirtualChannels = 0;
  for (int i = 0; i < get_max_channels(); i++)
  {
    FMOD::Channel *channel = nullptr;
    if (get_system()->getChannel(i, &channel) != FMOD_OK)
      continue;
    bool isPlaying = false;
    if (channel->isPlaying(&isPlaying) == FMOD_OK && isPlaying)
      ++numPlayingChannels;
    bool isVirtual = false;
    if (channel->isVirtual(&isVirtual) == FMOD_OK && isVirtual)
      ++numVirtualChannels;
  }

  int offset = g_offset.y;
  const int x = get_channels_x();
  offset += print(x, offset, (numPlayingChannels || numVirtualChannels) ? g_def_color : g_empty_color, "[CHANNELS]");

  if (numPlayingChannels || numVirtualChannels)
  {
    offset += print_format(x, offset, g_def_color, "playing: %d\nvirtual: %d\n", numPlayingChannels, numVirtualChannels);
    for (int i = 0; i < get_max_channels(); ++i)
    {
      FMOD::Channel *channel = nullptr;
      if (get_system()->getChannel(i, &channel) != FMOD_OK)
        continue;

      bool isPlaying = false;
      if (channel->isPlaying(&isPlaying) != FMOD_OK || !isPlaying)
        continue;

      bool isVirtual = false;
      channel->isVirtual(&isVirtual);

      char name[64] = {0};
      FMOD::Sound *sound = nullptr;
      if (channel->getCurrentSound(&sound) != FMOD_OK || !sound || sound->getName(name, sizeof(name)) != FMOD_OK)
        SNPRINTF(name, sizeof(name), "(unknown)");

      offset += print_format(x, offset, isVirtual ? g_virtual_color : g_def_color, "%d: %s", i, name);
      if (offset >= max_y)
        break;
    }
  }
}

static constexpr uint32_t g_warn_color = 0xffffffff;
static constexpr uint32_t g_err_color = 0xff000000;
static constexpr uint32_t g_log_color = 0xffffd000;

static inline void print_messages()
{
  SNDSYS_DEBUG_TRACE_BLOCK;
  int offset = StdGuiRender::get_viewport().rightBottom.y - 20;
  for (int i = 0; i < g_num_messages; ++i)
  {
    const DebugMessage &msg = debug_messages[(i + g_first_message_id) % debug_messages.count];
    const uint32_t color = (msg.level == TraceLevel::warn) ? g_warn_color : (msg.level == TraceLevel::err) ? g_err_color : g_log_color;
    const auto background = msg.level == TraceLevel::err ? g_background_err : g_background_none;
    if (msg.count <= 1)
      offset -= print_multiline_bottom_align(g_offset.x, offset, color, msg.text, background);
    else
    {
      FrameStr text;
      text.sprintf("%s (%d messages)", msg.text.c_str(), msg.count);
      offset -= print_multiline_bottom_align(g_offset.x, offset, color, text, background);
    }
  }
}

static inline void draw_streams()
{
  streams::debug_enum([](const char *info, const Point3 &pos, bool is_3d) {
    if (is_3d && lengthSq(pos) == 0.f && !g_have_3d_stream_in_zero)
    {
      g_have_3d_stream_in_zero = true;
      debug_trace_warn("3d stream \"%s\" is being played in (0,0,0)", info);
    }
    Point2 pointOnScreen = ZERO<Point2>();
    if (cvt_debug_text_pos(pos, pointOnScreen.x, pointOnScreen.y))
      print_format(int(pointOnScreen.x), int(pointOnScreen.y + g_interligne), 0xff00ffff, "%s", info);
  });
}

static inline void draw_cpu_stat(int &offset)
{
  sndsys::FmodCpuUsage fcpu = {};
  get_cpu_usage(fcpu);
  offset += print_format(g_offset.x, offset, g_def_color, "CPU: dsp %.f%%/ geom %.f%%/ stream %.f%%/ studio %.f%%/ upd %.f%%",
    fcpu.dspusage, fcpu.geometryusage, fcpu.streamusage, fcpu.studiousage, fcpu.updateusage);
}

static inline void draw_mem_stat(int &offset)
{
  sndsys::FmodMemoryUsage fmem = {};
  get_mem_usage(fmem);
  offset +=
    print_format(g_offset.x, offset, g_def_color, "Memory (MB): inclusive %.2f, exclusive %.2f, sampledata %.2f, cur %.2f, max %.2f",
      fmem.inclusive / 1024.f / 1024.f, fmem.exclusive / 1024.f / 1024.f, fmem.sampledata / 1024.f / 1024.f,
      fmem.currentalloced / 1024.f / 1024.f, fmem.maxalloced / 1024.f / 1024.f);
}

static int g_occlusion_x = 0;
static int g_occlusion_y = 0;
static int g_occlusion_max_y = 0;
static void draw_occlusion_src(FMOD::Studio::EventInstance *instance, occlusion::group_id_t group_id, const Point3 &, float value,
  bool is_in_group, bool is_first_in_group, bool debug_update, const Point3 &pos)
{
  if (g_occlusion_y >= g_occlusion_max_y)
    return;
  FrameStr name = "???";
  FMOD::Studio::EventDescription *description = nullptr;
  if (FMOD_OK == instance->getDescription(&description))
    name = eastl::move(get_debug_name(*description));

  const char *shortName = nullptr;
  for (const char *ptr = name.c_str(); ptr && *ptr; ++ptr)
    if (*ptr == '/' || *ptr == '\\')
      shortName = ptr + 1;

  g_occlusion_y +=
    print_format(g_occlusion_x, g_occlusion_y, debug_update ? g_update_color : g_def_color, "%s  %d   %.2f  %s  {%.1f, %.1f}",
      is_first_in_group ? "(*)"
      : is_in_group     ? "(^)"
                        : "( )",
      group_id, value, shortName, pos.x, pos.z);
}

static void print_occlusion(int y, int max_y)
{
  g_occlusion_x = get_channels_x();
  g_occlusion_y = y;
  g_occlusion_y += print(g_occlusion_x, g_occlusion_y, g_def_color, "[OCCLUSION]");
  g_occlusion_max_y = max_y;

  int totalTraces = 0;
  int maxTraces = 0;
  occlusion::get_debug_info(totalTraces, maxTraces);
  g_occlusion_y += print_format(g_occlusion_x, g_occlusion_y, g_def_color, "traces total/max: %d/%d", totalTraces, maxTraces);

  occlusion::debug_enum_sources(&draw_occlusion_src);
}

void debug_draw()
{
  SNDSYS_IF_NOT_INITED_RETURN;

  d3d::GpuAutoLock gpuLock;
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

  StdGuiRender::ScopeStarterOptional strt;
  StdGuiRender::reset_textures();

  StdGuiRender::set_font(0);
  g_font_height = StdGuiRender::get_font_cell_size().y;
  g_box_extent_x = ceil(g_font_height * 0.125f);
  g_box_extent_y = 0;
  g_box_offset = -0.8f * g_font_height;
  g_interligne = g_font_height + g_box_extent_y * 2;

  size_t usedHandles = 0, totalHandles = 0;
  sndsys::get_info(usedHandles, totalHandles);

  int offset = g_offset.y;
  offset += print_format(g_offset.x, offset, g_def_color, "Event handles (used/total): %d/%d", usedHandles, totalHandles);

  uint32_t streamHandles = 0, streamsToRelease = 0;
  streams::debug_get_info(streamHandles, streamsToRelease);
  offset += print_format(g_offset.x, offset, g_def_color, "Stream handles (used,releasing): %d/%d", streamHandles, streamsToRelease);

  size_t delayedEvents = 0, delayedActions = 0, maxDelayedEvents = 0, maxDelayedActions = 0;
  delayed::get_debug_info(delayedEvents, delayedActions, maxDelayedEvents, maxDelayedActions);
  offset += print_format(g_offset.x, offset, g_def_color, "Delayed events (current,max): %d/%d, actions (current,max): %d/%d",
    delayedEvents, maxDelayedEvents, delayedActions, maxDelayedActions);

  offset += print_format(g_offset.x, offset, g_def_color, "Cached event attributes: %d(path), %d(guid)",
    get_num_cached_path_attributes(), get_num_cached_guid_attributes());

  draw_cpu_stat(offset);
  draw_mem_stat(offset);

  offset += g_interligne;

  print_samples_instances(offset);

  const int screenTop = StdGuiRender::get_viewport().leftTop.y;
  const int screenBottom = StdGuiRender::get_viewport().rightBottom.y;
  const int occlusionY = (screenTop + screenBottom) / 2;
  print_channels(occlusionY - g_interligne);
  print_occlusion(occlusionY, screenBottom);

  print_messages();
  draw_streams();

  StdGuiRender::reset_draw_str_attr();
  StdGuiRender::flush_data();
}

void set_draw_audibility(bool enable) { g_draw_audibility = enable; }

void debug_enum_events()
{
  SNDSYS_IF_NOT_INITED_RETURN;
  debug("[SNDSYS] -- enum all events --");

  eastl::array<char, 512> bankName;
  eastl::array<char, 512> eventName;
  FrameStr vars;
  FMOD_STUDIO_PARAMETER_DESCRIPTION varDesc;
  int totalEvents = 0;

  int numBanks = 0;
  SOUND_VERIFY(get_studio_system()->getBankList(banks_list.begin(), banks_list.count, &numBanks));
  G_ASSERT(numBanks <= banks_list.count);
  numBanks = min(numBanks, (int)banks_list.count);
  const auto banksSlice = make_span(banks_list.begin(), numBanks);
  for (const FMOD::Studio::Bank *bank : banksSlice)
  {
    int len = 0;
    bank->getPath(bankName.begin(), bankName.count, &len);
    bankName[bankName.count - 1] = 0;

    int numDescs = 0;
    if (FMOD_OK != bank->getEventList(events_list.begin(), events_list.count, &numDescs))
      continue;
    G_ASSERT(numDescs <= events_list.count);
    numDescs = min(numDescs, (int)events_list.count);
    totalEvents += numDescs;
    const auto descsSlice = make_span(events_list.begin(), numDescs);
    for (const FMOD::Studio::EventDescription *desc : descsSlice)
    {
      len = 0;
      desc->getPath(eventName.begin(), eventName.count, &len);
      eventName[eventName.count - 1] = 0;

      bool is3d = false;
      desc->is3D(&is3d);

      bool isOneshot = false;
      desc->isOneshot(&isOneshot);

      float maxDist = 0.f;
#if FMOD_VERSION >= 0x00020200
      desc->getMinMaxDistance(nullptr, &maxDist);
#else
      desc->getMaximumDistance(&maxDist);
#endif

      vars.clear();
      int numVars = 0;
      desc->getParameterDescriptionCount(&numVars);
      for (int i = 0; i < numVars; ++i)
        if (FMOD_OK == desc->getParameterDescriptionByIndex(i, &varDesc))
          vars.append_sprintf(" '%s'[%.1f..%.1f];", varDesc.name, varDesc.minimum, varDesc.maximum);

      debug("[SNDSYS] '%s' '%s'; isOneshot=%s; is3d=%s; maxDist=%.0f;%s", bankName.begin(), eventName.begin(),
        isOneshot ? "true" : "false", is3d ? "true" : "false", maxDist, vars.c_str());
    }
  }
  debug("[SNDSYS] -- %d banks, %d events --", numBanks, totalEvents);
  debug_trace_info("%d banks, %d events, see full listing in log", numBanks, totalEvents);
}

void debug_enum_events(const char *bank_name, eastl::function<void(const char *)> &&fun)
{
  SNDSYS_IF_NOT_INITED_RETURN;
  FMOD::Studio::Bank *bank = nullptr;
  eastl::array<char, 1024> eventName;

  FMOD_RESULT result = get_studio_system()->getBank(bank_name ? bank_name : "", &bank);
  if (FMOD_OK != result)
  {
    logerr("Get bank '%s' failed: %s", bank_name, FMOD_ErrorString(result));
    return;
  }

  int numDescs = 0;
  result = bank->getEventList(events_list.begin(), events_list.count, &numDescs);
  if (FMOD_OK != result)
  {
    logerr("Get event list from bank '%s' failed: %s", bank_name, FMOD_ErrorString(result));
    return;
  }

  G_ASSERT(numDescs <= events_list.count);
  numDescs = min(numDescs, (int)events_list.count);
  const auto descsSlice = make_span(events_list.begin(), numDescs);
  for (const FMOD::Studio::EventDescription *desc : descsSlice)
  {
    int len = 0;
    result = desc->getPath(eventName.begin(), eventName.count, &len);
    eventName[eventName.count - 1] = 0;
    if (FMOD_OK != result)
    {
      logerr("Get event name from bank '%s' failed: %s", bank_name, FMOD_ErrorString(result));
      continue;
    }
    const char *str = eventName.cbegin();
    if (strstr(str, "event:/") == str)
      str += strlen("event:/");
    fun(str);
  }
}

static constexpr size_t g_max_log_message_types = 32;
static eastl::fixed_vector<str_hash_t, g_max_log_message_types, false> g_log_message_types;
static bool g_suppress_logerr_once = false;

static inline bool can_log_once(const char *text)
{
  const str_hash_t hash = SND_HASH_SLOW(text);
  auto it = eastl::find(g_log_message_types.begin(), g_log_message_types.end(), hash);
  if (it != g_log_message_types.end())
    return false;
  if (g_log_message_types.size() >= g_max_log_message_types)
    g_log_message_types.erase(g_log_message_types.begin());
  g_log_message_types.push_back(hash);
  return true;
}

static inline void debug_trace_impl(TraceLevel cur_level, bool log_once, const char *format, va_list args)
{
  G_ASSERT_RETURN(format, );
  SNDSYS_DEBUG_TRACE_BLOCK;

  decltype(DebugMessage::text) text;
  text.sprintf_va_list(format, args);
  text.rtrim(" \n");

  if (int(cur_level) >= int(g_log_level))
  {
    const char *fmt = log_once ? "[SNDSYS] (log_once) %s" : "[SNDSYS] %s";
    if (cur_level == TraceLevel::err)
    {
      if (!log_once || (!g_suppress_logerr_once && can_log_once(text.c_str())))
        logerr(fmt, text.c_str());
    }
    else if (cur_level == TraceLevel::warn)
    {
      if (!log_once || can_log_once(text.c_str()))
        logwarn(fmt, text.c_str());
    }
    else
    {
      if (!log_once || can_log_once(text.c_str()))
        debug(fmt, text.c_str());
    }
  }

  for (intptr_t idx = 0; idx < g_num_messages; ++idx)
    if (debug_messages[idx].text == text)
    {
      ++debug_messages[idx].count;
      return;
    }

  if (g_num_messages < debug_messages.count)
    ++g_num_messages;
  else
    g_first_message_id = (g_first_message_id + 1) % debug_messages.count;

  auto &msg = debug_messages[(g_first_message_id + g_num_messages - 1) % debug_messages.count];
  msg.text = eastl::move(text);
  msg.level = cur_level;
  msg.count = 1;
}

#define DEBUG_TRACE_IMPL(FUN, TYPE, ONCE)                   \
  void FUN(const char *format, ...)                         \
  {                                                         \
    va_list args;                                           \
    va_start(args, format);                                 \
    debug_trace_impl(TraceLevel::TYPE, ONCE, format, args); \
    va_end(args);                                           \
  }

DEBUG_TRACE_IMPL(debug_trace_info, info, false)
DEBUG_TRACE_IMPL(debug_trace_warn, warn, false)
DEBUG_TRACE_IMPL(debug_trace_err, err, false)
DEBUG_TRACE_IMPL(debug_trace_err_once, err, true)
DEBUG_TRACE_IMPL(debug_trace_log, log, false)

void debug_init(const DataBlock &blk)
{
  const char *loglevel = blk.getStr("loglevel", "");
  if (strcmp(loglevel, "all") == 0)
    g_log_level = TraceLevel::info;
  else if (strcmp(loglevel, "warnings") == 0)
    g_log_level = TraceLevel::warn;
  else if (strcmp(loglevel, "errors") == 0)
    g_log_level = TraceLevel::err;
  else
    g_log_level = def_log_level;
  g_suppress_logerr_once = blk.getBool("suppressLogerrOnce", false);
}
} // namespace sndsys
