#include <osApiWrappers/dag_critSec.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_mathUtils.h>
#include <fmod_studio.hpp>
#include <soundSystem/soundSystem.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/streams.h>

#include "internal/fmodCompatibility.h"
#include "internal/banks.h"
#include "internal/delayed.h"
#include "internal/events.h"
#include "internal/streams.h"
#include "internal/debug.h"

static WinCritSec g_listener_cs;
#define SNDSYS_LISTENER_BLOCK WinAutoLock listenerLock(g_listener_cs);

namespace sndsys
{
static TMatrix g_listener_tm = TMatrix::IDENT;
static Point3 g_listener_vel = ZERO<Point3>();
static bool g_reset_listener_tm = true;
static float g_override_time_speed = 0.f;
static float g_current_pitch = 1.f;
static const Point4 g_speed_to_pitch = Point4(0.f, 4.f, 0.f, 4.f);

using namespace fmodapi;

Point3 get_3d_listener_pos()
{
  SNDSYS_LISTENER_BLOCK;
  return g_listener_tm.getcol(3);
}

TMatrix get_3d_listener()
{
  SNDSYS_LISTENER_BLOCK;
  return g_listener_tm;
}

Point3 get_3d_listener_vel()
{
  SNDSYS_LISTENER_BLOCK;
  return g_listener_vel;
}

bool should_play(const Point3 &pos, float dist_threshold)
{
  SNDSYS_LISTENER_BLOCK;
  return dist_threshold <= 0.f || lengthSq(pos - g_listener_tm.getcol(3)) < dist_threshold * dist_threshold;
}

void reset_3d_listener()
{
  SNDSYS_LISTENER_BLOCK;
  g_reset_listener_tm = true;
}

void override_time_speed(float time_speed) { g_override_time_speed = max(0.f, time_speed); }

void update_listener(float delta_time, const TMatrix &listener_tm)
{
  SNDSYS_IF_NOT_INITED_RETURN;
  SNDSYS_LISTENER_BLOCK;

  if (g_reset_listener_tm)
  {
    g_reset_listener_tm = false;
    g_listener_tm = listener_tm;
  }

  g_listener_vel = listener_tm.getcol(3) - g_listener_tm.getcol(3);
  g_listener_vel =
    Point3(safediv(g_listener_vel.x, delta_time), safediv(g_listener_vel.y, delta_time), safediv(g_listener_vel.z, delta_time));
  g_listener_tm = listener_tm;
  float listenerSpeedSq = g_listener_vel.lengthSq();
  constexpr float maxListenerSpeedSq = 250000.f;
  if (listenerSpeedSq > maxListenerSpeedSq && listenerSpeedSq > 0.f)
    g_listener_vel *= sqrt(maxListenerSpeedSq / listenerSpeedSq);

  Attributes3D listener3dAttributes(listener_tm, g_listener_vel);
  SOUND_VERIFY(get_studio_system()->setListenerAttributes(0, &listener3dAttributes));
}

static inline void apply_time_speed(float time_speed)
{
  const float speed = g_override_time_speed > 0.f ? g_override_time_speed : time_speed;
  const float pitch = cvt(speed, g_speed_to_pitch.x, g_speed_to_pitch.y, g_speed_to_pitch.z, g_speed_to_pitch.w);
  if (g_current_pitch != pitch)
  {
    if (fmodapi::set_pitch(fmodapi::get_bus_nullable(""), pitch))
      g_current_pitch = pitch;
  }
}

void update(float dt, float time_speed /* = 1.f*/)
{
  SNDSYS_IF_NOT_INITED_RETURN;

  TIME_PROFILE(sndsys_update);

  apply_time_speed(time_speed);

  events_update(dt);

  delayed::update(dt);

  SOUND_VERIFY(get_studio_system()->update());

  banks::update();

  streams::update(dt);
}

} // namespace sndsys
