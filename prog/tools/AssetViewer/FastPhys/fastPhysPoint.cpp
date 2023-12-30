#include <libTools/fastPhysData/fp_edpoint.h>

#include <debug/dag_debug3d.h>

#include "fastPhysPanel.h"
#include "fastPhysPoint.h"

//------------------------------------------------------------------

real FPObjectPoint::pointRadius = 0.02f;

E3DCOLOR FPObjectPoint::normalColor(50, 255, 50);
E3DCOLOR FPObjectPoint::selectedColor(255, 255, 255);

enum
{
  PID_GROUPID = PID_OBJ_FIRST,
  PID_GRAVITY,
  PID_DAMPING,
  PID_WINDK,
  PID_POINT_NODE,
  PID_LOCALPOS,

  PID_EDPOINT__LAST
};


class FastPhysUndoEdPointParams : public UndoRedoObject
{
public:
  struct Params
  {
    int groupId;
    real gravity;
    real damping;
    real windK;

    void getFrom(FPObjectPoint *o)
    {
      FpdPoint *pointObject = (FpdPoint *)o->getObject();
      G_ASSERT(pointObject);

      groupId = pointObject->groupId;
      gravity = pointObject->gravity;
      damping = pointObject->damping;
      windK = pointObject->windK;
    }

    void setTo(FPObjectPoint *o)
    {
      FpdPoint *pointObject = (FpdPoint *)o->getObject();
      G_ASSERT(pointObject);

      pointObject->groupId = groupId;
      pointObject->gravity = gravity;
      pointObject->damping = damping;
      pointObject->windK = windK;
      o->getObjEditor()->refillPanel();
    }
  };

  Ptr<FPObjectPoint> object;
  Params undoParams, redoParams;


  FastPhysUndoEdPointParams(FPObjectPoint *o) : object(o)
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
  virtual void get_description(String &s) { s = "FastPhysUndoEdPointParams"; }
};


FPObjectPoint::FPObjectPoint(FpdObject *obj, FastPhysEditor &editor) : IFPObject(obj, editor) { IFPObject::setPos(obj->getPos()); }


void FPObjectPoint::refillPanel(PropPanel2 *panel)
{
  FpdPoint *pointObject = (FpdPoint *)getObject();
  G_ASSERT(pointObject);

  panel->createEditInt(PID_GROUPID, "Group ID:", pointObject->groupId);
  panel->createEditFloat(PID_GRAVITY, "Gravity:", pointObject->gravity);
  panel->createEditFloat(PID_DAMPING, "Damping:", pointObject->damping);
  panel->createEditFloat(PID_WINDK, "Wind K:", pointObject->windK);
  panel->createEditBox(PID_POINT_NODE, "Linked to:", pointObject->node ? mFPEditor.nodeTree.getNodeName(pointObject->node) : "",
    false);
  panel->createPoint3(PID_LOCALPOS, "Local pos:", as_point3(&pointObject->localPos), 2, false);

  /*

  for (int i=0; i<mFPEditor.selectedCount(); ++i)
  {
    RenderableEditableObject *o = mFPEditor.getSelected(i);

    IFPObject *wobj = NULL;

    if (!o || !o->isSubOf(IFPObject::HUID) || !(wobj = (IFPObject *) o) )
      return NULL;
  }

  for (int i=0; i<objects.size(); ++i)
  {
    FastPhysEdPoint *o=(FastPhysEdPoint*)objects[i];

    if (o->groupId!=groupId) op.setClear(PID_GROUPID, true);
    if (o->gravity!=gravity) op.setClear(PID_GRAVITY, true);
    if (o->damping!=damping) op.setClear(PID_DAMPING, true);
    if (o->windK!=windK) op.setClear(PID_WINDK, true);
    if (o->node!=node) op.setClear(PID_POINT_NODE, true);
    //==if (o->localPos!=localPos) op.setClear(PID_LOCALPOS, true);
  }
  */
}


void FPObjectPoint::onChange(int pcb_id, PropPanel2 *panel)
{
  FpdPoint *pointObject = (FpdPoint *)getObject();
  G_ASSERT(pointObject);

  /*
  #define CHANGE_VAL(type, pname, getfunc) \
  { \
    if (!edit_finished) return; \
    type val=panel.getfunc(pid, pname); \
    for (int i=0; i<objects.size(); ++i) \
    { \
      FastPhysEdPoint *o=(FastPhysEdPoint*)objects[i]; \
      objEditor->getUndoSystem()->put(new FastPhysUndoEdPointParams(o)); \
      o->pname=val; \
    } \
  }
  */

#define CHANGE_VAL(type, pname, getfunc)                                 \
  {                                                                      \
    type val = panel->getfunc(pcb_id);                                   \
    mFPEditor.getUndoSystem()->begin();                                  \
    mFPEditor.getUndoSystem()->put(new FastPhysUndoEdPointParams(this)); \
    mFPEditor.getUndoSystem()->accept("PointChange");                    \
    pointObject->pname = val;                                            \
  }

  if (pcb_id == PID_GROUPID)
    CHANGE_VAL(int, groupId, getInt)
  else if (pcb_id == PID_GRAVITY)
    CHANGE_VAL(real, gravity, getFloat)
  else if (pcb_id == PID_DAMPING)
    CHANGE_VAL(real, damping, getFloat)
  else if (pcb_id == PID_WINDK)
    CHANGE_VAL(real, windK, getFloat)

#undef CHANGE_VAL
}


E3DCOLOR FPObjectPoint::getColor() const
{
  if (isSelected())
    return selectedColor;
  if (isFrozen())
    return frozenColor;
  return normalColor;
}

void FPObjectPoint::render()
{
  if (!mFPEditor.isObjectDebugVisible)
    return;

  ::set_cached_debug_lines_wtm(mFPEditor.getRootWtm() * getWtm());

  E3DCOLOR color = getColor();

  real r = pointRadius;

  ::draw_cached_debug_line(Point3(-r, 0, 0), Point3(+r, 0, 0), color);
  ::draw_cached_debug_line(Point3(0, -r, 0), Point3(0, +r, 0), color);
  ::draw_cached_debug_line(Point3(0, 0, -r), Point3(0, 0, +r), color);
}


bool FPObjectPoint::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
{
  return ::is_sphere_hit_by_point(getPos(), pointRadius, vp, x, y);
}


bool FPObjectPoint::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
{
  return ::is_sphere_hit_by_rect(getPos(), pointRadius, vp, rect);
}


bool FPObjectPoint::getWorldBox(BBox3 &box) const
{
  box.makecube(getPos(), pointRadius * 2);
  return true;
}


bool FPObjectPoint::setPos(const Point3 &p)
{
  FpdPoint *pointObject = (FpdPoint *)getObject();
  G_ASSERT(pointObject);

  pointObject->setPos(p);

  return IFPObject::setPos(p);
}


//------------------------------------------------------------------
