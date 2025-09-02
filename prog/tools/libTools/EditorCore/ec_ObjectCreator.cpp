// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>

#include <EditorCore/ec_colors.h>
#include <EditorCore/ec_ObjectCreator.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/ec_rect.h>

#include <winGuiWrapper/wgw_input.h>

#include <math/dag_math2d.h>
#include <math/dag_plane3.h>
#include <math/dag_bezier.h>
#include <math/dag_capsule.h>

#include <EditorCore/captureCursor.h>

//--------------------------------------------------------------
//  BoxCreator
//--------------------------------------------------------------

BoxCreator::BoxCreator() :
  wrapedX(0),
  wrapedY(0),
  stageNo(0),
  point0(0, 0, 0),
  point1(0, 0, 0),
  point2(0, 0, 0),
  height(0),
  clientPoint0(0, 0),
  clientPoint2(0, 0)
{}


bool BoxCreator::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate)
{
  if (stateFinished)
    return false;

  if (stageNo)
  {
    int x1 = 0, x2, y1 = 0, y2;
    wnd->getViewportSize(x2, y2);
    wnd->clientToScreen(x1, y1);
    wnd->clientToScreen(x2, y2);
    EcRect rc = {x1, y1, x2, y2};

    ::area_cursor_wrap(rc, x, y, wrapedX, wrapedY);
  }

  if (stageNo == 1 || stageNo == 2)
  {
    Point3 worldPoint, worldDir;
    wnd->clientToWorld(Point2(x + wrapedX, y + wrapedY), worldPoint, worldDir);
    real t = (point0.y - worldPoint.y) / worldDir.y;
    Point3 point = worldPoint + t * worldDir;

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      point = grid.snapToGrid(point);

    if (key_modif & wingw::M_SHIFT)
    {
      if (fabsf(point.x - point0.x) < fabsf(point.z - point0.z))
        point.x = point0.x;
      else
        point.z = point0.z;
    }

    if (stageNo == 1)
    {
      point1 = point;
      if (!rotate)
        point1.x = point0.x;
    }
    else
    {
      point2 = point;
      clientPoint2 = Point2(x + wrapedX, y + wrapedY);
    }

    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  else if (stageNo == 3)
  {
    if (wnd->isOrthogonal())
    {
      height = clientPoint2.y - (y + wrapedY);
      IEditorCoreEngine::get()->invalidateViewportCache();
      return false;
    }

    Point3 worldPoint2, worldDir2;
    wnd->clientToWorld(Point2(0, clientPoint2.y), worldPoint2, worldDir2);

    Point3 worldPoint3, worldDir3;
    wnd->clientToWorld(Point2(0, y + wrapedY), worldPoint3, worldDir3);

    height = 2.f * (1 - (worldDir2 * worldDir3) * (worldDir2 * worldDir3)) * length(point2 - worldPoint3);
    if (y + wrapedY > clientPoint2.y)
      height = -height;

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap() && grid.getStep())
      height = (real)((int)(height / grid.getStep())) * grid.getStep();

    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  return true;
}


bool BoxCreator::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  switch (stageNo)
  {
    case 0:
    {
      real znear, zfar;
      wnd->getZnearZfar(znear, zfar);
      if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, point0, zfar))
      {
        stateFinished = true;
        return false;
      }

      const GridObject &grid = IEditorCoreEngine::get()->getGrid();
      if (grid.getMoveSnap())
        point0 = grid.snapToGrid(point0);

      point1 = point2 = /*point3 = */ point0;
      height = 0;
      clientPoint0 = Point2(x, y);
      stageNo = 1;
      wrapedX = 0;
      wrapedY = 0;
      IEditorCoreEngine::get()->invalidateViewportCache();

      break;
    }

    case 1:
      if (length(clientPoint0 - Point2(x, y)) < 4)
        stateFinished = true;
      else
      {
        stageNo = 2;
        handleMouseMove(wnd, x, y, inside, buttons, key_modif);
      }

      break;

    case 2:
      clientPoint0 = Point2(x, y);
      stageNo = 3;
      wrapedX = 0;
      wrapedY = 0;

      break;

    case 3:
      stateFinished = true;
      stateOk = height >= 0;
      wrapedX = 0;
      wrapedY = 0;

      break;
  }

  return true;
}


bool BoxCreator::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  wnd->captureMouse();

  if (stageNo == 1)
  {
    if (length(clientPoint0 - Point2(x, y)) >= 4)
      stageNo++;
  }
  else if (stageNo == 3)
  {
    if (length(clientPoint0 - Point2(x, y)) >= 4)
    {
      wnd->releaseMouse();
      stateFinished = true;
      stateOk = height >= 0;
      wrapedX = 0;
      wrapedY = 0;
    }
  }

  return !stateFinished;
}


bool BoxCreator::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  stateFinished = true;
  return true;
}


void BoxCreator::render()
{
  if (!height)
    height = 0.001; // FAKE

  const bool underClip = height < 0;

  matrix.setcol(3, Point3(0.f, 0.f, 0.f));
  matrix.setcol(2, normalize(point1 - point0));
  matrix.setcol(1, Point3(0.f, 1.f, 0.f));
  matrix.setcol(0, matrix.getcol(1) % matrix.getcol(2));
  float m_det = matrix.det();
  if (fabs(m_det) < 1e-6)
    return;

  TMatrix matrixInv = inverse(matrix, m_det);
  Point3 point0Box = point0 * matrixInv;
  Point3 point1Box = point1 * matrixInv;
  Point3 point2Box = point2 * matrixInv;
  Point3 centerBox = Point3(0.5f * (point0Box.x + point2Box.x), point0Box.y + 0.5f * height, 0.5f * (point0Box.z + point1Box.z));
  matrix.setcol(3, centerBox * matrix);
  Point3 halfSize(fabsf(point2Box.x - point0Box.x) * 0.5f, height * 0.5f, fabsf(point1Box.z - point0Box.z) * 0.5f);

  Point3 p[8];
  p[0] = matrix * Point3(-halfSize.x, -halfSize.y, -halfSize.z);
  p[1] = matrix * Point3(-halfSize.x, -halfSize.y, halfSize.z);
  p[2] = matrix * Point3(-halfSize.x, halfSize.y, -halfSize.z);
  p[3] = matrix * Point3(-halfSize.x, halfSize.y, halfSize.z);
  p[4] = matrix * Point3(halfSize.x, -halfSize.y, -halfSize.z);
  p[5] = matrix * Point3(halfSize.x, -halfSize.y, halfSize.z);
  p[6] = matrix * Point3(halfSize.x, halfSize.y, -halfSize.z);
  p[7] = matrix * Point3(halfSize.x, halfSize.y, halfSize.z);

  if (!underClip)
  {
    draw_debug_line(p[0], p[1], selected_color);
    draw_debug_line(p[0], p[2], selected_color);
    draw_debug_line(p[0], p[4], selected_color);
    draw_debug_line(p[1], p[3], selected_color);
    draw_debug_line(p[1], p[5], selected_color);
    draw_debug_line(p[2], p[3], selected_color);
    draw_debug_line(p[2], p[6], selected_color);
    draw_debug_line(p[4], p[6], selected_color);
    draw_debug_line(p[4], p[5], selected_color);
    draw_debug_line(p[3], p[7], selected_color);
    draw_debug_line(p[5], p[7], selected_color);
    draw_debug_line(p[6], p[7], selected_color);
  }
  else
  {
    draw_debug_line(p[0], p[1], E3DCOLOR(255, 0, 0, 255));
    draw_debug_line(p[1], p[5], E3DCOLOR(255, 0, 0, 255));
    draw_debug_line(p[5], p[4], E3DCOLOR(255, 0, 0, 255));
    draw_debug_line(p[4], p[0], E3DCOLOR(255, 0, 0, 255));
  }

  matrix.setcol(0, ::normalize(matrix.getcol(0)) * fabsf(point2Box.x - point0Box.x));

  if (!underClip)
    matrix.setcol(1, ::normalize(matrix.getcol(1)) * height);
  else
  {
    matrix.setcol(1, Point3(0, 0, 0));
    matrix.setcol(3, matrix.getcol(3) - Point3(0, height / 2, 0));
  }

  matrix.setcol(2, ::normalize(matrix.getcol(2)) * fabsf(point1Box.z - point0Box.z));
  matrix.setcol(3, matrix.getcol(3) - (matrix.getcol(1) * 0.5f));
}


//--------------------------------------------------------------
//  PointCreator
//--------------------------------------------------------------

PointCreator::PointCreator() : stageNo(0) {}


bool PointCreator::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate)
{
  if (stateFinished)
    return false;

  Point3 center;
  real znear, zfar;
  wnd->getZnearZfar(znear, zfar);
  if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, center, zfar, NULL))
  {
    return true;
  }
  const GridObject &grid = IEditorCoreEngine::get()->getGrid();
  if (grid.getMoveSnap())
    matrix.setcol(3, grid.snapToGrid(center));
  else
    matrix.setcol(3, center);

  clientCenter0 = Point2(x, y);
  IEditorCoreEngine::get()->invalidateViewportCache();

  return true;
}


bool PointCreator::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  if (stageNo == 0)
  {
    Point3 center;
    real znear, zfar;
    wnd->getZnearZfar(znear, zfar);
    if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, center, zfar, NULL))
    {
      stateFinished = true;
      return false;
    }
    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      matrix.setcol(3, grid.snapToGrid(center));
    else
      matrix.setcol(3, center);

    clientCenter0 = Point2(x, y);
    stageNo = 1;
    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  return true;
}


bool PointCreator::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  if (stageNo == 1)
  {
    stateFinished = true;
    stateOk = true;
  }

  return true;
}


bool PointCreator::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  stateFinished = true;
  return true;
}


void PointCreator::render() { draw_debug_sph(matrix.getcol(3), 0.1, selected_color); }


//--------------------------------------------------------------
//  SphereCreator
//--------------------------------------------------------------

SphereCreator::SphereCreator() : stageNo(0) {}


bool SphereCreator::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate)
{
  if (stateFinished)
    return false;

  if (stageNo == 1)
  {
    Point3 worldPoint, worldDir;
    wnd->clientToWorld(Point2(x, y), worldPoint, worldDir);

    real projection = (matrix.getcol(3) - worldPoint) * normalize(worldDir);
    radius = sqrtf(lengthSq(matrix.getcol(3) - worldPoint) - projection * projection);

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap() && grid.getStep())
      radius = (real)((int)(radius / grid.getStep())) * grid.getStep();

    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  return true;
}


bool SphereCreator::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  if (stageNo == 0)
  {
    Point3 center;
    real znear, zfar;
    wnd->getZnearZfar(znear, zfar);
    if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, center, zfar, NULL))
    {
      stateFinished = true;
      return false;
    }
    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      matrix.setcol(3, grid.snapToGrid(center));
    else
      matrix.setcol(3, center);

    radius = 0.f;
    clientCenter0 = Point2(x, y);
    stageNo = 1;
    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  return true;
}


bool SphereCreator::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  if (stageNo == 1)
  {
    stateOk = length(clientCenter0 - Point2(x, y)) >= 4;
    stateFinished = true;
  }

  return true;
}


bool SphereCreator::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  stateFinished = true;
  return true;
}


void SphereCreator::render()
{
  draw_debug_sph(matrix.getcol(3), radius, selected_color);
  TMatrix tm;
  tm.identity();
  const real diameter = radius * 2.0f;
  matrix.setcol(0, tm.getcol(0) * diameter);
  matrix.setcol(1, tm.getcol(1) * diameter);
  matrix.setcol(2, tm.getcol(2) * diameter);
}

void CircleCreator::render()
{
  draw_debug_xz_circle(matrix.getcol(3), radius, selected_color);
  TMatrix tm;
  tm.identity();
  const real diameter = radius * 2.0f;
  matrix.setcol(0, tm.getcol(0) * diameter);
  matrix.setcol(1, tm.getcol(1) * 0.01); // FAKE
  matrix.setcol(2, tm.getcol(2) * diameter);
}

//--------------------------------------------------------------
//  TargetCreator
//--------------------------------------------------------------

TargetCreator::TargetCreator() : stageNo(0) {}


bool TargetCreator::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate)
{
  if (stateFinished)
    return false;

  if (stageNo == 1)
  {
    real znear, zfar;
    wnd->getZnearZfar(znear, zfar);
    IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, target, zfar, NULL);

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      target = grid.snapToGrid(target);
  }

  IEditorCoreEngine::get()->invalidateViewportCache();

  return true;
}


bool TargetCreator::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  if (stageNo == 0)
  {
    real znear, zfar;
    wnd->getZnearZfar(znear, zfar);
    if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, point0, zfar, NULL))
    {
      stateFinished = true;
      return false;
    }

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      point0 = grid.snapToGrid(point0);

    client0 = Point2(x, y);
    target = point0;

    IEditorCoreEngine::get()->invalidateViewportCache();
    stageNo = 1;
  }

  return true;
}


bool TargetCreator::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  IEditorCoreEngine::get()->invalidateViewportCache();

  if (stateFinished)
    return false;

  if (stageNo == 1)
  {
    stateOk = length(client0 - Point2(x, y)) >= 4;
    stateFinished = true;
  }

  return true;
}


bool TargetCreator::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  IEditorCoreEngine::get()->invalidateViewportCache();

  stateFinished = true;
  return true;
}


void TargetCreator::render()
{
  matrix.setcol(3, point0);
  matrix.setcol(2, normalize(target - point0));
  matrix.setcol(0, Point3(0.f, 1.f, 0.f) % matrix.getcol(2));
  matrix.setcol(1, matrix.getcol(2) % matrix.getcol(0));
  draw_debug_line(point0, target, selected_color);
}


//--------------------------------------------------------------
//  PlaneCreator
//--------------------------------------------------------------

PlaneCreator::PlaneCreator() : wrapedX(0), wrapedY(0), stageNo(0) {}


bool PlaneCreator::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate)
{
  if (stateFinished || !stageNo)
    return false;

  if (stageNo)
  {
    int x1 = 0, x2, y1 = 0, y2;
    wnd->getViewportSize(x2, y2);
    wnd->clientToScreen(x1, y1);
    wnd->clientToScreen(x2, y2);
    EcRect rc = {x1, y1, x2, y2};

    ::area_cursor_wrap(rc, x, y, wrapedX, wrapedY);
  }

  Point3 worldPoint, worldDir, point;
  wnd->clientToWorld(Point2(x + wrapedX, y + wrapedY), worldPoint, worldDir);
  int vert = key_modif & wingw::M_CTRL;
  if (stageNo == 1 || !vert)
  {
    real t = (point0.y - worldPoint.y) / worldDir.y;
    point = worldPoint + t * worldDir;
  }
  else
  {
    Plane3 plane(point0, point1, point1 + Point3(0, 1, 0));
    point = plane.calcLineIntersectionPoint(worldPoint, worldPoint + worldDir);
  }

  const GridObject &grid = IEditorCoreEngine::get()->getGrid();
  if (grid.getMoveSnap())
    point = grid.snapToGrid(point);

  if (key_modif & wingw::M_SHIFT)
  {
    if (fabsf(point.x - point0.x) < fabsf(point.z - point0.z))
      point.x = point0.x;
    else
      point.z = point0.z;
  }

  if (stageNo == 1)
  {
    point1 = point;
    if (!rotate)
      point1.x = point0.x;
  }
  else if (stageNo == 2)
  {
    point2 = point;
    clientPoint2 = Point2(x + wrapedX, y + wrapedY);
  }

  matrix.setcol(3, Point3(0.f, 0.f, 0.f));
  matrix.setcol(0, normalize(point1 - point0));
  if (!vert)
  {
    matrix.setcol(1, Point3(0.f, 1.f, 0.f));
    matrix.setcol(2, matrix.getcol(0) % matrix.getcol(1));
  }
  else
  {
    matrix.setcol(2, Point3(0.f, 1.f, 0.f));
    matrix.setcol(1, matrix.getcol(2) % matrix.getcol(0));
  }

  float m_det = matrix.det();
  if (fabs(m_det) < 1e-6)
    return false;

  TMatrix matrixInv = inverse(matrix, m_det);
  Point3 point0Plane = point0 * matrixInv;
  Point3 point1Plane = point1 * matrixInv;
  Point3 point2Plane = point2 * matrixInv;
  Point3 centerPlane =
    Point3(0.5f * (point0Plane.x + point1Plane.x), 0.5f * (point0Plane.y + point2Plane.y), 0.5f * (point0Plane.z + point2Plane.z));
  matrix.setcol(3, centerPlane * matrix);

  const Point2 size(fabsf(point1Plane.x - point0Plane.x), fabsf(point2Plane.z - point0Plane.z));
  halfSize = size * 0.5f;

  matrix.setcol(0, matrix.getcol(0) * size.x);
  matrix.setcol(2, matrix.getcol(2) * size.y);

  IEditorCoreEngine::get()->invalidateViewportCache();
  return true;
}


bool PlaneCreator::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  if (stageNo == 0)
  {
    real znear, zfar;
    wnd->getZnearZfar(znear, zfar);
    if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, point0, zfar, NULL))
    {
      stateFinished = true;
      return false;
    }

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      point0 = grid.snapToGrid(point0);

    point1 = point2 = point0;
    clientPoint0 = Point2(x, y);
    stageNo = 1;
    wrapedX = 0;
    wrapedY = 0;
    memset(&matrix, 0, sizeof(matrix));
  }
  else if (stageNo == 2)
  {
    stateFinished = true;
    stateOk = true;
    wrapedX = 0;
    wrapedY = 0;
  }
  return true;
}


bool PlaneCreator::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  if (stageNo == 1)
  {
    if (length(clientPoint0 - Point2(x, y)) >= 4)
      stageNo = 2;

    wnd->captureMouse();
  }
  return true;
}


bool PlaneCreator::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  stateFinished = true;
  return true;
}


void PlaneCreator::render()
{
  if (!stageNo)
    return;

  TMatrix tm;
  tm.setcol(0, normalize(matrix.getcol(0)));
  tm.setcol(1, matrix.getcol(1));
  tm.setcol(2, normalize(matrix.getcol(2)));
  tm.setcol(3, matrix.getcol(3));

  Point3 p[4];
  p[0] = tm * Point3(-halfSize.x, 0, -halfSize.y);
  p[1] = tm * Point3(-halfSize.x, 0, halfSize.y);
  p[2] = tm * Point3(halfSize.x, 0, halfSize.y);
  p[3] = tm * Point3(halfSize.x, 0, -halfSize.y);
  draw_debug_line(p[0], p[1], selected_color);
  draw_debug_line(p[1], p[2], selected_color);
  draw_debug_line(p[2], p[3], selected_color);
  draw_debug_line(p[3], p[0], selected_color);
}


//--------------------------------------------------------------
//  SurfaceMoveCreator
//--------------------------------------------------------------

bool SurfaceMoveCreator::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate)
{
  if (stateFinished)
    return false;

  Point3 center;
  real znear, zfar;
  wnd->getZnearZfar(znear, zfar);
  if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, center, zfar, NULL))
  {
    stateFinished = true;
    return false;
  }
  const GridObject &grid = IEditorCoreEngine::get()->getGrid();
  if (grid.getMoveSnap())
    matrix.setcol(3, grid.snapToGrid(center));
  else
    matrix.setcol(3, center);

  IEditorCoreEngine::get()->invalidateViewportCache();
  return true;
}

bool SurfaceMoveCreator::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  handleMouseMove(wnd, x, y, inside, buttons, key_modif, false);
  return true;
}

bool SurfaceMoveCreator::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  stateOk = true;
  stateFinished = true;
  return true;
}

bool SurfaceMoveCreator::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  stateFinished = true;
  return true;
}


CylinderCreator::CylinderCreator() : wrapedX(0), wrapedY(0), stageNo(0) { matrix.zero(); }


bool CylinderCreator::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate)
{
  if (stateFinished)
    return false;

  //::window_cursor_wrap( wnd->getCtlWindow(), x, y, wrapedX, wrapedY );
  if (stageNo == 1)
  {
    Point3 worldPoint, worldDir;
    wnd->clientToWorld(Point2(x + wrapedX, y + wrapedY), worldPoint, worldDir);
    real t = (point0.y - worldPoint.y) / worldDir.y;
    point1 = worldPoint + t * worldDir;
    const Point3 dir(point1 - point0);
    const real radius = ::length(dir);

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      point1 = grid.snapToGrid(point1);

    matrix.setcol(0, Point3(1, 0, 0) * radius);
    matrix.setcol(1, Point3(0, 0, 0));
    matrix.setcol(2, Point3(0, 0, 1) * radius);
    matrix.setcol(3, point0);
    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  else if (stageNo == 2)
  {
    if (wnd->isOrthogonal())
    {
      const real height = clientPoint.y - (y + wrapedY);
      matrix.setcol(1, Point3(0.0f, 1.0f, 0.0f) * height);
      IEditorCoreEngine::get()->invalidateViewportCache();
      return false;
    }

    Point3 worldPoint2, worldDir2;
    wnd->clientToWorld(Point2(0, clientPoint.y), worldPoint2, worldDir2);

    Point3 worldPoint3, worldDir3;
    wnd->clientToWorld(Point2(0, y + wrapedY), worldPoint3, worldDir3);

    const real scalar = worldDir2 * worldDir3;
    real height = 2.0f * (1.0f - scalar * scalar) * ::length(point1 - worldPoint3);

    if ((y + wrapedY) > clientPoint.y)
      height = -height;

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap() && grid.getStep())
      height = (real)((int)(height / grid.getStep())) * grid.getStep();

    const Point3 dir(point1 - point0);

    if (height < 0)
      height = -0.00001;

    matrix.setcol(1, Point3(0, height, 0));
    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  return true;
}


bool CylinderCreator::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  if (stageNo == 0)
  {
    real znear, zfar;
    wnd->getZnearZfar(znear, zfar);
    if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, point0, zfar, NULL))
    {
      stateFinished = true;
      return false;
    }

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      point0 = grid.snapToGrid(point0);

    clientPoint = Point2(x, y);
    stageNo = 1;
    wrapedX = 0;
    wrapedY = 0;
    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  else if (stageNo == 2)
  {
    stateFinished = true;
    stateOk = clientPoint.y - (y + wrapedY) >= 0;
  }
  return true;
}


bool CylinderCreator::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  if (stageNo == 1)
  {
    if (length(clientPoint - Point2(x, y)) < 4)
      stateFinished = true;
    else
    {
      clientPoint = Point2(x + wrapedX, y + wrapedY);
      stageNo = 2;
      wrapedX = 0;
      wrapedY = 0;
    }
  }
  return true;
}


bool CylinderCreator::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  stateFinished = true;
  return true;
}


void CylinderCreator::render()
{
  const Point3 centre1(matrix.getcol(3));
  const Point3 centre2(centre1 + matrix.getcol(1));
  const real radius = matrix.getcol(0).length();

  const bool underClip = centre2.y < centre1.y;

  ::draw_debug_xz_circle(centre1, radius, underClip ? E3DCOLOR(255, 0, 0, 255) : ::selected_color);

  if (!underClip)
  {
    ::draw_debug_xz_circle(centre2, radius, ::selected_color);

    real an;
    for (int i = 0, an = 0; i <= 5; ++i, an += TWOPI / 5.0f)
    {
      const real cosinus = radius * cosf(an);
      const real sinus = radius * sinf(an);
      const Point3 pos1(cosinus + centre1.x, centre1.y, sinus + centre1.z);
      const Point3 pos2(cosinus + centre2.x, centre2.y, sinus + centre2.z);
      ::draw_debug_line(pos1, pos2, ::selected_color);
    }
  }
}

// --------------------------------------------------------------

const float POLYGON_POINT_SIZE = 1.0f;

PolygoneZoneCreator::PolygoneZoneCreator() { matrix.identity(); }

int trySelectPoint(const Point3 p, const Tab<Point3> &points, real maxDistance)
{
  real bestLen = FLT_MAX;
  int nearestIndex = -1;
  for (int i = 0; i < points.size(); i++)
  {
    float len = (p - points[i]).length();
    if (len < bestLen && len < maxDistance)
    {
      bestLen = len;
      nearestIndex = i;
    }
  }
  return nearestIndex;
}

bool PolygoneZoneCreator::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate)
{
  if (stateFinished)
    return false;

  if (stageNo == Stages::SetPoints)
  {
    Point3 worldPos;
    real znear, zfar;
    wnd->getZnearZfar(znear, zfar);
    if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, worldPos, zfar, NULL))
    {
      return true;
    }

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      worldPos = grid.snapToGrid(worldPos);

    cursorWorldPos = worldPos;
    IEditorCoreEngine::get()->invalidateViewportCache();

    if (selectedPointIndex != -1 && isMovingPoint)
    {
      poly.points[selectedPointIndex] = cursorWorldPos;
      return true;
    }

    selectedPointIndex = trySelectPoint(cursorWorldPos, poly.points, POLYGON_POINT_SIZE);
  }

  if (stageNo == Stages::SetHeight)
  {
    real floorY = poly.getMaxFloorY();
    Point3 floorPoint;
    for (auto &point : poly.points)
      if (point.y >= floorY)
        floorPoint = point;
    Point2 floorScreenPoint;
    wnd->worldToClient(floorPoint, floorScreenPoint);

    real height;
    if (wnd->isOrthogonal())
    {
      height = abs(floorScreenPoint.y - y);
      poly.setHeight(height);
      IEditorCoreEngine::get()->invalidateViewportCache();
      return false;
    }

    Point3 worldPointOriginal, worldDirOriginal;
    Point3 worldPoint, worldDir;
    wnd->clientToWorld(Point2(0, floorScreenPoint.y), worldPointOriginal, worldDirOriginal);
    wnd->clientToWorld(Point2(0, y), worldPoint, worldDir);

    const real scalar = worldDirOriginal * worldDir;
    height = 2.0f * (1.0f - scalar * scalar) * length(floorPoint - worldPoint);

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap() && grid.getStep())
      height = floorf(height / grid.getStep()) * grid.getStep();

    poly.setHeight(height);
    IEditorCoreEngine::get()->invalidateViewportCache();
  }

  return true;
}

bool PolygoneZoneCreator::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  if (stageNo == Stages::SetPoints)
  {
    Point3 worldPos;
    real znear, zfar;
    wnd->getZnearZfar(znear, zfar);
    if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, worldPos, zfar, NULL))
    {
      return true;
    }

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      worldPos = grid.snapToGrid(worldPos);

    if (selectedPointIndex != -1)
    {
      isMovingPoint = true;
    }
    else
    {
      poly.points.push_back(worldPos);
    }
  }

  if (stageNo == Stages::SetHeight)
    switchStages();

  return true;
}

bool PolygoneZoneCreator::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  isMovingPoint = false;
  return true;
}

bool PolygoneZoneCreator::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  if (stageNo == Stages::SetPoints)
  {
    if (selectedPointIndex != -1)
    {
      poly.points.erase(poly.points.begin() + selectedPointIndex);
      return true;
    }

    switchStages();
  }
  else if (stageNo == Stages::SetHeight)
  {
    stateFinished = true;
    stateOk = false;
  }

  return true;
}

void renderPoly(const Tab<Point3> &points, bool isClosed)
{
  if (points.size() < 2)
    return;

  for (int i = 1; i < points.size(); i++)
    draw_debug_line(points[i - 1], points[i], selected_color);

  if (isClosed)
    draw_debug_line(points.front(), points.back(), selected_color);
}

void PolygoneZoneCreator::render()
{
  if (stageNo == Stages::SetPoints)
  {
    draw_debug_sph(cursorWorldPos, POLYGON_POINT_SIZE, selected_color);

    if (poly.points.empty())
      return;

    for (const Point3 &point : poly.points)
      draw_debug_sph(point, POLYGON_POINT_SIZE, selected_color);

    if (poly.points.size() == 1)
      draw_debug_line(cursorWorldPos, poly.points[0], selected_color);
    if (poly.points.size() >= 2)
    {
      renderPoly(poly.points, true);
      if (selectedPointIndex == -1)
      {
        draw_debug_line(cursorWorldPos, poly.points.front(), selected_color);
        draw_debug_line(cursorWorldPos, poly.points.back(), selected_color);
      }
    }
    if (selectedPointIndex != -1)
      draw_debug_sph(poly.points[selectedPointIndex], POLYGON_POINT_SIZE, E3DCOLOR(0, 255, 0, 255));
  }
  if (stageNo == Stages::SetHeight)
  {
    for (const Point3 &point : poly.points)
    {
      draw_debug_sph(point, POLYGON_POINT_SIZE, selected_color);
      Point3 topPoint = point;
      topPoint.y = poly.topY;
      draw_debug_line(point, topPoint, selected_color);
    }

    renderPoly(poly.points, true);
    renderPoly(poly.getRoofPoints(), true);
  }
}

void PolygoneZoneCreator::switchStages()
{
  if (stageNo == Stages::SetPoints)
  {
    if (poly.points.size() < 3)
    {
      stateFinished = true;
      stateOk = false;
    }

    if (canEditHeight)
      stageNo = Stages::SetHeight;
    else
    {
      stateFinished = true;
      stateOk = true;
    }
  }
  else if (stageNo == Stages::SetHeight)
  {
    bool isValid = poly.points.size() >= 3 && poly.getHeight() > 0;
    stateFinished = true;
    stateOk = isValid;
  }
}

// --------------------------------------------------------------

void CapsuleCreator::render()
{
  const Point3 centre1(matrix.getcol(3));
  const Point3 centre2(centre1 + matrix.getcol(1));
  const real radius = matrix.getcol(0).length();
  const bool underClip = centre2.y < centre1.y;

  if (!underClip)
  {
    Capsule cap;
    // cap.set( centre1, centre2, radius );
    //::draw_debug_capsule(cap, ::selected_color, TMatrix::IDENT);
    cap.set(Point3(0, 0, 0), Point3(0, 1, 0), radius);
    cap.transform(matrix);
    ::draw_debug_capsule_w(cap, ::selected_color);
  }
  else
    ::draw_debug_sph(centre1, radius, E3DCOLOR(255, 0, 0, 255));
}


PolyMeshCreator::PolyMeshCreator() : points(tmpmem), wrapedX(0), wrapedY(0), stageNo(0), isRenderFirstPoint(false), isFailedMesh(false)
{}

bool PolyMeshCreator::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate)
{
  if (stateFinished || (points.size() < 2))
    return false;

  //::window_cursor_wrap( wnd->getCtlWindow(), x, y, wrapedX, wrapedY );
  const int pointCount = points.size();
  Point3 point(points[pointCount - 2].x, yPos, points[pointCount - 2].y);
  if (stageNo == 1)
  {
    Point3 worldPoint, worldDir;
    wnd->clientToWorld(Point2(x + wrapedX, y + wrapedY), worldPoint, worldDir);
    real t = (point.y - worldPoint.y) / worldDir.y;
    Point3 mousePos(worldPoint + t * worldDir);
    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      mousePos = grid.snapToGrid(mousePos);

    isFailedMesh = false;
    if (points.size() > 3)
    {
      const Point2 &p1 = points.back();
      const Point2 &p2 = points[0] + ::normalize(p1 - points[0]) * 0.01f;
      const Point2 &p3 = points[points.size() - 2] + ::normalize(p1 - points[points.size() - 2]) * 0.01f;
      for (int i = 0; i < (int)points.size() - 2; ++i)
        if (::is_lines_intersect(p1, p2, points[i], points[i + 1]) || ::is_lines_intersect(p1, p3, points[i], points[i + 1]))
        {
          isFailedMesh = true;
          break;
        }

      Tab<Point2> triangle(tmpmem);
      triangle.reserve(3);
      triangle.push_back(points.back());
      triangle.push_back(points[0]);
      triangle.push_back(points[points.size() - 2]);
      int i;
      for (i = 1; i < (int)points.size() - 2; ++i)
        if (::is_point_in_conv_poly(points[i], &triangle[0], 3))
          break;
      if (i != (points.size() - 2))
        isFailedMesh = true;
    }

    points[pointCount - 1] = Point2(mousePos.x, mousePos.z);

    Point2 startPointScreenPos;
    Point2 pointScreenPos;
    Point3 p;
    real znear, zfar;
    wnd->getZnearZfar(znear, zfar);
    IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, p, zfar, NULL);
    wnd->worldToClient(Point3(points[0].x, yPos, points[0].y), startPointScreenPos);
    wnd->worldToClient(p, pointScreenPos);
    if (::lengthSq(startPointScreenPos - pointScreenPos) < 9.0f)
      isRenderFirstPoint = true;
    else
      isRenderFirstPoint = false;
  }
  else if (stageNo == 2)
  {
    if (wnd->isOrthogonal())
    {
      height = clientPoint.y - (y + wrapedY);
      IEditorCoreEngine::get()->invalidateViewportCache();
      return false;
    }

    Point3 worldPoint2, worldDir2;
    wnd->clientToWorld(Point2(0, clientPoint.y), worldPoint2, worldDir2);

    Point3 worldPoint3, worldDir3;
    wnd->clientToWorld(Point2(0, y + wrapedY), worldPoint3, worldDir3);

    const real scalar = worldDir2 * worldDir3;
    height = 2.0f * (1.0f - scalar * scalar) * ::length(point - worldPoint3);
    if ((y + wrapedY) > clientPoint.y)
      height = -height;

    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap() && grid.getStep())
      height = (real)((int)(height / grid.getStep())) * grid.getStep();
  }
  IEditorCoreEngine::get()->invalidateViewportCache();
  return true;
}

void PolyMeshCreator::convertPoints()
{
  const Point3 v1(points[1].x - points[0].x, 0, points[1].y - points[0].y);
  const Point3 v2(points[2].x - points[1].x, 0, points[2].y - points[1].y);
  if ((v1 % v2).y > 0.0f)
  {
    Tab<Point2> oldPoints(points);
    points.resize(0);
    points.push_back(oldPoints[0]);
    for (int i = oldPoints.size() - 1; i > 0; --i)
      points.push_back(oldPoints[i]);
  }
}

bool PolyMeshCreator::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  Point3 point;
  real znear, zfar;
  wnd->getZnearZfar(znear, zfar);
  if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x + wrapedX, y + wrapedY, point, zfar, NULL))
  {
    stateFinished = true;
    return false;
  }
  const GridObject &grid = IEditorCoreEngine::get()->getGrid();
  if (grid.getMoveSnap())
    point = grid.snapToGrid(point);

  if (!stageNo)
  {
    stageNo = 1;
    height = 0;
    yPos = point.y;
    points.push_back(Point2(point.x, point.z));
    points.push_back(Point2(point.x, point.z));
    clientPoint = Point2(x, y);
    wrapedX = 0;
    wrapedY = 0;
  }
  else if (stageNo == 1)
  {
    if (isFailedMesh && !isRenderFirstPoint)
    {
      stateFinished = true;
      IEditorCoreEngine::get()->invalidateViewportCache();
    }
    else if (points.size())
    {
      if (isRenderFirstPoint && (points.size() > 2))
      {
        stageNo = 2;
        convertPoints();
      }
      else
      {
        points.back() = Point2(point.x, point.z);
        points.push_back(Point2(point.x, point.z));
      }

      clientPoint = Point2(x, y);
      wrapedX = 0;
      wrapedY = 0;
    }
  }
  else
  {
    stateFinished = true;
    const int pointCount = points.size();
    if (pointCount < 3)
      return false;
    stateOk = true;
    BBox2 box;
    for (int i = 0; i < pointCount; ++i)
      box += points[i];
    for (int i = 0; i < pointCount; ++i)
    {
      points[i] -= box.center();
      points[i].x /= box.width().x;
      points[i].y /= box.width().y;
    }
    matrix.setcol(0, matrix.getcol(0) * box.width().x);
    matrix.setcol(1, matrix.getcol(1) * height);
    matrix.setcol(2, matrix.getcol(2) * box.width().y);
    matrix.setcol(3, Point3(box.center().x, yPos, box.center().y));
  }
  return true;
}

bool PolyMeshCreator::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;
  return true;
}

bool PolyMeshCreator::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if ((stageNo != 1) || isFailedMesh)
    stateFinished = true;
  else
  {
    stageNo = 2;
    erase_items(points, points.size() - 1, 1);
    convertPoints();
    clientPoint = Point2(x, y);
    wrapedX = 0;
    wrapedY = 0;
  }

  IEditorCoreEngine::get()->invalidateViewportCache();
  return true;
}

void PolyMeshCreator::renderSegment(int point1_id, int point2_id)
{
  if (point1_id == -1)
    ++point1_id;

  const Point3 pos1(points[point1_id].x, yPos, points[point1_id].y);
  const Point3 h(0, height, 0);
  ::draw_debug_line(pos1, pos1 + h, ::selected_color);

  if (point1_id == point2_id)
    return;
  const Point3 pos2(points[point2_id].x, yPos, points[point2_id].y);
  ::draw_debug_line(pos1, pos2, ::selected_color);
  ::draw_debug_line(pos1 + h, pos2 + h, ::selected_color);
}

void PolyMeshCreator::render()
{
  const int pointCount = isFailedMesh ? points.size() - 1 : points.size();
  if (!pointCount)
    return;

  matrix.setcol(1, Point3(0, 1, 0) * height);

  for (int i = 0; i < pointCount; ++i)
    renderSegment(i - 1, i);
  if (!isFailedMesh)
    renderSegment(pointCount - 1, 0);

  if (isRenderFirstPoint)
  {
    if (isFailedMesh)
      renderSegment(pointCount - 1, 0);

    Point2 coord;
    Point3 pos, dir;
    IGenViewportWnd *wnd = IEditorCoreEngine::get()->getRenderViewport();
    if (!wnd)
      return;
    const Point3 originPos(points[0].x, yPos, points[0].y);
    wnd->worldToClient(originPos, coord);
    wnd->clientToWorld(coord, pos, dir);
    ::draw_debug_sph(pos + dir * 5.0f, 0.04f, E3DCOLOR(255, 0, 0));
  }
}

///////////////////////////////////////////////////////////////////////////////
// SplineCreator
///////////////////////////////////////////////////////////////////////////////

SplineCreator::SplineCreator() : points(tmpmem), isRenderFirstPoint(false), isClosed(false) {}

void SplineCreator::render()
{
  ::begin_draw_cached_debug_lines();
  ::set_cached_debug_lines_wtm(TMatrix::IDENT);
  if (points.size() > 1)
    ::draw_cached_debug_line(&points[0], points.size(), ::neutral_color);
  if (points.size() > 2)
  {
    BezierSpline3d spline;
    spline.calculateCatmullRom(&points[0], isClosed ? points.size() - 1 : points.size(), isRenderFirstPoint);
    float length = spline.getLength();
    const int numAllPoints = 1000;
    Point3 allPoints[numAllPoints];
    for (int i = 0; i < numAllPoints; ++i)
      allPoints[i] = spline.interp_pt(length * ((float)i) / numAllPoints);
    ::draw_cached_debug_line(&allPoints[0], numAllPoints, ::higlighted_selected_color);
  }
  ::end_draw_cached_debug_lines();

  if (isRenderFirstPoint)
  {
    Point2 coord;
    Point3 pos, dir;
    IGenViewportWnd *wnd = IEditorCoreEngine::get()->getRenderViewport();
    if (!wnd)
      return;
    const Point3 originPos = points[0];
    wnd->worldToClient(originPos, coord);
    wnd->clientToWorld(coord, pos, dir);
    ::draw_debug_sph(pos + dir * 5.0f, 0.04f, E3DCOLOR(255, 0, 0));
  }
}

bool SplineCreator::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate)
{
  if (stateFinished || (points.size() < 2))
    return false;


  Point3 p;
  real znear, zfar;
  wnd->getZnearZfar(znear, zfar);
  if (IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, p, zfar, NULL))
  {
    Point2 startPointScreenPos;
    Point2 pointScreenPos;
    const GridObject &grid = IEditorCoreEngine::get()->getGrid();
    if (grid.getMoveSnap())
      p = grid.snapToGrid(p);
    points.back() = p;
    wnd->worldToClient(points[0], startPointScreenPos);
    wnd->worldToClient(p, pointScreenPos);
    if (points.size() > 3 && ::lengthSq(startPointScreenPos - pointScreenPos) < 9.0f)
      isClosed = isRenderFirstPoint = true;
    else
      isClosed = isRenderFirstPoint = false;

    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  return true;
}

bool SplineCreator::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (points.size() && !isClosed)
    points.pop_back();
  if (points.size() >= 2)
    stateOk = true;
  else
    stateOk = false;
  stateFinished = true;
  return true;
}

bool SplineCreator::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (stateFinished)
    return false;

  Point3 point;
  real znear, zfar;
  wnd->getZnearZfar(znear, zfar);
  if (!IEditorCoreEngine::get()->clientToWorldTrace(wnd, x, y, point, zfar, NULL))
  {
    // stateFinished = true;
    return false;
  }
  const GridObject &grid = IEditorCoreEngine::get()->getGrid();
  if (grid.getMoveSnap())
    point = grid.snapToGrid(point);
  Point2 prevPointScreenPos, pointScreenPos;
  if (points.size() > 1)
  {
    wnd->worldToClient(points[points.size() - 2], prevPointScreenPos);
    wnd->worldToClient(point, pointScreenPos);
    if (::lengthSq(prevPointScreenPos - pointScreenPos) < 9.0f)
    {
      return false;
    }
  }
  if (!points.size())
    points.push_back(point);
  if (isClosed)
    stateOk = stateFinished = true;
  else
    points.push_back(point);
  return true;
}

void StairCreator::render()
{
  matrix.setcol(3, Point3(0.f, 0.f, 0.f));
  matrix.setcol(2, normalize(point1 - point0));
  matrix.setcol(1, Point3(0.f, 1.f, 0.f));
  matrix.setcol(0, matrix.getcol(1) % matrix.getcol(2));
  float m_det = matrix.det();
  if (fabs(m_det) < 1e-6)
    return;

  TMatrix matrixInv = inverse(matrix, m_det);
  Point3 point0Box = point0 * matrixInv;
  Point3 point1Box = point1 * matrixInv;
  Point3 point2Box = point2 * matrixInv;
  Point3 centerBox = Point3(0.5f * (point0Box.x + point2Box.x), point0Box.y + 0.5f * height, 0.5f * (point0Box.z + point1Box.z));
  matrix.setcol(3, centerBox * matrix);
  Point3 halfSize(fabsf(point2Box.x - point0Box.x) * 0.5f, height * 0.5f, fabsf(point1Box.z - point0Box.z) * 0.5f);

  Point3 p[8];
  p[0] = matrix * Point3(-halfSize.x, -halfSize.y, -halfSize.z);
  p[1] = matrix * Point3(-halfSize.x, -halfSize.y, halfSize.z);
  p[2] = matrix * Point3(-halfSize.x, halfSize.y, -halfSize.z);
  p[3] = matrix * Point3(-halfSize.x, halfSize.y, halfSize.z);
  p[4] = matrix * Point3(halfSize.x, -halfSize.y, -halfSize.z);
  p[5] = matrix * Point3(halfSize.x, -halfSize.y, halfSize.z);
  p[6] = matrix * Point3(halfSize.x, halfSize.y, -halfSize.z);
  p[7] = matrix * Point3(halfSize.x, halfSize.y, halfSize.z);

  const bool underClip = height < 0;

  if (!underClip)
  {
    draw_debug_line(p[0], p[1], selected_color);
    draw_debug_line(p[0], p[2], selected_color);
    draw_debug_line(p[0], p[4], selected_color);
    draw_debug_line(p[1], p[5], selected_color);
    draw_debug_line(p[4], p[6], selected_color);
    draw_debug_line(p[4], p[5], selected_color);
    draw_debug_line(p[1], p[2], selected_color);
    draw_debug_line(p[5], p[6], selected_color);
    draw_debug_line(p[2], p[6], selected_color);
  }
  else
  {
    ::draw_debug_line(p[0], p[1], E3DCOLOR(255, 0, 0, 255));
    ::draw_debug_line(p[1], p[5], E3DCOLOR(255, 0, 0, 255));
    ::draw_debug_line(p[5], p[4], E3DCOLOR(255, 0, 0, 255));
    ::draw_debug_line(p[4], p[0], E3DCOLOR(255, 0, 0, 255));
  }

  matrix.setcol(0, ::normalize(matrix.getcol(0)) * fabsf(point2Box.x - point0Box.x));

  if (!underClip)
    matrix.setcol(1, ::normalize(matrix.getcol(1)) * height);
  else
  {
    matrix.setcol(1, Point3(0, 0, 0));
  }

  matrix.setcol(2, ::normalize(matrix.getcol(2)) * fabsf(point1Box.z - point0Box.z));
  matrix.setcol(3, matrix.getcol(3) - Point3(0, height / 2, 0));
}


void SpiralStairCreator::render()
{
  const Point3 centre1(matrix.getcol(3));
  const Point3 centre2(centre1 + matrix.getcol(1));
  const real radius = matrix.getcol(0).length();

  const bool underClip = centre2.y < centre1.y;

  ::draw_debug_xz_circle(centre1, radius, underClip ? E3DCOLOR(255, 0, 0, 255) : ::selected_color);

  if (!underClip)
  {
    ::draw_debug_xz_circle(centre2, radius, ::selected_color);

    const real height = matrix.getcol(1).y;
    real cosinus = radius * cosf(0);
    real sinus = radius * sinf(0);
    real dh = 0;
    real an = 0;
    for (int i = 1; i <= 15; ++i)
    {
      an += TWOPI / 13.0f;
      const real nextCosinus = radius * cosf(an);
      const real nextSinus = radius * sinf(an);
      const real nextDh = dh + height / 15.0f;
      const Point3 pos1(cosinus + centre1.x, centre1.y + dh, sinus + centre1.z);
      const Point3 pos2(nextCosinus + centre1.x, centre1.y + nextDh, nextSinus + centre1.z);
      ::draw_debug_line(pos1, pos2, ::selected_color);
      cosinus = nextCosinus;
      sinus = nextSinus;
      dh = nextDh;
    }
  }
}
