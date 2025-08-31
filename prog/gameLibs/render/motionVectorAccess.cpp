// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/motionVectorAccess.h>

#include <shaders/dag_shaders.h>

#include <EASTL/utility.h>

#define VARS_LIST                  \
  VAR(jitter_offset_uv)            \
  VAR(uvz_to_prev_frame_uvz)       \
  VAR(prev_frame_uvz_to_uvz)       \
  VAR(uvz_to_prev_frame_hero_bbox) \
  VAR(uvz_to_prev_frame_hero_uvz)  \
  VAR(zn_zfar_current_prev)        \
  VAR(motion_vector_type)          \
  VAR(can_ignore_bbox)

#define VAR(a) ShaderVariableInfo a##VarId(#a, true);
VARS_LIST
#undef VAR

namespace motion_vector_access
{

static void set_hero_matrix_params(const HeroMatrixParams &params, const TMatrix4 &uvzToWorldRel, const TMatrix4 &prevWorldToUvzRel)
{
  can_ignore_bboxVarId.set_int(params.canIgnoreBBox);
  uvz_to_prev_frame_hero_bboxVarId.set_float4x4(uvzToWorldRel * params.worldToBBox);
  uvz_to_prev_frame_hero_uvzVarId.set_float4x4(uvzToWorldRel * params.worldToPrevWorld * prevWorldToUvzRel);
}

static void set_identity_hero_matrix_params()
{
  static const TMatrix4 disable_hero_bbox(0.0f, 0.0f, 0.0f, 2.0f, //
    0.0f, 0.0f, 0.0f, 2.0f,                                       //
    0.0f, 0.0f, 0.0f, 2.0f,                                       //
    0.0f, 0.0f, 0.0f, 2.0f);

  can_ignore_bboxVarId.set_int(1);
  uvz_to_prev_frame_hero_bboxVarId.set_float4x4(disable_hero_bbox);
  uvz_to_prev_frame_hero_uvzVarId.set_float4x4(TMatrix4::IDENT);
}

static void set_reprojection_params(const CameraParams &currentCamera, const CameraParams &previousCamera,
  const eastl::optional<HeroMatrixParams> &heroMatrixParams)
{
  // Doing calculations around view.viewPos increase precision robustness.
  Point3 posDiff = currentCamera.position - previousCamera.position;
  TMatrix viewItm_rel = currentCamera.viewInverse;
  viewItm_rel.setcol(3, Point3(0, 0, 0));
  TMatrix prevViewTm_rel = previousCamera.view;
  prevViewTm_rel.setcol(3, prevViewTm_rel % posDiff);

  float det;
  TMatrix4 invProjTmNoJitter;
  if (!inverse44(currentCamera.projectionNoJitter, invProjTmNoJitter, det))
    invProjTmNoJitter = TMatrix4::IDENT;

  static const TMatrix4 uvz_to_clip(2.0f, 0.0f, 0.0f, 0.0f, //
    0.0f, -2.0f, 0.0f, 0.0f,                                //
    0.0f, 0.0f, 1.0f, 0.0f,                                 //
    -1.0f, 1.0f, 0.0f, 1.0f);

  static const TMatrix4 clip_to_uvz(0.5f, 0.0f, 0.0f, 0.0f, //
    0.0f, -0.5f, 0.0f, 0.0f,                                //
    0.0f, 0.0f, 1.0f, 0.0f,                                 //
    0.5f, 0.5f, 0.0f, 1.0f);


  TMatrix4 uvzToWorldRel = uvz_to_clip * invProjTmNoJitter * viewItm_rel;
  TMatrix4 prevWorldToUvzRel = TMatrix4(prevViewTm_rel) * previousCamera.projectionNoJitter * clip_to_uvz;
  TMatrix4 uvzToPrevFrameUvz = uvzToWorldRel * prevWorldToUvzRel;

  uvz_to_prev_frame_uvzVarId.set_float4x4(uvzToPrevFrameUvz);
  zn_zfar_current_prevVarId.set_color4(currentCamera.znear, currentCamera.zfar, previousCamera.znear, previousCamera.zfar);

  if (heroMatrixParams)
    set_hero_matrix_params(*heroMatrixParams, uvzToWorldRel, prevWorldToUvzRel);
  else
    set_identity_hero_matrix_params();
}

static void set_jitter_params(const Point2 &currentJitter, const Point2 &previousJitter)
{
  jitter_offset_uvVarId.set_color4(currentJitter.x, currentJitter.y, previousJitter.x, previousJitter.y);
}

void set_reprojection_params_prev_to_curr(const CameraParams &currentCamera, const CameraParams &previousCamera)
{
  set_reprojection_params(currentCamera, previousCamera, eastl::nullopt);
  TMatrix4 tm = uvz_to_prev_frame_uvzVarId.get_float4x4();
  TMatrix4 itm = inverse44(tm);
  prev_frame_uvz_to_uvzVarId.set_float4x4(itm);
}

void set_params(const CameraParams &currentCamera, const CameraParams &previousCamera, const Point2 &currentJitter,
  const Point2 &previousJitter, const eastl::optional<HeroMatrixParams> &heroMatrixParams)
{
  set_jitter_params(currentJitter, previousJitter);
  set_reprojection_params(currentCamera, previousCamera, heroMatrixParams);
}

void set_motion_vector_type(MotionVectorType type) { motion_vector_typeVarId.set_int(static_cast<int>(type)); }

} // namespace motion_vector_access