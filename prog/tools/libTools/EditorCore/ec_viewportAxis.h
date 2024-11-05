// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_viewportAxisId.h>
#include <math/dag_e3dColor.h>
#include <math/dag_TMatrix.h>
#include <math/integer/dag_IPoint2.h>

class ViewportAxis
{
public:
  ViewportAxis(const TMatrix &view_tm, const IPoint2 &viewport_size);

  ViewportAxisId draw(const IPoint2 *mouse_pos, bool force_draw_rotator) const;

private:
  void calculateAxisClientPosition(Point3 &ax, Point3 &ay, Point3 &az) const;
  void drawGizmoArrow(const Point2 &line_start, const Point2 &line_end, E3DCOLOR color, const char *axis_name, bool highlight) const;

  static TMatrix calculateAxisViewTm();

  const TMatrix &viewTm;
  const TMatrix axisViewTm; // The view used for rendering the world axis.
  const IPoint2 viewportSize;
  const int fontAscent;
  const int axisCirclePadding;
  const int axisCircleRadius;

  static inline const float gizmoMeter = 0.01f;
  static const int gizmoPaddingInPixels = 15;
  static const int axisLengthInPixels = 50;
};
