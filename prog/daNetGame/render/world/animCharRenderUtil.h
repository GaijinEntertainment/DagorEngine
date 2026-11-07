// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/render/updateStageRender.h>
#include <math/dag_frustum.h>
#include <util/dag_stdint.h>
#include <vecmath/dag_vecMathDecl.h>

namespace AnimV20
{
class AnimcharRendComponent;
}

void preprocess_visible_animchars_in_frustum(
  // Before render stage is needed, because it holds the correct rounded camera positions (those are NOT the same as the eye pos)
  const UpdateStageInfoBeforeRender &stg,
  const Frustum &frustum,
  const vec3f &eye_pos,
  AnimcharVisbits custom_flag_to_add,
  float lod_dist_mul);

void animchar_render_single_opaque(ecs::EntityId eid, const UpdateStageInfoRender &stg);
// The given animchar will be rendered to reactive mask in the current frame
void mark_animchar_for_reactive_mask_pass(ecs::EntityId eid);

// Culls one animchar for a pass (vis bits, frustum, occlusion), marks add_vis_bits on a hit and adds it to the dynrend context.
// The caller draws the context later with the view it was filled for.
void add_animchar_if_visible(dynrend::ContextId ctx,
  const Frustum &frustum,
  const Occlusion *occlusion,
  animchar_visbits_t &animchar_visbits,
  animchar_visbits_t require_any_vis_bits,
  animchar_visbits_t reject_if_any_vis_bits,
  animchar_visbits_t add_vis_bits,
  const vec4f &animchar_bsph,
  const bbox3f &animchar_bbox,
  const bbox3f *animchar_attaches_bbox,
  const AnimV20::AnimcharRendComponent &animchar_render,
  const ecs::Point4List *additional_data,
  const ecs::UInt8List *animchar_render__nodeVisibleStgFilters,
  const ecs::Tag *invisibleUpdatableAnimchar,
  bool animchar__renderPriority,
  dynrend::NeedPreviousMatrices needPreviousMatrices,
  uint8_t render_mask,
  TexStreamingContext tex_ctx);
