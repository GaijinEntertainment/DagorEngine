//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_drv3dConsts.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <3d/dag_drv3d.h>
#include <EASTL/vector.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint2.h>

class MotionBlur
{
public:
  enum class DepthType
  {
    RAW = 0,
    LINEAR = 1
  };
  struct Settings
  {
    float velocityMultiplier = 0.005f; // Used to be 0.1f, but that too much
    float maxVelocity = 0.1f;
    float alphaMulOnApply = 0.125f;
    float overscanMul = 0.01f;
    float forwardBlur = 0.f;
    float scale = 1.0f; // Was hard coded to 1 before.
  };
  static constexpr float SCALE_EPS = 0.01f;
  MotionBlur();

  static bool isSupported();

  void accumulateDepthVersion(ManagedTexView source_color_tex, ManagedTexView blur_depth_tex, DepthType depth_type,
    TMatrix4 currentGlobTm, TMatrix4 prevGlobTm, ManagedTexView accumulationTex);
  void accumulateMotionVectorVersion(ManagedTexView source_color_tex, ManagedTexView motion_vector_tex,
    ManagedTexView accumulationTex);
  void combine(ManagedTexView target_tex, ManagedTexView accumulationTex);

  void update(float dt, const Point3 camera_pos, float jump_dist = 20);
  void onCameraLeap();

  void useSettings(const Settings &settings);
  static constexpr uint32_t getAccumulationFormat() { return TEXFMT_A16B16G16R16F; }


protected:
  void accumulateInternal(ManagedTexView source_color_tex, ShaderElement *elem, ManagedTexView accumulationTex);

  Ptr<ShaderMaterial> motionBlurMaterial, motionBlurMvMaterial;
  Ptr<ShaderElement> motionBlurShElem, motionBlurMvShElem;

  float velocityMultiplier;
  float maxVelocity;
  float alphaMulOnApply;
  float overscanMul;

  PostFxRenderer applyMotionBlurRenderer;

  float scale = 1;
  float overscan = 0;
  // Depth version used to support more, but we'll only supply 1 with frame graph
  static constexpr unsigned int numHistoryFrames = 1;
  unsigned int numValidFrames = 0;
  unsigned int skipFrames = 10;
  Point3 prevCamPos;
};
