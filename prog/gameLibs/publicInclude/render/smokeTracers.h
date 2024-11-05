//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_tab.h>
#include <util/dag_fixedBitArray.h>
#include <math/dag_Point3.h>
#include <math/dag_e3dColor.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <3d/dag_resPtr.h>

#include <math/dag_hlsl_floatx.h>
#include <render/smokeTracers.hlsli>

struct Frustum;
class ComputeShaderElement;
class DataBlock;

class SmokeTracerManager
{
public:
  SmokeTracerManager();
  ~SmokeTracerManager();

  void update(float dt)
  {
    if (!usedTracers[currentUsed].empty())
      updateAlive();
    else
      usedTracers[1 - currentUsed].clear();
    cTime += dt;
    lastDt = dt;
  }
  void beforeRender(const Frustum &, float exposure_time, float pixel_scale); // exposure_time is either "frame" time (video, i.e.
                                                                              // fixed frame for video or real fps) or adjusted value
  void renderTrans();
  int createTracer(const Point3 &start_pos, const Point3 &normalized_dir, float smoke_radius, const Color4 &smoke_color,
    const Color4 &head_color, float time_to_live, const Color3 &start_head_color, float start_time);
  int updateTracerPos(unsigned id, const Point3 &pos); // we guide tracer, until it is off
  void leaveTracer(unsigned id);                       // we don't auto destroy tracers, which are referenced by someone
  void init(const DataBlock &settings) { initGPU(settings); }
  void afterDeviceReset();

protected:
  void initGPU(const DataBlock &settings);
  void closeGPU();
  void initFillIndirectBuffer();
  Tab<TracerCreateCommand> createCommands;
  Tab<TracerUpdateCommand> updateCommands;
  void updateAlive();
  void performGPUCommands();

  class SmokeTracer
  {
  public:
    Point3 startPos;
    Point3 lastPos;
    float vaporTime; // amount for smoke to totally disappear
#if DAGOR_DBGLEVEL > 0
    int framesWithoutUpdate;
#endif
  };
  BufPtr tracerVertsBuffer, tracerBuffer, tracerBufferDynamic;
  BufPtr culledTracerHeads, culledTracerTails;
  BufPtr drawIndirectBuffer; // can be made multidarw indirect, saving some vertex shader time
  eastl::unique_ptr<ComputeShaderElement> updateCommands_cs, createCommands_cs, cullTracers_cs, clearIndirectBuffers;

  carray<SmokeTracer, MAX_TRACERS> tracers;
  FixedBitArray<MAX_TRACERS> aliveTracers;
  // carray<bbox3f, MAX_TRACERS> tracersBox;
  StaticTab<int16_t, MAX_TRACERS> freeTracers;
  carray<StaticTab<int16_t, MAX_TRACERS>, 2> usedTracers;
  int currentUsed;
  DynamicShaderHelper headRenderer, tailRenderer;
  float cTime, lastDt;
  SharedTexHolder tailTex;
#if DAGOR_DBGLEVEL > 0
  unsigned int lastGpuUpdateFrame = 0;
#endif
};
