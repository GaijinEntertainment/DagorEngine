// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ecs/anim/animchar_visbits.h>

void preprocess_visible_animchars_in_frustum(
  // Before render stage is needed, because it holds the correct rounded camera positions (those are NOT the same as the eye pos)
  const UpdateStageInfoBeforeRender &stg,
  const Frustum &frustum,
  const vec3f &eye_pos,
  AnimcharVisbits custom_flag_to_add);

void animchar_render_single_opaque(ecs::EntityId eid, const UpdateStageInfoRender &stg);
// The given animchar will be rendered to reactive mask in the current frame
void mark_animchar_for_reactive_mask_pass(ecs::EntityId eid);