#include <EASTL/map.h>
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
#include <soundSystem/debug.h>
#include <3d/dag_drv3dCmd.h>
#include <shaders/dag_shaderBlock.h>

#include "internal/fmodCompatibility.h"
#include "internal/framememString.h"
#include "internal/attributes.h"
#include "internal/delayed.h"
#include "internal/events.h"
#include "internal/streams.h"
#include "internal/soundSystem.h"
#include "internal/debug.h"

static WinCritSec g_debug_trace_cs;
#define SNDSYS_DEBUG_TRACE_BLOCK WinAutoLock debugTraceLock(g_debug_trace_cs);

namespace sndsys
{
using namespace fmodapi;

static const Point2 g_offset = {10, 70};
static const E3DCOLOR g_def_color = E3DCOLOR_MAKE(0xff, 0xff, 0xff, 0xff);
static const E3DCOLOR g_shadow_color = E3DCOLOR_MAKE(0, 0, 0x40, 0xff);
static const E3DCOLOR g_empty_color = E3DCOLOR_MAKE(0xc0, 0xc0, 0xc0, 0xff);
static const E3DCOLOR g_virtual_color = E3DCOLOR_MAKE(0x80, 0x80, 0xff, 0xff);
static const E3DCOLOR g_keyoff_color = E3DCOLOR_MAKE(0x80, 0xff, 0xff, 0xff);
static const E3DCOLOR g_stopped_color = E3DCOLOR_MAKE(0xff, 0x80, 0, 0xff);
static const E3DCOLOR g_stopping_color = E3DCOLOR_MAKE(0, 0, 0, 0xff);
static const E3DCOLOR g_invalid_color = E3DCOLOR_MAKE(0xff, 0xff, 0, 0xff);
static const E3DCOLOR g_snapshot_color = E3DCOLOR_MAKE(0x66, 0, 0xff, 0xff);
static bool g_enable_debug_draw = false;
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
static bool g_have_3d_stream_in_zero = false;

static constexpr TraceLevel def_log_level = TraceLevel::err;
static TraceLevel g_log_level = def_log_level;
static eastl::array<DebugMessage, 24> debug_messages;

static inline int print(int x, int y, const E3DCOLOR &color, const char *text, int len = -1)
{
  StdGuiRender::goto_xy(x, y);
  StdGuiRender::set_color(color);
  StdGuiRender::draw_str(text, len);
  return g_interligne;
}

template <typename String>
static inline int print(int x, int y, const E3DCOLOR &color, const String &text, int len = -1)
{
  return print(x, y, color, text.c_str(), len);
}

template <typename String, bool Print = true>
static inline int print_multiline_impl(int x, int y, const E3DCOLOR &color, const String &text)
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
      print(x, y + height, color, ptr, separator - ptr);
    height += g_interligne;
    ptr = separator + 1;
  }
  if constexpr (Print)
    print(x, y + height, color, ptr, text.end() - ptr);
  height += g_interligne;
  return height;
}

template <typename String>
static inline int print_multiline(int x, int y, const E3DCOLOR &color, const String &text)
{
  return print_multiline_impl(x, y, color, text);
}

template <typename String>
static inline int print_multiline_bottom_align(int x, int y, const E3DCOLOR &color, const String &text)
{
  const int height = print_multiline_impl<String, false>(x, y, color, text);
  print_multiline_impl(x, y - height, color, text);
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
static inline bool is_snapshot(const FMOD::Studio::EventInstance &event_instance)
{
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  event_instance.getDescription(&eventDescription);
  bool snapshot = false;
  if (eventDescription)
    eventDescription->isSnapshot(&snapshot);
  return snapshot;
}

static inline E3DCOLOR get_color(const FMOD::Studio::EventInstance &event_instance)
{
  return !event_instance.isValid()       ? g_invalid_color
         : is_sustaining(event_instance) ? g_keyoff_color
         : is_stopping(event_instance)   ? g_stopping_color
         : !is_playing(event_instance)   ? g_stopped_color
         : is_virtual(event_instance)    ? g_virtual_color
         : is_snapshot(event_instance)   ? g_snapshot_color
                                         : g_def_color;
}

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

static inline void draw_instances(int offset, const TMatrix4 &glob_tm)
{
  offset += print(g_offset.x, offset, g_def_color, "[INSTANCES]");

  FrameString text;
  int numBanks = 0;
  SOUND_VERIFY(get_studio_system()->getBankList(banks_list.begin(), banks_list.count, &numBanks));
  G_ASSERT(numBanks <= banks_list.count);
  numBanks = min(numBanks, (int)banks_list.count);
  eastl::map<int, int, eastl::less<int>, framemem_allocator> commonPos;
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
    for (const FMOD::Studio::EventDescription *desc : descsSlice)
    {
      int numInstances = 0;
      if (FMOD_OK != desc->getInstanceList(instance_list.begin(), instance_list.count, &numInstances))
        continue;
      G_ASSERT(numInstances <= instance_list.count);
      numInstances = min(numInstances, (int)instance_list.count);
      if (!numInstances)
        continue;

      bool is3d = false;
      desc->is3D(&is3d);

      int numPlaying = 0, numVirtual = 0, numMuted = 0;
      const auto instancesSlice = make_span(instance_list.begin(), numInstances);
      for (const FMOD::Studio::EventInstance *instance : instancesSlice)
      {
        numPlaying += is_playing(*instance) ? 1 : 0;
        numVirtual += is_virtual(*instance) ? 1 : 0;
        numMuted += is_muted(*instance) ? 1 : 0;

        if (!is3d)
          continue;

        FMOD_3D_ATTRIBUTES attributes = {};
        if (FMOD_OK != instance->get3DAttributes(&attributes))
          continue;
        Point2 pointOnScreen = ZERO<Point2>();
        if (cvt_debug_text_pos(glob_tm, sndsys::as_point3(attributes.position), pointOnScreen.x, pointOnScreen.y))
        {
          int pos = pointOnScreen.x + 10000 * pointOnScreen.y;
          if (commonPos.find(pos) == commonPos.end())
            commonPos.insert(eastl::make_pair(pos, 0));
          else
            commonPos[pos]++;
          int yOffset = int(float(commonPos[pos]) * StdGuiRender::get_font_cell_size().y);
          E3DCOLOR color = get_color(*instance);
          if (g_draw_audibility)
            get_name_audibility(*instance, *desc, text);
          else
            get_event_name(*instance, *desc, text);
          print(int(pointOnScreen.x), int(pointOnScreen.y + yOffset), color, text);
        }

        if (lengthSq(sndsys::as_point3(attributes.position)) == 0.f)
          if (eastl::find(g_3d_instances_in_zero.begin(), g_3d_instances_in_zero.end(), desc) == g_3d_instances_in_zero.end())
          {
            g_3d_instances_in_zero.push_back(desc);
            get_event_name(*instance, *desc, text);
            debug_trace_warn("3d event \"%s\" is being played in (0,0,0)", text.c_str());
          }
      }

      const FMOD::Studio::EventInstance &firstInstance = *instance_list.front();

      if (g_draw_audibility)
        get_name_audibility(firstInstance, *desc, text);
      else
        get_event_name(firstInstance, *desc, text);

      if (numInstances > 1)
        text.append_sprintf(" (%d)", numInstances);

      char fmodPath[DAGOR_MAX_PATH];
      int fmodPathSize = 0;
      EventAttributes attributes;
      if (FMOD_OK != desc->getPath(fmodPath, countof(fmodPath), &fmodPathSize) ||
          !(attributes = find_event_attributes(fmodPath, fmodPathSize - 1)))
      {
        FMODGUID id;
        G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
        if (desc->getID((FMOD_GUID *)&id) != FMOD_OK || !(attributes = find_event_attributes(id)))
        {
          text += " (missing attributes)";
        }
      }

      if (attributes.isOneshot())
        text += " (oneshot)";

      if (attributes.hasSustainPoint())
        text += " (sustainPt)";

      text += is3d ? " (3D)" : " (2D)";

      E3DCOLOR color = g_def_color;
      if ((!numPlaying || numPlaying == numInstances) && (!numVirtual || numVirtual == numInstances))
        color = get_color(firstInstance);

      if (numInstances > 1)
      {
        if (numVirtual)
          text.append_sprintf(" (%d virtual)", numVirtual);
        if (numMuted)
          text.append_sprintf(" (%d muted)", numMuted);
        if (numPlaying < numInstances)
          text.append_sprintf(" (%d stopped)", numInstances - numPlaying);
      }

      offset += print(g_offset.x, offset, color, text);

      if (offset >= bottom)
        return;
    }
  }
}

static inline void print_channels()
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

  const int rightOffset = 400;
  const int left = StdGuiRender::get_viewport().rightBottom.x - rightOffset;
  const int bottom = StdGuiRender::get_viewport().rightBottom.y;
  int offset = g_offset.y;
  offset += print(left, offset, (numPlayingChannels || numVirtualChannels) ? g_def_color : g_empty_color, "[CHANNELS]");

  if (numPlayingChannels || numVirtualChannels)
  {
    offset += print_format(left, offset, g_def_color, "playing: %d\nvirtual: %d\n", numPlayingChannels, numVirtualChannels);
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

      offset += print_format(left, offset, isVirtual ? g_virtual_color : g_def_color, "%d: %s", i, name);
      if (offset >= bottom)
        break;
    }
  }
}

static constexpr uint32_t g_warn_color = 0xffffffff;
static constexpr uint32_t g_err_color = 0xff804000;
static constexpr uint32_t g_log_color = 0xffffd000;

static inline void print_messages()
{
  SNDSYS_DEBUG_TRACE_BLOCK;
  int offset = StdGuiRender::get_viewport().rightBottom.y - 20;
  for (int i = 0; i < g_num_messages; ++i)
  {
    const DebugMessage &msg = debug_messages[(i + g_first_message_id) % debug_messages.count];
    const uint32_t color = (msg.level == TraceLevel::warn) ? g_warn_color : (msg.level == TraceLevel::err) ? g_err_color : g_log_color;
    if (msg.count <= 1)
      offset -= print_multiline_bottom_align(g_offset.x, offset, color, msg.text);
    else
    {
      FrameStr text;
      text.sprintf("%s (%d messages)", msg.text.c_str(), msg.count);
      offset -= print_multiline_bottom_align(g_offset.x, offset, color, text);
    }
  }
}

static inline void draw_streams(const TMatrix4 &glob_tm)
{
  streams::debug_enum([&glob_tm](const char *info, const Point3 &pos, bool is_3d) {
    if (is_3d && lengthSq(pos) == 0.f && !g_have_3d_stream_in_zero)
    {
      g_have_3d_stream_in_zero = true;
      debug_trace_warn("3d stream \"%s\" is being played in (0,0,0)", info);
    }
    Point2 pointOnScreen = ZERO<Point2>();
    if (cvt_debug_text_pos(glob_tm, pos, pointOnScreen.x, pointOnScreen.y))
      print_format(int(pointOnScreen.x), int(pointOnScreen.y + StdGuiRender::get_font_cell_size().y), 0xff00ffff, "%s", info);
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

void debug_draw(const TMatrix4 &glob_tm)
{
  SNDSYS_IF_NOT_INITED_RETURN;

  d3d::GpuAutoLock gpuLock;
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

  StdGuiRender::ScopeStarterOptional strt;
  StdGuiRender::set_texture(BAD_TEXTUREID);

  StdGuiRender::set_font(0);
  g_interligne = StdGuiRender::get_font_cell_size().y;
  StdGuiRender::set_draw_str_attr(FFT_SHADOW, 0, 0, g_shadow_color, 24);

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

  draw_instances(offset, glob_tm);
  print_channels();
  print_messages();
  draw_streams(glob_tm);

  StdGuiRender::reset_draw_str_attr();
  StdGuiRender::flush_data();
}

void set_enable_debug_draw(bool enable) { g_enable_debug_draw = enable; }
bool get_enable_debug_draw() { return g_enable_debug_draw; }

void set_draw_audibility(bool enable) { g_draw_audibility = enable; }

void debug_enum_events()
{
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

static inline void debug_trace_impl(TraceLevel cur_level, const char *format, va_list args)
{
  G_ASSERT_RETURN(format, );
  SNDSYS_DEBUG_TRACE_BLOCK;

  decltype(DebugMessage::text) text;
  text.sprintf_va_list(format, args);
  text.rtrim(" \n");

  if (int(cur_level) >= int(g_log_level))
  {
    if (cur_level == TraceLevel::err)
    {
      logerr("[SNDSYS] %s", text.begin());
    }
    else if (cur_level == TraceLevel::warn)
    {
      logwarn("[SNDSYS] %s", text.begin());
    }
    else
    {
      debug("[SNDSYS] %s", text.begin());
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

#define DEBUG_TRACE_IMPL(TYPE)                        \
  void debug_trace_##TYPE(const char *format, ...)    \
  {                                                   \
    va_list args;                                     \
    va_start(args, format);                           \
    debug_trace_impl(TraceLevel::TYPE, format, args); \
    va_end(args);                                     \
  }

DEBUG_TRACE_IMPL(info)
DEBUG_TRACE_IMPL(warn)
DEBUG_TRACE_IMPL(err)
DEBUG_TRACE_IMPL(log)

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
}
} // namespace sndsys
