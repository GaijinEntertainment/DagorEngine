// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <gamePhys/props/atmosphere.h>
#include "render/skies.h"
#include "render/screencap.h"

template <typename Callable>
static void query_camera_pos_ecs_query(Callable c);

ECS_TAG(render)
static void timelapse_screener_es(const ecs::UpdateStageInfoAct &info,
  float &timelapse_screener__curTime,
  float &timelapse_screener__curWaitTimer,
  int &timelapse_screener__sequenceNum,
  float timelapse_screener__toTime,
  float timelapse_screener__timeStep,
  float timelapse_screener__waitTime,
  Point3 timelapse_screener__deltaPos = Point3(0.f, 0.f, 0.f))
{
  if (timelapse_screener__curTime > timelapse_screener__toTime)
    return;

  if (timelapse_screener__curWaitTimer <= timelapse_screener__waitTime * 0.5f &&
      timelapse_screener__curWaitTimer + info.dt > timelapse_screener__waitTime * 0.5f)
  {
    timelapse_screener__curTime = timelapse_screener__curTime + timelapse_screener__timeStep;
    set_daskies_time(timelapse_screener__curTime);
    Point2 amount = Point2::xz(gamephys::atmosphere::get_wind()) * (timelapse_screener__timeStep * 3600.f);
    move_skies(amount);
  }

  timelapse_screener__curWaitTimer = timelapse_screener__curWaitTimer + info.dt;
  if (timelapse_screener__curWaitTimer > timelapse_screener__waitTime)
  {
    query_camera_pos_ecs_query([&](TMatrix &transform ECS_REQUIRE(eastl::true_type camera__active)) {
      transform.setcol(3, transform * timelapse_screener__deltaPos);
    });
    timelapse_screener__curWaitTimer = 0.f;
    screencap::schedule_screenshot(false, timelapse_screener__sequenceNum);
    timelapse_screener__sequenceNum = timelapse_screener__sequenceNum + 1;
  }
}
