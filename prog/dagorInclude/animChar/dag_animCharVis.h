//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <animChar/dag_animCharacter2.h>
#include <math/dag_frustum.h>
#include <scene/dag_occlusion.h>

inline bool AnimV20::AnimcharRendComponent::check_visibility(uint16_t vis_bits, bbox3f_cref bbox, bool is_main, const Frustum &f,
  Occlusion *occl)
{
  if (!(vis_bits & VISFLG_IN_RANGE))
    return false;
  if (is_main)
    return (vis_bits & VISFLG_MAIN) != 0;
  if (!(vis_bits & VISFLG_SHADOW))
    return false;

  if (occl)
    return occl->isVisibleBox(bbox.bmin, bbox.bmax);
  return f.testBoxB(bbox.bmin, bbox.bmax);
}
