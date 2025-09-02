// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix4.h>


struct CameraParams;
struct ReprojectionTransforms
{
  TMatrix4_vec4 jitteredCamPosToUnjitteredHistoryClip;
  TMatrix4_vec4 prevOrigoRelativeViewProjTm;
  TMatrix4 motionVecReprojectionTm;
};

ReprojectionTransforms calc_reprojection_transforms(const CameraParams &prev_frame_camera, const CameraParams &current_frame_camera);
