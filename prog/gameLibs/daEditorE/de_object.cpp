#include <daEditorE/de_object.h>
#include <daEditorE/de_objEditor.h>
#include <daEditorE/de_interface.h>

#include <math/dag_rayIntersectBox.h>
#include <math/dag_math2d.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>


#define MAX_TRACE_DIST 1000.0


void EditableObject::setFlags(int value, int mask, bool use_undo)
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


EditableObject::~EditableObject() {}


void EditableObject::removeFromEditor()
{
  if (objEditor)
  {
    EditableObject *o = this;
    objEditor->removeObject(o, false);
  }
}

void EditableObject::renderBox(const BBox3 &box) const
{
#define BOUND_BOX_LEN_DIV    4
#define BOUND_BOX_INDENT_MUL 0.03

  const real deltaX = box[1].x - box[0].x;
  const real deltaY = box[1].y - box[0].y;
  const real deltaZ = box[1].z - box[0].z;

  const real dx = deltaX / BOUND_BOX_LEN_DIV;
  const real dy = deltaY / BOUND_BOX_LEN_DIV;
  const real dz = deltaZ / BOUND_BOX_LEN_DIV;

  const E3DCOLOR c = isSelected() ? E3DCOLOR(0xff, 0, 0) : E3DCOLOR(0xff, 0xff, 0xff);

  ::set_cached_debug_lines_wtm(getWtm());

  ::draw_cached_debug_line(box[0], box[0] + Point3(dx, 0, 0), c);
  ::draw_cached_debug_line(box[0], box[0] + Point3(0, dy, 0), c);
  ::draw_cached_debug_line(box[0], box[0] + Point3(0, 0, dz), c);

  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, 0), box[0] + Point3(deltaX - dx, 0, 0), c);
  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, 0), box[0] + Point3(deltaX, dy, 0), c);
  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, 0), box[0] + Point3(deltaX, 0, dz), c);

  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, deltaZ), box[0] + Point3(deltaX - dx, 0, deltaZ), c);
  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, deltaZ), box[0] + Point3(deltaX, dy, deltaZ), c);
  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, deltaZ), box[0] + Point3(deltaX, 0, deltaZ - dz), c);

  ::draw_cached_debug_line(box[0] + Point3(0, 0, deltaZ), box[0] + Point3(dx, 0, deltaZ), c);
  ::draw_cached_debug_line(box[0] + Point3(0, 0, deltaZ), box[0] + Point3(0, dy, deltaZ), c);
  ::draw_cached_debug_line(box[0] + Point3(0, 0, deltaZ), box[0] + Point3(0, 0, deltaZ - dz), c);


  ::draw_cached_debug_line(box[1], box[1] - Point3(dx, 0, 0), c);
  ::draw_cached_debug_line(box[1], box[1] - Point3(0, dy, 0), c);
  ::draw_cached_debug_line(box[1], box[1] - Point3(0, 0, dz), c);

  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, 0), box[1] - Point3(deltaX - dx, 0, 0), c);
  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, 0), box[1] - Point3(deltaX, dy, 0), c);
  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, 0), box[1] - Point3(deltaX, 0, dz), c);

  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, deltaZ), box[1] - Point3(deltaX - dx, 0, deltaZ), c);
  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, deltaZ), box[1] - Point3(deltaX, dy, deltaZ), c);
  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, deltaZ), box[1] - Point3(deltaX, 0, deltaZ - dz), c);

  ::draw_cached_debug_line(box[1] - Point3(0, 0, deltaZ), box[1] - Point3(dx, 0, deltaZ), c);
  ::draw_cached_debug_line(box[1] - Point3(0, 0, deltaZ), box[1] - Point3(0, dy, deltaZ), c);
  ::draw_cached_debug_line(box[1] - Point3(0, 0, deltaZ), box[1] - Point3(0, 0, deltaZ - dz), c);

#undef BOUND_BOX_LEN_DIV
#undef BOUND_BOX_INDENT_MUL
}
bool EditableObject::isPivotSelectedByPointClick(int x, int y, int max_scr_dist) const
{
  Point2 p;
  if (!DAEDITOR4.worldToClient(matrix.getcol(3), p, NULL))
    return false;
  return p.x >= x - max_scr_dist && p.y >= y - max_scr_dist && p.x <= x + max_scr_dist && p.y <= y + max_scr_dist;
}
bool EditableObject::isPivotSelectedByRectangle(const IBBox2 &rect) const
{
  Point2 p;
  if (!DAEDITOR4.worldToClient(matrix.getcol(3), p, NULL))
    return false;
  return rect & IPoint2(p.x, p.y);
}
bool EditableObject::isBoxSelectedByPointClick(const BBox3 &lbb, int x, int y) const
{
  Point3 dir, p0;
  float out_t;

  DAEDITOR4.clientToWorld(Point2(x, y), p0, dir);
  return ray_intersect_box(p0, dir, lbb, getWtm(), out_t);
}
bool EditableObject::isBoxSelectedByRectangle(const BBox3 &box, const IBBox2 &rect) const
{
  Point2 cp[8];
  BBox2 rbox(Point2::xy(rect[0]), Point2::xy(rect[1]));
  BBox2 box2;
  real z;
  bool in_frustum = false;

  TMatrix tm = getWtm();
#define TEST_POINT(i, P)                                             \
  in_frustum |= DAEDITOR4.worldToClient(tm * P, cp[i], &z) && z > 0; \
  if (z > 0 && rbox & cp[i])                                         \
    return true;                                                     \
  else                                                               \
    box2 += cp[i];

#define TEST_SEGMENT(i, j)                          \
  if (::isect_line_segment_box(cp[i], cp[j], rbox)) \
  return true

  for (int i = 0; i < 8; i++)
  {
    TEST_POINT(i, box.point(i));
  }

  if (!in_frustum)
    return false;
  if (!(box2 & rbox))
    return false;

  TEST_SEGMENT(0, 4);
  TEST_SEGMENT(4, 5);
  TEST_SEGMENT(5, 1);
  TEST_SEGMENT(1, 0);
  TEST_SEGMENT(2, 6);
  TEST_SEGMENT(6, 7);
  TEST_SEGMENT(7, 3);
  TEST_SEGMENT(3, 2);
  TEST_SEGMENT(0, 2);
  TEST_SEGMENT(1, 3);
  TEST_SEGMENT(4, 6);
  TEST_SEGMENT(5, 7);

#undef TEST_POINT
#undef TEST_SEGMENT

  return isSelectedByPointClick(rect[0].x, rect[0].y);
}
bool EditableObject::isSphSelectedByPointClick(const BSphere3 &lbs, int x, int y) const
{
  return is_sphere_hit_by_point(getWtm() * lbs.c, lbs.r * getWtm().getcol(0).length(), x, y);
}
bool EditableObject::isSphSelectedByRectangle(const BSphere3 &lbs, const IBBox2 &rect) const
{
  return is_sphere_hit_by_rect(getWtm() * lbs.c, lbs.r * getWtm().getcol(0).length(), rect);
}

bool EditableObject::setPos(const Point3 &p)
{
  TMatrix wtm = matrix;
  wtm.setcol(3, p);
  setWtm(wtm);
  return true;
}

bool EditableObject::setSize(const Point3 &p)
{
  TMatrix wtm = matrix;
  wtm.setcol(0, normalize(matrix.getcol(0)) * p.x);
  wtm.setcol(1, normalize(matrix.getcol(1)) * p.y);
  wtm.setcol(2, normalize(matrix.getcol(2)) * p.z);
  setWtm(wtm);
  return true;
}

bool EditableObject::setRotM3(const Matrix3 &tm)
{
  Point3 p = Point3(matrix.getcol(0).length(), matrix.getcol(1).length(), matrix.getcol(2).length());
  TMatrix wtm = matrix;
  wtm.setcol(0, normalize(tm.getcol(0)) * p.x);
  wtm.setcol(1, normalize(tm.getcol(1)) * p.y);
  wtm.setcol(2, normalize(tm.getcol(2)) * p.z);
  setWtm(wtm);
  return true;
}

bool EditableObject::setRotTm(const TMatrix &tm)
{
  Point3 p = Point3(matrix.getcol(0).length(), matrix.getcol(1).length(), matrix.getcol(2).length());
  TMatrix wtm = matrix;
  wtm.setcol(0, normalize(tm.getcol(0)) * p.x);
  wtm.setcol(1, normalize(tm.getcol(1)) * p.y);
  wtm.setcol(2, normalize(tm.getcol(2)) * p.z);
  setWtm(wtm);
  return true;
}

void EditableObject::setMatrix(const Matrix3 &tm)
{
  TMatrix wtm = matrix;
  wtm.setcol(0, tm.getcol(0));
  wtm.setcol(1, tm.getcol(1));
  wtm.setcol(2, tm.getcol(2));
  setWtm(wtm);
}

void EditableObject::setWtm(const TMatrix &wtm)
{
  matrix = wtm;
  if (objEditor)
    objEditor->onObjectGeomChange(this);
}


Point3 EditableObject::getSize() const
{
  return Point3(matrix.getcol(0).length(), matrix.getcol(1).length(), matrix.getcol(2).length());
}

Matrix3 EditableObject::getRotM3() const
{
  Matrix3 m3;
  m3.setcol(0, normalize(matrix.getcol(0)));
  m3.setcol(1, normalize(matrix.getcol(1)));
  m3.setcol(2, normalize(matrix.getcol(2)));
  return m3;
}

TMatrix EditableObject::getRotTm() const
{
  TMatrix tm;
  tm.setcol(0, normalize(matrix.getcol(0)));
  tm.setcol(1, normalize(matrix.getcol(1)));
  tm.setcol(2, normalize(matrix.getcol(2)));
  return tm;
}


Matrix3 EditableObject::getMatrix() const
{
  Matrix3 m3;
  m3.setcol(0, matrix.getcol(0));
  m3.setcol(1, matrix.getcol(1));
  m3.setcol(2, matrix.getcol(2));
  return m3;
}

void EditableObject::moveObject(const Point3 &delta, IDaEditor4Engine::BasisType basis)
{
  G_UNUSED(basis);
  setPos(getPos() + delta);
}
void EditableObject::moveSurfObject(const Point3 &delta, IDaEditor4Engine::BasisType basis)
{
  G_UNUSED(basis);
  setPos(delta + Point3(0, surfaceDist, 0));
}

void EditableObject::rememberSurfaceDist()
{
  float dist = MAX_TRACE_DIST;
  Point3 pos = getPos();
  pos.y += 0.01;

  if (DAEDITOR4.traceRayEx(pos, Point3(0, -1, 0), dist, this))
    surfaceDist = dist - 0.01;
  else
    surfaceDist = 0.0;
}


void EditableObject::rotateObject(const Point3 &delta, const Point3 &origin, IDaEditor4Engine::BasisType basis)
{
  Matrix3 dtm = rotxM3(delta.x) * rotyM3(delta.y) * rotzM3(delta.z);

  const float sx = matrix.getcol(0).length();
  const float sy = matrix.getcol(1).length();
  const float sz = matrix.getcol(2).length();

  if (basis == IDaEditor4Engine::BASIS_local)
  {
    Matrix3 tm;
    tm.setcol(0, matrix.getcol(0) / sx);
    tm.setcol(1, matrix.getcol(1) / sy);
    tm.setcol(2, matrix.getcol(2) / sz);
    tm = tm * dtm;

    TMatrix wtm;
    wtm.setcol(0, tm.getcol(0));
    wtm.setcol(1, tm.getcol(1));
    wtm.setcol(2, tm.getcol(2));
    wtm.setcol(3, matrix.getcol(3));
    wtm.orthonormalize();
    wtm.col[0] *= sx;
    wtm.col[1] *= sy;
    wtm.col[2] *= sz;
    setWtm(wtm);
  }
  else
  {
    Point3 lorg = inverse(getWtm()) * origin;
    Matrix3 tm = dtm * getMatrix();

    TMatrix wtm;
    wtm.setcol(0, tm.getcol(0));
    wtm.setcol(1, tm.getcol(1));
    wtm.setcol(2, tm.getcol(2));
    wtm.setcol(3, matrix.getcol(3));
    wtm.orthonormalize();
    wtm.col[0] *= sx;
    wtm.col[1] *= sy;
    wtm.col[2] *= sz;
    setWtm(wtm);

    setPos(getPos() + origin - getWtm() * lorg);
  }
}


void EditableObject::scaleObject(const Point3 &delta, const Point3 &origin, IDaEditor4Engine::BasisType basis)
{
  Matrix3 dtm;
  dtm.zero();
  dtm[0][0] = delta.x;
  dtm[1][1] = delta.y;
  dtm[2][2] = delta.z;

  if (basis == IDaEditor4Engine::BASIS_local)
  {
    setMatrix(getMatrix() * dtm);

    if (objEditor && DAEDITOR4.getGizmoCenterType() == IDaEditor4Engine::CENTER_sel)
    {
      const Point3 pos = getPos();
      const Point3 dir = pos - origin;
      setPos(pos - dir + dtm * dir);
    }
  }
  else
  {
    Point3 lorg = inverse(getWtm()) * origin;

    Matrix3 m = getMatrix();
    Point3 scale(m.getcol(0).length(), m.getcol(1).length(), m.getcol(2).length());
    m = dtm * m;
    dtm[0][0] = m.getcol(0).length() / scale.x;
    dtm[1][1] = m.getcol(1).length() / scale.y;
    dtm[2][2] = m.getcol(2).length() / scale.z;
    setMatrix(getMatrix() * dtm);
    setPos(getPos() + origin - getWtm() * lorg);
  }
}


void EditableObject::putMoveUndo()
{
  if (objEditor)
  {
    objEditor->getUndoSystem()->put(new (midmem) UndoMove(this));
    objEditor->getUndoSystem()->put(new (midmem) UndoMatrix(this));
  }
}


void EditableObject::putRotateUndo()
{
  if (objEditor)
  {
    objEditor->getUndoSystem()->put(new (midmem) UndoMove(this));
    objEditor->getUndoSystem()->put(new (midmem) UndoMatrix(this));
  }
}


void EditableObject::putScaleUndo()
{
  if (objEditor)
  {
    objEditor->getUndoSystem()->put(new (midmem) UndoMove(this));
    objEditor->getUndoSystem()->put(new (midmem) UndoMatrix(this));
  }
}


/*
void EditableObject::fillProps(PropertyContainerControlBase &op, DClassID for_class_id,
  dag::ConstSpan<EditableObject*> objects)
{
}
*/

DClassID EditableObject::getCommonClassId(EditableObject **objects, int num)
{
  for (int i = 0; i < num; ++i)
    if (!objects[i]->isSubOf(CID_EditableObject))
      return NullCID;

  return CID_EditableObject;
}

bool EditableObject::is_sphere_hit_by_point(const Point3 &center, float wrad, int x, int y)
{
  Point2 cp;
  float z;
  DAEDITOR4.worldToClient(center, cp, &z);

  if (z < 0)
    return false;

  float rx2 = DAEDITOR4.getLinearSizeSq(center, wrad, 0);
  float ry2 = DAEDITOR4.getLinearSizeSq(center, wrad, 1);

  Point2 dp = Point2(x, y) - cp;

  return dp.x * dp.x * ry2 + dp.y * dp.y * rx2 <= rx2 * ry2;
}


bool EditableObject::is_sphere_hit_by_rect(const Point3 &center, float wrad, const IBBox2 &rect)
{
  Point2 cp;
  float z;
  DAEDITOR4.worldToClient(center, cp, &z);

  if (z < 0)
    return false;

  //== tested as rectangle for now, not ellipse

  float rx = sqrtf(DAEDITOR4.getLinearSizeSq(center, wrad, 0));
  float ry = sqrtf(DAEDITOR4.getLinearSizeSq(center, wrad, 1));

  return cp.x + rx >= rect[0].x && cp.x - rx <= rect[1].x && cp.y + ry >= rect[0].y && cp.y - ry <= rect[1].y;
}

float EditableObject::get_world_rad(const Point3 &size)
{
  float wrad = 0.5;
  if (wrad < size.x)
    wrad = size.x;
  if (wrad < size.y)
    wrad = size.y;
  if (wrad < size.z)
    wrad = size.z;
  return wrad;
}
