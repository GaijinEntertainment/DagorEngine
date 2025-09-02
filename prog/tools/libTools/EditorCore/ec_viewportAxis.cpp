// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_viewportAxis.h"
#include <EditorCore/ec_gizmoSettings.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <libTools/util/hdpiUtil.h>
#include <math/dag_viewMatrix.h>
#include <EASTL/sort.h>
#include <propPanel/colors.h>

ViewportAxis::ViewportAxis(const TMatrix &view_tm, const IPoint2 &viewport_size) :
  viewTm(view_tm),
  viewportSize(viewport_size),
  axisViewTm(calculateAxisViewTm()),
  fontAscent(StdGuiRender::get_font_ascent()),
  axisCirclePadding(hdpi::_pxS(3)),
  axisCircleRadius((fontAscent + StdGuiRender::get_font_descent() + axisCirclePadding) / 2)
{}

TMatrix ViewportAxis::calculateAxisViewTm()
{
  TMatrix tm = TMatrix::IDENT;
  tm.setcol(3, 0.0f, 1.0f, 0.0f);
  view_matrix_from_look(Point3(0.0f, 0.0f, 1.0f), tm);
  return tm;
}

void ViewportAxis::calculateAxisClientPosition(Point3 &ax, Point3 &ay, Point3 &az) const
{
  TMatrix cameraTm = inverse(viewTm);
  cameraTm.setcol(3, 0.0f, 0.0f, 0.0f);

  const TMatrix tm = inverse(axisViewTm) * inverse(cameraTm);
  const Point3 ax1 = tm.getcol(0);
  const Point3 ay1 = tm.getcol(1);
  const Point3 az1 = tm.getcol(2);

  const float axisLength = hdpi::_pxS(axisLengthInPixels) * GizmoSettings::viewportGizmoScaleFactor;
  ax = Point3(ax1.x, -ax1.y, ax1.z) * axisLength;
  ay = Point3(ay1.x, -ay1.y, ay1.z) * axisLength;
  az = Point3(az1.x, -az1.y, az1.z) * axisLength;
}

void ViewportAxis::drawGizmoArrow(const Point2 &line_start, const Point2 &line_end, E3DCOLOR color, const char *axis_name,
  bool highlight, float axis_circle_radius_scaled) const
{
  StdGuiRender::set_color(color);
  StdGuiRender::draw_line(line_start, line_end, GizmoSettings::lineThickness);

  if (highlight)
  {
    const float circleRadius = axis_circle_radius_scaled + hdpi::_pxS(2) * GizmoSettings::viewportGizmoScaleFactor;
    StdGuiRender::render_rounded_box(line_end - Point2(circleRadius, circleRadius), line_end + Point2(circleRadius, circleRadius),
      E3DCOLOR(255, 255, 255), E3DCOLOR(255, 255, 255), Point4(circleRadius, circleRadius, circleRadius, circleRadius), 0.0);
  }

  const Point2 circleLeftTop = line_end - Point2(axis_circle_radius_scaled, axis_circle_radius_scaled);
  StdGuiRender::render_rounded_box(circleLeftTop, line_end + Point2(axis_circle_radius_scaled, axis_circle_radius_scaled), color,
    color, Point4(axis_circle_radius_scaled, axis_circle_radius_scaled, axis_circle_radius_scaled, axis_circle_radius_scaled), 0.0);

  if (!axis_name)
    return;

  const float textWidth = StdGuiRender::get_str_bbox(axis_name).width().x;
  const Point2 textCenterInCircle =
    Point2(axis_circle_radius_scaled - (textWidth / 2.0f), axis_circle_radius_scaled + fontAscent / 2.0f);

  StdGuiRender::goto_xy(circleLeftTop + textCenterInCircle);
  StdGuiRender::set_color(E3DCOLOR(255, 255, 255));
  StdGuiRender::draw_str(axis_name);
}

ViewportAxisId ViewportAxis::draw(const IPoint2 *mouse_pos, bool force_draw_rotator) const
{
  struct Axis
  {
    void set(ViewportAxisId axis_id, const char *in_name, E3DCOLOR in_color, const Point3 &end_pos_delta)
    {
      axisId = axis_id;
      name = in_name;
      color = in_color;
      endPosDelta = end_pos_delta;
    }

    bool isPositiveAxis() const { return name[0] != '-'; }

    Point3 endPosDelta;
    E3DCOLOR color;
    const char *name;
    ViewportAxisId axisId;
  };

  E3DCOLOR colorX, colorY, colorZ;
  GizmoSettings::retrieveAxisColors(colorX, colorY, colorZ);

  const float axisCircleRadiusScaled = axisCircleRadius * GizmoSettings::viewportGizmoScaleFactor;
  const float gizmoPadding = hdpi::_pxS(gizmoPaddingInPixels);
  const float axisLength = hdpi::_pxS(axisLengthInPixels) * GizmoSettings::viewportGizmoScaleFactor;
  const float centerX = viewportSize.x - (gizmoPadding + axisLength + axisCircleRadiusScaled * 2);
  const float centerY = gizmoPadding + axisLength + axisCircleRadiusScaled * 2;
  const Point2 center(centerX, centerY);

  Point3 ax, ay, az;
  calculateAxisClientPosition(ax, ay, az);

  Axis axes[6];
  axes[0].set(ViewportAxisId::X, "X", colorX, ax);
  axes[1].set(ViewportAxisId::Y, "Y", colorY, ay);
  axes[2].set(ViewportAxisId::Z, "Z", colorZ, az);
  axes[3].set(ViewportAxisId::NegativeX, "-X", colorX, -ax);
  axes[4].set(ViewportAxisId::NegativeY, "-Y", colorY, -ay);
  axes[5].set(ViewportAxisId::NegativeZ, "-Z", colorZ, -az);

  eastl::sort(axes, axes + 6, [](const Axis &left, const Axis &right) {
    if (left.endPosDelta.z < right.endPosDelta.z)
      return true;
    else if (left.endPosDelta.z > right.endPosDelta.z)
      return false;

    return left.name < right.name; // Comparing their addresses is enough for consistent ordering.
  });

  const int rotatorPadding = hdpi::_pxS(3);
  const float rotatorRadius = axisLength + rotatorPadding + axisCircleRadiusScaled;

  ViewportAxisId highlightedAxis = ViewportAxisId::None;
  if (mouse_pos)
  {
    if ((center - *mouse_pos).length() <= rotatorRadius)
    {
      highlightedAxis = ViewportAxisId::RotatorCircle;
      force_draw_rotator = true;
    }

    // Find the nearest axis which is the mouse is hovered over.
    for (int i = 0; i < 6; ++i)
    {
      const Point2 lineEnd = center + Point2::xy(axes[i].endPosDelta);
      if ((lineEnd - *mouse_pos).length() <= axisCircleRadiusScaled)
      {
        highlightedAxis = axes[i].axisId;
        break;
      }
    }
  }

  if (force_draw_rotator)
  {
    StdGuiRender::set_alpha_blend(NONPREMULTIPLIED);
    StdGuiRender::render_rounded_box(center - Point2(rotatorRadius, rotatorRadius), center + Point2(rotatorRadius, rotatorRadius),
      E3DCOLOR(128, 128, 128, 128), E3DCOLOR(128, 128, 128, 128), Point4(rotatorRadius, rotatorRadius, rotatorRadius, rotatorRadius),
      0.0);
    StdGuiRender::set_alpha_blend(NO_BLEND);
  }

  // Draw in reverse order, from the farthest to the nearest.
  for (int i = 5; i >= 0; --i)
  {
    const Point2 lineEnd = center + Point2::xy(axes[i].endPosDelta);
    const bool highlight = axes[i].axisId == highlightedAxis;
    const char *axisName = (axes[i].isPositiveAxis() || highlight) ? axes[i].name : nullptr;
    drawGizmoArrow(center, lineEnd, axes[i].color, axisName, highlight, axisCircleRadiusScaled);
  }

  return highlightedAxis;
}
