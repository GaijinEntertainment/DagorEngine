// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

void preprocess_visible_animchars_in_frustum(
  // Before render stage is needed, because it holds the correct rounded camera positions (those are NOT the same as the eye pos)
  const UpdateStageInfoBeforeRender &stg,
  const Frustum &frustum,
  const vec3f &eye_pos,
  AnimV20::AnimcharRendComponent &animchar_render,
  const AnimcharNodesMat44 &animchar_node_wtm,
  const vec4f &animchar_bsph,
  uint8_t &animchar_visbits,
  bool animchar_render__enabled);
