// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_gizmofilter.h>
#include <EditorCore/captureCursor.h>
#include <EditorCore/ec_input.h>
#include <winGuiWrapper/wgw_input.h>
#include "ec_gizmoRenderer.h"

#include <imgui/imgui.h>

using hdpi::_pxS;

#define GIZMO_METER    0.01
#define MAX_TRACE_DIST 1000.0


//==============================================================================
GizmoEventFilter::GizmoEventFilter(GeneralEditorData &ged_, const GridObject &grid_) :
  ged(ged_), grid(grid_), moveStarted(false), scale(1, 1, 1), rotAngle(0), startRotAngle(0), deltaX(0), deltaY(0)
{
  startPos2d.reserve(16);
  vp.reserve(16);
  s_vp.reserve(16);
  memset(&gizmo, 0, sizeof(gizmo));
  renderer.reset(new GizmoRendererClassic(this));
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
bool GizmoEventFilter::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  mouseCurrentPos = Point2(x, y);

  if (gizmo.prevMode != (IEditorCoreEngine::GIZMO_MASK_Mode & int(gizmo.type)))
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
    mousePos = mouseCurrentPos;

    const Point3 gcPos = gizmo.client->shouldComputeDeltaFromStartPos() ? startPos : gizmo.client->getPt();

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
            {
              TMatrix viewTm;
              wnd->getCameraTransform(viewTm);
              planeNormal = viewTm.getcol(2);
              break;
            }
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

        const bool shouldSnapMove = ec_is_key_down(ImGuiKey_CapsLock) ? !grid.getMoveSnap() : grid.getMoveSnap();
        if (shouldSnapMove)
        {
          Point3 movedToPos = gcPos + movedDelta;
          Point3 fullySnappedPos = grid.snapToGrid(movedToPos);

          if (key_modif & wingw::M_ALT)
          {
            const float step = grid.getInfiniteGridMajorSubdivisions();
            fullySnappedPos = Point3(grid.snap(movedToPos.x, step), grid.snap(movedToPos.y, step), grid.snap(movedToPos.z, step));
          }

          if (gizmo.selected & AXIS_X)
            movedToPos.x = fullySnappedPos.x;
          if (gizmo.selected & AXIS_Y)
            movedToPos.y = fullySnappedPos.y;
          if (gizmo.selected & AXIS_Z)
            movedToPos.z = fullySnappedPos.z;

          movedDelta = movedToPos - gcPos;
        }

        break;
      }

      case IEditorCoreEngine::MODE_MoveSurface: surfaceMove(wnd, x, y, index, gcPos, movedDelta); break;

      case IEditorCoreEngine::MODE_Scale:
      {
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
            max = ::max(movedDelta.x, movedDelta.y);
            movedDelta.x = max;
            movedDelta.y = max;
            break;
          case AXIS_Y | AXIS_Z:
            max = ::max(movedDelta.y, movedDelta.z);
            movedDelta.y = max;
            movedDelta.z = max;
            break;
          case AXIS_X | AXIS_Z:
            max = ::max(movedDelta.z, movedDelta.x);
            movedDelta.z = max;
            movedDelta.x = max;
            break;
          case AXIS_X | AXIS_Y | AXIS_Z: movedDelta.x = movedDelta.z = movedDelta.y; break;
        }

        const bool shouldSnapScale = ec_is_key_down(ImGuiKey_CapsLock) ? !grid.getScaleSnap() : grid.getScaleSnap();
        if (shouldSnapScale)
          movedDelta = grid.snapToScale(movedDelta);

        scale = movedDelta;
        break;
      }

      case IEditorCoreEngine::MODE_Rotate:
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
        const bool shouldSnapRotate = ec_is_key_down(ImGuiKey_CapsLock) ? !grid.getRotateSnap() : grid.getRotateSnap();
        if (shouldSnapRotate)
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

        break;
      }

      case IEditorCoreEngine::MODE_None: break; // to prevent the unhandled switch case error
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
    int delta = GIZMO_PIXEL_WIDTH;
    int count = 0;
    float dist2, min_dist2 = float(GIZMO_PIXEL_WIDTH * GIZMO_PIXEL_WIDTH * 32);


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
        delta += GIZMO_PIXEL_WIDTH;
        count = 0;
        if (min_dist2 >= float(GIZMO_PIXEL_WIDTH * GIZMO_PIXEL_WIDTH * 16) || delta >= GIZMO_PIXEL_WIDTH * 4)
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
  if (!moveStarted)
    return;

  moveStarted = false;
  gizmo.client->gizmoEnded(apply);
  scale = Point3(1, 1, 1);

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
void GizmoEventFilter::setGizmoStyle(GizmoEventFilter::Style style)
{
  if (renderer->getStyle() == style)
    return;

  switch (style)
  {
    case Style::New: renderer.reset(new GizmoRendererNew(this)); break;
    case Style::Classic: renderer.reset(new GizmoRendererClassic(this)); break;
  }

  repaint();
}

GizmoEventFilter::Style GizmoEventFilter::getGizmoStyle() const { return renderer->getStyle(); }

//==============================================================================
bool GizmoEventFilter::checkGizmo(IGenViewportWnd *wnd, int x, int y)
{
  Point2 mouse = Point2(x, y);
  int vp_i = ged.findViewportIndex(wnd);
  Point2 axes[3] = {vp[vp_i].ax, vp[vp_i].ay, vp[vp_i].az};
  gizmo.over = renderer->intersection(mouse, vp[vp_i].center, axes, gizmo.selected, rotateDir);
  return gizmo.over != 0;
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

    Matrix3 basis;
    basis.setcol(0, ax1);
    basis.setcol(1, ay1);
    basis.setcol(2, az1);
    renderer->flip(vpw, basis, ax, ay, az);

    real xl = length(ax);
    real yl = length(ay);
    real zl = length(az);

    real maxlen = ::max(::max(xl, yl), zl);

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
void GizmoEventFilter::drawGizmo(IGenViewportWnd *w)
{
  if (!gizmo.client || gizmo.type == IEditorCoreEngine::MODE_None)
    return;

  recalcViewportGizmo();

  bool isMouseOver = gizmo.client->isMouseOver(w, mouseCurrentPos.x, mouseCurrentPos.y);
  int vp_i = ged.findViewportIndex(w);
  int sel = ((moveStarted || isMouseOver) && !gizmo.over) ? gizmo.selected : gizmo.over;
  if ((gizmo.type & IEditorCoreEngine::MODE_Rotate)) // TODO: remove this
  {
    sel = moveStarted ? gizmo.selected : gizmo.over;
  }

  Point2 screen;
  if (!w->worldToClient(gizmo.client->getPt(), screen))
    return;

  const Point2 axes[3] = {vp[vp_i].ax, vp[vp_i].ay, vp[vp_i].az};

  StdGuiRender::reset_textures();
  StdGuiRender::start_raw_layer();
  switch (gizmo.type)
  {
    case IEditorCoreEngine::MODE_Move: renderer->renderMove(w, vp[vp_i].center, axes, sel); break;

    case IEditorCoreEngine::MODE_Scale: renderer->renderScale(w, vp[vp_i].center, axes, sel, scale); break;

    case IEditorCoreEngine::MODE_Rotate:
      renderer->renderRotate(w, vp[vp_i].center, axes, startPos2d[vp_i], rotAngle, startRotAngle, sel);
      break;

    case IEditorCoreEngine::MODE_MoveSurface: renderer->renderSurfMove(vp[vp_i].center, axes, sel); break;

    default: debug("FATAL: gizmo.type == 0x%X", gizmo.type); G_ASSERT(0);
  }
  StdGuiRender::end_raw_layer();

  repaint();
}
