#include <libTools/fastPhysData/fp_edbone.h>
#include <libTools/fastPhysData/fp_edpoint.h>
#include <libTools/fastPhysData/fp_edclipper.h>

#include <phys/dag_fastPhys.h>


#include <debug/dag_debug3d.h>

#include <propPanel2/comWnd/list_dialog.h>

#include "fastPhysEd.h"
#include "fastPhysPanel.h"
#include "fastPhysClipper.h"
#include "fastPhysPoint.h"

//------------------------------------------------------------------
using hdpi::_pxScaled;

enum
{
  PID_NODE = PID_OBJ_FIRST,
  PID_RADIUS,
  PID_AXISLENGTH,
  PID_ANGLE,
  PID_LINESEGLEN,
  PID_CLIPTYPE,

  PID_POINTSLIST,
  PID_ADDPOINT,
  PID_REMOVEPOINT,

  PID_LINESLIST,
  PID_ADDLINE,
  PID_REMOVELINE,

  PID_EDCLIPPER__LAST
};


E3DCOLOR FPObjectClipper::normalColor(255, 255, 50);
E3DCOLOR FPObjectClipper::selectedColor(255, 255, 255);
E3DCOLOR FPObjectClipper::pointColor(200, 25, 25);
E3DCOLOR FPObjectClipper::selPointColor(255, 50, 255);


class FastPhysUndoEdClipperParams : public UndoRedoObject
{
public:
  struct Params
  {
    real radius;
    real axisLength;
    real angle;
    real lineSegLen;
    int clipType;
    Tab<String> pointNames;
    Tab<String> lineNames;

    Params() : pointNames(midmem), lineNames(midmem) {}

    void getFrom(FPObjectClipper *o)
    {
      FpdClipper *clipperObject = (FpdClipper *)o->getObject();
      G_ASSERT(clipperObject);

      radius = clipperObject->radius;
      axisLength = clipperObject->axisLength;
      angle = clipperObject->angle;
      lineSegLen = clipperObject->lineSegLen;
      clipType = clipperObject->clipType;

      clear_and_shrink(pointNames);
      for (int i = 0; i < clipperObject->pointNames.size(); ++i)
        pointNames.push_back() = clipperObject->pointNames[i].str();

      clear_and_shrink(lineNames);
      for (int i = 0; i < clipperObject->lineNames.size(); ++i)
        lineNames.push_back() = clipperObject->lineNames[i].str();
    }

    void setTo(FPObjectClipper *o)
    {
      FpdClipper *clipperObject = (FpdClipper *)o->getObject();
      G_ASSERT(clipperObject);

      clipperObject->radius = radius;
      clipperObject->axisLength = axisLength;
      clipperObject->angle = angle;
      clipperObject->lineSegLen = lineSegLen;
      clipperObject->clipType = clipType;

      clear_and_shrink(clipperObject->pointNames);
      for (int i = 0; i < pointNames.size(); ++i)
        clipperObject->pointNames.push_back() = pointNames[i].str();

      clear_and_shrink(clipperObject->lineNames);
      for (int i = 0; i < lineNames.size(); ++i)
        clipperObject->lineNames.push_back() = lineNames[i].str();

      o->getObjEditor()->refillPanel();
    }
  };

  Ptr<FPObjectClipper> object;
  Params undoParams, redoParams;


  FastPhysUndoEdClipperParams(FPObjectClipper *o) : object(o)
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
  virtual void get_description(String &s) { s = "FastPhysUndoEdClipperParams"; }
};


FPObjectClipper::FPObjectClipper(FpdObject *obj, FastPhysEditor &editor) : IFPObject(obj, editor)
{
  IFPObject::setPos(obj->getPos());
  Matrix3 m3;
  if (obj->getMatrix(m3))
    IFPObject::setMatrix(m3);
}


void FPObjectClipper::refillPanel(PropPanel2 *panel)
{
  FpdClipper *clipperObject = (FpdClipper *)getObject();
  G_ASSERT(clipperObject);

  PropPanel2 *rgrp = panel->createRadioGroup(PID_CLIPTYPE, "Type:");
  rgrp->createRadio(FpdClipper::CLIP_SPHERICAL, "spherical");
  rgrp->createRadio(FpdClipper::CLIP_CYLINDRICAL, "cylindrical");
  panel->setInt(PID_CLIPTYPE, clipperObject->clipType);

  panel->createEditFloat(PID_RADIUS, "Radius:", clipperObject->radius);
  panel->createEditFloat(PID_ANGLE, "Angle:", clipperObject->angle);
  panel->createEditFloat(PID_AXISLENGTH, "Axis length (visual):", clipperObject->axisLength);
  panel->createEditBox(PID_NODE, "Linked to:", clipperObject->node ? mFPEditor.nodeTree.getNodeName(clipperObject->node) : "", false);


  // points list
  Tab<String> lines(tmpmem);
  for (int i = 0; i < clipperObject->pointNames.size(); ++i)
    lines.push_back() = clipperObject->pointNames[i].str();

  panel->createList(PID_POINTSLIST, "Affected points:", lines, -1);

  panel->createButton(PID_ADDPOINT, "Add");
  panel->createButton(PID_REMOVEPOINT, "Remove", true, false);


  // line list
  clear_and_shrink(lines);
  for (int i = 0; i < clipperObject->lineNames.size(); ++i)
    lines.push_back() = clipperObject->lineNames[i].str();

  panel->createEditFloat(PID_LINESEGLEN, "Seg. len. (radius %):", clipperObject->lineSegLen);
  panel->createList(PID_LINESLIST, "Affected lines:", lines, -1);

  panel->createButton(PID_ADDLINE, "Add");
  panel->createButton(PID_REMOVELINE, "Remove", true, false);

  /*
  // multiselection handling
  for (int i=0; i<objects.size(); ++i)
  {
    FastPhysEdClipper *o=(FastPhysEdClipper*)objects[i];

    if (o->clipType!=clipType) rgrp->setValue(-1);
    if (o->radius!=radius) op.setClear(PID_RADIUS, true);
    if (o->axisLength!=axisLength) op.setClear(PID_AXISLENGTH, true);
    if (o->angle!=angle) op.setClear(PID_ANGLE, true);
    if (o->lineSegLen!=lineSegLen) op.setClear(PID_LINESEGLEN, true);
    if (o->node!=node) op.setClear(PID_NODE, true);
  }
  */
}

void FPObjectClipper::onChange(int pcb_id, PropPanel2 *panel)
{
  FpdClipper *clipperObject = (FpdClipper *)getObject();
  G_ASSERT(clipperObject);

  /*
  #define CHANGE_VAL(type, pname, getfunc) \
  { \
    if (!edit_finished) return; \
    type val=panel.getfunc(pid, pname); \
    for (int i=0; i<objects.size(); ++i) \
    { \
      FastPhysEdClipper *o=(FastPhysEdClipper*)objects[i]; \
      objEditor->getUndoSystem()->put(new FastPhysUndoEdClipperParams(o)); \
      o->pname=val; \
    } \
  }
  */

#define CHANGE_VAL(type, pname, getfunc)                                   \
  {                                                                        \
    type val = panel->getfunc(pcb_id);                                     \
    mFPEditor.getUndoSystem()->begin();                                    \
    mFPEditor.getUndoSystem()->put(new FastPhysUndoEdClipperParams(this)); \
    mFPEditor.getUndoSystem()->accept("ClipperChange");                    \
    clipperObject->pname = val;                                            \
  }

  if (pcb_id == PID_CLIPTYPE)
    CHANGE_VAL(int, clipType, getInt)
  else if (pcb_id == PID_RADIUS)
    CHANGE_VAL(real, radius, getFloat)
  else if (pcb_id == PID_AXISLENGTH)
    CHANGE_VAL(real, axisLength, getFloat)
  else if (pcb_id == PID_ANGLE)
    CHANGE_VAL(real, angle, getFloat)
  else if (pcb_id == PID_LINESEGLEN)
    CHANGE_VAL(real, lineSegLen, getFloat)

#undef CHANGE_VAL
}


void FPObjectClipper::onClick(int pcb_id, PropPanel2 *panel)
{
  FpdClipper *clipperObject = (FpdClipper *)getObject();
  G_ASSERT(clipperObject);

  Tab<String> _names(tmpmem), _sel_names(tmpmem);

  mFPEditor.getUndoSystem()->begin();
  mFPEditor.getUndoSystem()->put(new FastPhysUndoEdClipperParams(this));
  mFPEditor.getUndoSystem()->accept("ClipperListChange");

  if (pcb_id == PID_ADDPOINT)
  {
    // add point

    for (int i = 0; i < mFPEditor.objectCount(); ++i)
    {
      if (!mFPEditor.getObject(i)->isSubOf(IFPObject::HUID))
        continue;

      IFPObject *wo = (IFPObject *)mFPEditor.getObject(i);
      FpdObject *obj = wo->getObject();

      if (obj && obj->isSubOf(FpdPoint::HUID))
        _names.push_back() = obj->getName();
    }

    MultiListDialog dlg("Select points", _pxScaled(300), _pxScaled(400), _names, _sel_names);
    dlg.showDialog();

    if (!_sel_names.size())
      return;

    /*
    for (int j=0; j<objects.size(); ++j)
    {
      FastPhysEdClipper *o=(FastPhysEdClipper*)objects[j];
      objEditor->getUndoSystem()->put(new FastPhysUndoEdClipperParams(o));

      for (int i=0; i<list.selected.size(); ++i)
        o->addPoint(list.selected[i]->getName());
    }
    */

    for (int i = 0; i < _sel_names.size(); ++i)
      clipperObject->addPoint(_sel_names[i]);

    clear_and_shrink(_names);
    for (int i = 0; i < clipperObject->pointNames.size(); ++i)
      _names.push_back() = clipperObject->pointNames[i].str();

    panel->setStrings(PID_POINTSLIST, _names);
  }
  else if (pcb_id == PID_REMOVEPOINT)
  {
    // remove point

    /*
    for (int i=0; i<objects.size(); ++i)
    {
      FastPhysEdClipper *o=(FastPhysEdClipper*)objects[i];
      o->removePoint(name);
    }
    */

    int sel = panel->getInt(PID_POINTSLIST);
    if (sel > -1 && sel < clipperObject->pointNames.size())
      clipperObject->removePoint(clipperObject->pointNames[sel]);

    clear_and_shrink(_names);
    for (int i = 0; i < clipperObject->pointNames.size(); ++i)
      _names.push_back() = clipperObject->pointNames[i].str();

    panel->setStrings(PID_POINTSLIST, _names);
  }
  else if (pcb_id == PID_ADDLINE)
  {
    // add line

    for (int i = 0; i < mFPEditor.objectCount(); ++i)
    {
      if (!mFPEditor.getObject(i)->isSubOf(IFPObject::HUID))
        continue;

      IFPObject *wo = (IFPObject *)mFPEditor.getObject(i);
      FpdObject *obj = wo->getObject();

      if (obj && obj->isSubOf(FpdBone::HUID))
        _names.push_back() = obj->getName();
    }

    MultiListDialog dlg("Select lines", _pxScaled(300), _pxScaled(400), _names, _sel_names);
    dlg.showDialog();

    if (!_sel_names.size())
      return;

    /*
    for (int j=0; j<objects.size(); ++j)
    {
      FastPhysEdClipper *o=(FastPhysEdClipper*)objects[j];
      objEditor->getUndoSystem()->put(new FastPhysUndoEdClipperParams(o));

      for (int i=0; i<list.selected.size(); ++i)
        o->addLine(list.selected[i]->getName());
    }
    */

    for (int i = 0; i < _sel_names.size(); ++i)
      clipperObject->addLine(_sel_names[i]);

    clear_and_shrink(_names);
    for (int i = 0; i < clipperObject->lineNames.size(); ++i)
      _names.push_back() = clipperObject->lineNames[i].str();

    panel->setStrings(PID_LINESLIST, _names);
  }
  else if (pcb_id == PID_REMOVELINE)
  {
    // remove line
    /*
    for (int i=0; i<objects.size(); ++i)
    {
      FastPhysEdClipper *o=(FastPhysEdClipper*)objects[i];
      o->removeLine(name);
    }
    */

    int sel = panel->getInt(PID_LINESLIST);
    if (sel > -1 && sel < clipperObject->lineNames.size())
      clipperObject->removeLine(clipperObject->lineNames[sel]);

    clear_and_shrink(_names);
    for (int i = 0; i < clipperObject->lineNames.size(); ++i)
      _names.push_back() = clipperObject->lineNames[i].str();

    panel->setStrings(PID_LINESLIST, _names);
  }
}


E3DCOLOR FPObjectClipper::getColor() const
{
  if (isSelected())
    return selectedColor;
  if (isFrozen())
    return frozenColor;
  return normalColor;
}


int FPObjectClipper::calcLineSegs(real len)
{
  FpdClipper *clipperObject = (FpdClipper *)getObject();
  G_ASSERT(clipperObject);

  real step = clipperObject->radius * clipperObject->lineSegLen / 100;

  if (step == 0)
    return 2;

  int n = int(ceilf(len / step));
  if (n < 2)
    n = 2;

  return n;
}


void FPObjectClipper::render()
{
  if (!mFPEditor.isObjectDebugVisible)
    return;

  FpdClipper *clipperObject = (FpdClipper *)getObject();
  G_ASSERT(clipperObject);

  ::set_cached_debug_lines_wtm(mFPEditor.getRootWtm() * getWtm());

  E3DCOLOR color = getColor();

  const int MAX_SEGS = 32;
  Point2 pt[MAX_SEGS];

  if (clipperObject->clipType == FpdClipper::CLIP_SPHERICAL)
    FastPhysRender::draw_sphere(clipperObject->radius, clipperObject->angle, clipperObject->axisLength, color, isSelected());
  else
    FastPhysRender::draw_cylinder(clipperObject->radius, clipperObject->angle, clipperObject->axisLength, color, isSelected());


  if (isSelected())
  {
    // draw affected points
    ::set_cached_debug_lines_wtm(mFPEditor.getRootWtm());

    // Point3 center=getPos()+getMatrix().getcol(1)*radius;
    // Point3 center=clipperObject->getPos();
    Point3 center = getPos();


    for (int i = 0; i < clipperObject->pointNames.size(); ++i)
    {
      FpdPoint *pt = mFPEditor.getPointByName(clipperObject->pointNames[i]);
      if (!pt)
        continue;

      Point3 pos = pt->getPos();
      real r = FPObjectPoint::pointRadius * 0.5f;

      E3DCOLOR color = (i == clipperObject->selectedPoint) ? selPointColor : pointColor;

      //::draw_cached_debug_line(center, pos, color);

      ::draw_cached_debug_line(pos + Point3(-r, -r, -r), pos + Point3(+r, +r, +r), color);
      ::draw_cached_debug_line(pos + Point3(+r, -r, -r), pos + Point3(-r, +r, +r), color);
      ::draw_cached_debug_line(pos + Point3(-r, +r, -r), pos + Point3(+r, -r, +r), color);
      ::draw_cached_debug_line(pos + Point3(+r, +r, -r), pos + Point3(-r, -r, +r), color);
    }

    // draw affected lines
    for (int i = 0; i < clipperObject->lineNames.size(); ++i)
    {
      FpdBone *line = mFPEditor.getBoneByName(clipperObject->lineNames[i]);
      if (!line)
        continue;

      FpdPoint *p1, *p2;

      p1 = mFPEditor.getPointByName(line->point1Name);
      p2 = mFPEditor.getPointByName(line->point2Name);

      if (!p1 || !p2)
        continue;

      Point3 pos1 = p1->getPos();
      Point3 pos2 = p2->getPos();

      Point3 dir = pos2 - pos1;

      int segs = calcLineSegs(length(dir));

      real r = FPObjectPoint::pointRadius * 0.25f;
      E3DCOLOR color = (i == clipperObject->selectedLine) ? selPointColor : pointColor;

      for (int i = 1; i < segs; ++i)
      {
        Point3 pos = pos1 + dir * (real(i) / segs);
        ::draw_cached_debug_line(pos + Point3(-r, -r, -r), pos + Point3(+r, +r, +r), color);
        ::draw_cached_debug_line(pos + Point3(+r, -r, -r), pos + Point3(-r, +r, +r), color);
        ::draw_cached_debug_line(pos + Point3(-r, +r, -r), pos + Point3(+r, -r, +r), color);
        ::draw_cached_debug_line(pos + Point3(+r, +r, -r), pos + Point3(-r, -r, +r), color);
      }
    }
  }
}


bool FPObjectClipper::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
{
  FpdClipper *clipperObject = (FpdClipper *)getObject();
  G_ASSERT(clipperObject);

  return ::is_sphere_hit_by_point(getPos(), clipperObject->radius, vp, x, y);
}


bool FPObjectClipper::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
{
  FpdClipper *clipperObject = (FpdClipper *)getObject();
  G_ASSERT(clipperObject);

  return ::is_sphere_hit_by_rect(getPos(), clipperObject->radius, vp, rect);
}


bool FPObjectClipper::getWorldBox(BBox3 &box) const
{
  FpdClipper *clipperObject = (FpdClipper *)getObject();
  G_ASSERT(clipperObject);

  box.makecube(getPos(), clipperObject->radius * 2);
  return true;
}


bool FPObjectClipper::setPos(const Point3 &p)
{
  FpdClipper *clipperObject = (FpdClipper *)getObject();
  G_ASSERT(clipperObject);

  clipperObject->setPos(p);

  return IFPObject::setPos(p);
}

void FPObjectClipper::setMatrix(const Matrix3 &tm)
{
  FpdClipper *clipperObject = (FpdClipper *)getObject();
  G_ASSERT(clipperObject);

  clipperObject->setMatrix(tm);

  return IFPObject::setMatrix(tm);
}


//------------------------------------------------------------------
