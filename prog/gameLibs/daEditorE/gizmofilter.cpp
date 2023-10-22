#include "gizmofilter.h"

#include <stdlib.h>

#include <debug/dag_debug.h>
#include <3d/dag_render.h>
#include <math/dag_math2d.h>
#include <humanInput/dag_hiKeybIds.h>
#include <gui/dag_stdGuiRenderEx.h>


#define AXIS_LEN_PIX      100
#define ELLIPSE_SEG_COUNT 30
#define ELLIPSE_STEP      (TWOPI / ELLIPSE_SEG_COUNT)
#define GISMO_PIXEL_WIDTH 4
#define GISMO_BOX_WIDTH   4
#define MAX_MOVE_LEN      10
#define FILL_MASK         0xAAAAAAAA
#define GIZMO_METER       0.01
#define MAX_TRACE_DIST    1000.0


void GizmoEventFilter::handleKeyPress(int dkey, int modif)
{
  if (moveStarted && dkey == HumanInput::DKEY_ESCAPE)
  {
    endGizmo(false);
    return;
  }

  if (DAEDITOR4.getCurEH())
    DAEDITOR4.getCurEH()->handleKeyPress(dkey, modif);
}


bool GizmoEventFilter::handleMouseMove(int x, int y, bool inside, int buttons, int key_modif)
{
  if (gizmo.prevMode != (IDaEditor4Engine::GIZMO_MASK_Mode & gizmo.type))
  {
    gizmo.prevMode = gizmo.type;
    gizmo.selected = 0;
  }

  if (moveStarted)
  {
    if (!gizmo.selected)
      return true;

    const float maxDownscale = 0.05;
    mousePos = Point2(x, y);

    const Point3 gcPos = gizmo.client->getPt();

    Point2 gPos;
    DAEDITOR4.worldToClient(gcPos, gPos);

    Point2 delta = mousePos - gPos + gizmoDelta;

    movedDelta = Point3(0, 0, 0);
    float len2;

    Point3 ax, ay, az;
    if (!gizmo.client->getAxes(ax, ay, az))
    {
      ax = Point3(1, 0, 0);
      ay = Point3(0, 1, 0);
      az = Point3(0, 0, 1);
    }

    switch (gizmo.type)
    {
      case IDaEditor4Engine::MODE_move:
      {
        Point3 mouseWorld, mouseDir;
        Point2 startPosScreen;
        DAEDITOR4.worldToClient(startPos, startPosScreen);
        DAEDITOR4.clientToWorld(mousePos - (startPos2d - startPosScreen), mouseWorld, mouseDir);

        if (planeNormal == Point3(0, 0, 0))
        {
          planeNormal = ax;

          switch (gizmo.selected & 7)
          {
            case AXIS_X | AXIS_Y | AXIS_Z:
            case AXIS_X | AXIS_Y: planeNormal = az; break;
            case AXIS_X | AXIS_Z: planeNormal = ay; break;
            case AXIS_Y | AXIS_Z: planeNormal = ax; break;
            case AXIS_X: planeNormal = fabsf(ay * mouseDir) > fabsf(az * mouseDir) ? ay : az; break;
            case AXIS_Y: planeNormal = fabsf(ax * mouseDir) > fabsf(az * mouseDir) ? ax : az; break;
            case AXIS_Z: planeNormal = fabsf(ax * mouseDir) > fabsf(ay * mouseDir) ? ax : ay; break;
          }
        }

        if (fabsf(planeNormal * mouseDir) > 0.05)
        {
          movedDelta = (planeNormal * (gcPos - mouseWorld) / (planeNormal * mouseDir)) * mouseDir + mouseWorld;

          movedDelta -= gcPos;
        }

        switch (gizmo.selected & 7)
        {
          case AXIS_X: movedDelta = (movedDelta * ax) * ax; break;
          case AXIS_Y: movedDelta = (movedDelta * ay) * ay; break;
          case AXIS_Z: movedDelta = (movedDelta * az) * az; break;
        }

        if ((gcPos + movedDelta - mouseWorld) * mouseDir < 0)
          movedDelta = Point3(0, 0, 0);

        if (DAEDITOR4.getMoveSnap())
          movedDelta = DAEDITOR4.snapToGrid(gcPos + movedDelta) - gcPos;

        break;
      }

      case IDaEditor4Engine::MODE_movesurf: surfaceMove(x, y, gcPos, movedDelta); break;

      case IDaEditor4Engine::MODE_scale:
      {
        movedDelta = Point3(1, 1, 1);
        if (gizmo.selected & AXIS_X)
        {
          len2 = lengthSq(s_vp.ax);
          if (len2 > 1e-5)
            movedDelta.x += s_vp.ax * delta / len2;
        }

        if (gizmo.selected & AXIS_Y)
        {
          len2 = lengthSq(s_vp.ay);
          if (len2 > 1e-5)
            movedDelta.y += s_vp.ay * delta / len2;
        }

        if (gizmo.selected & AXIS_Z)
        {
          len2 = lengthSq(s_vp.az);
          if (len2 > 1e-5)
            movedDelta.z += s_vp.az * delta / len2;
        }

        if (movedDelta.x < maxDownscale)
          movedDelta.x = maxDownscale;
        if (movedDelta.y < maxDownscale)
          movedDelta.y = maxDownscale;
        if (movedDelta.z < maxDownscale)
          movedDelta.z = maxDownscale;

        float dMax;

        switch (gizmo.selected)
        {
          case AXIS_X | AXIS_Y:
            dMax = max(movedDelta.x, movedDelta.y);
            movedDelta.x = dMax;
            movedDelta.y = dMax;
            break;
          case AXIS_Y | AXIS_Z:
            dMax = max(movedDelta.y, movedDelta.z);
            movedDelta.y = dMax;
            movedDelta.z = dMax;
            break;
          case AXIS_X | AXIS_Z:
            dMax = max(movedDelta.z, movedDelta.x);
            movedDelta.z = dMax;
            movedDelta.x = dMax;
            break;
          case AXIS_X | AXIS_Y | AXIS_Z: movedDelta.x = movedDelta.z = movedDelta.y; break;
        }

        if (DAEDITOR4.getScaleSnap())
          movedDelta = DAEDITOR4.snapToScale(movedDelta);

        scale = movedDelta;
        break;
      }

      case IDaEditor4Engine::MODE_rotate:
      {
        movedDelta = Point3(0, 0, 0);
        len2 = (delta + Point2(deltaX, deltaY)) * rotateDir / AXIS_LEN_PIX;

        rotAngle = len2 + startRotAngle;

        if (gizmo.selected & AXIS_X)
          movedDelta.x = len2;

        if (gizmo.selected & AXIS_Y)
          movedDelta.y = len2;

        if (gizmo.selected & AXIS_Z)
          movedDelta.z = len2;

        // snap correction (if presented)
        if (DAEDITOR4.getRotateSnap())
          movedDelta = DAEDITOR4.snapToAngle(movedDelta);

        // wrap cursor

        DAEDITOR4.clientToScreen(x, y);

        /*
        int p1 = x;
        int p2 = y;

        ::cursor_wrap(p1, p2);

        if (x != p1)
          deltaX += x - p1;

        if (y != p2)
          deltaY += y - p2;
        */
        break;
      }

      default: break;
    }

    gizmo.client->changed(movedDelta);
    return true;
  }

  if (inside)
  {
    const int lastOver = gizmo.over;
    checkGizmo(x, y);
  }

  if (DAEDITOR4.getCurEH())
    return DAEDITOR4.getCurEH()->handleMouseMove(x, y, inside, buttons, key_modif);

  return false;
}


void GizmoEventFilter::surfaceMove(int x, int y, const Point3 &pos, Point3 &move_delta)
{
  Point3 world;
  Point3 dir;
  float dist = MAX_TRACE_DIST;

  int mx = 0, my = 0;

  const int dx = mx - x;
  const int dy = my - y;

  Point2 mouse = Point2(x, y) + gizmoDelta;

  switch (gizmo.selected)
  {
    case AXIS_X | AXIS_Z: break;

    case AXIS_X:
    case AXIS_Z:
    {
      const Point2 dir = normalize(gizmo.selected == AXIS_X ? vp.ax : vp.az);
      const float proj = (Point2(x, y) - vp.center) * dir;

      mouse = vp.center + proj * dir;

      break;
    }

    default: debug("Unknown movement."); break;
  }

  DAEDITOR4.clientToWorld(mouse, world, dir);

  if (DAEDITOR4.traceRayEx(world, dir, dist, gizmo.exclude))
    move_delta = world + dir * dist;
  else
    move_delta = pos;
}


void GizmoEventFilter::startGizmo(int x, int y)
{
  scale = Point3(1, 1, 1);
  moveStarted = true;
  movedDelta.set(0, 0, 0);
  planeNormal = Point3(0, 0, 0);
  deltaX = deltaY = 0;

  startPos = gizmo.client->getPt();
  startPos2d = Point2(x, y);

  if ((gizmo.type & IDaEditor4Engine::MODE_rotate) && (gizmo.selected & 7))
  {
    startRotAngle = 0;

    Point2 pos, dx, dy;
    int delta = GISMO_PIXEL_WIDTH;
    int count = 0;
    float dist2, min_dist2 = GISMO_PIXEL_WIDTH * GISMO_PIXEL_WIDTH * 32;


    startPos2d -= vp.center;
    switch (gizmo.selected)
    {
      case AXIS_X:
        dx = vp.ay;
        dy = vp.az;
        break;
      case AXIS_Y:
        dx = vp.az;
        dy = vp.ax;
        break;
      case AXIS_Z:
        dx = vp.ax;
        dy = vp.ay;
        break;
      default: dx.set(0, 0); dy.set(0, 0);
    }

    do
    {
      pos = cos(startRotAngle) * dx + sin(startRotAngle) * dy;
      startRotAngle += PI / 190;
      ++count;

      dist2 = lengthSq(startPos2d - pos);
      if (dist2 < min_dist2)
        min_dist2 = dist2;

      if (count > 380)
      {
        delta += GISMO_PIXEL_WIDTH;
        count = 0;
        if (min_dist2 >= (GISMO_PIXEL_WIDTH * GISMO_PIXEL_WIDTH * 16) || delta >= GISMO_PIXEL_WIDTH * 4)
        {
          pos = Point2(x, y) - vp.center;
          startRotAngle = 0;
          gizmo.selected = 0;
          break;
        }
      }
    } while (dist2 > float(delta * delta));

    if (fabs(startRotAngle) >= TWOPI)
    {
      const int div = startRotAngle / TWOPI;
      startRotAngle -= TWOPI * div;
    }

    rotAngle = startRotAngle;
    startPos2d = pos + vp.center;
  }

  gizmo.client->gizmoStarted();
  gizmoDelta = vp.center - Point2(x, y);
}


void GizmoEventFilter::endGizmo(bool apply)
{
  moveStarted = false;
  scale = Point3(1, 1, 1);

  if (gizmo.type & IDaEditor4Engine::MODE_scale)
    gizmo.selected = 0;

  gizmo.client->gizmoEnded(apply);
}


bool GizmoEventFilter::handleMouseLBPress(int x, int y, bool inside, int buttons, int key_modif)
{
  if (inside)
  {
    if (checkGizmo(x, y))
    {
      gizmo.selected = gizmo.over;
      startGizmo(x, y);
      return true;
    }
    else if (gizmo.client->canStartChangeAt(x, y, gizmo.selected))
    {
      startGizmo(x, y);
      return true;
    }
  }

  if (DAEDITOR4.getCurEH())
    return DAEDITOR4.getCurEH()->handleMouseLBPress(x, y, inside, buttons, key_modif);

  return false;
}


bool GizmoEventFilter::handleMouseLBRelease(int x, int y, bool inside, int buttons, int shifts)
{
  if (moveStarted)
    endGizmo(true);

  if (DAEDITOR4.getCurEH())
    return DAEDITOR4.getCurEH()->handleMouseLBRelease(x, y, inside, buttons, shifts);

  return false;
}


bool GizmoEventFilter::handleMouseRBPress(int x, int y, bool inside, int buttons, int shifts)
{
  if (moveStarted)
  {
    endGizmo(false);
    return true;
  }

  if (DAEDITOR4.getCurEH())
    return DAEDITOR4.getCurEH()->handleMouseRBPress(x, y, inside, buttons, shifts);

  return false;
}


void GizmoEventFilter::handlePaint2D()
{
  if (DAEDITOR4.getCurEH())
    DAEDITOR4.getCurEH()->handlePaint2D();

  drawGizmo();
}


bool GizmoEventFilter::checkGizmo(int x, int y)
{
  Point2 mouse = Point2(x, y);

  if (gizmo.type != IDaEditor4Engine::MODE_rotate)
  {
    if (::is_point_in_rect(mouse, vp.center, vp.center + vp.ax, GISMO_PIXEL_WIDTH))
    {
      gizmo.over = AXIS_X;
      return true;
    }

    if (::is_point_in_rect(mouse, vp.center, vp.center + vp.ay, GISMO_PIXEL_WIDTH) && gizmo.type != IDaEditor4Engine::MODE_movesurf)
    {
      gizmo.over = AXIS_Y;
      return true;
    }

    if (::is_point_in_rect(mouse, vp.center, vp.center + vp.az, GISMO_PIXEL_WIDTH))
    {
      gizmo.over = AXIS_Z;
      return true;
    }
  }
  else
  {
    if (isPointInEllipse(vp.center, vp.ay, vp.az, GISMO_PIXEL_WIDTH * 2, mouse))
    {
      gizmo.over = AXIS_X;
      return true;
    }

    if (isPointInEllipse(vp.center, vp.az, vp.ax, GISMO_PIXEL_WIDTH * 2, mouse))
    {
      gizmo.over = AXIS_Y;
      return true;
    }

    if (isPointInEllipse(vp.center, vp.ax, vp.ay, GISMO_PIXEL_WIDTH * 2, mouse))
    {
      gizmo.over = AXIS_Z;
      return true;
    }
  }

  switch (gizmo.type)
  {
    case IDaEditor4Engine::MODE_move:
    {
      Point2 rectXY[4];
      rectXY[0] = vp.center;
      rectXY[1] = vp.center + vp.ax / 3.0;
      rectXY[2] = rectXY[1] + vp.ay / 3.0;
      rectXY[3] = vp.center + vp.ay / 3.0;

      Point2 rectYZ[4];
      rectYZ[0] = vp.center;
      rectYZ[1] = vp.center + vp.ay / 3.0;
      rectYZ[2] = rectYZ[1] + vp.az / 3.0;
      rectYZ[3] = vp.center + vp.az / 3.0;

      Point2 rectZX[4];
      rectZX[0] = vp.center;
      rectZX[1] = vp.center + vp.az / 3.0;
      rectZX[2] = rectZX[1] + vp.ax / 3.0;
      rectZX[3] = vp.center + vp.ax / 3.0;


      if (::is_point_in_rect(mouse, rectZX[1], rectZX[2], GISMO_BOX_WIDTH) ||
          ::is_point_in_rect(mouse, rectZX[3], rectZX[2], GISMO_BOX_WIDTH))
      {
        gizmo.over = AXIS_X | AXIS_Z;
        return true;
      }

      if (gizmo.selected == (AXIS_X | AXIS_Z) && ::is_point_in_conv_poly(mouse, rectZX, 4))
      {
        gizmo.over = AXIS_X | AXIS_Z;
        return true;
      }

      if (::is_point_in_rect(mouse, rectXY[1], rectXY[2], GISMO_BOX_WIDTH) ||
          ::is_point_in_rect(mouse, rectXY[3], rectXY[2], GISMO_BOX_WIDTH))
      {
        gizmo.over = AXIS_X | AXIS_Y;
        return true;
      }

      if (::is_point_in_rect(mouse, rectYZ[1], rectYZ[2], GISMO_BOX_WIDTH) ||
          ::is_point_in_rect(mouse, rectYZ[3], rectYZ[2], GISMO_BOX_WIDTH))
      {
        gizmo.over = AXIS_Y | AXIS_Z;
        return true;
      }


      if (gizmo.selected == (AXIS_X | AXIS_Y) && ::is_point_in_conv_poly(mouse, rectXY, 4))
      {
        gizmo.over = AXIS_X | AXIS_Y;
        return true;
      }

      if (gizmo.selected == (AXIS_Y | AXIS_Z) && ::is_point_in_conv_poly(mouse, rectYZ, 4))
      {
        gizmo.over = AXIS_Y | AXIS_Z;
        return true;
      }

      break;
    }

    case IDaEditor4Engine::MODE_movesurf:
    {
      Point2 rectZX[4];
      rectZX[0] = vp.center;
      rectZX[1] = vp.center + vp.az / 2.0;
      rectZX[2] = rectZX[1] + vp.ax / 2.0;
      rectZX[3] = vp.center + vp.ax / 2.0;

      if (::is_point_in_rect(mouse, rectZX[1], rectZX[2], GISMO_BOX_WIDTH) ||
          ::is_point_in_rect(mouse, rectZX[3], rectZX[2], GISMO_BOX_WIDTH))
      {
        gizmo.over = AXIS_X | AXIS_Z;
        return true;
      }

      if (::is_point_in_conv_poly(mouse, rectZX, 4))
      {
        gizmo.over = AXIS_X | AXIS_Z;
        return true;
      }

      break;
    }

    case IDaEditor4Engine::MODE_scale:
    {
      Point2 start, end;
      start = vp.center + vp.ax * 7 / 12;
      end = vp.center + vp.ay * 7 / 12;

      if (::is_point_in_rect(mouse, start, end, GISMO_BOX_WIDTH * 2))
      {
        gizmo.over = AXIS_X | AXIS_Y;
        return true;
      }

      start = vp.center + vp.ay * 7 / 12;
      end = vp.center + vp.az * 7 / 12;

      if (::is_point_in_rect(mouse, start, end, GISMO_BOX_WIDTH * 2))
      {
        gizmo.over = AXIS_Y | AXIS_Z;
        return true;
      }

      start = vp.center + vp.az * 7 / 12;
      end = vp.center + vp.ax * 7 / 12;

      if (::is_point_in_rect(mouse, start, end, GISMO_BOX_WIDTH * 2))
      {
        gizmo.over = AXIS_X | AXIS_Z;
        return true;
      }

      Point2 triangle[3];
      triangle[0] = vp.center + vp.ax / 2;
      triangle[1] = vp.center + vp.ay / 2;
      triangle[2] = vp.center + vp.az / 2;

      if (::is_point_in_triangle(mouse, triangle))
      {
        gizmo.over = AXIS_X | AXIS_Y | AXIS_Z;
        return true;
      }

      break;
    }

    default: break;
  }

  gizmo.over = 0;
  return false;
}


void GizmoEventFilter::recalcViewportGizmo()
{
  if (!gizmo.client || gizmo.type == IDaEditor4Engine::MODE_none)
    return;

  Point2 center, ax, ay, az, pt2;
  Point3 ax1, ay1, az1;

  if (!gizmo.client->getAxes(ax1, ay1, az1))
  {
    ax1 = Point3(1, 0, 0);
    ay1 = Point3(0, 1, 0);
    az1 = Point3(0, 0, 1);
  }

  const Point3 pt = gizmo.client->getPt();

  DAEDITOR4.worldToClient(pt, center);

  DAEDITOR4.worldToClient(pt + GIZMO_METER * ax1, pt2);
  ax = (pt2 - center);
  DAEDITOR4.worldToClient(pt + GIZMO_METER * ay1, pt2);
  ay = (pt2 - center);
  DAEDITOR4.worldToClient(pt + GIZMO_METER * az1, pt2);
  az = (pt2 - center);

  if (gizmo.type & IDaEditor4Engine::MODE_scale)
    correctScaleGizmo(ax1, ay1, az1, ax, ay, az);

  float xl = length(ax);
  float yl = length(ay);
  float zl = length(az);

  float maxlen = max(max(xl, yl), zl);

  if (xl == 0)
    xl = 1e-5;
  if (yl == 0)
    yl = 1e-5;
  if (zl == 0)
    zl = 1e-5;

  vp.center = center;
  vp.ax = ax * (AXIS_LEN_PIX / maxlen);
  vp.ay = ay * (AXIS_LEN_PIX / maxlen);
  vp.az = az * (AXIS_LEN_PIX / maxlen);

  s_vp = vp;
  s_vp.ax = ax * (AXIS_LEN_PIX / xl);
  s_vp.ay = ay * (AXIS_LEN_PIX / yl);
  s_vp.az = az * (AXIS_LEN_PIX / zl);
}


void GizmoEventFilter::drawGizmoLine(const Point2 &from, const Point2 &delta, float offset)
{
  StdGuiRender::draw_line(from.x + delta.x * offset, from.y + delta.y * offset, from.x + delta.x, from.y + delta.y);
}


void GizmoEventFilter::drawGizmoLineFromTo(const Point2 &from, const Point2 &to, float offset)
{
  G_UNUSED(offset);
  StdGuiRender::draw_line(from, to);
}


void GizmoEventFilter::drawGizmoEllipse(const Point2 &center, const Point2 &a, const Point2 &b, float start, float end)
{
  Point2 prev = center + a * cos(start) + b * sin(start);
  Point2 next;

  for (float t = start; t < end; t += ELLIPSE_STEP) //-V1034
  {
    next = center + a * cos(t) + b * sin(t);
    StdGuiRender::draw_line(prev, next);

    prev = next;
  }

  next = center + a * cos(end) + b * sin(end);
  StdGuiRender::draw_line(prev, next);
}


void GizmoEventFilter::fillGizmoEllipse(const Point2 &center, const Point2 &a, const Point2 &b, E3DCOLOR color, int fp, float start,
  float end)
{
  G_UNUSED(fp);
  Point2 points[ELLIPSE_SEG_COUNT + 2];
  int cur = 0;

  drawGizmoLineFromTo(center, center); // mega hack for StdGuiRender!!!

  for (float t = start; t < end; t += ELLIPSE_STEP) //-V1034
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


static inline void swap(float &a, float &b)
{
  float tmp = a;
  a = b;
  b = tmp;
}


void GizmoEventFilter::drawGizmoArrow(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, float offset)
{
  Point2 tri[3];
  Point2 norm = normalize(delta);
  ::swap(norm.x, norm.y);
  norm.y = -norm.y;

  tri[0] = from + delta;
  tri[1] = from + delta * 0.8 + norm * 3;
  tri[2] = from + delta * 0.8 - norm * 3;

  drawGizmoLine(from, delta * 0.9, offset);
  StdGuiRender::draw_fill_poly_fan(tri, 3, col_fill);
}


void GizmoEventFilter::drawGizmoArrowScale(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, float offset)
{
  Point2 tri[4];
  Point2 norm = normalize(delta);
  ::swap(norm.x, norm.y);
  norm.y = -norm.y;

  tri[0] = from + delta - norm * 3;
  tri[1] = from + delta + norm * 3;
  tri[2] = from + delta * 0.9 + norm * 3;
  tri[3] = from + delta * 0.9 - norm * 3;

  drawGizmoLine(from, delta * 0.95, offset);
  StdGuiRender::draw_fill_poly_fan(tri, 4, col_fill);
}


void GizmoEventFilter::drawGizmoQuad(const Point2 &from, const Point2 &delta1, const Point2 &delta2, E3DCOLOR col1, E3DCOLOR col2,
  E3DCOLOR col_sel, bool sel, float div)
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


void GizmoEventFilter::drawMoveGizmo(int sel)
{
  StdGuiRender::set_color((sel & AXIS_X) ? COLOR_YELLOW : COLOR_LTRED);
  drawGizmoArrow(vp.center, vp.ax, COLOR_LTRED);

  StdGuiRender::set_color((sel & AXIS_Y) ? COLOR_YELLOW : COLOR_LTGREEN);
  drawGizmoArrow(vp.center, vp.ay, COLOR_LTGREEN);

  StdGuiRender::set_color((sel & AXIS_Z) ? COLOR_YELLOW : COLOR_LTBLUE);
  drawGizmoArrow(vp.center, vp.az, COLOR_LTBLUE);

  drawGizmoQuad(vp.center, vp.az, vp.ax, COLOR_LTBLUE, COLOR_LTRED, COLOR_YELLOW, (sel & 5) == 5);
  drawGizmoQuad(vp.center, vp.ax, vp.ay, COLOR_LTRED, COLOR_LTGREEN, COLOR_YELLOW, (sel & 3) == 3);
  drawGizmoQuad(vp.center, vp.ay, vp.az, COLOR_LTGREEN, COLOR_LTBLUE, COLOR_YELLOW, (sel & 6) == 6);
}


void GizmoEventFilter::drawSurfMoveGizmo(int sel)
{
  StdGuiRender::set_color((sel & AXIS_X) ? COLOR_YELLOW : COLOR_LTRED);
  drawGizmoArrow(vp.center, vp.ax, COLOR_LTRED);

  StdGuiRender::set_color((sel & AXIS_Z) ? COLOR_YELLOW : COLOR_LTBLUE);
  drawGizmoArrow(vp.center, vp.az, COLOR_LTBLUE);

  drawGizmoQuad(vp.center, vp.az, vp.ax, COLOR_LTBLUE, COLOR_LTRED, COLOR_YELLOW, (sel & 5) == 5, 2.0);
}


void GizmoEventFilter::drawScaleGizmo(int sel)
{
  Point2 center = vp.center;
  Point2 ax = vp.ax + vp.ax * (scale.x - 1);
  Point2 ay = vp.ay + vp.ay * (scale.y - 1);
  Point2 az = vp.az + vp.az * (scale.z - 1);

  Point2 points[4];

  if (sel)
  {
    Point2 start, end;

    StdGuiRender::set_color(sel == 3 ? COLOR_YELLOW : COLOR_LTRED);

    start = center + ax / 3 * 2;
    end = center + ay / 3 * 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    StdGuiRender::set_color(sel == 5 ? COLOR_YELLOW : COLOR_LTRED);

    end = center + az / 3 * 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    StdGuiRender::set_color(sel == 3 ? COLOR_YELLOW : COLOR_LTRED);

    start = center + ax / 2;
    end = center + ay / 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    StdGuiRender::set_color(sel == 5 ? COLOR_YELLOW : COLOR_LTRED);

    end = center + az / 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    StdGuiRender::set_color(sel == 3 ? COLOR_YELLOW : COLOR_LTGREEN);

    start = center + ay / 3 * 2;
    end = center + ax / 3 * 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    StdGuiRender::set_color(sel == 6 ? COLOR_YELLOW : COLOR_LTGREEN);

    end = center + az / 3 * 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    StdGuiRender::set_color(sel == 3 ? COLOR_YELLOW : COLOR_LTGREEN);

    start = center + ay / 2;
    end = center + ax / 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    StdGuiRender::set_color(sel == 6 ? COLOR_YELLOW : COLOR_LTGREEN);

    end = center + az / 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    StdGuiRender::set_color(sel == 5 ? COLOR_YELLOW : COLOR_LTBLUE);

    start = center + az / 3 * 2;
    end = center + ax / 3 * 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    StdGuiRender::set_color(sel == 6 ? COLOR_YELLOW : COLOR_LTBLUE);

    end = center + ay / 3 * 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    StdGuiRender::set_color(sel == 5 ? COLOR_YELLOW : COLOR_LTBLUE);

    start = center + az / 2;
    end = center + ax / 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    StdGuiRender::set_color(sel == 6 ? COLOR_YELLOW : COLOR_LTBLUE);

    end = center + ay / 2;
    drawGizmoLineFromTo(start, start + (end - start) / 2);

    // dc.selectFillColor(PC_Yellow);
    // dc.selectFillPattern(FILL_MASK);

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
  }
  else
  {
    StdGuiRender::set_color(COLOR_YELLOW);

    drawGizmoLineFromTo(center + ax / 3 * 2, center + ay / 3 * 2);
    drawGizmoLineFromTo(center + ax / 2, center + ay / 2);

    drawGizmoLineFromTo(center + ay / 3 * 2, center + az / 3 * 2);
    drawGizmoLineFromTo(center + ay / 2, center + az / 2);

    drawGizmoLineFromTo(center + az / 3 * 2, center + ax / 3 * 2);
    drawGizmoLineFromTo(center + az / 2, center + ax / 2);

    points[0] = center + ax / 2;
    points[1] = center + ay / 2;
    points[2] = center + az / 2;

    // dc.selectFillPattern(FILL_MASK);
    StdGuiRender::draw_fill_poly_fan(points, 3, COLOR_YELLOW);
  }

  StdGuiRender::set_color((sel & 1) ? COLOR_YELLOW : COLOR_LTRED);
  drawGizmoArrowScale(center, ax, COLOR_LTRED);

  StdGuiRender::set_color((sel & 2) ? COLOR_YELLOW : COLOR_LTGREEN);
  drawGizmoArrowScale(center, ay, COLOR_LTGREEN);

  StdGuiRender::set_color((sel & 4) ? COLOR_YELLOW : COLOR_LTBLUE);
  drawGizmoArrowScale(center, az, COLOR_LTBLUE);
}


void GizmoEventFilter::drawRotateGizmo(int sel)
{
  if (moveStarted && sel)
  {
    Point2 a, b;
    E3DCOLOR col1, col2;

    switch (sel)
    {
      case AXIS_X:
        a = vp.ay;
        b = vp.az;
        col1 = COLOR_LTRED;
        col2 = COLOR_RED;
        break;
      case AXIS_Y:
        a = vp.az;
        b = vp.ax;
        col1 = COLOR_LTGREEN;
        col2 = COLOR_GREEN;
        break;
      case AXIS_Z:
        a = vp.ax;
        b = vp.ay;
        col1 = COLOR_LTBLUE;
        col2 = COLOR_BLUE;
        break;
    }

    const float deltaAngle = rotAngle - startRotAngle;
    const float rotAbs = fabs(deltaAngle);
    const bool rotWorld = DAEDITOR4.getGizmoBasisType() == IDaEditor4Engine::BASIS_world;

    float angle = rotWorld ? rotAngle : startRotAngle - deltaAngle;

    if (rotAbs < TWOPI)
    {
      fillGizmoEllipse(vp.center, a, b, col1, FILL_MASK, min(startRotAngle, angle), max(startRotAngle, angle));
    }
    else if (rotAbs < TWOPI * 2)
    {
      const int divizer = (deltaAngle) / TWOPI;
      const float endRot = rotAngle - TWOPI * divizer;

      angle = rotWorld ? endRot : startRotAngle - (endRot - startRotAngle);

      const float start = min(startRotAngle, angle);
      const float end = max(startRotAngle, angle);

      fillGizmoEllipse(vp.center, a, b, col2, 0xFFFFFFFF, start, end);
      fillGizmoEllipse(vp.center, a, b, col1, FILL_MASK, end - TWOPI, start);
    }
    else
      fillGizmoEllipse(vp.center, a, b, col2, 0xFFFFFFFF);

    StdGuiRender::set_color(rotAbs < TWOPI ? col1 : col2);
    drawGizmoLineFromTo(vp.center, startPos2d);

    angle = rotWorld ? rotAngle : startRotAngle;

    Point2 end = vp.center + a * cos(angle) + b * sin(angle);
    drawGizmoLineFromTo(vp.center, end);

    StdGuiRender::set_font(0);
    StdGuiRender::set_color(COLOR_YELLOW);

    StdGuiRender::end_raw_layer();
    StdGuiRender::draw_strf_to(vp.center.x - 80, vp.center.y - AXIS_LEN_PIX - 40, "[%.2f, %.2f, %.2f]", RadToDeg(movedDelta.x),
      RadToDeg(movedDelta.y), RadToDeg(movedDelta.z));
    StdGuiRender::set_texture(BAD_TEXTUREID);
    StdGuiRender::start_raw_layer();
  }

  StdGuiRender::set_color((sel == 1) ? COLOR_YELLOW : COLOR_RED);
  drawGizmoEllipse(vp.center, vp.ay, vp.az);

  StdGuiRender::set_color((sel == 2) ? COLOR_YELLOW : COLOR_LTGREEN);
  drawGizmoEllipse(vp.center, vp.az, vp.ax);

  StdGuiRender::set_color((sel == 4) ? COLOR_YELLOW : COLOR_LTBLUE);
  drawGizmoEllipse(vp.center, vp.ax, vp.ay);
}


void GizmoEventFilter::drawGizmo()
{
  if (!gizmo.client || gizmo.type == IDaEditor4Engine::MODE_none)
    return;

  recalcViewportGizmo();

  int sel = gizmo.over ? gizmo.over : gizmo.selected;

  Point2 screen;
  if (!DAEDITOR4.worldToClient(gizmo.client->getPt(), screen))
    return;

  StdGuiRender::set_texture(BAD_TEXTUREID);
  StdGuiRender::start_raw_layer();
  switch (gizmo.type)
  {
    case IDaEditor4Engine::MODE_move: drawMoveGizmo(sel); break;

    case IDaEditor4Engine::MODE_scale: drawScaleGizmo(sel); break;

    case IDaEditor4Engine::MODE_rotate: drawRotateGizmo(sel); break;

    case IDaEditor4Engine::MODE_movesurf: drawSurfMoveGizmo(sel); break;

    default: debug("FATAL: gizmo.type == 0x%X", gizmo.type); G_ASSERT(0);
  }
  StdGuiRender::end_raw_layer();
}


bool GizmoEventFilter::isPointInEllipse(const Point2 &center, const Point2 &a, const Point2 &b, float width, const Point2 &point,
  float start, float end)
{
  Point2 prev = center + a * cos(start) + b * sin(start);
  Point2 next;

  for (float t = start; t < end; t += ELLIPSE_STEP) //-V1034
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


void GizmoEventFilter::correctScaleGizmo(const Point3 &x_dir, const Point3 &y_dir, const Point3 &z_dir, Point2 &ax, Point2 &ay,
  Point2 &az)
{
  TMatrix tm;
  DAEDITOR4.getCameraTransform(tm);
  tm = inverse(tm);

  const Point3 camDir = Point3(tm.m[0][2], tm.m[1][2], tm.m[2][2]);

  if (x_dir * camDir > 0)
    ax = -ax;

  if (y_dir * camDir > 0)
    ay = -ay;

  if (z_dir * camDir > 0)
    az = -az;
}
