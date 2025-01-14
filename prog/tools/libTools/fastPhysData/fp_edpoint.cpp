// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/fastPhysData/fp_edpoint.h>

#include <libTools/util/makeBindump.h>
#include <phys/dag_fastPhys.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_geomNodeUtils.h>


FpdPoint::FpdPoint() : groupId(1), gravity(9.8f), damping(5), windK(1)
{
  localPos = v_zero();
  initAction = new FpdInitPointAction(this);
  linkAction = new FpdLinkPointAction(this);
}


void FpdPoint::save(DataBlock &blk, const GeomNodeTree &tree)
{
  blk.setStr("class", "point");

  blk.setPoint3("localPos", as_point3(&localPos));
  if (node)
    blk.setStr("node", tree.getNodeName(node));
  blk.setInt("groupId", groupId);
  blk.setReal("gravity", gravity);
  blk.setReal("damping", damping);
  blk.setReal("windK", windK);

  DataBlock *sb;
  sb = blk.addBlock("initAction");
  initAction->save(*sb, tree);

  sb = blk.addBlock("linkAction");
  linkAction->save(*sb, tree);
}


bool FpdPoint::load(const DataBlock &blk, IFpdLoad &loader)
{
  as_point3(&localPos) = blk.getPoint3("localPos", as_point3(&localPos));
  auto &tree = loader.getGeomTree();
  const char *nodeName = blk.getStr("node", NULL);
  if (nodeName && nodeName[0])
    node = tree.findINodeIndex(nodeName);
  nodeWtm = get_node_wtm_rel_ptr(tree, node);
  groupId = blk.getInt("groupId", groupId);
  gravity = blk.getReal("gravity", gravity);
  damping = blk.getReal("damping", damping);
  windK = blk.getReal("windK", windK);

  initAction = new FpdInitPointAction(this);
  if (!initAction->load(*blk.getBlockByNameEx("initAction"), loader))
    return false;

  linkAction = new FpdLinkPointAction(this);
  if (!linkAction->load(*blk.getBlockByNameEx("linkAction"), loader))
    return false;

  recalcPos();
  return true;
}


void FpdPoint::initActions(FpdContainerAction *init_a, FpdContainerAction *upd_a, IFpdLoad &ld)
{
  init_a->insertAction(initAction);

  FpdContainerAction *cont;

  cont = upd_a->getContainerByName("begin");
  if (!cont)
    cont = upd_a;
  cont->insertAction(linkAction);
}


void FpdPoint::getActions(Tab<FpdAction *> &actions)
{
  actions.push_back(initAction);
  actions.push_back(linkAction);
}


void FpdPoint::exportFastPhys(mkbindump::BinDumpSaveCB &cwr)
{
  cwr.writeReal(gravity);
  cwr.writeReal(damping);
  cwr.writeReal(windK);
}


FpdObject *FpdInitPointAction::getObject() { return pointObj; }


void FpdInitPointAction::exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp)
{
  int index = exp.getPointIndex(pointObj);
  G_ASSERT(index >= 0);

  exp.countAction();
  cwr.writeInt32e(FastPhys::AID_INIT_POINT);

  cwr.writeInt32e(index);
  cwr.write32ex(&pointObj->localPos, 3 * sizeof(float));
  cwr.writeDwString(pointObj->node ? exp.getNodeName(pointObj->node) : NULL);
}


FpdObject *FpdLinkPointAction::getObject() { return pointObj; }


void FpdLinkPointAction::exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp)
{
  int index = exp.getPointIndex(pointObj);
  G_ASSERT(index >= 0);

  if (pointObj->groupId != 0 || !pointObj->node)
    return;

  exp.countAction();
  cwr.writeInt32e(FastPhys::AID_MOVE_POINT_WITH_NODE);

  cwr.writeInt32e(index);
  cwr.write32ex(&pointObj->localPos, 3 * sizeof(float));
  cwr.writeDwString(pointObj->node ? exp.getNodeName(pointObj->node) : NULL);
}
