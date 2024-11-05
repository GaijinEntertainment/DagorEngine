//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_texMgr.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <gui/dag_stdGuiRender.h>

namespace darg
{

namespace uishader
{

void set_mask(StdGuiRender::GuiContext &ctx, TEXTUREID texture_id, d3d::SamplerHandle sampler_id, const Point3 &matrix_line_0,
  const Point3 &matrix_line_1, const Point2 &tc0 = Point2(0.f, 0.f), const Point2 &tc1 = Point2(1.f, 1.f));
void set_mask(StdGuiRender::GuiContext &ctx, TEXTUREID texture_id, d3d::SamplerHandle sampler_id, const Point2 &center_pos,
  float angle, const Point2 &scale, const Point2 &tc0 = Point2(0.f, 0.f), const Point2 &tc1 = Point2(1.f, 1.f));


} // namespace uishader

} // namespace darg
