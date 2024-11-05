// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_rendEdObject.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <EditorCore/ec_rect.h>

#include <debug/dag_debug.h>


#define MAX_TRACE_DIST 1000.0


bool is_sphere_hit_by_point(const Point3 &center, real wrad, IGenViewportWnd *vp, int x, int y)
{
  Point2 cp;
  real z;
  vp->worldToClient(center, cp, &z);

  if (z < 0)
    return false;

  real rx2 = vp->getLinearSizeSq(center, wrad, 0);
  real ry2 = vp->getLinearSizeSq(center, wrad, 1);

  Point2 dp = Point2(x, y) - cp;

  return dp.x * dp.x * ry2 + dp.y * dp.y * rx2 <= rx2 * ry2;
}


bool is_sphere_hit_by_rect(const Point3 &center, real wrad, IGenViewportWnd *vp, const EcRect &rect)
{
  Point2 cp;
  real z;
  vp->worldToClient(center, cp, &z);

  if (z < 0)
    return false;

  //== tested as rectangle for now, not ellipse

  real rx = sqrtf(vp->getLinearSizeSq(center, wrad, 0));
  real ry = sqrtf(vp->getLinearSizeSq(center, wrad, 1));

  return cp.x + rx >= rect.l && cp.x - rx <= rect.r && cp.y + ry >= rect.t && cp.y - ry <= rect.b;
}

real get_world_rad(const Point3 &size)
{
  real wrad = 0.5;
  if (wrad < size.x)
    wrad = size.x;
  if (wrad < size.y)
    wrad = size.y;
  if (wrad < size.z)
    wrad = size.z;
  return wrad;
}


void RenderableEditableObject::setFlags(int value, int mask, bool use_undo)
{

  int flagsBefore = objFlags;
  int newFlags = (objFlags & ~mask) | (value & mask);

  if (newFlags != flagsBefore)
  {
    if (use_undo && objEditor)
      objEditor->getUndoSystem()->put(new (midmem) UndoObjFlags(this));

    objFlags = newFlags;

    if (objEditor)
      objEditor->onObjectFlagsChange(this, newFlags ^ flagsBefore);
  }
}


RenderableEditableObject::~RenderableEditableObject() {}


void RenderableEditableObject::removeFromEditor()
{
  if (objEditor)
  {
    RenderableEditableObject *o = this;
    objEditor->removeObject(o, false);
  }
}

bool RenderableEditableObject::setPos(const Point3 &p)
{
  TMatrix wtm = matrix;
  wtm.setcol(3, p);
  setWtm(wtm);
  return true;
}

bool RenderableEditableObject::setSize(const Point3 &p)
{
  TMatrix wtm = matrix;
  wtm.setcol(0, normalize(matrix.getcol(0)) * p.x);
  wtm.setcol(1, normalize(matrix.getcol(1)) * p.y);
  wtm.setcol(2, normalize(matrix.getcol(2)) * p.z);
  setWtm(wtm);
  return true;
}

bool RenderableEditableObject::setRotM3(const Matrix3 &tm)
{
  Point3 p = Point3(matrix.getcol(0).length(), matrix.getcol(1).length(), matrix.getcol(2).length());
  TMatrix wtm = matrix;
  wtm.setcol(0, normalize(tm.getcol(0)) * p.x);
  wtm.setcol(1, normalize(tm.getcol(1)) * p.y);
  wtm.setcol(2, normalize(tm.getcol(2)) * p.z);
  setWtm(wtm);
  return true;
}

bool RenderableEditableObject::setRotTm(const TMatrix &tm)
{
  Point3 p = Point3(matrix.getcol(0).length(), matrix.getcol(1).length(), matrix.getcol(2).length());
  TMatrix wtm = matrix;
  wtm.setcol(0, normalize(tm.getcol(0)) * p.x);
  wtm.setcol(1, normalize(tm.getcol(1)) * p.y);
  wtm.setcol(2, normalize(tm.getcol(2)) * p.z);
  setWtm(wtm);
  return true;
}

void RenderableEditableObject::setMatrix(const Matrix3 &tm)
{
  TMatrix wtm = matrix;
  wtm.setcol(0, tm.getcol(0));
  wtm.setcol(1, tm.getcol(1));
  wtm.setcol(2, tm.getcol(2));
  setWtm(wtm);
}

void RenderableEditableObject::setWtm(const TMatrix &wtm)
{
  matrix = wtm;
  if (objEditor)
    objEditor->onObjectGeomChange(this);
}


// virtual Point3 getPos();
Point3 RenderableEditableObject::getSize() const
{
  return Point3(matrix.getcol(0).length(), matrix.getcol(1).length(), matrix.getcol(2).length());
}

Matrix3 RenderableEditableObject::getRotM3() const
{
  Matrix3 m3;
  m3.setcol(0, normalize(matrix.getcol(0)));
  m3.setcol(1, normalize(matrix.getcol(1)));
  m3.setcol(2, normalize(matrix.getcol(2)));
  return m3;
}

TMatrix RenderableEditableObject::getRotTm() const
{
  TMatrix tm;
  tm.setcol(0, normalize(matrix.getcol(0)));
  tm.setcol(1, normalize(matrix.getcol(1)));
  tm.setcol(2, normalize(matrix.getcol(2)));
  return tm;
}


Matrix3 RenderableEditableObject::getMatrix() const
{
  Matrix3 m3;
  m3.setcol(0, matrix.getcol(0));
  m3.setcol(1, matrix.getcol(1));
  m3.setcol(2, matrix.getcol(2));
  return m3;
}


void RenderableEditableObject::moveObject(const Point3 &delta, IEditorCoreEngine::BasisType basis) { setPos(getPos() + delta); }


void RenderableEditableObject::moveSurfObject(const Point3 &delta, IEditorCoreEngine::BasisType basis)
{
  setPos(delta + Point3(0, surfaceDist, 0));
}


void RenderableEditableObject::rememberSurfaceDist()
{
  if (usesRendinstPlacement())
  {
    surfaceDist = 0.0f; // placement forces object on surface
    return;
  }

  real dist = MAX_TRACE_DIST;
  Point3 pos = getPos();
  pos.y += 0.01;

  if (IEditorCoreEngine::get()->traceRay(pos, Point3(0, -1, 0), dist))
    surfaceDist = dist - 0.01;
  else
    surfaceDist = 0.0;
}


void RenderableEditableObject::rotateObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis)
{
  Matrix3 dtm = rotxM3(delta.x) * rotyM3(delta.y) * rotzM3(delta.z);

  if (basis == IEditorCoreEngine::BASIS_Local)
  {
    Matrix3 tm;
    TMatrix wtm;
    float sx = matrix.getcol(0).length();
    float sy = matrix.getcol(1).length();
    float sz = matrix.getcol(2).length();
    tm.setcol(0, matrix.getcol(0) / sx);
    tm.setcol(1, matrix.getcol(1) / sy);
    tm.setcol(2, matrix.getcol(2) / sz);
    tm = tm * dtm;

    wtm.setcol(0, tm.getcol(0) * sx);
    wtm.setcol(1, tm.getcol(1) * sy);
    wtm.setcol(2, tm.getcol(2) * sz);
    wtm.setcol(3, matrix.getcol(3));
    setWtm(wtm);
  }
  else
  {
    float wtm_det = getWtm().det();
    if (fabsf(wtm_det) < 1e-12)
      return;
    Point3 lorg = inverse(getWtm(), wtm_det) * origin;
    setMatrix(dtm * getMatrix());
    setPos(getPos() + origin - getWtm() * lorg);
  }
}


void RenderableEditableObject::scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis)
{
  Matrix3 dtm;
  dtm.zero();
  dtm[0][0] = delta.x;
  dtm[1][1] = delta.y;
  dtm[2][2] = delta.z;

  if (basis == IEditorCoreEngine::BASIS_Local)
  {
    setMatrix(getMatrix() * dtm);

    if (objEditor && IEditorCoreEngine::get()->getGizmoCenterType() == IEditorCoreEngine::CENTER_Selection)
    {
      const Point3 pos = getPos();
      const Point3 dir = pos - origin;
      setPos(pos - dir + dtm * dir);
    }
  }
  else
  {
    float wtm_det = getWtm().det();
    if (fabsf(wtm_det) < 1e-12)
      return;
    Point3 lorg = inverse(getWtm(), wtm_det) * origin;
    setMatrix(dtm * getMatrix());
    setPos(getPos() + origin - getWtm() * lorg);
  }
}


void RenderableEditableObject::putMoveUndo()
{
  if (objEditor)
    objEditor->getUndoSystem()->put(new (midmem) UndoMove(this));
}


void RenderableEditableObject::putRotateUndo()
{
  if (objEditor)
  {
    objEditor->getUndoSystem()->put(new (midmem) UndoMove(this));
    objEditor->getUndoSystem()->put(new (midmem) UndoMatrix(this));
  }
}


void RenderableEditableObject::putScaleUndo()
{
  if (objEditor)
  {
    objEditor->getUndoSystem()->put(new (midmem) UndoMove(this));
    objEditor->getUndoSystem()->put(new (midmem) UndoMatrix(this));
  }
}


void RenderableEditableObject::fillProps(PropPanel::ContainerPropertyControl &op, DClassID for_class_id,
  dag::ConstSpan<RenderableEditableObject *> objects)
{}


DClassID RenderableEditableObject::getCommonClassId(RenderableEditableObject **objects, int num)
{
  for (int i = 0; i < num; ++i)
    if (!objects[i]->isSubOf(CID_RenderableEditableObject))
      return NullCID;

  return CID_RenderableEditableObject;
}
