//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace ecs
{
// DO NOT ADD ANY MORE STAGES!
// shouldDecreaseSize
// allowedSizeIncrease = 200
//  Actually, we better remove UpdateStage concept altogether.
//  Right now it's only differences to broadcast immediate are:
//  * requires ordering within stage (and is not required for event).
//  * there is one hashmap lookup for event (that's a small cost)
//  We can make ordering part of event declaration/registration in the future.
//   We can also make hashmap lookup not required by updating static variable inside a declaration (if present),
//     that would optimize almost all events send from c++ code
//  Stages DO have cost (memory and cognitive)!
//  Which is more, they add cohesion, and add implicit requirements for all games
//  by adding stage, let's say 'VR-view' we implicitly assumes all code using ECS is supporting VR, with a certain stage payload,
//  even if particular doesn't need it at all!
#define ECS_UPDATE_STAGES_LIST                                                                                                     \
  /* DO NOT ADD ANY MORE STAGES!*/                                                                                                 \
  S(US_ACT) /*called for game logic, physics, etc. It can be called several times per frame, with timeSpeed dt, zero dt, or can be \
               not called at all (on pause).*/                                                                                     \
  S(US_BEFORE_RENDER) /*called once per frame. Have two dt (game and real). Supposed to be called on client only (and only when    \
                         render is called at all).*/                                                                               \
  S(US_RENDER)        /*called several times frame. Have a lot of hints about what are we rendering. TO BE REMOVED*/               \
  S(US_RENDER_TRANS)  /*called several times frame. Have a lot of hints about what are we rendering. TO BE REMOVED*/               \
  S(US_RENDER_DEBUG)  /*can be called several times frame, but usually once.*/                                                     \
  S(US_USER)          /* start your own update stage enumeration from here*/

enum UpdateStage
{
#define S(x) x,
  ECS_UPDATE_STAGES_LIST
#undef S
    US_COUNT
};

extern const char *const es_stage_names[US_COUNT];

// TODO: rewrite this to template specialization to avoid match enumeration and coresponding structs manually
struct UpdateStageInfo
{
  const int stage;
  UpdateStageInfo(int stage_) : stage(stage_) {}
  template <typename T>
  const T *cast() const
  {
    return T::STAGE == stage ? static_cast<const T *>(this) : nullptr;
  }
};

struct UpdateStageInfoAct : public UpdateStageInfo
{
  static constexpr int STAGE = US_ACT;
  float dt = 0.f;
  float curTime = 0.f;
  UpdateStageInfoAct(float dt_, float cur_time) : UpdateStageInfo(STAGE), dt(dt_), curTime(cur_time) {}
};

struct UpdateStageInfoBeforeRender : public UpdateStageInfo
{
  static constexpr int STAGE = US_BEFORE_RENDER;
  float dt = 0, actDt = 0, realDt = 0; // dt is gameDt (scaled by time speed), actDt is dagor_game_act_time, realDt is real time passed
                                       // since last frame
  UpdateStageInfoBeforeRender(float dt_, float actDt_, float realDt_) : UpdateStageInfo(STAGE), dt(dt_), actDt(actDt_), realDt(realDt_)
  {}
};
struct UpdateStageInfoRender : public UpdateStageInfo
{
  static constexpr int STAGE = US_RENDER;
  UpdateStageInfoRender() : UpdateStageInfo(STAGE) {}
};
struct UpdateStageInfoRenderTrans : public UpdateStageInfo
{
  static constexpr int STAGE = US_RENDER_TRANS;
  UpdateStageInfoRenderTrans() : UpdateStageInfo(STAGE) {}
};

struct UpdateStageInfoRenderDebug : public UpdateStageInfo
{
  static constexpr int STAGE = US_RENDER_DEBUG;
  UpdateStageInfoRenderDebug() : UpdateStageInfo(STAGE) {}
};

} // namespace ecs
