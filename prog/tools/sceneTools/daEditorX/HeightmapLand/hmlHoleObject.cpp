#include "hmlHoleObject.h"
#include "hmlCm.h"
#include "common.h"

#include <dllPluginCore/core.h>
#include <math/dag_math2d.h>

#include <EditorCore/ec_rect.h>


HmapLandHoleObject::HmapLandHoleObject() : boxSize(1, 1, 1)
{
  normal_color = E3DCOLOR(200, 10, 10);
  selected_color = E3DCOLOR(255, 255, 255);
}


void HmapLandHoleObject::update(real dt) {}


void HmapLandHoleObject::beforeRender() {}


void HmapLandHoleObject::render()
{
  BBox3 box;
  getWorldBox(box);

  dagRender->setLinesTm(TMatrix::IDENT);
  dagRender->renderBox(box, isSelected() ? selected_color : normal_color);
}


void HmapLandHoleObject::renderTrans() {}


void HmapLandHoleObject::buildBoxEdges(IGenViewportWnd *vp, Point2 edges[12][2]) const
{
  BBox3 box;
  getWorldBox(box);

  Point2 verts[2][2][2];

  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k)
        vp->worldToClient(Point3(box[i].x, box[j].y, box[k].z), verts[i][j][k]);

  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
    {
      edges[i * 2 + j + 0][0] = verts[0][i][j];
      edges[i * 2 + j + 0][1] = verts[1][i][j];
    }

  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
    {
      edges[i * 2 + j + 4][0] = verts[i][0][j];
      edges[i * 2 + j + 4][1] = verts[i][1][j];
    }

  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
    {
      edges[i * 2 + j + 8][0] = verts[i][j][0];
      edges[i * 2 + j + 8][1] = verts[i][j][1];
    }
}


bool HmapLandHoleObject::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
{
  Point2 edges[12][2];
  buildBoxEdges(vp, edges);

  BBox2 rectBox;
  int r = 2;
  rectBox[0].x = rect.l - r;
  rectBox[1].x = rect.r + r;
  rectBox[0].y = rect.t - r;
  rectBox[1].y = rect.b + r;

  for (int i = 0; i < 12; ++i)
    if (::isect_line_segment_box(edges[i][0], edges[i][1], rectBox))
      return true;

  return false;
}


bool HmapLandHoleObject::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
{
  Point2 edges[12][2];
  buildBoxEdges(vp, edges);

  Point2 p(x, y);

  for (int i = 0; i < 12; ++i)
    if (::distance_point_to_line_segment(p, edges[i][0], edges[i][1]) < 3)
      return true;

  return false;
}


bool HmapLandHoleObject::getWorldBox(BBox3 &box) const
{
  Point3 center = getPos();

  Point3 size;
  size.x = fabsf(boxSize.x);
  size.y = fabsf(boxSize.y);
  size.z = fabsf(boxSize.z);

  box[0] = center - size * 0.5f;
  box[1] = center + size * 0.5f;
  return true;
}


void HmapLandHoleObject::save(DataBlock &blk)
{
  blk.setStr("name", getName());
  blk.setPoint3("pos", getPos());
  blk.setPoint3("boxSize", boxSize);
}


void HmapLandHoleObject::load(const DataBlock &blk)
{
  getObjEditor()->setUniqName(this, blk.getStr("name", "HoleBox"));
  setPos(blk.getPoint3("pos", getPos()));
  boxSize = blk.getPoint3("boxSize", boxSize);
}


class UndoHmapLandObjectParams : public UndoRedoObject
{
public:
  struct Params
  {
    Point3 boxSize;

    void getFrom(HmapLandHoleObject *o)
    {
      boxSize = o->getBoxSize();
      o->getObjEditor()->invalidateObjectProps();
    }

    void setTo(HmapLandHoleObject *o)
    {
      o->setBoxSize(boxSize);
      o->getObjEditor()->invalidateObjectProps();
      if (HmapLandPlugin::self)
        HmapLandPlugin::self->invalidateRenderer();
    }
  };

  Ptr<HmapLandHoleObject> object;
  Params undoParams, redoParams;


  UndoHmapLandObjectParams(HmapLandHoleObject *o) : object(o)
  {
    undoParams.getFrom(object);
    redoParams.getFrom(object);
  }

  virtual void restore(bool save_redo)
  {
    if (save_redo)
      redoParams.getFrom(object);
    undoParams.setTo(object);
  }

  virtual void redo() { redoParams.setTo(object); }

  virtual size_t size() { return sizeof(*this); }
  virtual void accepted() {}
  virtual void get_description(String &s) { s = "UndoHmapLandObjectParams"; }
};


void HmapLandHoleObject::fillProps(PropPanel2 &panel, DClassID for_class_id, dag::ConstSpan<RenderableEditableObject *> objects)
{
  panel.createEditFloat(PID_BOX_SIZE_X, "Box size X:", boxSize.x);
  panel.createEditFloat(PID_BOX_SIZE_Y, "Box size Y (visual):", boxSize.y);
  panel.createEditFloat(PID_BOX_SIZE_Z, "Box size Z:", boxSize.z);

  for (int i = 0; i < objects.size(); ++i)
  {
    HmapLandHoleObject *o = RTTI_cast<HmapLandHoleObject>(objects[i]);
    if (!o)
      continue;

    if (o->boxSize.x != boxSize.x)
      panel.resetById(PID_BOX_SIZE_X);
    if (o->boxSize.y != boxSize.y)
      panel.resetById(PID_BOX_SIZE_Y);
    if (o->boxSize.z != boxSize.z)
      panel.resetById(PID_BOX_SIZE_Z);
  }
}


void HmapLandHoleObject::onPPChange(int pid, bool edit_finished, PropPanel2 &panel, dag::ConstSpan<RenderableEditableObject *> objects)
{
#define CHANGE_VAL(type, pname, getfunc)                                     \
  {                                                                          \
    if (!edit_finished)                                                      \
      return;                                                                \
    type val = panel.getfunc(pid);                                           \
    if (fabsf(val) < 1e-6)                                                   \
      val = 1;                                                               \
    for (int i = 0; i < objects.size(); ++i)                                 \
    {                                                                        \
      HmapLandHoleObject *o = (HmapLandHoleObject *)objects[i];              \
      getObjEditor()->getUndoSystem()->put(new UndoHmapLandObjectParams(o)); \
      o->pname = val;                                                        \
    }                                                                        \
    HmapLandPlugin::self->resetRenderer();                                   \
  }

  if (pid == PID_BOX_SIZE_X)
    CHANGE_VAL(float, boxSize.x, getFloat)
  else if (pid == PID_BOX_SIZE_Y)
    CHANGE_VAL(float, boxSize.y, getFloat)
  else if (pid == PID_BOX_SIZE_Z)
    CHANGE_VAL(float, boxSize.z, getFloat)

  setBoxSize(boxSize);

  // else __super::onPPChange(pid, edit_finished, panel, objects);

#undef CHANGE_VAL
}

void HmapLandHoleObject::scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis)
{
  __super::scaleObject(delta, origin, basis);

  boxSize.x = matrix.getcol(0).x;
  boxSize.y = matrix.getcol(1).y;
  boxSize.z = matrix.getcol(2).z;

  objectWasScaled = true;
}

void HmapLandHoleObject::setBoxSize(Point3 &s)
{
  boxSize = s;
  Point3 pos = getPos();

  matrix.setcol(0, Point3(s.x, 0, 0));
  matrix.setcol(1, Point3(0, s.y, 0));
  matrix.setcol(2, Point3(0, 0, s.z));
  matrix.setcol(3, pos);
}
