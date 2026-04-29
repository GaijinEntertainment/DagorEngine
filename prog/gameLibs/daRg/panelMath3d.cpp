// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "panelMath3d.h"
#include <drv/3d/dag_tex3d.h>
#include <math/dag_plane3.h>


namespace darg
{

namespace panelmath3d
{

/*
    Below there are fragments of code written independently, they are a subject to unification and refactoring.
*/

bool get_valid_uv_from_intersection_point(const TMatrix &panel_tm, const Point3 &panel_size, const Point3 &point, Point2 &out_uv)
{
  Point3 dir = point - panel_tm.getcol(3);
  Point2 pos{dir * normalize(panel_tm.getcol(0)), dir * normalize(panel_tm.getcol(1))};
  out_uv.set((pos.x + 0.5f * panel_size.x) / panel_size.x, 1.f - (pos.y + 0.5f * panel_size.y) / panel_size.y);
  return out_uv.x >= 0.f && out_uv.x < 1.f && out_uv.y >= 0.f && out_uv.y < 1.f;
}


Point2 uv_to_panel_pixels(const Panel &panel, const Point2 &uv)
{
  if (panel.canvas)
  {
    TextureInfo tinfo;
    panel.canvas->getTex()->getinfo(tinfo);
    return {uv.x * tinfo.w, uv.y * tinfo.h};
  }

  return Point2::ZERO;
}


PossiblePanelIntersection cast_at_rectangle(const Panel &panel, const Point3 &src, const Point3 &target)
{
  const TMatrix &transform = panel.renderInfo.transform;
  Point3 planeP = transform.getcol(3);
  Point3 planeZ = transform.getcol(2);
  planeZ.normalize();

  Point2 uv;
  Plane3 plane(planeZ, planeP);
  Point3 isect = plane.calcLineIntersectionPoint(src, target);
  if (isect != src && get_valid_uv_from_intersection_point(transform, panel.spatialInfo.size, isect, uv))
    return PossiblePanelIntersection({(isect - src).length(), uv});

  return {};
}


PossiblePanelIntersection project_at_rectangle(const Panel &panel, const Point3 &point, float max_dist)
{
  const TMatrix &panelTm = panel.renderInfo.transform;
  const Point3 panelOrigin = panelTm.getcol(3);
  const Point3 panelNormal = normalize(panelTm.getcol(2));
  Plane3 plane(panelNormal, panelOrigin);

  Point2 uv;
  Point3 projectedPoint = plane.project(point);
  float dist = -plane.distance(point); // FIXME: plane.distance(point) gives negative value... Wrong normals?
  if (dist > max_dist)
    return {};

  if (get_valid_uv_from_intersection_point(panelTm, panel.spatialInfo.size, projectedPoint, uv))
    return PossiblePanelIntersection({dist, uv});

  return {};
}


PossiblePanelIntersection cast_at_hit_panel(const Panel &panel, const Point3 &world_from_pos, const Point3 &world_target_pos)
{
  switch (panel.spatialInfo.geometry)
  {
    case PanelGeometry::Rectangle: return cast_at_rectangle(panel, world_from_pos, world_target_pos);

    default: DAG_FATAL("Implement casting for this type of geometry!"); return {};
  }
}


PossiblePanelIntersection project_at_hit_panel(const Panel &panel, const Point3 &point, float max_dist)
{
  G_ASSERTF_RETURN(panel.spatialInfo.geometry == PanelGeometry::Rectangle, {}, "Touching non-rectangular panels is not implemented");
  return project_at_rectangle(panel, point, max_dist);
}


static bool is_frustum_inited(const Frustum &frustum) { return v_test_any_bit_set(frustum.camPlanes[0]); }


static Point2 extract_fov_tangents(const TMatrix &inv_camera_tm, const Frustum &camera_frustum)
{
  Point3 right, left, top, bottom;
  v_stu_p3(&right.x, camera_frustum.camPlanes[Frustum::RIGHT]);
  v_stu_p3(&left.x, camera_frustum.camPlanes[Frustum::LEFT]);
  v_stu_p3(&top.x, camera_frustum.camPlanes[Frustum::TOP]);
  v_stu_p3(&bottom.x, camera_frustum.camPlanes[Frustum::BOTTOM]);

  Point3 rightNormal = inv_camera_tm % right;
  Point3 leftNormal = inv_camera_tm % left;
  Point3 topNormal = inv_camera_tm % top;
  Point3 bottomNormal = inv_camera_tm % bottom;

  float fovTanX = max(fabsf(rightNormal.x / rightNormal.z), fabsf(leftNormal.x / leftNormal.z));
  float fovTanY = max(fabsf(topNormal.y / topNormal.z), fabsf(bottomNormal.y / bottomNormal.z));

  return Point2(fovTanX, fovTanY);
}


static Point2 panel_local_to_uv(const Point2 &pos_2d, const Point3 &panel_size)
{
  return Point2((pos_2d.x + 0.5f * panel_size.x) / panel_size.x, 1.0f - (pos_2d.y + 0.5f * panel_size.y) / panel_size.y);
}


static Point2 uv_to_panel_local(const Point2 &uv, const Point3 &panel_size)
{
  return Point2(uv.x * panel_size.x - 0.5f * panel_size.x, (1.0f - uv.y) * panel_size.y - 0.5f * panel_size.y);
}


bool panel_to_screen_pixels(const Point2 &panel_pos, const Panel &panel, const TMatrix &camera_tm, const Frustum &camera_frustum,
  Point2 &out_screen_pos)
{
  if (!is_frustum_inited(camera_frustum))
    return false;

  IPoint2 canvasSize;
  G_ASSERT_RETURN(panel.getCanvasSize(canvasSize), false);

  Point2 uv(panel_pos.x / canvasSize.x, panel_pos.y / canvasSize.y);

  const TMatrix &panelTm = panel.renderInfo.transform;
  const Point3 &panelSize = panel.spatialInfo.size;
  Point2 pos2d = uv_to_panel_local(uv, panelSize);

  Point3 worldPos = panelTm.getcol(3) + normalize(panelTm.getcol(0)) * pos2d.x + normalize(panelTm.getcol(1)) * pos2d.y;

  TMatrix invCamera = inverse(camera_tm);
  Point3 viewPos = invCamera * worldPos;

  if (viewPos.z <= 0.01f)
    return false;

  Point2 fovTan = extract_fov_tangents(invCamera, camera_frustum);
  Point2 screenSize = {StdGuiRender::screen_width(), StdGuiRender::screen_height()};

  float normX = viewPos.x * fovTan.x / viewPos.z;
  float normY = viewPos.y * fovTan.y / viewPos.z;

  out_screen_pos = Point2((normX * 0.5f + 0.5f) * screenSize.x, (-normY * 0.5f + 0.5f) * screenSize.y);
  return true;
}


bool screen_to_panel_pixels(const Point2 &screen_pos, const Panel &panel, const TMatrix &camera_tm, const Frustum &camera_frustum,
  Point2 &out_panel_pos)
{
  if (!is_frustum_inited(camera_frustum))
    return false;

  float det = camera_tm.det();
  if (det < 0.1f) // not expecting a scaled down TM here, but let the threshold be that low just in case
    return false;

  Point2 screenSize = {StdGuiRender::screen_width(), StdGuiRender::screen_height()};

  float normX = (screen_pos.x / screenSize.x - 0.5f) * 2.0f;
  float normY = -(screen_pos.y / screenSize.y - 0.5f) * 2.0f;

  TMatrix invCamera = inverse(camera_tm, det);
  Point2 fovTan = extract_fov_tangents(invCamera, camera_frustum);

  Point3 viewDir(normX / fovTan.x, normY / fovTan.y, 1.0f);
  viewDir.normalize();

  Point3 worldDir = camera_tm % viewDir;
  Point3 worldCamPos = camera_tm.getcol(3);

  // Intersect ray with panel plane
  const TMatrix &panelTm = panel.renderInfo.transform;
  Point3 panelOrigin = panelTm.getcol(3);
  Point3 panelNormal = normalize(panelTm.getcol(2));

  Plane3 panelPlane(panelNormal, panelOrigin);
  Point3 worldPosOnPanel = panelPlane.calcLineIntersectionPoint(worldCamPos, worldCamPos + worldDir * 10.0f);

  // World position to panel-local 2D position
  const Point3 &panelSize = panel.spatialInfo.size;
  Point3 dir = worldPosOnPanel - panelTm.getcol(3);
  Point2 pos2d(dir * normalize(panelTm.getcol(0)), dir * normalize(panelTm.getcol(1)));

  Point2 uv = panel_local_to_uv(pos2d, panelSize);

  // UV to panel pixels

  // TextureInfo tinfo;
  // panel.canvas->getTex()->getinfo(tinfo);

  IPoint2 canvasSize;
  G_ASSERT_RETURN(panel.getCanvasSize(canvasSize), false);

  out_panel_pos = Point2(uv.x * canvasSize.x, uv.y * canvasSize.y);
  return true;
}

} // namespace panelmath3d

} // namespace darg
