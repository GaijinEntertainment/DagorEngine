// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/fastPhysData/fp_edbone.h>
#include <libTools/fastPhysData/fp_edpoint.h>

#include <debug/dag_debug3d.h>
#include <math/dag_math2d.h>

#include <EditorCore/ec_rect.h>

#include "fastPhysEd.h"
#include "fastPhysPanel.h"
#include "fastPhysBone.h"
#include "fastPhysPoint.h"

//------------------------------------------------------------------

enum
{
  PID_POINT1 = PID_OBJ_FIRST,
  PID_POINT2,
  PID_USEBONECTRL,

  PID_CO_TYPE,
  PID_CO_MINLEN,
  PID_CO_MAXLEN,
  PID_CO_DAMPING,

  PID_ANG_SPRING_K,
  PID_ANG_SPRING_AXIS,

  PID_LIMIT_ANGLE,
  PID_LIMIT_AXIS,

  PID_USELOOKATBONECTRL,

  PID_EDBONE__LAST
};

real FPObjectBone::unlinkedRadius = 0.10f, FPObjectBone::unlinkedLength = -0.50f;

E3DCOLOR FPObjectBone::normalColor(50, 50, 255);
E3DCOLOR FPObjectBone::badColor(255, 20, 20);
E3DCOLOR FPObjectBone::selectedColor(255, 255, 255);
E3DCOLOR FPObjectBone::boneAxisColor(150, 150, 255);
E3DCOLOR FPObjectBone::boneConeColor(255, 150, 150);


FPObjectBone::FPObjectBone(FpdObject *obj, FastPhysEditor &editor) : IFPObject(obj, editor) {}


void FPObjectBone::refillPanel(PropPanel::ContainerPropertyControl *panel)
{
  FpdBone *boneObject = (FpdBone *)getObject();
  G_ASSERT(boneObject);

  panel->createEditBox(PID_POINT1, "Point 1:", boneObject->point1Name, false);
  panel->createEditBox(PID_POINT2, "Point 2:", boneObject->point2Name, false);

  // bone ctrl

  FpdContainerAction *c_action = FastPhysEditor::getContainerAction(mFPEditor.updateAction);
  bool hasCtrl = FastPhysEditor::hasSubAction(c_action, boneObject->ctrlAction);
  panel->createCheckBox(PID_USEBONECTRL, "Use bone controller", hasCtrl);
  if (hasCtrl)
    panel->createCheckBox(PID_USELOOKATBONECTRL, "Use look at bone controller", boneObject->ctrlAction->useLookAtMode);

  // bone constr
  PropPanel::ContainerPropertyControl *rgrp = panel->createRadioGroup(PID_CO_TYPE, "Constraint type:");
  rgrp->createRadio(FpdBoneConstraintAction::DIRECTIONAL, "directional");
  rgrp->createRadio(FpdBoneConstraintAction::LENCONSTR, "undirected");
  panel->setInt(PID_CO_TYPE, boneObject->constrAction->type);

  panel->createEditFloat(PID_CO_MINLEN, "Min length (%):", boneObject->constrAction->minLen);
  panel->createEditFloat(PID_CO_MAXLEN, "Max length (%):", boneObject->constrAction->maxLen);

  panel->createIndent();

  panel->createEditFloat(PID_CO_DAMPING, "Damping:", boneObject->constrAction->damping);

  // cone limit
  panel->createEditFloat(PID_LIMIT_ANGLE, "Limit ang.:", boneObject->constrAction->limitAngle);
  panel->createPoint3(PID_LIMIT_AXIS, "Limit axis:", boneObject->constrAction->limitAxis);

  // bone angular spring
  panel->createEditFloat(PID_ANG_SPRING_K, "Ang. spring K:", boneObject->angSprAction->springK);
  panel->createPoint3(PID_ANG_SPRING_AXIS, "Ang. spring axis:", boneObject->angSprAction->localAxis);

  /*
  // multiselection handling
  for (int i=0; i<objects.size(); ++i)
  {
    FastPhysEdBone *o=(FastPhysEdBone*)objects[i];

    bool hc=((FastPhysObjectEditor*)objEditor)->updateAction->hasSubAction(o->ctrlAction);

    if (strcmp(o->point1Name, point1Name)!=0) op.setClear(PID_POINT1, true);
    if (strcmp(o->point2Name, point1Name)!=0) op.setClear(PID_POINT2, true);
    if (hc!=hasCtrl) op.setClear(PID_USEBONECTRL, true);
    if (o->constrAction->type!=constrAction->type) op.getRadioGroup(PID_CO_TYPE)->setValue(-1);
    if (o->constrAction->minLen!=constrAction->minLen) op.setClear(PID_CO_MINLEN, true);
    if (o->constrAction->maxLen!=constrAction->maxLen) op.setClear(PID_CO_MAXLEN, true);
    if (o->constrAction->damping!=constrAction->damping) op.setClear(PID_CO_DAMPING, true);
    if (o->constrAction->limitAngle!=constrAction->limitAngle) op.setClear(PID_LIMIT_ANGLE, true);
    if (o->constrAction->limitAxis!=constrAction->limitAxis) op.setClear(PID_LIMIT_AXIS, true);
    if (o->angSprAction->springK!=angSprAction->springK) op.setClear(PID_ANG_SPRING_K, true);
    if (o->angSprAction->localAxis!=angSprAction->localAxis) op.setClear(PID_ANG_SPRING_AXIS, true);
  }
  */
}


void FPObjectBone::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  FpdBone *boneObject = (FpdBone *)getObject();
  G_ASSERT(boneObject);

  /*
  #define CHANGE_VAL(type, pname, getfunc) \
  { \
    type val=panel.getfunc(pid, pname); \
    for (int i=0; i<objects.size(); ++i) \
    { \
      FastPhysEdBone *o=(FastPhysEdBone*)objects[i]; \
      o->pname=val; \
    } \
  }
  */

#define CHANGE_VAL(type, pname, getfunc) \
  {                                      \
    type val = panel->getfunc(pcb_id);   \
    boneObject->pname = val;             \
  }

  if (pcb_id == PID_USEBONECTRL)
  {
    bool val = panel->getBool(pcb_id);

    /*
    FastPhysObjectEditor &objEd=*(FastPhysObjectEditor*)objEditor;

    for (int i=0; i<objects.size(); ++i)
    {
      FastPhysEdBone *o=(FastPhysEdBone*)objects[i];
      if (objEd.updateAction->hasSubAction(o->ctrlAction)==val) continue;

      if (val)
      {
        FastPhysEdContainerAction* cont;
        cont=objEd.updateAction->getContainerByName("updateNodes");
        if (!cont) cont=objEd.updateAction;
        cont->insertAction(o->ctrlAction);
      }
      else
      {
        objEd.updateAction->removeSubAction(o->ctrlAction);
      }
    }
    */

    mFPEditor.invalidateObjectProps();

    if (mFPEditor.hasSubAction(mFPEditor.getContainerAction(mFPEditor.updateAction.get()), boneObject->ctrlAction) == val)
      return;

    FpdContainerAction *_cont_base = mFPEditor.getContainerAction(mFPEditor.updateAction);

    if (_cont_base)
    {
      if (val)
      {
        FpdContainerAction *cont = _cont_base->getContainerByName("updateNodes");
        if (!cont)
          cont = _cont_base;
        cont->insertAction(boneObject->ctrlAction);
      }
      else
      {
        //_cont_base->removeSubAction(boneObject->ctrlAction);
      }

      // update tree
    }
  }
  else if (pcb_id == PID_CO_TYPE)
    CHANGE_VAL(int, constrAction->type, getInt)
  else if (pcb_id == PID_CO_MINLEN)
    CHANGE_VAL(float, constrAction->minLen, getFloat)
  else if (pcb_id == PID_CO_MAXLEN)
    CHANGE_VAL(float, constrAction->maxLen, getFloat)
  else if (pcb_id == PID_CO_DAMPING)
    CHANGE_VAL(float, constrAction->damping, getFloat)
  else if (pcb_id == PID_LIMIT_ANGLE)
    CHANGE_VAL(float, constrAction->limitAngle, getFloat)
  else if (pcb_id == PID_LIMIT_AXIS)
    CHANGE_VAL(Point3, constrAction->limitAxis, getPoint3)
  else if (pcb_id == PID_ANG_SPRING_K)
    CHANGE_VAL(float, angSprAction->springK, getFloat)
  else if (pcb_id == PID_ANG_SPRING_AXIS)
  {
    Point3 val = panel->getPoint3(pcb_id);
    /*
    for (int i=0; i<objects.size(); ++i)
    {
      FastPhysEdBone *o=(FastPhysEdBone*)objects[i];
      o->angSprAction->localAxis=val;
      o->angSprAction->orgAxis=normalize(o->ctrlAction->node->tm%o->angSprAction->localAxis);
    }
    */

    TMatrix tm;
    GeomNodeTree::mat44f_to_TMatrix(*boneObject->ctrlAction->nodeTm, tm);
    boneObject->angSprAction->localAxis = val;
    boneObject->angSprAction->orgAxis = normalize(tm % boneObject->angSprAction->localAxis);
  }
  else if (pcb_id == PID_USELOOKATBONECTRL)
  {
    if (boneObject->ctrlAction)
      boneObject->ctrlAction->useLookAtMode = panel->getBool(pcb_id);
  }

#undef CHANGE_VAL
}


void FPObjectBone::render()
{
  if (!mFPEditor.isObjectDebugVisible)
    return;

  FpdBone *boneObject = (FpdBone *)getObject();
  G_ASSERT(boneObject);
  FpdPoint *p1, *p2;

  p1 = mFPEditor.getPointByName(boneObject->point1Name);
  p2 = mFPEditor.getPointByName(boneObject->point2Name);

  E3DCOLOR color = normalColor;

  if (!p1 || !p2 || p2->groupId == 0)
    color = badColor;

  if (isFrozen())
    color = frozenColor;
  if (isSelected())
    color = selectedColor;

  if (p2)
  {
    Point3 dir = p2->getPos() - p1->getPos();
    real len = length(dir);
    if (len != 0)
      dir /= len;

    Point3 up(0, 1, 0);

    if (rabs(up * dir) >= 0.9f)
      up = Point3(0, 0, 1);

    Point3 side = normalize(dir % up);

    TMatrix wtm;
    wtm.setcol(0, dir);
    wtm.setcol(1, side);
    wtm.setcol(2, dir % side);
    wtm.setcol(3, p1->getPos());

    ::set_cached_debug_lines_wtm(mFPEditor.getRootWtm() * wtm);

    if (boneObject->constrAction->type == FpdBoneConstraintAction::DIRECTIONAL)
      ::draw_skeleton_link(Point3(len, 0, 0), FPObjectPoint::pointRadius, color);
    else
    {
      ::draw_cached_debug_line(Point3(0, 0, 0), Point3(len, 0, 0), color);
    }

    if (isSelected() && float_nonzero(boneObject->angSprAction->springK))
    {
      if (auto nodeWtm = boneObject->ctrlAction->nodeWtm)
      {
        TMatrix tm;
        GeomNodeTree::mat44f_to_TMatrix(*nodeWtm, tm);
        ::set_cached_debug_lines_wtm(tm);
        ::draw_cached_debug_line(Point3(0, 0, 0), normalize(boneObject->angSprAction->localAxis), FPObjectBone::boneAxisColor);
      }
    }

    if (isSelected() && boneObject->constrAction->limitAngle > 0)
    {
      auto nodeWtm = boneObject->ctrlAction->nodeWtm;
      if (auto pnode = nodeWtm ? mFPEditor.getGeomTree().getParentNodeIdx(boneObject->ctrlAction->node) : dag::Index16())
      {
        TMatrix tm;
        mFPEditor.getGeomTree().getNodeWtmRelScalar(pnode, tm);

        tm = makeTM(quat_rotation_arc(Point3(0, 0, 1), tm % boneObject->constrAction->limitAxis));

        tm.setcol(3, as_point3(&nodeWtm->col3));

        ::set_cached_debug_lines_wtm(tm);
        ::draw_cached_debug_line(Point3(0, 0, 0), Point3(0, 0, 1), FPObjectBone::boneConeColor);

        float angle = DegToRad(boneObject->constrAction->limitAngle);
        float limitCos = cosf(angle);
        float limitSin = sinf(angle);

        int numSegs = 16;
        for (int i = 0; i < numSegs; ++i)
        {
          float a = i * TWOPI / numSegs;
          ::draw_cached_debug_line(Point3(0, 0, 0), Point3(cosf(a) * limitSin, sinf(a) * limitSin, limitCos),
            FPObjectBone::boneConeColor);
        }
      }
    }
  }
  else if (p1)
  {
    ::set_cached_debug_lines_wtm(mFPEditor.getRootWtm());
    ::draw_cached_debug_line(p1->getPos(), p1->getPos() + Point3(0, unlinkedLength, 0), color);
  }
  else
  {
    ::set_cached_debug_lines_wtm(mFPEditor.getRootWtm());
    ::draw_cached_debug_sphere(Point3(0, 0, 0), unlinkedRadius, color);
  }

  // if (((FastPhysObjectEditor*)objEditor)->updateAction->hasSubAction(ctrlAction)) ctrlAction->render();
}


bool FPObjectBone::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
{
  FpdBone *boneObject = (FpdBone *)getObject();
  G_ASSERT(boneObject);

  FpdPoint *p1, *p2;

  p1 = mFPEditor.getPointByName(boneObject->point1Name);
  p2 = mFPEditor.getPointByName(boneObject->point2Name);

  if (!p1)
    return ::is_sphere_hit_by_point(Point3(0, 0, 0), unlinkedRadius, vp, x, y);

  Point2 pt1, pt2;
  vp->worldToClient(p1->getPos(), pt1);
  if (p2)
    vp->worldToClient(p2->getPos(), pt2);
  else
    vp->worldToClient(p1->getPos() + Point3(0, unlinkedLength, 0), pt2);

  return ::distance_point_to_line_segment(Point2(x, y), pt1, pt2) <= 5;
}


bool FPObjectBone::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
{
  FpdBone *boneObject = (FpdBone *)getObject();
  G_ASSERT(boneObject);

  FpdPoint *p1, *p2;

  p1 = mFPEditor.getPointByName(boneObject->point1Name);
  p2 = mFPEditor.getPointByName(boneObject->point2Name);

  if (!p1)
    return ::is_sphere_hit_by_rect(Point3(0, 0, 0), unlinkedRadius, vp, rect);

  Point2 pt1, pt2;
  vp->worldToClient(p1->getPos(), pt1);
  if (p2)
    vp->worldToClient(p2->getPos(), pt2);
  else
    vp->worldToClient(p1->getPos() + Point3(0, unlinkedLength, 0), pt2);

  BBox2 box;
  int r = 2;
  box[0].x = rect.l - r;
  box[1].x = rect.r + r;
  box[0].y = rect.t - r;
  box[1].y = rect.b + r;

  return ::isect_line_segment_box(pt1, pt2, box);
}


bool FPObjectBone::getWorldBox(BBox3 &box) const
{
  FpdBone *boneObject = (FpdBone *)getObject();
  G_ASSERT(boneObject);

  FpdPoint *p1, *p2;

  p1 = mFPEditor.getPointByName(boneObject->point1Name);
  p2 = mFPEditor.getPointByName(boneObject->point2Name);

  if (!p1)
  {
    box.makecube(Point3(0, 0, 0), unlinkedRadius);
    return true;
  }

  box.makecube(p1->getPos(), 0);
  if (p2)
    box += p2->getPos();
  else
    box += p1->getPos() + Point3(0, unlinkedLength, 0);

  return true;
}

//------------------------------------------------------------------
