// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "reprojectionTm.h"

#include <render/world/cameraParams.h>


static TMatrix4 calc_motion_vec_reprojection_tm(const CameraParams &current, const CameraParams &prev)
{
  if (prev.viewRotJitterProjTm == TMatrix4::IDENT)
    return TMatrix4::IDENT;

  // Doing calculations around view.viewPos increase precision robustness.
  Point3 posDiff = current.cameraWorldPos - prev.cameraWorldPos;
  TMatrix viewItm_rel = current.viewItm;
  viewItm_rel.setcol(3, Point3(0, 0, 0));
  TMatrix prevViewTm_rel = prev.viewTm;
  prevViewTm_rel.setcol(3, prevViewTm_rel % posDiff);

  float det;
  TMatrix4 invProjTmNoJitter;
  if (!inverse44(current.noJitterProjTm, invProjTmNoJitter, det))
    invProjTmNoJitter = TMatrix4::IDENT;

  TMatrix4 uvToNdc = TMatrix4(2.0f, 0.0f, 0.0f, 0.0f, 0.0f, -2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f);
  TMatrix4 ndcToUv = TMatrix4(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f);

  return uvToNdc * invProjTmNoJitter * viewItm_rel * prevViewTm_rel * prev.noJitterProjTm * ndcToUv;
}

ReprojectionTransforms calc_reprojection_transforms(const CameraParams &prev_frame_camera, const CameraParams &current_frame_camera)
{
  const DPoint3 move = current_frame_camera.cameraWorldPos - prev_frame_camera.cameraWorldPos;
  const Point2 jitterOffset = current_frame_camera.jitterOffsetUv;

  TMatrix prevViewTm = prev_frame_camera.viewTm;
  prevViewTm.setcol(3, 0, 0, 0);
  TMatrix4_vec4 prevProjTm = prev_frame_camera.noJitterProjTm;
  prevProjTm.setrow(2, prevProjTm.getrow(2) + Point4(jitterOffset.x, jitterOffset.y, 0, 0));
  TMatrix4_vec4 prevViewProjTM = TMatrix4(prevViewTm) * prevProjTm;
  prevViewProjTM = prevViewProjTM.transpose();

  double rprWorldPos[4] = {(double)prevViewProjTM[0][0] * move.x + (double)prevViewProjTM[0][1] * move.y +
                             (double)prevViewProjTM[0][2] * move.z + (double)prevViewProjTM[0][3],
    prevViewProjTM[1][0] * move.x + (double)prevViewProjTM[1][1] * move.y + (double)prevViewProjTM[1][2] * move.z +
      (double)prevViewProjTM[1][3],
    prevViewProjTM[2][0] * move.x + (double)prevViewProjTM[2][1] * move.y + (double)prevViewProjTM[2][2] * move.z +
      (double)prevViewProjTM[2][3],
    prevViewProjTM[3][0] * move.x + (double)prevViewProjTM[3][1] * move.y + (double)prevViewProjTM[3][2] * move.z +
      (double)prevViewProjTM[3][3]};
  TMatrix4_vec4 jitteredCamPosToUnjitteredHistoryClip = prevViewProjTM;
  jitteredCamPosToUnjitteredHistoryClip.setcol(3, Point4(rprWorldPos[0], rprWorldPos[1], rprWorldPos[2], rprWorldPos[3]));

  TMatrix4_vec4 prevOrigoRelativeViewProjTm = TMatrix4(prev_frame_camera.viewTm) * prevProjTm;
  prevOrigoRelativeViewProjTm = prevOrigoRelativeViewProjTm.transpose();

  ReprojectionTransforms res;
  res.jitteredCamPosToUnjitteredHistoryClip = jitteredCamPosToUnjitteredHistoryClip;
  res.prevOrigoRelativeViewProjTm = prevOrigoRelativeViewProjTm;
  res.motionVecReprojectionTm = calc_motion_vec_reprojection_tm(current_frame_camera, prev_frame_camera);

  return res;
}
