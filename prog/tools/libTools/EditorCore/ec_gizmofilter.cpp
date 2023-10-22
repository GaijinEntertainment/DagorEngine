#include <EditorCore/ec_gizmofilter.h>
#include <EditorCore/captureCursor.h>

#include <stdlib.h>

#include <debug/dag_debug.h>
#include <3d/dag_render.h>
#include <math/dag_math2d.h>

#include <gui/dag_stdGuiRenderEx.h>
#include <winGuiWrapper/wgw_input.h>

using hdpi::_pxS;

#define AXIS_LEN_PIX      _pxS(100)
#define ELLIPSE_SEG_COUNT 30
#define ELLIPSE_STEP      (TWOPI / ELLIPSE_SEG_COUNT)
#define GISMO_PIXEL_WIDTH _pxS(4)
#define GISMO_BOX_WIDTH   _pxS(4)
#define FILL_MASK         0xAAAAAAAA
#define GIZMO_METER       0.01
#define MAX_TRACE_DIST    1000.0


//==============================================================================
GizmoEventFilter::GizmoEventFilter(GeneralEditorData &ged_, const GridObject &grid_) :
  ged(ged_), grid(grid_), moveStarted(false), scale(1, 1, 1), deltaX(0), deltaY(0)
{
  startPos2d.reserve(16);
  vp.reserve(16);
  s_vp.reserve(16);
  memset(&gizmo, 0, sizeof(gizmo));
}


//==============================================================================
void GizmoEventFilter::correctCursorInSurfMove(const Point3 &delta)
{
  IGenViewportWnd *wnd = IEditorCoreEngine::get()->getCurrentViewport();

  Point3 gizmoPos = gizmo.client->getPt();
  Point2 truePos, deltaPos;

  wnd->worldToClient(gizmoPos, truePos);
  wnd->worldToClient(gizmoPos + delta, deltaPos);

  Point2 dp = truePos - deltaPos;

  /*
  ::_ctl_InpDev.setPosition(
    ::_ctl_InpDev.getCursorX() + dp.x, ::_ctl_InpDev.getCursorY() + dp.y);
  */
}


//==============================================================================
void GizmoEventFilter::handleKeyPress(IGenViewportWnd *wnd, int vk, int modif)
{
  if (moveStarted && vk == wingw::V_ESC)
  {
    endGizmo(false);
    return;
  }

  if (ged.curEH)
    ged.curEH->handleKeyPress(wnd, vk, modif);
}


//==============================================================================
bool GizmoEventFilter::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (gizmo.prevMode != (IEditorCoreEngine::GIZMO_MASK_Mode & gizmo.type))
  {
    gizmo.prevMode = gizmo.type;
    gizmo.selected = 0;
  }

  if (moveStarted)
  {
    if (!wnd->isActive() || !gizmo.selected)
      return true;

    const real maxDownscale = 0.05;
    const int index = ged.findViewportIndex(wnd);
    mousePos = Point2(x, y);

    const Point3 gcPos = gizmo.client->getPt();

    Point2 gPos;
    wnd->worldToClient(gcPos, gPos);

    Point2 delta = mousePos - gPos + gizmoDelta;

    movedDelta = Point3(0, 0, 0);
    real len2;

    Point3 ax, ay, az;
    if (!gizmo.client->getAxes(ax, ay, az))
    {
      ax = Point3(1, 0, 0);
      ay = Point3(0, 1, 0);
      az = Point3(0, 0, 1);
    }

    switch (gizmo.type)
    {
      case IEditorCoreEngine::MODE_Move:
      {
        Point3 mouseWorld, mouseDir;
        Point2 startPosScreen;
        wnd->worldToClient(startPos, startPosScreen);
        wnd->clientToWorld(mousePos - (startPos2d[index] - startPosScreen), mouseWorld, mouseDir);

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

        if (grid.getMoveSnap())
          movedDelta = grid.snapToGrid(gcPos + movedDelta) - gcPos;

        break;
      }

      case IEditorCoreEngine::MODE_MoveSurface: surfaceMove(wnd, x, y, index, gcPos, movedDelta); break;

      case IEditorCoreEngine::MODE_Scale:
        movedDelta = Point3(1, 1, 1);
        if (gizmo.selected & AXIS_X)
        {
          len2 = lengthSq(s_vp[index].ax);
          if (len2 > 1e-5)
            movedDelta.x += s_vp[index].ax * delta / len2;
        }

        if (gizmo.selected & AXIS_Y)
        {
          len2 = lengthSq(s_vp[index].ay);
          if (len2 > 1e-5)
            movedDelta.y += s_vp[index].ay * delta / len2;
        }

        if (gizmo.selected & AXIS_Z)
        {
          len2 = lengthSq(s_vp[index].az);
          if (len2 > 1e-5)
            movedDelta.z += s_vp[index].az * delta / len2;
        }

        if (movedDelta.x < maxDownscale)
          movedDelta.x = maxDownscale;
        if (movedDelta.y < maxDownscale)
          movedDelta.y = maxDownscale;
        if (movedDelta.z < maxDownscale)
          movedDelta.z = maxDownscale;

        real max;

        switch (gizmo.selected)
        {
          case AXIS_X | AXIS_Y:
            max = __max(movedDelta.x, movedDelta.y);
            movedDelta.x = max;
            movedDelta.y = max;
            break;
          case AXIS_Y | AXIS_Z:
            max = __max(movedDelta.y, movedDelta.z);
            movedDelta.y = max;
            movedDelta.z = max;
            break;
          case AXIS_X | AXIS_Z:
            max = __max(movedDelta.z, movedDelta.x);
            movedDelta.z = max;
            movedDelta.x = max;
            break;
          case AXIS_X | AXIS_Y | AXIS_Z: movedDelta.x = movedDelta.z = movedDelta.y; break;
        }

        if (grid.getScaleSnap())
          movedDelta = grid.snapToScale(movedDelta);

        scale = movedDelta;
        break;

      case IEditorCoreEngine::MODE_Rotate:
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
        if (grid.getRotateSnap())
          movedDelta = grid.snapToAngle(movedDelta);

        // wrap cursor

        wnd->clientToScreen(x, y);

        int p1 = x;
        int p2 = y;

        ::cursor_wrap(p1, p2);

        if (x != p1)
          deltaX += x - p1;

        if (y != p2)
          deltaY += y - p2;
    }

    gizmo.client->changed(movedDelta);
    repaint();

    return true;
  }

  if (wnd->isActive() && inside)
  {
    const int lastOver = gizmo.over;
    checkGizmo(wnd, x, y);

    if (lastOver != gizmo.over)
      repaint();
  }

  if (ged.curEH)
    return ged.curEH->handleMouseMove(wnd, x, y, inside, buttons, key_modif);

  return false;
}


//==============================================================================
void GizmoEventFilter::surfaceMove(IGenViewportWnd *wnd, int x, int y, int vp_i, const Point3 &pos, Point3 &move_delta)
{
  Point3 world;
  Point3 dir;
  real dist = MAX_TRACE_DIST;

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
      const Point2 dir = normalize(gizmo.selected == AXIS_X ? vp[vp_i].ax : vp[vp_i].az);
      const real proj = (Point2(x, y) - vp[vp_i].center) * dir;

      mouse = vp[vp_i].center + proj * dir;

      break;
    }

    default: debug("Unknown movement."); break;
  }

  wnd->clientToWorld(mouse, world, dir);

  bool usesRi = getGizmoClient()->usesRendinstPlacement();
  Point3 norm = Point3(0, 1, 0);
  if (usesRi)
  {
    getGizmoClient()->setCollisionIgnoredOnSelection();
    IEditorCoreEngine::get()->setupColliderParams(1, BBox3());
  }
  bool traceSuccess = IEditorCoreEngine::get()->traceRay(world, dir, dist, &norm);
  if (usesRi)
  {
    getGizmoClient()->saveNormalOnSelection(norm);
    IEditorCoreEngine::get()->setupColliderParams(0, BBox3());
    getGizmoClient()->resetCollisionIgnoredOnSelection();
  }
  else
    getGizmoClient()->saveNormalOnSelection(Point3(0, 1, 0));
  if (traceSuccess)
    move_delta = world + dir * dist;
}


//==============================================================================
void GizmoEventFilter::startGizmo(IGenViewportWnd *wnd, int x, int y)
{
  scale = Point3(1, 1, 1);
  moveStarted = true;
  movedDelta.set(0, 0, 0);
  planeNormal = Point3(0, 0, 0);
  deltaX = deltaY = 0;

  int vp_i = ged.findViewportIndex(wnd);

  startPos2d.resize(ged.getViewportCount());

  startPos = gizmo.client->getPt();
  startPos2d[vp_i] = Point2(x, y);

  if ((gizmo.type & IEditorCoreEngine::MODE_Rotate) && (gizmo.selected & 7))
  {
    startRotAngle = 0;

    Point2 pos, dx, dy;
    int delta = GISMO_PIXEL_WIDTH;
    int count = 0;
    float dist2, min_dist2 = float(GISMO_PIXEL_WIDTH * GISMO_PIXEL_WIDTH * 32);


    startPos2d[vp_i] -= vp[vp_i].center;
    switch (gizmo.selected)
    {
      case AXIS_X:
        dx = vp[vp_i].ay;
        dy = vp[vp_i].az;
        break;
      case AXIS_Y:
        dx = vp[vp_i].az;
        dy = vp[vp_i].ax;
        break;
      case AXIS_Z:
        dx = vp[vp_i].ax;
        dy = vp[vp_i].ay;
        break;
    }

    do
    {
      pos = cos(startRotAngle) * dx + sin(startRotAngle) * dy;
      startRotAngle += PI / 190;
      ++count;

      dist2 = lengthSq(startPos2d[vp_i] - pos);
      if (dist2 < min_dist2)
        min_dist2 = dist2;

      if (count > 380)
      {
        delta += GISMO_PIXEL_WIDTH;
        count = 0;
        if (min_dist2 >= float(GISMO_PIXEL_WIDTH * GISMO_PIXEL_WIDTH * 16) || delta >= GISMO_PIXEL_WIDTH * 4)
        {
          pos = Point2(x, y) - vp[vp_i].center;
          startRotAngle = 0;
          gizmo.selected = 0;
          break;
        }
      }
    } while (dist2 > float(delta) * delta);

    if (fabs(startRotAngle) >= TWOPI)
    {
      const int div = startRotAngle / TWOPI;
      startRotAngle -= TWOPI * div;
    }

    rotAngle = startRotAngle;
    startPos2d[vp_i] = pos + vp[vp_i].center;

    for (int i = 0; i < ged.getViewportCount(); ++i)
      if (i != vp_i)
        switch (gizmo.selected)
        {
          case AXIS_X: startPos2d[i] = vp[i].center + cos(startRotAngle) * vp[i].ay + sin(startRotAngle) * vp[i].az; break;
          case AXIS_Y: startPos2d[i] = vp[i].center + cos(startRotAngle) * vp[i].az + sin(startRotAngle) * vp[i].ax; break;
          case AXIS_Z: startPos2d[i] = vp[i].center + cos(startRotAngle) * vp[i].ax + sin(startRotAngle) * vp[i].az; break;
        }
  }

  gizmo.client->gizmoStarted();

  gizmoDelta = vp[vp_i].center - Point2(x, y);
}


//==============================================================================
void GizmoEventFilter::endGizmo(bool apply)
{
  moveStarted = false;
  gizmo.client->gizmoEnded(apply);
  scale = Point3(1, 1, 1);

  if (gizmo.type & IEditorCoreEngine::MODE_Scale)
    gizmo.selected = 0;

  repaint();
}


//==============================================================================
bool GizmoEventFilter::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (wnd->isActive() && inside)
  {
    if (checkGizmo(wnd, x, y))
    {
      gizmo.selected = gizmo.over;
      startGizmo(wnd, x, y);
      return true;
    }
    else if (gizmo.client->canStartChangeAt(wnd, x, y, gizmo.selected))
    {
      startGizmo(wnd, x, y);
      return true;
    }
  }

  if (ged.curEH)
    return ged.curEH->handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);

  return false;
}


//==============================================================================
bool GizmoEventFilter::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int shifts)
{
  if (moveStarted)
    endGizmo(true);

  if (ged.curEH)
    return ged.curEH->handleMouseLBRelease(wnd, x, y, inside, buttons, shifts);

  return false;
}


//==============================================================================
bool GizmoEventFilter::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int shifts)
{
  if (moveStarted)
  {
    endGizmo(false);
    return true;
  }

  if (ged.curEH)
    return ged.curEH->handleMouseRBPress(wnd, x, y, inside, buttons, shifts);

  return false;
}


//==============================================================================
void GizmoEventFilter::handleViewportPaint(IGenViewportWnd *wnd)
{
  if (ged.curEH)
    ged.curEH->handleViewportPaint(wnd);

  drawGizmo(wnd);
}


//==============================================================================
void GizmoEventFilter::handleViewChange(IGenViewportWnd *wnd) { wnd->redrawClientRect(); }


//==============================================================================
bool GizmoEventFilter::checkGizmo(IGenViewportWnd *wnd, int x, int y)
{
  Point2 mouse = Point2(x, y);
  int vp_i = ged.findViewportIndex(wnd);

  if (gizmo.type != IEditorCoreEngine::MODE_Rotate)
  {
    if (::is_point_in_rect(mouse, vp[vp_i].center, vp[vp_i].center + vp[vp_i].ax, GISMO_PIXEL_WIDTH))
    {
      gizmo.over = AXIS_X;
      return true;
    }

    if (::is_point_in_rect(mouse, vp[vp_i].center, vp[vp_i].center + vp[vp_i].ay, GISMO_PIXEL_WIDTH) &&
        gizmo.type != IEditorCoreEngine::MODE_MoveSurface)
    {
      gizmo.over = AXIS_Y;
      return true;
    }

    if (::is_point_in_rect(mouse, vp[vp_i].center, vp[vp_i].center + vp[vp_i].az, GISMO_PIXEL_WIDTH))
    {
      gizmo.over = AXIS_Z;
      return true;
    }
  }
  else
  {
    if (isPointInEllipse(vp[vp_i].center, vp[vp_i].ay, vp[vp_i].az, GISMO_PIXEL_WIDTH * 2, mouse))
    {
      gizmo.over = AXIS_X;
      return true;
    }

    if (isPointInEllipse(vp[vp_i].center, vp[vp_i].az, vp[vp_i].ax, GISMO_PIXEL_WIDTH * 2, mouse))
    {
      gizmo.over = AXIS_Y;
      return true;
    }

    if (isPointInEllipse(vp[vp_i].center, vp[vp_i].ax, vp[vp_i].ay, GISMO_PIXEL_WIDTH * 2, mouse))
    {
      gizmo.over = AXIS_Z;
      return true;
    }
  }

  switch (gizmo.type)
  {
    case IEditorCoreEngine::MODE_Move:
    {
      Point2 rectXY[4];
      rectXY[0] = vp[vp_i].center;
      rectXY[1] = vp[vp_i].center + vp[vp_i].ax / 3.0;
      rectXY[2] = rectXY[1] + vp[vp_i].ay / 3.0;
      rectXY[3] = vp[vp_i].center + vp[vp_i].ay / 3.0;

      Point2 rectYZ[4];
      rectYZ[0] = vp[vp_i].center;
      rectYZ[1] = vp[vp_i].center + vp[vp_i].ay / 3.0;
      rectYZ[2] = rectYZ[1] + vp[vp_i].az / 3.0;
      rectYZ[3] = vp[vp_i].center + vp[vp_i].az / 3.0;

      Point2 rectZX[4];
      rectZX[0] = vp[vp_i].center;
      rectZX[1] = vp[vp_i].center + vp[vp_i].az / 3.0;
      rectZX[2] = rectZX[1] + vp[vp_i].ax / 3.0;
      rectZX[3] = vp[vp_i].center + vp[vp_i].ax / 3.0;


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

    case IEditorCoreEngine::MODE_MoveSurface:
    {
      Point2 rectZX[4];
      rectZX[0] = vp[vp_i].center;
      rectZX[1] = vp[vp_i].center + vp[vp_i].az / 2.0;
      rectZX[2] = rectZX[1] + vp[vp_i].ax / 2.0;
      rectZX[3] = vp[vp_i].center + vp[vp_i].ax / 2.0;

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

    case IEditorCoreEngine::MODE_Scale:
    {
      Point2 start, end;
      start = vp[vp_i].center + vp[vp_i].ax * 7 / 12;
      end = vp[vp_i].center + vp[vp_i].ay * 7 / 12;

      if (::is_point_in_rect(mouse, start, end, GISMO_BOX_WIDTH * 2))
      {
        gizmo.over = AXIS_X | AXIS_Y;
        return true;
      }

      start = vp[vp_i].center + vp[vp_i].ay * 7 / 12;
      end = vp[vp_i].center + vp[vp_i].az * 7 / 12;

      if (::is_point_in_rect(mouse, start, end, GISMO_BOX_WIDTH * 2))
      {
        gizmo.over = AXIS_Y | AXIS_Z;
        return true;
      }

      start = vp[vp_i].center + vp[vp_i].az * 7 / 12;
      end = vp[vp_i].center + vp[vp_i].ax * 7 / 12;

      if (::is_point_in_rect(mouse, start, end, GISMO_BOX_WIDTH * 2))
      {
        gizmo.over = AXIS_X | AXIS_Z;
        return true;
      }

      Point2 triangle[3];
      triangle[0] = vp[vp_i].center + vp[vp_i].ax / 2;
      triangle[1] = vp[vp_i].center + vp[vp_i].ay / 2;
      triangle[2] = vp[vp_i].center + vp[vp_i].az / 2;

      if (::is_point_in_triangle(mouse, triangle))
      {
        gizmo.over = AXIS_X | AXIS_Y | AXIS_Z;
        return true;
      }

      break;
    }
  }

  gizmo.over = 0;
  return false;
}


//==============================================================================
void GizmoEventFilter::recalcViewportGizmo()
{
  if (!gizmo.client || gizmo.type == IEditorCoreEngine::MODE_None)
    return;

  Point2 center, ax, ay, az, pt2;
  Point3 ax1, ay1, az1;

  if (!gizmo.client->getAxes(ax1, ay1, az1))
  {
    ax1 = Point3(1, 0, 0);
    ay1 = Point3(0, 1, 0);
    az1 = Point3(0, 0, 1);
  }

  vp.resize(ged.getViewportCount());
  s_vp.resize(ged.getViewportCount());

  for (int i = 0; i < ged.getViewportCount(); ++i)
  {
    const Point3 pt = gizmo.client->getPt();

    ViewportWindow *vpw = ged.getViewport(i);
    G_ASSERT(vpw);

    vpw->worldToClient(pt, center);

    vpw->worldToClient(pt + GIZMO_METER * ax1, pt2);
    ax = (pt2 - center);
    vpw->worldToClient(pt + GIZMO_METER * ay1, pt2);
    ay = (pt2 - center);
    vpw->worldToClient(pt + GIZMO_METER * az1, pt2);
    az = (pt2 - center);

    if (gizmo.type & IEditorCoreEngine::MODE_Scale)
      correctScaleGizmo(i, ax1, ay1, az1, ax, ay, az);

    real xl = length(ax);
    real yl = length(ay);
    real zl = length(az);

    real maxlen = __max(__max(xl, yl), zl);

    if (xl == 0)
      xl = 1e-5;
    if (yl == 0)
      yl = 1e-5;
    if (zl == 0)
      zl = 1e-5;

    vp[i].center = center;
    vp[i].ax = ax * (AXIS_LEN_PIX / maxlen);
    vp[i].ay = ay * (AXIS_LEN_PIX / maxlen);
    vp[i].az = az * (AXIS_LEN_PIX / maxlen);

    s_vp[i] = vp[i];
    s_vp[i].ax = ax * (AXIS_LEN_PIX / xl);
    s_vp[i].ay = ay * (AXIS_LEN_PIX / yl);
    s_vp[i].az = az * (AXIS_LEN_PIX / zl);
  }
}


//==============================================================================
void GizmoEventFilter::drawGizmoLine(const Point2 &from, const Point2 &delta, real offset)
{
  StdGuiRender::draw_line(from.x + delta.x * offset, from.y + delta.y * offset, from.x + delta.x, from.y + delta.y);
}


//==============================================================================
void GizmoEventFilter::drawGizmoLineFromTo(const Point2 &from, const Point2 &to, real offset) { StdGuiRender::draw_line(from, to); }


//==============================================================================
void GizmoEventFilter::drawGizmoEllipse(const Point2 &center, const Point2 &a, const Point2 &b, real start, real end)
{
  Point2 prev = center + a * cos(start) + b * sin(start);
  Point2 next;

  for (real t = start; t < end; t += ELLIPSE_STEP) //-V1034
  {
    next = center + a * cos(t) + b * sin(t);
    StdGuiRender::draw_line(prev, next);

    prev = next;
  }

  next = center + a * cos(end) + b * sin(end);
  StdGuiRender::draw_line(prev, next);
}


//==============================================================================
void GizmoEventFilter::fillGizmoEllipse(const Point2 &center, const Point2 &a, const Point2 &b, E3DCOLOR color, int fp, real start,
  real end)
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


static inline void swap(float &a, float &b)
{
  float tmp = a;
  a = b;
  b = tmp;
}


//==============================================================================
void GizmoEventFilter::drawGizmoArrow(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, real offset)
{
  Point2 tri[3];
  Point2 norm = normalize(delta);
  ::swap(norm.x, norm.y);
  norm.y = -norm.y;

  tri[0] = from + delta;
  tri[1] = from + delta * 0.8 + norm * _pxS(3);
  tri[2] = from + delta * 0.8 - norm * _pxS(3);

  drawGizmoLine(from, delta * 0.9, offset);

  // dc.selectFillPattern(0xFFFFFFFF);
  StdGuiRender::draw_fill_poly_fan(tri, 3, col_fill);
}


//==============================================================================
void GizmoEventFilter::drawGizmoArrowScale(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, real offset)
{
  Point2 tri[4];
  Point2 norm = normalize(delta);
  ::swap(norm.x, norm.y);
  norm.y = -norm.y;

  tri[0] = from + delta - norm * _pxS(3);
  tri[1] = from + delta + norm * _pxS(3);
  tri[2] = from + delta * 0.9 + norm * _pxS(3);
  tri[3] = from + delta * 0.9 - norm * _pxS(3);

  drawGizmoLine(from, delta * 0.95, offset);

  // dc.selectFillPattern(0xFFFFFFFF);
  StdGuiRender::draw_fill_poly_fan(tri, 4, col_fill);
}


//==============================================================================
void GizmoEventFilter::drawGizmoQuad(const Point2 &from, const Point2 &delta1, const Point2 &delta2, E3DCOLOR col1, E3DCOLOR col2,
  E3DCOLOR col_sel, bool sel, real div)
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


//==============================================================================
void GizmoEventFilter::drawMoveGizmo(int vp_i, int sel)
{
  StdGuiRender::set_color((sel & AXIS_X) ? COLOR_YELLOW : COLOR_LTRED);
  drawGizmoArrow(vp[vp_i].center, vp[vp_i].ax, COLOR_LTRED);

  StdGuiRender::set_color((sel & AXIS_Y) ? COLOR_YELLOW : COLOR_LTGREEN);
  drawGizmoArrow(vp[vp_i].center, vp[vp_i].ay, COLOR_LTGREEN);

  StdGuiRender::set_color((sel & AXIS_Z) ? COLOR_YELLOW : COLOR_LTBLUE);
  drawGizmoArrow(vp[vp_i].center, vp[vp_i].az, COLOR_LTBLUE);

  drawGizmoQuad(vp[vp_i].center, vp[vp_i].az, vp[vp_i].ax, COLOR_LTBLUE, COLOR_LTRED, COLOR_YELLOW, (sel & 5) == 5);
  drawGizmoQuad(vp[vp_i].center, vp[vp_i].ax, vp[vp_i].ay, COLOR_LTRED, COLOR_LTGREEN, COLOR_YELLOW, (sel & 3) == 3);
  drawGizmoQuad(vp[vp_i].center, vp[vp_i].ay, vp[vp_i].az, COLOR_LTGREEN, COLOR_LTBLUE, COLOR_YELLOW, (sel & 6) == 6);
}


//==============================================================================
void GizmoEventFilter::drawSurfMoveGizmo(int vp_i, int sel)
{
  StdGuiRender::set_color((sel & AXIS_X) ? COLOR_YELLOW : COLOR_LTRED);
  drawGizmoArrow(vp[vp_i].center, vp[vp_i].ax, COLOR_LTRED);

  StdGuiRender::set_color((sel & AXIS_Z) ? COLOR_YELLOW : COLOR_LTBLUE);
  drawGizmoArrow(vp[vp_i].center, vp[vp_i].az, COLOR_LTBLUE);

  drawGizmoQuad(vp[vp_i].center, vp[vp_i].az, vp[vp_i].ax, COLOR_LTBLUE, COLOR_LTRED, COLOR_YELLOW, (sel & 5) == 5, 2.0);
}


//==============================================================================
void GizmoEventFilter::drawScaleGizmo(int vp_i, int sel)
{
  Point2 center = vp[vp_i].center;
  Point2 ax = vp[vp_i].ax + vp[vp_i].ax * (scale.x - 1);
  Point2 ay = vp[vp_i].ay + vp[vp_i].ay * (scale.y - 1);
  Point2 az = vp[vp_i].az + vp[vp_i].az * (scale.z - 1);

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


//==============================================================================
void GizmoEventFilter::drawRotateGizmo(IGenViewportWnd *w, int vp_i, int sel)
{
  if (moveStarted && sel)
  {
    Point2 a, b;
    E3DCOLOR col1, col2;

    switch (sel)
    {
      case AXIS_X:
        a = vp[vp_i].ay;
        b = vp[vp_i].az;
        col1 = COLOR_LTRED;
        col2 = COLOR_RED;
        break;
      case AXIS_Y:
        a = vp[vp_i].az;
        b = vp[vp_i].ax;
        col1 = COLOR_LTGREEN;
        col2 = COLOR_GREEN;
        break;
      case AXIS_Z:
        a = vp[vp_i].ax;
        b = vp[vp_i].ay;
        col1 = COLOR_LTBLUE;
        col2 = COLOR_BLUE;
        break;
    }

    const real deltaAngle = rotAngle - startRotAngle;
    const real rotAbs = fabs(deltaAngle);
    const bool rotWorld = IEditorCoreEngine::get()->getGizmoBasisType() == IEditorCoreEngine::BASIS_World;

    real angle = rotWorld ? rotAngle : startRotAngle - deltaAngle;

    if (rotAbs < TWOPI)
    {
      fillGizmoEllipse(vp[vp_i].center, a, b, col1, FILL_MASK, __min(startRotAngle, angle), __max(startRotAngle, angle));
    }
    else if (rotAbs < TWOPI * 2)
    {
      const int divizer = (deltaAngle) / TWOPI;
      const real endRot = rotAngle - TWOPI * divizer;

      angle = rotWorld ? endRot : startRotAngle - (endRot - startRotAngle);

      const real start = __min(startRotAngle, angle);
      const real end = __max(startRotAngle, angle);

      fillGizmoEllipse(vp[vp_i].center, a, b, col2, 0xFFFFFFFF, start, end);
      fillGizmoEllipse(vp[vp_i].center, a, b, col1, FILL_MASK, end - TWOPI, start);
    }
    else
      fillGizmoEllipse(vp[vp_i].center, a, b, col2, 0xFFFFFFFF);

    StdGuiRender::set_color(rotAbs < TWOPI ? col1 : col2);
    drawGizmoLineFromTo(vp[vp_i].center, startPos2d[vp_i]);

    angle = rotWorld ? rotAngle : startRotAngle;

    Point2 end = vp[vp_i].center + a * cos(angle) + b * sin(angle);
    drawGizmoLineFromTo(vp[vp_i].center, end);

    StdGuiRender::set_font(0);
    StdGuiRender::set_color(COLOR_YELLOW);

    StdGuiRender::end_raw_layer();
    StdGuiRender::draw_strf_to(vp[vp_i].center.x - _pxS(80), vp[vp_i].center.y - AXIS_LEN_PIX - _pxS(40), "[%.2f, %.2f, %.2f]",
      RadToDeg(movedDelta.x), RadToDeg(movedDelta.y), RadToDeg(movedDelta.z));
    StdGuiRender::set_texture(BAD_TEXTUREID);
    StdGuiRender::start_raw_layer();
  }

  StdGuiRender::set_color((sel == 1) ? COLOR_YELLOW : COLOR_RED);
  drawGizmoEllipse(vp[vp_i].center, vp[vp_i].ay, vp[vp_i].az);

  StdGuiRender::set_color((sel == 2) ? COLOR_YELLOW : COLOR_LTGREEN);
  drawGizmoEllipse(vp[vp_i].center, vp[vp_i].az, vp[vp_i].ax);

  StdGuiRender::set_color((sel == 4) ? COLOR_YELLOW : COLOR_LTBLUE);
  drawGizmoEllipse(vp[vp_i].center, vp[vp_i].ax, vp[vp_i].ay);
}


//==============================================================================
void GizmoEventFilter::drawGizmo(IGenViewportWnd *w)
{
  if (!gizmo.client || gizmo.type == IEditorCoreEngine::MODE_None)
    return;

  recalcViewportGizmo();

  int vp_i = ged.findViewportIndex(w);
  int sel = gizmo.over ? gizmo.over : gizmo.selected;

  Point2 screen;
  if (!w->worldToClient(gizmo.client->getPt(), screen))
    return;

  StdGuiRender::set_texture(BAD_TEXTUREID);
  StdGuiRender::start_raw_layer();
  switch (gizmo.type)
  {
    case IEditorCoreEngine::MODE_Move: drawMoveGizmo(vp_i, sel); break;

    case IEditorCoreEngine::MODE_Scale: drawScaleGizmo(vp_i, sel); break;

    case IEditorCoreEngine::MODE_Rotate: drawRotateGizmo(w, vp_i, sel); break;

    case IEditorCoreEngine::MODE_MoveSurface: drawSurfMoveGizmo(vp_i, sel); break;

    default: debug("FATAL: gizmo.type == 0x%X", gizmo.type); G_ASSERT(0);
  }
  StdGuiRender::end_raw_layer();

  repaint();
}


//==============================================================================
bool GizmoEventFilter::isPointInEllipse(const Point2 &center, const Point2 &a, const Point2 &b, real width, const Point2 &point,
  real start, real end)
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


//==============================================================================
void GizmoEventFilter::correctScaleGizmo(int idx, const Point3 &x_dir, const Point3 &y_dir, const Point3 &z_dir, Point2 &ax,
  Point2 &ay, Point2 &az)
{
  const IGenViewportWnd *vpw = IEditorCoreEngine::get()->getViewport(idx);

  if (!vpw)
    return;

  TMatrix tm;
  vpw->getCameraTransform(tm);
  tm = inverse(tm);

  const Point3 camDir = Point3(tm.m[0][2], tm.m[1][2], tm.m[2][2]);

  if (x_dir * camDir > 0)
    ax = -ax;

  if (y_dir * camDir > 0)
    ay = -ay;

  if (z_dir * camDir > 0)
    az = -az;
}
