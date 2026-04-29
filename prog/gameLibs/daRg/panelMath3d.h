// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_math3d.h>
#include <EASTL/optional.h>

#include "panel.h"

struct Frustum;


namespace darg
{

namespace panelmath3d
{

static constexpr float max_aim_distance = 8.f;
static constexpr float max_touch_distance = 0.02f;
static constexpr float new_touch_threshold = 0.25f * max_touch_distance;

struct PanelIntersection
{
  float distance = max_aim_distance + 1.f;
  Point2 uv;
};
using PossiblePanelIntersection = eastl::optional<PanelIntersection>;


bool get_valid_uv_from_intersection_point(const TMatrix &panel_tm, const Point3 &panel_size, const Point3 &point, Point2 &out_uv);
Point2 uv_to_panel_pixels(const Panel &panel, const Point2 &uv);

PossiblePanelIntersection cast_at_rectangle(const Panel &panel, const Point3 &src, const Point3 &target);
PossiblePanelIntersection project_at_rectangle(const Panel &panel, const Point3 &point, float max_dist);
PossiblePanelIntersection cast_at_hit_panel(const Panel &panel, const Point3 &world_from_pos, const Point3 &world_target_pos);
PossiblePanelIntersection project_at_hit_panel(const Panel &panel, const Point3 &point, float max_dist);

bool panel_to_screen_pixels(const Point2 &panel_pos, const Panel &panel, const TMatrix &camera_tm, const Frustum &camera_frustum,
  Point2 &out_screen_pos);

bool screen_to_panel_pixels(const Point2 &screen_pos, const Panel &panel, const TMatrix &camera_tm, const Frustum &camera_frustum,
  Point2 &out_panel_pos);

} // namespace panelmath3d

} // namespace darg
