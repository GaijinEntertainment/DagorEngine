// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_Point3.h>
#include <math/dag_frustum.h>
#include <vecmath/dag_vecMathDecl.h>
#include <drv/3d/dag_decl.h>


struct CameraParams
{
  TMatrix4 viewRotJitterProjTm = TMatrix4::IDENT;
  TMatrix4 viewRotTm = TMatrix4::IDENT;

  TMatrix viewItm = TMatrix::IDENT;
  TMatrix viewTm = TMatrix::IDENT;

  DPoint3 cameraWorldPos = DPoint3();
  vec4f negRoundedCamPos = v_make_vec4f(0, 0, 0, 0);
  vec4f negRemainderCamPos = v_make_vec4f(0, 0, 0, 0);
  Driver3dPerspective noJitterPersp = Driver3dPerspective();
  Driver3dPerspective jitterPersp = Driver3dPerspective();

  TMatrix4_vec4 noJitterProjTm = TMatrix4::IDENT;
  TMatrix4_vec4 noJitterGlobtm = TMatrix4::IDENT;
  TMatrix4_vec4 jitterProjTm = TMatrix4::IDENT;
  TMatrix4_vec4 jitterGlobtm = TMatrix4::IDENT;

  Point2 jitterOffsetUv;

  Frustum noJitterFrustum;
  Frustum jitterFrustum;
  float znear = 0.01, zfar = 1000;
  uint16_t subSampleIndex = 0, subSamples = 1;
  uint16_t superSampleIndex = 0, superSamples = 1;
};

// This is supposed to be used for culling, and possibly other usages that
// work better with a single frustum, even when there are technically many,
// e.g. in VR or when jittering the camera and rendering stuff multiple times.
struct CameraParamsUnified
{
  DPoint3 cameraWorldPos = DPoint3();
  Frustum unionFrustum;
};