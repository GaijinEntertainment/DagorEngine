//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>

#include <EASTL/optional.h>

namespace motion_vector_access
{

struct CameraParams
{
  TMatrix view;
  TMatrix viewInverse;
  TMatrix4 projectionNoJitter;
  Point3 position;
  float znear;
  float zfar;
};

struct HeroMatrixParams
{
  TMatrix worldToBBox;
  TMatrix worldToPrevWorld;
  bool canIgnoreBBox;
};

void set_reprojection_params_prev_to_curr(const CameraParams &currentCamera, const CameraParams &previousCamera);
void set_params(const CameraParams &currentCamera, const CameraParams &previousCamera, const Point2 &currentJitter,
  const Point2 &previousJitter, const eastl::optional<HeroMatrixParams> &heroMatrixParams);

// Chooses how motion vectors are calculated which has enourmous impact on both quality and performance
// In all cases UV+Z motion vectors are provided by the shader.
// Static: each sample is assumed to be static and reprojected from the previous frame
// Dynamic: each sample is read from the rendered motion vector texture and no reprojection is taking place
enum class MotionVectorType
{
  StaticUVZ,        // For no motion vectors
  DynamicUVStaticZ, // For 2D motion vectors, expensive due to Z needing reprojection
  DynamicUVZ        // For 3D motion vectors
};

void set_motion_vector_type(MotionVectorType type);

} // namespace motion_vector_access