// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/updateStage.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <stddef.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathUtils.h>
#include <gamePhys/collision/collisionLib.h>
#include <math/random/dag_random.h>
#include <math/dag_mathAng.h>
#include <scene/dag_physMat.h>
#include "main/level.h"
#include "camera/sceneCam.h"

namespace shootercam
{
struct CameraSetting
{
  float atPos; // linear controller position

  Point3 offset;
  Point3 pivotPos;
  float vertOffset;
  float velTau;
  float velFactor;
  float tau;
  float tauPos;
  float tmToAnimRatio;
  float animTau;

  void loadFromBlk(const DataBlock &blk)
  {
    atPos = blk.getReal("atPos", 0.f);
    offset = blk.getPoint3("offset", ZERO<Point3>());
    pivotPos = blk.getPoint3("pivotPos", ZERO<Point3>());
    velTau = blk.getReal("velTau", 0.f);
    velFactor = blk.getReal("velFactor", 1.f);
    tau = blk.getReal("tau", 0.f);
    vertOffset = DegToRad(blk.getReal("vertOffset", 0.f));
    tauPos = blk.getReal("tauPos", 0.f);
    tmToAnimRatio = blk.getReal("tmToAnimRatio", 0.f);
    animTau = blk.getReal("animTau", 0.f);
  }
};

struct CameraController //-V730
{
  int numSettings = 0;
  float inputVal;
  float inputSpd;
  union
  {
    CameraSetting *heapSettings;
    CameraSetting inlineSettings[2];
  };

  CameraController() = default;
  CameraController(const CameraController &) = delete;
  CameraController &operator=(const CameraController &) = delete;
  ~CameraController()
  {
    if (DAGOR_UNLIKELY(numSettings > countof(inlineSettings)))
      delete[] heapSettings;
  }

  dag::Span<CameraSetting> getSettings()
  {
    if (DAGOR_LIKELY(numSettings <= countof(inlineSettings)))
      return dag::Span<CameraSetting>(&inlineSettings[0], numSettings);
    else
      return dag::Span<CameraSetting>(heapSettings, numSettings);
  }

  void init(ecs::EntityId eid, const DataBlock &blk)
  {
    inputVal = g_entity_mgr->getOr(eid, ECS_HASH("zoom"), 0.0f);
    inputSpd = blk.getReal("inputSpd", 2.f);
    numSettings = blk.blockCount();
    if (DAGOR_UNLIKELY(numSettings > countof(inlineSettings)))
      heapSettings = new CameraSetting[numSettings];
    int i = 0;
    for (auto &sett : getSettings())
      sett.loadFromBlk(*blk.getBlock(i++));
  }

  bool solve(Point3 &offs,
    Point3 &pivot_pos,
    float &vel_tau,
    float &vel_fact,
    float &tau,
    float &vert_offset,
    float &tau_pos,
    float &tm_to_anim_ratio,
    float &anim_tau)
  {
    auto settings = getSettings();
    if (settings.empty())
      return false;
    if (inputVal <= settings[0].atPos)
    {
      offs = settings[0].offset;
      pivot_pos = settings[0].pivotPos;
      vel_tau = settings[0].velTau;
      vel_fact = settings[0].velFactor;
      tau = settings[0].tau;
      vert_offset = settings[0].vertOffset;
      tau_pos = settings[0].tauPos;
      tm_to_anim_ratio = settings[0].tmToAnimRatio;
      anim_tau = settings[0].animTau;
      return true;
    }
    // simple linear solver
    for (int i = 0; i < (int)settings.size() - 1; ++i)
    {
      if (inputVal <= settings[i].atPos)
        continue;
      float k = (inputVal - settings[i].atPos) / (settings[i + 1].atPos - settings[i].atPos);
      offs = lerp(settings[i].offset, settings[i + 1].offset, k);
      pivot_pos = lerp(settings[i].pivotPos, settings[i + 1].pivotPos, k);
      vel_tau = lerp(settings[i].velTau, settings[i + 1].velTau, k);
      vel_fact = lerp(settings[i].velFactor, settings[i + 1].velFactor, k);
      tau = lerp(settings[i].tau, settings[i + 1].tau, k);
      vert_offset = lerp(settings[i].vertOffset, settings[i + 1].vertOffset, k);
      tau_pos = lerp(settings[i].tauPos, settings[i + 1].tauPos, k);
      tm_to_anim_ratio = lerp(settings[i].tmToAnimRatio, settings[i + 1].tmToAnimRatio, k);
      anim_tau = lerp(settings[i].animTau, settings[i + 1].animTau, k);
      return true;
    }
    offs = settings.back().offset;
    pivot_pos = settings.back().pivotPos;
    vel_tau = settings.back().velTau;
    vel_fact = settings.back().velFactor;
    tau = settings.back().tau;
    vert_offset = settings.back().vertOffset;
    tau_pos = settings.back().tauPos;
    tm_to_anim_ratio = settings.back().tmToAnimRatio;
    anim_tau = settings.back().animTau;
    return true;
  }

  void act(float, float zoom)
  {
    inputVal = zoom; // move_to(inputVal, input.get(), dt, inputSpd);
  }
};

struct Camera
{
  CameraController controller;
  CameraController alternativeController;

  void init(const DataBlock &blk, const DataBlock *alt_blk, ecs::EntityId eid)
  {
    controller.init(eid, blk);
    alternativeController.init(eid, alt_blk ? *alt_blk : blk);
  }
};
} // namespace shootercam

using namespace shootercam;

ECS_DECLARE_RELOCATABLE_TYPE(Camera);
ECS_REGISTER_RELOCATABLE_TYPE(Camera, nullptr);
ECS_AUTO_REGISTER_COMPONENT(Camera, "shooter_cam", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(DPoint3, "camera__accuratePos", nullptr, ecs::DataComponent::DONT_REPLICATE);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void shooter_cam_init_es_event_handler(const ecs::Event &,
  const ecs::EntityId eid,
  const ecs::string &shooter_cam__blk,
  const ecs::string *shooter_cam__alt_blk,
  const bool shooter_cam__alternative_settings,
  shootercam::Camera &shooter_cam,
  Point3 &shooter_cam__lastDir,
  bool &shooter_cam__wasAlternative,
  float &shooter_cam__punchTau,
  float &shooter_cam__punchFadeoutTau,
  Point2 &shooter_cam__punchXRange,
  Point2 &shooter_cam__punchYRange,
  float &shooter_cam__punchStrength,
  float &shooter_cam__tauOnChange,
  int &shooter_cam__rayMatId,
  const ecs::string *shooter_cam__rayMat,
  const float shooter_cam__punch_tau = 0.03f,
  const float shooter_cam__punch_fadeout_tau = 0.1f,
  const Point2 &shooter_cam__punch_x_range = Point2(-0.25f, 0.25f),
  const Point2 &shooter_cam__punch_y_range = Point2(-0.125f, 0.25f),
  const float shooter_cam__punch_strength = 3.f,
  const float shooter_cam__tau_on_change = 0.4f)
{
  {
    const char *blkNameAttr = shooter_cam__blk.c_str();
    const DataBlock blk(blkNameAttr);
    const DataBlock altBlk(shooter_cam__alt_blk ? shooter_cam__alt_blk->c_str() : blkNameAttr);
    shooter_cam.init(blk, &altBlk, eid);
  }

  shooter_cam__lastDir = get_cam_itm().getcol(2);

  shooter_cam__wasAlternative = shooter_cam__alternative_settings;
  shooter_cam__punchTau = shooter_cam__punch_tau;
  shooter_cam__punchFadeoutTau = shooter_cam__punch_fadeout_tau;
  shooter_cam__punchXRange = shooter_cam__punch_x_range;
  shooter_cam__punchYRange = shooter_cam__punch_y_range;
  shooter_cam__punchStrength = shooter_cam__punch_strength;
  shooter_cam__tauOnChange = shooter_cam__tau_on_change;

  shooter_cam__rayMatId = PhysMat::getMaterialId(shooter_cam__rayMat ? shooter_cam__rayMat->c_str() : "default");
}

ECS_TAG(render)
ECS_AFTER(before_camera_sync)
ECS_BEFORE(after_camera_sync)
static void shooter_cam_act_es(const ecs::UpdateStageInfoAct &info,
  shootercam::Camera &shooter_cam,
  Point3 &camera__offset,
  Point3 &camera__pivotPos,
  float &camera__velTau,
  float &camera__velFactor,
  float &camera__tau,
  float &camera__vertOffset,
  float &camera__tauPos,
  float &camera__tmToAnimRatio,
  float &camera__animTau,
  float zoom = 0.f,
  bool shooter_cam__alternative_settings = false)
{
  if (is_level_loading())
    return;

  shooter_cam.controller.act(info.dt, zoom);
  shooter_cam.alternativeController.act(info.dt, zoom);

  Point3 &offset = camera__offset;
  Point3 &pivotPos = camera__pivotPos;

  float &velTau = camera__velTau;
  float &velFactor = camera__velFactor;
  float &tau = camera__tau;
  float &vertOffset = camera__vertOffset;
  float &tauPos = camera__tauPos;
  float &tmToAnimRatio = camera__tmToAnimRatio;
  float &animTau = camera__animTau;

  if (!shooter_cam__alternative_settings)
    shooter_cam.controller.solve(offset, pivotPos, velTau, velFactor, tau, vertOffset, tauPos, tmToAnimRatio, animTau);
  else
    shooter_cam.alternativeController.solve(offset, pivotPos, velTau, velFactor, tau, vertOffset, tauPos, tmToAnimRatio, animTau);
}
