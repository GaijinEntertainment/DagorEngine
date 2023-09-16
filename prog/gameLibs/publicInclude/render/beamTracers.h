//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/*
This is a copy paste of smoke tracers for fools moon event with some modifications
TODO: do this properly
*/
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
#include <render/beamTracers.hlsli>

struct Frustum;
class ComputeShaderElement;
class DataBlock;

class BeamTracerManager
{
public:
  BeamTracerManager();
  ~BeamTracerManager();

  void update(float dt)
  {
    updateAlive();
    cTime += dt;
    lastDt = dt;
  }
  void beforeRender(const Frustum &, float exposure_time, float pixel_scale); // exposure_time is either "frame" time (video, i.e.
                                                                              // fixed frame for video or real fps) or adjusted value
  void renderTrans();
  int createTracer(const Point3 &start_pos, const Point3 &normalized_dir, float smoke_radius, const Color4 &smoke_color,
    const Color4 &head_color, float time_to_live, float fade_dist, float begin_fade_time, float end_fade_time, float scroll_speed,
    bool is_ray);
  int updateTracerPos(unsigned id, const Point3 &pos, const Point3 &start_pos); // we guide tracer, until it is off
  void leaveTracer(unsigned id);  // we don't auto destroy tracers, which are referenced by someone
  void removeTracer(unsigned id); // set tracer's ttl to 0 and leave it, so it'll be removed next frame
  void init(const DataBlock &settings) { initGPU(settings); }
  void afterDeviceReset();

protected:
  void initGPU(const DataBlock &settings);
  void closeGPU();
  void initFillIndirectBuffer();
  Tab<BeamTracerCreateCommand> createCommands;
  Tab<BeamTracerUpdateCommand> updateCommands;
  void updateAlive();
  void performGPUCommands();

  class BeamTracer
  {
  public:
    Point3 startPos;
    Point3 lastPos;
    float vaporTimeInterval; // amount for smoke to totally disappear
  };
  BufPtr tracerVertsBuffer, tracerBuffer, tracerBufferDynamic;
  BufPtr culledTracerHeads, culledTracerTails;
  BufPtr drawIndirectBuffer; // can be made multidarw indirect, saving some vertex shader time
  eastl::unique_ptr<ComputeShaderElement> updateCommands_cs, createCommands_cs, cullTracers_cs, clearIndirectBuffers;

  carray<BeamTracer, MAX_TRACERS> tracers;
  carray<float, MAX_TRACERS> tracersVaporTime;
#if DAGOR_DBGLEVEL > 0
  FixedBitArray<MAX_TRACERS> tracersLeft;
#endif
  // carray<bbox3f, MAX_TRACERS> tracersBox;
  StaticTab<int16_t, MAX_TRACERS> freeTracers;
  StaticTab<int16_t, MAX_TRACERS> usedTracers[2];
  int currentUsed;
  DynamicShaderHelper headRenderer, tailRenderer;
  float cTime, lastDt;
  SharedTexHolder tailTex;
  SharedTexHolder beamTex;
};
