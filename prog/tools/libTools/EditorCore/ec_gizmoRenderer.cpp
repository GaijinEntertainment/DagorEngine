// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_carray.h>
#include <EASTL/sort.h>
#include <math/dag_math2d.h>
#include <propPanel/colors.h>
#include <EditorCore/ec_gizmoSettings.h>
#include "ec_gizmoRenderer.h"

using hdpi::_pxS;
#define ELLIPSE_SEG_COUNT 30
#define ELLIPSE_STEP      (TWOPI / ELLIPSE_SEG_COUNT)
#define FILL_MASK         0xAAAAAAAA

static bool isPointInEllipse(const Point2 &center, const Point2 &a, const Point2 &b, real width, const Point2 &point,
  Point2 &rotateDir, real start = 0, real end = TWOPI)
{
  Point2 prev = center + a * cos(start) + b * sin(start);
  Point2 next;

  for (real t = start; t < end; t += ELLIPSE_STEP) //-V1034
  {
    next = center + a * cos(t) + b * sin(t);
    if (::is_point_in_rect(point, prev, next, width))
    {
      rotateDir = normalize(next - prev);
      return true;
    }

    prev = next;
  }

  next = center + a * cos(end) + b * sin(end);
  return ::is_point_in_rect(point, prev, next, width);
}

static void drawGizmoLine(const Point2 &from, const Point2 &delta, real offset)
{
  Point2 leftTop(from.x + delta.x * offset, from.y + delta.y * offset);
  Point2 rightBottom(from.x + delta.x, from.y + delta.y);
  StdGuiRender::draw_line(leftTop, rightBottom, GizmoSettings::lineThickness);
}

static void drawGizmoLineFromTo(const Point2 &from, const Point2 &to)
{
  StdGuiRender::draw_line(from, to, GizmoSettings::lineThickness);
}

static void drawGizmoEllipse(const Point2 &center, const Point2 &a, const Point2 &b, real start = 0, real end = TWOPI)
{
  Point2 prev = center + a * cos(start) + b * sin(start);
  Point2 next;

  for (real t = start; t < end; t += ELLIPSE_STEP) //-V1034
  {
    next = center + a * cos(t) + b * sin(t);
    StdGuiRender::draw_line(prev, next, GizmoSettings::lineThickness);

    prev = next;
  }

  next = center + a * cos(end) + b * sin(end);
  StdGuiRender::draw_line(prev, next, GizmoSettings::lineThickness);
}

static void fillGizmoEllipse(const Point2 &center, const Point2 &a, const Point2 &b, E3DCOLOR color, int fp, real start = 0,
  real end = TWOPI)
{
  Point2 points[ELLIPSE_SEG_COUNT + 2];
  int cur = 0;

  drawGizmoLineFromTo(center, center); // mega hack for StdGuiRender!!!

  for (real t = start; t < end; t += ELLIPSE_STEP) //-V1034
  {
    points[cur++] = center + a * cos(t) + b * sin(t);
  }

  if (start != 0 || end != TWOPI)
  {
    points[cur++] = center + a * cos(end) + b * sin(end);
    points[cur++] = center;
  }

  // dc.selectFillPattern(fp);
  StdGuiRender::draw_fill_poly_fan(points, cur, color);
}

#define AXIS_X GizmoEventFilter::SelectedAxes::AXIS_X
#define AXIS_Y GizmoEventFilter::SelectedAxes::AXIS_Y
#define AXIS_Z GizmoEventFilter::SelectedAxes::AXIS_Z

/********************************************************************
 * Common
 ********************************************************************/
void IGizmoRenderer::drawGizmoArrow(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, real offset) const
{
  Point2 tri[3];
  Point2 norm = normalize(delta);
  eastl::swap(norm.x, norm.y);
  norm.y = -norm.y;

  float side = eastl::max(arrowSideSize, arrowSideSize * GizmoSettings::lineThickness * arrowSideThicknessRatio);
  tri[0] = from + delta;
  tri[1] = from + delta * 0.8 + norm * side;
  tri[2] = from + delta * 0.8 - norm * side;

  drawGizmoLine(from, delta * 0.9, offset);

  // dc.selectFillPattern(0xFFFFFFFF);
  StdGuiRender::draw_fill_poly_fan(tri, 3, col_fill);
}

void IGizmoRenderer::drawGizmoArrowScale(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, real offset) const
{
  Point2 tri[4];
  Point2 norm = normalize(delta);
  eastl::swap(norm.x, norm.y);
  norm.y = -norm.y;

  float side = eastl::max(arrowSideSize, arrowSideSize * GizmoSettings::lineThickness * arrowSideThicknessRatio);
  tri[0] = from + delta - norm * side;
  tri[1] = from + delta + norm * side;
  tri[2] = from + delta * 0.9 + norm * side;
  tri[3] = from + delta * 0.9 - norm * side;

  drawGizmoLine(from, delta * 0.95, offset);

  // dc.selectFillPattern(0xFFFFFFFF);
  StdGuiRender::draw_fill_poly_fan(tri, 4, col_fill);
}

void IGizmoRenderer::renderRotate(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], const Point2 &startPos2d,
  real rotAngle, real startAngle, int sel) const
{
  E3DCOLOR colorX, colorY, colorZ;
  GizmoSettings::retrieveAxisColors(colorX, colorY, colorZ);

  if (gizmo->isStarted() && sel)
  {
    Point2 a, b;
    E3DCOLOR col1, col2;

    switch (sel)
    {
      case AXIS_X:
        a = axes[1];
        b = axes[2];
        col1 = colorX;
        col2 = E3DCOLOR(colorX.r / 2, colorX.g / 2, colorX.b / 2);
        break;
      case AXIS_Y:
        a = axes[2];
        b = axes[0];
        col1 = colorY;
        col2 = E3DCOLOR(colorY.r / 2, colorY.g / 2, colorY.b / 2);
        break;
      case AXIS_Z:
        a = axes[0];
        b = axes[1];
        col1 = colorZ;
        col2 = E3DCOLOR(colorZ.r / 2, colorZ.g / 2, colorZ.b / 2);
        break;
    }

    const real deltaAngle = rotAngle - startAngle;
    const real rotAbs = fabs(deltaAngle);
    const bool rotWorld = IEditorCoreEngine::get()->getGizmoBasisType() == IEditorCoreEngine::BASIS_World;

    real angle = rotWorld ? rotAngle : startAngle - deltaAngle;

    if (rotAbs < TWOPI)
    {
      fillGizmoEllipse(center, a, b, col1, FILL_MASK, ::min(startAngle, angle), ::max(startAngle, angle));
    }
    else if (rotAbs < TWOPI * 2)
    {
      const int divizer = (deltaAngle) / TWOPI;
      const real endRot = rotAngle - TWOPI * divizer;

      angle = rotWorld ? endRot : startAngle - (endRot - startAngle);

      const real start = ::min(startAngle, angle);
      const real end = ::max(startAngle, angle);

      fillGizmoEllipse(center, a, b, col2, 0xFFFFFFFF, start, end);
      fillGizmoEllipse(center, a, b, col1, FILL_MASK, end - TWOPI, start);
    }
    else
      fillGizmoEllipse(center, a, b, col2, 0xFFFFFFFF);

    StdGuiRender::set_color(rotAbs < TWOPI ? col1 : col2);
    drawGizmoLineFromTo(center, startPos2d);

    angle = rotWorld ? rotAngle : startAngle;

    Point2 end = center + a * cos(angle) + b * sin(angle);
    drawGizmoLineFromTo(center, end);

    StdGuiRender::set_font(0);
    StdGuiRender::set_color(COLOR_YELLOW);

    StdGuiRender::end_raw_layer();
    Point3 movedDelta = gizmo->getMoveDelta();
    StdGuiRender::draw_strf_to(center.x - _pxS(80), center.y - AXIS_LEN_PIX - _pxS(40), "[%.2f, %.2f, %.2f]", RadToDeg(movedDelta.x),
      RadToDeg(movedDelta.y), RadToDeg(movedDelta.z));
    StdGuiRender::reset_textures();
    StdGuiRender::start_raw_layer();
  }

  StdGuiRender::set_color((sel == 1) ? COLOR_YELLOW : colorX);
  drawGizmoEllipse(center, axes[1], axes[2]);

  StdGuiRender::set_color((sel == 2) ? COLOR_YELLOW : colorY);
  drawGizmoEllipse(center, axes[2], axes[0]);

  StdGuiRender::set_color((sel == 4) ? COLOR_YELLOW : colorZ);
  drawGizmoEllipse(center, axes[0], axes[1]);
}

/********************************************************************
 * New
 ********************************************************************/

#define CENTRAL_HANDLE_DIV      5.0f
#define PLANE_HANDLE_DIV        3.0f
#define PLANE_HANDLE_ORIGIN_DIV 2.5f

static void fillCentralBoxSide(Point2 *rect, const Point2 &center, const Point2 &offset, const Point2 &ax, const Point2 &ay)
{
  rect[0] = center + offset / CENTRAL_HANDLE_DIV;
  rect[1] = rect[0] + ax / CENTRAL_HANDLE_DIV;
  rect[2] = rect[1] + ay / CENTRAL_HANDLE_DIV;
  rect[3] = rect[0] + ay / CENTRAL_HANDLE_DIV;
}

GizmoRendererNew::GizmoRendererNew(const GizmoEventFilter *gizmo) : IGizmoRenderer(gizmo, _pxS(4)) { arrowSideThicknessRatio = 0.2f; }

void GizmoRendererNew::drawGizmoQuad(const Point2 &from, const Point2 &delta1, const Point2 &delta2, E3DCOLOR col, E3DCOLOR col_sel,
  bool sel, real div) const
{
  G_ASSERT(div > 0);
  const Point2 d1 = delta1 / div;
  const Point2 d2 = delta2 / div;
  Point2 quad[4] = {from, from + d2, from + d2 + d1, from + d1};
  StdGuiRender::draw_fill_poly_fan(quad, 4, sel ? col_sel : col);
}

void GizmoRendererNew::drawCentralHandle(const IGenViewportWnd *wnd, const Point2 &center, const Point2 axes[3], const E3DCOLOR col[3],
  bool isSelected) const
{
  struct FaceDrawData
  {
    Point2 origin;
    Point2 dx;
    Point2 dy;
    E3DCOLOR color;
    Point3 middle;
    float proj;
  };
  TMatrix cameraTm;
  wnd->getCameraTransform(cameraTm);
  Point3 cameraDir = cameraTm.getcol(2);
  Point3 axesWorld[3];
  if (gizmo->getGizmoClient()->getAxes(axesWorld[0], axesWorld[1], axesWorld[2]))
    cameraDir = Point3(cameraDir * axesWorld[0], cameraDir * axesWorld[1], cameraDir * axesWorld[2]);

  carray<FaceDrawData, 6> faces = {{{center + axes[0] / CENTRAL_HANDLE_DIV, axes[2], axes[1], col[0], Point3(1, 0.5, 0.5), 0.f},
    {center + axes[1] / CENTRAL_HANDLE_DIV, axes[0], axes[2], col[1], Point3(0.5, 1, 0.5), 0.f},
    {center + axes[2] / CENTRAL_HANDLE_DIV, axes[0], axes[1], col[2], Point3(0.5, 0.5, 1), 0.f},
    {center, axes[2], axes[1], col[0], Point3(0, 0.5, 0.5), 0.f}, {center, axes[0], axes[2], col[1], Point3(0.5, 0, 0.5), 0.f},
    {center, axes[0], axes[1], col[2], Point3(0.5, 0.5, 0), 0.f}}};
  for (auto &face : faces)
  {
    face.proj = face.middle * cameraDir;
  }
  eastl::sort(faces.begin(), faces.end(), [](const FaceDrawData &a, const FaceDrawData &b) { return a.proj > b.proj; });
  for (int i = 2; i < 6; i++)
  {
    auto &face = faces[i];
    drawGizmoQuad(face.origin, face.dx, face.dy, face.color, COLOR_YELLOW, isSelected, CENTRAL_HANDLE_DIV);
  }
}

void GizmoRendererNew::renderMove(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], int sel) const
{
  E3DCOLOR colorX, colorY, colorZ;
  GizmoSettings::retrieveAxisColors(colorX, colorY, colorZ);

  const bool isBoxSelected = (sel & 7) == 7;
  const bool hideX = gizmo->isStarted() && (sel == (AXIS_Y | AXIS_Z));
  const bool hideY = gizmo->isStarted() && (sel == (AXIS_X | AXIS_Z));
  const bool hideZ = gizmo->isStarted() && (sel == (AXIS_X | AXIS_Y));
  const bool hidePlanes = gizmo->isStarted() && (sel == AXIS_X || sel == AXIS_Y || sel == AXIS_Z || isBoxSelected);
  const bool hideBox = gizmo->isStarted();

  // axes
  if (!hideX)
  {
    StdGuiRender::set_color((sel & AXIS_X) ? COLOR_YELLOW : colorX);
    drawGizmoArrow(center, axes[0], colorX);
  }

  if (!hideY)
  {
    StdGuiRender::set_color((sel & AXIS_Y) ? COLOR_YELLOW : colorY);
    drawGizmoArrow(center, axes[1], colorY);
  }

  if (!hideZ)
  {
    StdGuiRender::set_color((sel & AXIS_Z) ? COLOR_YELLOW : colorZ);
    drawGizmoArrow(center, axes[2], colorZ);
  }

  // central handle
  if (!hideBox)
  {
    E3DCOLOR col[3] = {colorX, colorY, colorZ};
    drawCentralHandle(w, center, axes, col, isBoxSelected);
  }

  // plane handles
  if (!hidePlanes)
  {
    const uint8_t alpha = 127;
    StdGuiRender::set_alpha_blend(NONPREMULTIPLIED);
    if (!hideY && !hideZ)
    {
      drawGizmoQuad(center + (axes[1] + axes[2]) / PLANE_HANDLE_ORIGIN_DIV, axes[1], axes[2],
        E3DCOLOR(colorX.r, colorX.g, colorX.b, alpha), colorX, (sel & (AXIS_Y | AXIS_Z)) == (AXIS_Y | AXIS_Z), PLANE_HANDLE_DIV);
    }
    if (!hideX && !hideZ)
    {
      drawGizmoQuad(center + (axes[2] + axes[0]) / PLANE_HANDLE_ORIGIN_DIV, axes[2], axes[0],
        E3DCOLOR(colorY.r, colorY.g, colorY.b, alpha), colorY, (sel & (AXIS_X | AXIS_Z)) == (AXIS_X | AXIS_Z), PLANE_HANDLE_DIV);
    }
    if (!hideX && !hideY)
    {
      drawGizmoQuad(center + (axes[0] + axes[1]) / PLANE_HANDLE_ORIGIN_DIV, axes[0], axes[1],
        E3DCOLOR(colorZ.r, colorZ.g, colorZ.b, alpha), colorZ, (sel & (AXIS_X | AXIS_Y)) == (AXIS_X | AXIS_Y), PLANE_HANDLE_DIV);
    }
    StdGuiRender::set_alpha_blend(NO_BLEND);
  }
}

void GizmoRendererNew::renderSurfMove(const Point2 &center, const Point2 axes[3], int sel) const
{
  E3DCOLOR colorX, colorY, colorZ;
  GizmoSettings::retrieveAxisColors(colorX, colorY, colorZ);

  const bool hideX = gizmo->isStarted() && (sel == (AXIS_Y | AXIS_Z));
  const bool hideZ = gizmo->isStarted() && (sel == (AXIS_X | AXIS_Y));
  const bool hidePlanes = gizmo->isStarted() && (sel == AXIS_X || sel == AXIS_Z);

  // axes
  if (!hideX)
  {
    StdGuiRender::set_color((sel & AXIS_X) ? COLOR_YELLOW : colorX);
    drawGizmoArrow(center, axes[0], colorX);
  }
  if (!hideZ)
  {
    StdGuiRender::set_color((sel & AXIS_Z) ? COLOR_YELLOW : colorZ);
    drawGizmoArrow(center, axes[2], colorZ);
  }

  // plane handles
  if (!hidePlanes)
  {
    const uint8_t alpha = 127;
    StdGuiRender::set_alpha_blend(NONPREMULTIPLIED);
    drawGizmoQuad(center + (axes[2] + axes[0]) / PLANE_HANDLE_ORIGIN_DIV, axes[2], axes[0],
      E3DCOLOR(colorY.r, colorY.g, colorY.b, alpha), colorY, (sel & (AXIS_X | AXIS_Z)) == (AXIS_X | AXIS_Z), PLANE_HANDLE_DIV);
    StdGuiRender::set_alpha_blend(NO_BLEND);
  }
}

void GizmoRendererNew::renderScale(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], int sel, const Point3 &scale) const
{
  E3DCOLOR colorX, colorY, colorZ;
  GizmoSettings::retrieveAxisColors(colorX, colorY, colorZ);

  Point2 ax = axes[0] + axes[0] * (scale.x - 1);
  Point2 ay = axes[1] + axes[1] * (scale.y - 1);
  Point2 az = axes[2] + axes[2] * (scale.z - 1);

  const bool isBoxSelected = (sel & 7) == 7;
  const bool hideX = gizmo->isStarted() && (sel == (AXIS_Y | AXIS_Z));
  const bool hideY = gizmo->isStarted() && (sel == (AXIS_X | AXIS_Z));
  const bool hideZ = gizmo->isStarted() && (sel == (AXIS_X | AXIS_Y));
  const bool hidePlanes = gizmo->isStarted() && (sel == AXIS_X || sel == AXIS_Y || sel == AXIS_Z || isBoxSelected);
  const bool hideBox = gizmo->isStarted();

  // axes
  if (!hideX)
  {
    StdGuiRender::set_color((sel & AXIS_X) ? COLOR_YELLOW : colorX);
    drawGizmoArrowScale(center, ax, colorX);
  }

  if (!hideY)
  {
    StdGuiRender::set_color((sel & AXIS_Y) ? COLOR_YELLOW : colorY);
    drawGizmoArrowScale(center, ay, colorY);
  }

  if (!hideZ)
  {
    StdGuiRender::set_color((sel & AXIS_Z) ? COLOR_YELLOW : colorZ);
    drawGizmoArrowScale(center, az, colorZ);
  }

  // central handle
  if (!hideBox)
  {
    E3DCOLOR col[3] = {colorX, colorY, colorZ};
    drawCentralHandle(w, center, axes, col, isBoxSelected);
  }

  // plane handles
  if (!hidePlanes)
  {
    const uint8_t alpha = 127;
    StdGuiRender::set_alpha_blend(NONPREMULTIPLIED);
    if (!hideY && !hideZ)
    {
      real div = PLANE_HANDLE_DIV * eastl::max(scale.y, scale.z);
      drawGizmoQuad(center + (ay + az) / PLANE_HANDLE_ORIGIN_DIV, ay, az, E3DCOLOR(colorX.r, colorX.g, colorX.b, alpha), colorX,
        (sel & (AXIS_Y | AXIS_Z)) == (AXIS_Y | AXIS_Z), div);
    }
    if (!hideX && !hideZ)
    {
      real div = PLANE_HANDLE_DIV * eastl::max(scale.x, scale.z);
      drawGizmoQuad(center + (az + ax) / PLANE_HANDLE_ORIGIN_DIV, az, ax, E3DCOLOR(colorY.r, colorY.g, colorY.b, alpha), colorY,
        (sel & (AXIS_X | AXIS_Z)) == (AXIS_X | AXIS_Z), div);
    }
    if (!hideX && !hideY)
    {
      real div = PLANE_HANDLE_DIV * eastl::max(scale.x, scale.y);
      drawGizmoQuad(center + (ax + ay) / PLANE_HANDLE_ORIGIN_DIV, ax, ay, E3DCOLOR(colorZ.r, colorZ.g, colorZ.b, alpha), colorZ,
        (sel & (AXIS_X | AXIS_Y)) == (AXIS_X | AXIS_Y), div);
    }
    StdGuiRender::set_alpha_blend(NO_BLEND);
  }
}

int GizmoRendererNew::intersection(const Point2 &mouse, const Point2 &center, const Point2 axes[3], int selected,
  Point2 &rotateDir) const
{
  IEditorCoreEngine::ModeType type = gizmo->getGizmoType();
  switch (type)
  {
    case IEditorCoreEngine::MODE_None: return 0;

    case IEditorCoreEngine::MODE_Move:
    case IEditorCoreEngine::MODE_Scale:
    case IEditorCoreEngine::MODE_MoveSurface:
    {
      Point2 rectXY[4], rectYZ[4], rectZX[4];

      if (type != IEditorCoreEngine::MODE_MoveSurface)
      {
        // central handle
        fillCentralBoxSide(rectXY, center, axes[2], axes[0], axes[1]);
        fillCentralBoxSide(rectYZ, center, axes[0], axes[1], axes[2]);
        fillCentralBoxSide(rectZX, center, axes[1], axes[2], axes[0]);
        bool isOverCentralBox = ::is_point_in_conv_poly(mouse, rectXY, 4) || ::is_point_in_conv_poly(mouse, rectYZ, 4) ||
                                ::is_point_in_conv_poly(mouse, rectZX, 4);

        fillCentralBoxSide(rectXY, center, Point2::ZERO, axes[0], axes[1]);
        fillCentralBoxSide(rectYZ, center, Point2::ZERO, axes[1], axes[2]);
        fillCentralBoxSide(rectZX, center, Point2::ZERO, axes[2], axes[0]);
        isOverCentralBox |= ::is_point_in_conv_poly(mouse, rectXY, 4) || ::is_point_in_conv_poly(mouse, rectYZ, 4) ||
                            ::is_point_in_conv_poly(mouse, rectZX, 4);

        if (isOverCentralBox)
          return AXIS_X | AXIS_Y | AXIS_Z;

        // XY handle
        rectXY[0] = center + (axes[0] + axes[1]) / PLANE_HANDLE_ORIGIN_DIV;
        rectXY[1] = rectXY[0] + axes[0] / PLANE_HANDLE_DIV;
        rectXY[2] = rectXY[1] + axes[1] / PLANE_HANDLE_DIV;
        rectXY[3] = rectXY[0] + axes[1] / PLANE_HANDLE_DIV;
        if (::is_point_in_conv_poly(mouse, rectXY, 4))
          return AXIS_X | AXIS_Y;

        // YZ handle
        rectYZ[0] = center + (axes[1] + axes[2]) / PLANE_HANDLE_ORIGIN_DIV;
        rectYZ[1] = rectYZ[0] + axes[1] / PLANE_HANDLE_DIV;
        rectYZ[2] = rectYZ[1] + axes[2] / PLANE_HANDLE_DIV;
        rectYZ[3] = rectYZ[0] + axes[2] / PLANE_HANDLE_DIV;
        if (::is_point_in_conv_poly(mouse, rectYZ, 4))
          return AXIS_Y | AXIS_Z;
      }

      // ZX handle
      rectZX[0] = center + (axes[2] + axes[0]) / PLANE_HANDLE_ORIGIN_DIV;
      rectZX[1] = rectZX[0] + axes[2] / PLANE_HANDLE_DIV;
      rectZX[2] = rectZX[1] + axes[0] / PLANE_HANDLE_DIV;
      rectZX[3] = rectZX[0] + axes[0] / PLANE_HANDLE_DIV;
      if (::is_point_in_conv_poly(mouse, rectZX, 4))
        return AXIS_X | AXIS_Z;

      break;
    }

    case IEditorCoreEngine::MODE_Rotate:
    {
      if (isPointInEllipse(center, axes[1], axes[2], GIZMO_PIXEL_WIDTH * 2, mouse, rotateDir))
        return AXIS_X;
      if (isPointInEllipse(center, axes[2], axes[0], GIZMO_PIXEL_WIDTH * 2, mouse, rotateDir))
        return AXIS_Y;
      if (isPointInEllipse(center, axes[0], axes[1], GIZMO_PIXEL_WIDTH * 2, mouse, rotateDir))
        return AXIS_Z;
      break;
    }
  }

  if (type != IEditorCoreEngine::MODE_Rotate)
  {
    if (::is_point_in_rect(mouse, center, center + axes[0], GIZMO_PIXEL_WIDTH))
      return AXIS_X;
    if (::is_point_in_rect(mouse, center, center + axes[1], GIZMO_PIXEL_WIDTH) && type != IEditorCoreEngine::MODE_MoveSurface)
      return AXIS_Y;
    if (::is_point_in_rect(mouse, center, center + axes[2], GIZMO_PIXEL_WIDTH))
      return AXIS_Z;
  }

  return 0;
}

/********************************************************************
 * Classic
 ********************************************************************/

#define GIZMO_BOX_WIDTH _pxS(4)

GizmoRendererClassic::GizmoRendererClassic(const GizmoEventFilter *gizmo) : IGizmoRenderer(gizmo, _pxS(3)) {}

void GizmoRendererClassic::drawGizmoQuad(const Point2 &from, const Point2 &delta1, const Point2 &delta2, E3DCOLOR col1, E3DCOLOR col2,
  E3DCOLOR col_sel, bool sel, real div) const
{
  G_ASSERT(div > 0);
  const Point2 d1 = delta1 / div;
  const Point2 d2 = delta2 / div;
  if (!sel)
  {
    StdGuiRender::set_color(col1);
    drawGizmoLine(from + d1, d2, 0);
    StdGuiRender::set_color(col2);
    drawGizmoLine(from + d2, d1, 0);
  }
  else
  {
    StdGuiRender::set_color(col_sel);
    StdGuiRender::render_quad(from, from + d2, from + d1 + d2, from + d1);
    drawGizmoLine(from + d1, d2, 0);
    drawGizmoLine(from + d2, d1, 0);
    drawGizmoLine(from, d1, 0);
    drawGizmoLine(from, d2, 0);
  }
}

void GizmoRendererClassic::renderMove(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], int sel) const
{
  E3DCOLOR colorX, colorY, colorZ;
  GizmoSettings::retrieveAxisColors(colorX, colorY, colorZ);

  StdGuiRender::set_color((sel & AXIS_X) ? COLOR_YELLOW : colorX);
  drawGizmoArrow(center, axes[0], colorX);
  StdGuiRender::set_color((sel & AXIS_Y) ? COLOR_YELLOW : colorY);
  drawGizmoArrow(center, axes[1], colorY);
  StdGuiRender::set_color((sel & AXIS_Z) ? COLOR_YELLOW : colorZ);
  drawGizmoArrow(center, axes[2], colorZ);
  drawGizmoQuad(center, axes[2], axes[0], colorZ, colorX, COLOR_YELLOW, (sel & 5) == 5);
  drawGizmoQuad(center, axes[0], axes[1], colorX, colorY, COLOR_YELLOW, (sel & 3) == 3);
  drawGizmoQuad(center, axes[1], axes[2], colorY, colorZ, COLOR_YELLOW, (sel & 6) == 6);
}

void GizmoRendererClassic::renderSurfMove(const Point2 &center, const Point2 axes[3], int sel) const
{
  E3DCOLOR colorX, colorY, colorZ;
  GizmoSettings::retrieveAxisColors(colorX, colorY, colorZ);

  StdGuiRender::set_color((sel & AXIS_X) ? COLOR_YELLOW : colorX);
  drawGizmoArrow(center, axes[0], colorX);
  StdGuiRender::set_color((sel & AXIS_Z) ? COLOR_YELLOW : colorZ);
  drawGizmoArrow(center, axes[2], colorZ);
  drawGizmoQuad(center, axes[2], axes[0], colorZ, colorX, COLOR_YELLOW, (sel & 5) == 5, 2.0);
}

void GizmoRendererClassic::renderScale(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], int sel,
  const Point3 &scale) const
{
  E3DCOLOR colorX, colorY, colorZ;
  GizmoSettings::retrieveAxisColors(colorX, colorY, colorZ);

  Point2 ax = axes[0] + axes[0] * (scale.x - 1);
  Point2 ay = axes[1] + axes[1] * (scale.y - 1);
  Point2 az = axes[2] + axes[2] * (scale.z - 1);
  Point2 points[4];

  Point2 start, end;
  StdGuiRender::set_color(sel == 3 ? COLOR_YELLOW : colorX);
  start = center + ax / 3 * 2;
  end = center + ay / 3 * 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);
  StdGuiRender::set_color(sel == 5 ? COLOR_YELLOW : colorX);
  end = center + az / 3 * 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);
  StdGuiRender::set_color(sel == 3 ? COLOR_YELLOW : colorX);
  start = center + ax / 2;
  end = center + ay / 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);
  StdGuiRender::set_color(sel == 5 ? COLOR_YELLOW : colorX);
  end = center + az / 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);
  StdGuiRender::set_color(sel == 3 ? COLOR_YELLOW : colorY);
  start = center + ay / 3 * 2;
  end = center + ax / 3 * 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);
  StdGuiRender::set_color(sel == 6 ? COLOR_YELLOW : colorY);
  end = center + az / 3 * 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);
  StdGuiRender::set_color(sel == 3 ? COLOR_YELLOW : colorY);
  start = center + ay / 2;
  end = center + ax / 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);
  StdGuiRender::set_color(sel == 6 ? COLOR_YELLOW : colorY);
  end = center + az / 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);
  StdGuiRender::set_color(sel == 5 ? COLOR_YELLOW : colorZ);
  start = center + az / 3 * 2;
  end = center + ax / 3 * 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);
  StdGuiRender::set_color(sel == 6 ? COLOR_YELLOW : colorZ);
  end = center + ay / 3 * 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);
  StdGuiRender::set_color(sel == 5 ? COLOR_YELLOW : colorZ);
  start = center + az / 2;
  end = center + ax / 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);
  StdGuiRender::set_color(sel == 6 ? COLOR_YELLOW : colorZ);
  end = center + ay / 2;
  drawGizmoLineFromTo(start, start + (end - start) / 2);

  switch (sel)
  {
    case 3:
      points[0] = center + ax / 2;
      points[1] = center + ax / 3 * 2;
      points[2] = center + ay / 3 * 2;
      points[3] = center + ay / 2;
      StdGuiRender::draw_fill_poly_fan(points, 4, COLOR_YELLOW);
      break;
    case 6:
      points[0] = center + ay / 2;
      points[1] = center + ay / 3 * 2;
      points[2] = center + az / 3 * 2;
      points[3] = center + az / 2;
      StdGuiRender::draw_fill_poly_fan(points, 4, COLOR_YELLOW);
      break;
    case 5:
      points[0] = center + az / 2;
      points[1] = center + az / 3 * 2;
      points[2] = center + ax / 3 * 2;
      points[3] = center + ax / 2;
      StdGuiRender::draw_fill_poly_fan(points, 4, COLOR_YELLOW);
      break;
    case 7:
      points[0] = center + ax / 2;
      points[1] = center + ay / 2;
      points[2] = center + az / 2;
      StdGuiRender::draw_fill_poly_fan(points, 3, COLOR_YELLOW);
      break;
  }

  StdGuiRender::set_color((sel & 1) ? COLOR_YELLOW : colorX);
  drawGizmoArrowScale(center, ax, colorX);
  StdGuiRender::set_color((sel & 2) ? COLOR_YELLOW : colorY);
  drawGizmoArrowScale(center, ay, colorY);
  StdGuiRender::set_color((sel & 4) ? COLOR_YELLOW : colorZ);
  drawGizmoArrowScale(center, az, colorZ);
}

int GizmoRendererClassic::intersection(const Point2 &mouse, const Point2 &center, const Point2 axes[3], int selected,
  Point2 &rotateDir) const
{
  IEditorCoreEngine::ModeType type = gizmo->getGizmoType();
  if (type != IEditorCoreEngine::MODE_Rotate)
  {
    if (::is_point_in_rect(mouse, center, center + axes[0], GIZMO_PIXEL_WIDTH))
    {
      return AXIS_X;
    }
    if (::is_point_in_rect(mouse, center, center + axes[1], GIZMO_PIXEL_WIDTH) && type != IEditorCoreEngine::MODE_MoveSurface)
    {
      return AXIS_Y;
    }
    if (::is_point_in_rect(mouse, center, center + axes[2], GIZMO_PIXEL_WIDTH))
    {
      return AXIS_Z;
    }
  }
  else
  {
    if (isPointInEllipse(center, axes[1], axes[2], GIZMO_PIXEL_WIDTH * 2, mouse, rotateDir))
    {
      return AXIS_X;
    }
    if (isPointInEllipse(center, axes[2], axes[0], GIZMO_PIXEL_WIDTH * 2, mouse, rotateDir))
    {
      return AXIS_Y;
    }
    if (isPointInEllipse(center, axes[0], axes[1], GIZMO_PIXEL_WIDTH * 2, mouse, rotateDir))
    {
      return AXIS_Z;
    }
  }
  switch (type)
  {
    case IEditorCoreEngine::MODE_Move:
    {
      Point2 rectXY[4];
      rectXY[0] = center;
      rectXY[1] = center + axes[0] / 3.0;
      rectXY[2] = rectXY[1] + axes[1] / 3.0;
      rectXY[3] = center + axes[1] / 3.0;
      Point2 rectYZ[4];
      rectYZ[0] = center;
      rectYZ[1] = center + axes[1] / 3.0;
      rectYZ[2] = rectYZ[1] + axes[2] / 3.0;
      rectYZ[3] = center + axes[2] / 3.0;
      Point2 rectZX[4];
      rectZX[0] = center;
      rectZX[1] = center + axes[2] / 3.0;
      rectZX[2] = rectZX[1] + axes[0] / 3.0;
      rectZX[3] = center + axes[0] / 3.0;
      if (::is_point_in_rect(mouse, rectZX[1], rectZX[2], GIZMO_BOX_WIDTH) ||
          ::is_point_in_rect(mouse, rectZX[3], rectZX[2], GIZMO_BOX_WIDTH))
      {
        return AXIS_X | AXIS_Z;
      }
      if (selected == (AXIS_X | AXIS_Z) && ::is_point_in_conv_poly(mouse, rectZX, 4))
      {
        return AXIS_X | AXIS_Z;
      }
      if (::is_point_in_rect(mouse, rectXY[1], rectXY[2], GIZMO_BOX_WIDTH) ||
          ::is_point_in_rect(mouse, rectXY[3], rectXY[2], GIZMO_BOX_WIDTH))
      {
        return AXIS_X | AXIS_Y;
      }
      if (::is_point_in_rect(mouse, rectYZ[1], rectYZ[2], GIZMO_BOX_WIDTH) ||
          ::is_point_in_rect(mouse, rectYZ[3], rectYZ[2], GIZMO_BOX_WIDTH))
      {
        return AXIS_Y | AXIS_Z;
      }
      if (selected == (AXIS_X | AXIS_Y) && ::is_point_in_conv_poly(mouse, rectXY, 4))
      {
        return AXIS_X | AXIS_Y;
      }
      if (selected == (AXIS_Y | AXIS_Z) && ::is_point_in_conv_poly(mouse, rectYZ, 4))
      {
        return AXIS_Y | AXIS_Z;
      }
      break;
    }
    case IEditorCoreEngine::MODE_MoveSurface:
    {
      Point2 rectZX[4];
      rectZX[0] = center;
      rectZX[1] = center + axes[2] / 2.0;
      rectZX[2] = rectZX[1] + axes[0] / 2.0;
      rectZX[3] = center + axes[0] / 2.0;
      if (::is_point_in_rect(mouse, rectZX[1], rectZX[2], GIZMO_BOX_WIDTH) ||
          ::is_point_in_rect(mouse, rectZX[3], rectZX[2], GIZMO_BOX_WIDTH))
      {
        return AXIS_X | AXIS_Z;
      }
      if (::is_point_in_conv_poly(mouse, rectZX, 4))
      {
        return AXIS_X | AXIS_Z;
      }
      break;
    }
    case IEditorCoreEngine::MODE_Scale:
    {
      Point2 start, end;
      start = center + axes[0] * 7 / 12;
      end = center + axes[1] * 7 / 12;
      if (::is_point_in_rect(mouse, start, end, GIZMO_BOX_WIDTH * 2))
      {
        return AXIS_X | AXIS_Y;
      }
      start = center + axes[1] * 7 / 12;
      end = center + axes[2] * 7 / 12;
      if (::is_point_in_rect(mouse, start, end, GIZMO_BOX_WIDTH * 2))
      {
        return AXIS_Y | AXIS_Z;
      }
      start = center + axes[2] * 7 / 12;
      end = center + axes[0] * 7 / 12;
      if (::is_point_in_rect(mouse, start, end, GIZMO_BOX_WIDTH * 2))
      {
        return AXIS_X | AXIS_Z;
      }
      Point2 triangle[3];
      triangle[0] = center + axes[0] / 2;
      triangle[1] = center + axes[1] / 2;
      triangle[2] = center + axes[2] / 2;
      if (::is_point_in_triangle(mouse, triangle))
      {
        return AXIS_X | AXIS_Y | AXIS_Z;
      }
      break;
    }

    // to prevent the unhandled switch case error
    case IEditorCoreEngine::MODE_None:
    case IEditorCoreEngine::MODE_Rotate: break;
  }
  return 0;
}

void GizmoRendererClassic::flip(IGenViewportWnd *w, const Matrix3 &basis, Point2 &ax, Point2 &ay, Point2 &az) const
{
  if ((gizmo->getGizmoType() & IEditorCoreEngine::MODE_Scale) == 0)
    return;

  TMatrix tm;
  w->getCameraTransform(tm);
  tm = inverse(tm);
  const Point3 camDir = Point3(tm.m[0][2], tm.m[1][2], tm.m[2][2]);
  if (basis.getcol(0) * camDir > 0)
    ax = -ax;
  if (basis.getcol(1) * camDir > 0)
    ay = -ay;
  if (basis.getcol(2) * camDir > 0)
    az = -az;
}