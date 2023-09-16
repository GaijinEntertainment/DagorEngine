#include <EASTL/fixed_vector.h>
#include <osApiWrappers/dag_critSec.h>
#include <ioSys/dag_dataBlock.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/handle.h>
#include <soundSystem/delayed.h>

#include "internal/fmodCompatibility.h"
#include "internal/delayed.h"
#include "internal/debug.h"

static WinCritSec g_delayed_cs;
#define SNDSYS_DELAYED_BLOCK WinAutoLock delayedLock(g_delayed_cs);

namespace sndsys
{
namespace delayed
{
static constexpr float g_sound_air_speed = 335.f;
static float g_dist_threshold = 0.f;
static float g_cur_time = 0.f;
static bool g_enable_distant_delay = false;

static size_t g_debug_num_events = 0;
static size_t g_debug_num_actions = 0;
static size_t g_debug_max_events = 0;
static size_t g_debug_max_actions = 0;

struct Event
{
  enum class Type
  {
    start,
    stop,
    stop_fadeout,
    stop_fadeout_keyoff,
    release,
    abandon,
  };
  eastl::fixed_vector<eastl::pair<Type, float /*timeCreated*/>, 8> actions;
  EventHandle handle;
  Event(EventHandle handle) : handle(handle) {}
};

static eastl::fixed_vector<Event, 32> g_events;

static Event *get(EventHandle handle)
{
  auto pred = [&](const Event &it) { return it.handle == handle; };
  auto it = eastl::find_if(g_events.begin(), g_events.end(), pred);
  return it != g_events.end() ? it : nullptr;
}

bool is_enable_distant_delay() { return g_enable_distant_delay; }
bool is_far_enough_for_distant_delay(const Point3 &pos)
{
  return g_enable_distant_delay && lengthSq(pos - get_3d_listener_pos()) > sqr(g_dist_threshold);
}
bool is_delayed(EventHandle handle)
{
  SNDSYS_DELAYED_BLOCK;
  return get(handle) != nullptr;
}

static Event &append(EventHandle handle)
{
  Event *e = get(handle);
  return e ? *e : g_events.emplace_back(handle);
}

static void append(EventHandle handle, Event::Type type, float add_delay)
{
  Event &de = append(handle);
  const float time = g_cur_time + add_delay;
  auto pred = [&](const auto &it) { return it.first == type && fabs(it.second - time) < 0.05f; };
  if (de.actions.empty() || !pred(de.actions.back()))
    de.actions.emplace_back(type, time);
}

void start(EventHandle handle, float add_delay)
{
  SNDSYS_DELAYED_BLOCK;
  if (is_valid_handle(handle))
    append(handle, Event::Type::start, add_delay);
}

void stop(EventHandle handle, bool allow_fadeout, bool try_keyoff, float add_delay)
{
  SNDSYS_DELAYED_BLOCK;
  if (is_valid_handle(handle))
    append(handle,
      allow_fadeout && try_keyoff ? Event::Type::stop_fadeout_keyoff
      : allow_fadeout             ? Event::Type::stop_fadeout
                                  : Event::Type::stop,
      add_delay);
}

bool is_starting(EventHandle handle)
{
  SNDSYS_DELAYED_BLOCK;
  auto it = eastl::find_if(g_events.begin(), g_events.end(), [handle](const Event &it) { return it.handle == handle; });
  return it != g_events.end() && !it->actions.empty() && it->actions.front().first == Event::Type::start;
}

void release(EventHandle handle, float add_delay)
{
  SNDSYS_DELAYED_BLOCK;
  if (is_valid_handle(handle))
    append(handle, Event::Type::release, add_delay);
}

void abandon(EventHandle handle, float add_delay)
{
  SNDSYS_DELAYED_BLOCK;
  if (is_valid_handle(handle))
    append(handle, Event::Type::abandon, add_delay);
}

void init(const DataBlock &blk)
{
  g_enable_distant_delay = blk.getBool("enableDistantDelay", false);
  g_dist_threshold = blk.getReal("distantDelayThreshold", 50.f);
}

void close()
{
  SNDSYS_DELAYED_BLOCK;
  for (Event &it : g_events)
    release(it.handle);
  g_events.clear();
}

void get_debug_info(size_t &events, size_t &actions, size_t &max_events, size_t &max_actions)
{
  events = g_debug_num_events;
  actions = g_debug_num_actions;
  max_events = g_debug_max_events;
  max_actions = g_debug_max_actions;
}

void update(float dt)
{
  SNDSYS_DELAYED_BLOCK;
  g_debug_num_events = g_events.size();
  g_debug_num_actions = 0;
  for (Event &e : g_events)
  {
    g_debug_num_actions += g_events.size();
    if (!is_valid_handle(e.handle))
    {
      e.handle.reset();
      continue;
    }
    float distanceSq = -1.f;
    if (g_enable_distant_delay)
    {
      Point3 pos;
      if (get_3d_attr(e.handle, pos))
        distanceSq = lengthSq(pos - get_3d_listener_pos());
    }
    size_t numActions = 0;
    for (auto &action : e.actions)
    {
      if (g_cur_time < action.second)
        break; // (g_cur_time + add_delay)
      if (distanceSq >= 0.f)
      {
        const float soundRadius = (g_cur_time - action.second) * g_sound_air_speed;
        if (sqr(soundRadius) < distanceSq)
          break;
      }
      if (action.first == Event::Type::start)
        sndsys::start_immediate(e.handle);
      else if (action.first == Event::Type::stop_fadeout_keyoff)
        sndsys::stop_immediate(e.handle, true, true);
      else if (action.first == Event::Type::stop_fadeout)
        sndsys::stop_immediate(e.handle, true, false);
      else if (action.first == Event::Type::stop)
        sndsys::stop_immediate(e.handle, false, false);
      else if (action.first == Event::Type::abandon)
        sndsys::abandon_immediate(e.handle);
      else if (action.first == Event::Type::release)
        sndsys::release_immediate(e.handle);
      ++numActions;
    }
    e.actions.erase(e.actions.begin(), e.actions.begin() + numActions);
  }
  g_debug_max_events = max(g_debug_max_events, g_debug_num_events);
  g_debug_max_actions = max(g_debug_max_actions, g_debug_num_actions);
  g_cur_time += dt;
  auto pred = [](const Event &it) { return !it.handle || it.actions.empty(); };
  auto it = eastl::remove_if(g_events.begin(), g_events.end(), pred);
  g_events.reserve(g_events.size());
  g_events.erase(it, g_events.end());
}

void enable_distant_delay(bool enable) { g_enable_distant_delay = enable; }

void release_delayed_events()
{
  for (Event &e : g_events)
    release_immediate(e.handle);
  g_events.clear();
}
} // namespace delayed
} // namespace sndsys
