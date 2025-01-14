// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/fastPhysData/fp_edbone.h>
#include <libTools/fastPhysData/fp_edpoint.h>

#include <libTools/util/makeBindump.h>
#include <ioSys/dag_dataBlock.h>
#include <phys/dag_fastPhys.h>
#include <debug/dag_log.h>
#include <util/dag_string.h>
#include <math/dag_geomNodeUtils.h>


FpdBone::FpdBone()
{
  constrAction = new FpdBoneConstraintAction(this);
  ctrlAction = new FpdBoneControllerAction(this);
  angSprAction = new FpdBoneAngularSpringAction(this);
}

static inline int orderPoints(FpdPoint *&p1, FpdPoint *&p2)
{
  if (!p1 && !p2)
    return 0;
  if (p1 && p2)
  {
    if (p2->groupId == 0)
    {
      FpdPoint *p = p1;
      p1 = p2;
      p2 = p;
    }
    return 2;
  }
  if (p1)
    return 1;
  p1 = p2;
  p2 = NULL;
  return 1;
}

int FpdBone::getPoints(FpdPoint *&p1, FpdPoint *&p2, IFpdExport &exp) const
{
  p1 = exp.getPointByName(point1Name);
  p2 = exp.getPointByName(point2Name);
  return orderPoints(p1, p2);
}

int FpdBone::getPointsLd(FpdPoint *&p1, FpdPoint *&p2, IFpdLoad &ld) const
{
  p1 = rtti_cast<FpdPoint>(ld.getEdObjectByName(point1Name));
  p2 = rtti_cast<FpdPoint>(ld.getEdObjectByName(point2Name));
  return orderPoints(p1, p2);
}


void FpdBone::save(DataBlock &blk, const GeomNodeTree &tree)
{
  blk.setStr("class", "bone");

  blk.setStr("point1", point1Name);
  blk.setStr("point2", point2Name);

  constrAction->save(*blk.addBlock("constrAction"), tree);
  ctrlAction->save(*blk.addBlock("ctrlAction"), tree);
  angSprAction->save(*blk.addBlock("angSprAction"), tree);
}


bool FpdBone::load(const DataBlock &blk, IFpdLoad &loader)
{
  point1Name = blk.getStr("point1", String(point1Name));
  point2Name = blk.getStr("point2", String(point2Name));

  constrAction = new FpdBoneConstraintAction(this);
  if (!constrAction->load(*blk.getBlockByNameEx("constrAction"), loader))
    return false;

  ctrlAction = new FpdBoneControllerAction(this);
  if (!ctrlAction->load(*blk.getBlockByNameEx("ctrlAction"), loader))
    return false;

  angSprAction = new FpdBoneAngularSpringAction(this);
  if (!angSprAction->load(*blk.getBlockByNameEx("angSprAction"), loader))
    return false;
  return true;
}


void FpdBone::initActions(FpdContainerAction *init_a, FpdContainerAction *upd_a, IFpdLoad &ld)
{
  ctrlAction->findNode(ld);
  angSprAction->localAxis = ctrlAction->localBoneAxis;

  FpdContainerAction *cont;

  cont = upd_a->getContainerByName("constrain");
  if (!cont)
    cont = upd_a;
  cont->insertAction(constrAction);

  cont = upd_a->getContainerByName("updateNodes");
  if (!cont)
    cont = upd_a;
  cont->insertAction(ctrlAction);

  cont = upd_a->getContainerByName("forces");
  if (!cont)
    cont = upd_a;
  cont->insertAction(angSprAction);
}


void FpdBone::getActions(Tab<FpdAction *> &actions)
{
  actions.push_back(constrAction);
  actions.push_back(ctrlAction);
  actions.push_back(angSprAction);
}


void FpdBoneConstraintAction::save(DataBlock &blk, const GeomNodeTree &tree)
{
  switch (type)
  {
    case DIRECTIONAL: blk.setStr("type", "directional"); break;
    case LENCONSTR: blk.setStr("type", "lenconstr"); break;
    default: logerr("BUG: unknow FastPhys bone type %d", type);
  }

  blk.setReal("minLen", minLen);
  blk.setReal("maxLen", maxLen);
  blk.setReal("damping", damping);

  blk.setPoint3("limitAxis", limitAxis);
  blk.setReal("limitAngle", limitAngle);
}


bool FpdBoneConstraintAction::load(const DataBlock &blk, IFpdLoad &loader)
{
  const char *s = blk.getStr("type", "");

  if (stricmp(s, "directional") == 0)
    type = DIRECTIONAL;
  else if (stricmp(s, "lenconstr") == 0)
    type = LENCONSTR;
  else
  {
    logerr("unknown FastPhys bone type '%s'", s);
    return false;
  }

  minLen = blk.getReal("minLen", minLen);
  maxLen = blk.getReal("maxLen", maxLen);
  damping = blk.getReal("damping", damping);

  limitAxis = blk.getPoint3("limitAxis", limitAxis);
  limitAngle = blk.getReal("limitAngle", limitAngle);
  return true;
}


FpdObject *FpdBoneConstraintAction::getObject() { return boneObj; }


void FpdBoneConstraintAction::exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp)
{
  FpdPoint *p1, *p2;
  boneObj->getPoints(p1, p2, exp);

  if (!p1 || !p2 || p2->groupId == 0)
    return;

  int index1 = exp.getPointIndex(p1);
  int index2 = exp.getPointIndex(p2);
  G_ASSERT(index1 >= 0);
  G_ASSERT(index2 >= 0);

  if (type == DIRECTIONAL)
  {
    exp.countAction();
    bool useLimit = (limitAngle > 0);

    if (!boneObj->ctrlAction->node)
      useLimit = false;

    cwr.writeInt32e(useLimit ? FastPhys::AID_SET_BONE_LENGTH_WITH_CONE : FastPhys::AID_SET_BONE_LENGTH);

    cwr.writeInt32e(index1);
    cwr.writeInt32e(index2);

    cwr.writeReal(damping);

    if (useLimit)
    {
      cwr.write32ex(&limitAxis, sizeof(limitAxis));
      cwr.writeReal(cosf(DegToRad(limitAngle)));

      cwr.writeDwString(exp.getNodeName(boneObj->ctrlAction->node));
    }
  }
  else if (type == LENCONSTR)
  {
    exp.countAction();
    cwr.writeInt32e(FastPhys::AID_MIN_MAX_LENGTH);

    cwr.writeInt32e(index1);
    cwr.writeInt32e(index2);

    cwr.writeReal(minLen / 100);
    cwr.writeReal(maxLen / 100);
  }
  else
    G_ASSERT(false);
}


void FpdBoneControllerAction::save(DataBlock &blk, const GeomNodeTree &tree)
{
  if (node)
    blk.setStr("node", tree.getNodeName(node));
  blk.setPoint3("localBoneAxis", localBoneAxis);
  blk.setBool("useLookAtMode", useLookAtMode);
}


bool FpdBoneControllerAction::load(const DataBlock &blk, IFpdLoad &loader)
{
  auto &tree = loader.getGeomTree();
  const char *nodeName = blk.getStr("node", NULL);
  if (nodeName && nodeName[0])
    node = tree.findINodeIndex(nodeName);
  nodeWtm = get_node_wtm_rel_ptr(tree, node);
  nodeTm = get_node_tm_ptr(tree, node);
  localBoneAxis = blk.getPoint3("localBoneAxis", localBoneAxis);
  useLookAtMode = blk.getBool("useLookAtMode", useLookAtMode);

  if (!node)
  {
    logerr("unknown FastPhys bone <%s> (localBoneAxis = " FMT_P3 ")", blk.getStr("node", NULL), P3D(localBoneAxis));
    return false;
  }
  G_ASSERT(node);
  orgTm.setcol(0, as_point3(&nodeTm->col0));
  orgTm.setcol(1, as_point3(&nodeTm->col1));
  orgTm.setcol(2, as_point3(&nodeTm->col2));
  return true;
}


FpdObject *FpdBoneControllerAction::getObject() { return boneObj; }


void FpdBoneControllerAction::exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp)
{
  if (!node)
    return;

  FpdPoint *p1, *p2;
  boneObj->getPoints(p1, p2, exp);

  if (!p1 || !p2)
    return;

  int index1 = exp.getPointIndex(p1);
  int index2 = exp.getPointIndex(p2);
  G_ASSERT(index1 >= 0);
  G_ASSERT(index2 >= 0);

  exp.countAction();
  cwr.writeInt32e(useLookAtMode ? FastPhys::AID_LOOK_AT_BONE_CTRL : FastPhys::AID_SIMPLE_BONE_CTRL);

  cwr.writeInt32e(index1);
  cwr.writeInt32e(index2);
  cwr.write32ex(&localBoneAxis, sizeof(localBoneAxis));

  cwr.write32ex(&orgTm, sizeof(orgTm));

  cwr.writeDwString(exp.getNodeName(node));
}

void FpdBoneControllerAction::findNode(IFpdLoad &ld)
{
  if (node)
    return;

  FpdPoint *p1, *p2;
  boneObj->getPointsLd(p1, p2, ld);

  if (!p1)
    return;

  node = p1->node;
  if (!p2)
    return;
  auto &tree = ld.getGeomTree();
  if (!node || tree.getParentNodeIdx(node) == p2->node)
    node = p2->node;

  if (!node)
    return;

  nodeWtm = &tree.getNodeWtmRel(node);
  nodeTm = &tree.getNodeTm(node);

  TMatrix tm;
  GeomNodeTree::mat44f_to_TMatrix(*nodeWtm, tm);
  localBoneAxis = inverse(tm) % (p2->getPos() - p1->getPos());
}

int FpdBoneControllerAction::getAxisIndex(const Point3 &dir)
{
  int ai;
  real m, v;
  m = rabs(dir[ai = 0]);

  v = rabs(dir[1]);
  if (v > m)
  {
    m = v;
    ai = 1;
  }
  v = rabs(dir[2]);
  if (v > m)
  {
    m = v;
    ai = 2;
  }

  if (dir[ai] >= 0)
    return ai + 1;
  return -ai - 1;
}


void FpdBoneAngularSpringAction::save(DataBlock &blk, const GeomNodeTree &tree)
{
  blk.setReal("springK", springK);
  blk.setPoint3("localAxis", localAxis);
}


bool FpdBoneAngularSpringAction::load(const DataBlock &blk, IFpdLoad &loader)
{
  springK = blk.getReal("springK", springK);
  localAxis = blk.getPoint3("localAxis", localAxis);

  if (!float_nonzero(lengthSq(localAxis)))
  {
    logerr("localAxis is zero in boneObj->ctrlAction <%s>", boneObj->ctrlAction->actionName);
    return false;
  }

  auto nodeTm = boneObj->ctrlAction->nodeTm;
  if (!nodeTm)
  {
    logerr("missing node in boneObj->ctrlAction <%s>", boneObj->ctrlAction->actionName);
    return false;
  }
  G_ASSERT(nodeTm);
  TMatrix tm;
  GeomNodeTree::mat44f_to_TMatrix(*nodeTm, tm);
  orgAxis = normalize(tm % localAxis);
  return true;
}


FpdObject *FpdBoneAngularSpringAction::getObject() { return boneObj; }


void FpdBoneAngularSpringAction::exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp)
{
  if (springK <= 0)
    return;

  FpdPoint *p1, *p2;
  boneObj->getPoints(p1, p2, exp);

  auto node = boneObj->ctrlAction->node;

  if (!node || !p1 || !p2 || p2->groupId == 0)
    return;

  int index1 = exp.getPointIndex(p1);
  int index2 = exp.getPointIndex(p2);
  G_ASSERT(index1 >= 0);
  G_ASSERT(index2 >= 0);

  exp.countAction();
  cwr.writeInt32e(FastPhys::AID_ANGULAR_SPRING);

  cwr.writeInt32e(index1);
  cwr.writeInt32e(index2);

  cwr.write32ex(&localAxis, sizeof(localAxis));
  cwr.write32ex(&orgAxis, sizeof(orgAxis));
  cwr.writeDwString(exp.getNodeName(node));

  cwr.writeReal(springK);
}
