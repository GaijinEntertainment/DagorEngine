// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/fastPhysData/fp_edclipper.h>
#include <libTools/fastPhysData/fp_edbone.h>
#include <libTools/fastPhysData/fp_edpoint.h>

#include <libTools/util/makeBindump.h>
#include <phys/dag_fastPhys.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_log.h>
#include <math/dag_geomNodeUtils.h>


FpdClipper::FpdClipper() :
  radius(0.05f),
  axisLength(0.50f),
  angle(90),
  lineSegLen(50),
  clipType(CLIP_SPHERICAL),
  pointNames(midmem),
  selectedPoint(-1),
  lineNames(midmem),
  selectedLine(-1)
{
  localTm.identity();
  clipAction = new FpdClipPointsAction(this);
}


int FpdClipper::calcLineSegs(real len)
{
  real step = radius * lineSegLen / 100;

  if (step == 0)
    return 2;

  int n = int(ceilf(len / step));
  if (n < 2)
    n = 2;

  return n;
}


void FpdClipper::save(DataBlock &blk, const GeomNodeTree &tree)
{
  blk.setStr("class", "clipper");

  if (node)
    blk.setStr("node", tree.getNodeName(node));

  blk.setPoint3("tm0", localTm.getcol(0));
  blk.setPoint3("tm1", localTm.getcol(1));
  blk.setPoint3("tm2", localTm.getcol(2));
  blk.setPoint3("tm3", localTm.getcol(3));

  blk.setReal("radius", radius);
  blk.setReal("axisLength", axisLength);
  blk.setReal("angle", angle);
  blk.setReal("lineSegLen", lineSegLen);

  switch (clipType)
  {
    case CLIP_SPHERICAL: blk.setStr("type", "spherical"); break;
    case CLIP_CYLINDRICAL: blk.setStr("type", "cylindrical"); break;
    default: logerr("BUG: unknow FastPhys clipper type %d", clipType);
  }

  DataBlock *sb;
  sb = blk.addBlock("points");
  for (int i = 0; i < pointNames.size(); ++i)
    sb->addStr("point", pointNames[i]);

  sb = blk.addBlock("lines");
  for (int i = 0; i < lineNames.size(); ++i)
    sb->addStr("line", lineNames[i]);

  sb = blk.addBlock("clipAction");
  clipAction->save(*sb, tree);
}


bool FpdClipper::load(const DataBlock &blk, IFpdLoad &loader)
{
  auto &tree = loader.getGeomTree();
  const char *nodeName = blk.getStr("node", NULL);
  if (nodeName && nodeName[0])
    node = tree.findINodeIndex(nodeName);
  nodeWtm = get_node_wtm_rel_ptr(tree, node);

  localTm.setcol(0, blk.getPoint3("tm0", localTm.getcol(0)));
  localTm.setcol(1, blk.getPoint3("tm1", localTm.getcol(1)));
  localTm.setcol(2, blk.getPoint3("tm2", localTm.getcol(2)));
  localTm.setcol(3, blk.getPoint3("tm3", localTm.getcol(3)));

  radius = blk.getReal("radius", radius);
  axisLength = blk.getReal("axisLength", axisLength);
  angle = blk.getReal("angle", angle);
  lineSegLen = blk.getReal("lineSegLen", lineSegLen);

  const char *s = blk.getStr("type", "");

  if (stricmp(s, "spherical") == 0)
    clipType = CLIP_SPHERICAL;
  else if (stricmp(s, "cylindrical") == 0)
    clipType = CLIP_CYLINDRICAL;
  else
  {
    logerr("unknown FastPhys clipper type '%s'", s);
    return false;
  }

  const DataBlock *sb = blk.getBlockByNameEx("points");
  clear_and_shrink(pointNames);
  int nameId = sb->getNameId("point");
  for (int i = 0; i < sb->paramCount(); ++i)
  {
    if (sb->getParamType(i) != DataBlock::TYPE_STRING || sb->getParamNameId(i) != nameId)
      continue;
    pointNames.push_back() = sb->getStr(i);
  }

  sb = blk.getBlockByNameEx("lines");
  clear_and_shrink(lineNames);
  nameId = sb->getNameId("line");
  for (int i = 0; i < sb->paramCount(); ++i)
  {
    if (sb->getParamType(i) != DataBlock::TYPE_STRING || sb->getParamNameId(i) != nameId)
      continue;
    lineNames.push_back() = sb->getStr(i);
  }

  clipAction = new FpdClipPointsAction(this);
  if (!clipAction->load(*blk.getBlockByNameEx("clipAction"), loader))
    return false;

  recalcTm();
  return true;
}


void FpdClipper::initActions(FpdContainerAction *init_a, FpdContainerAction *upd_a, IFpdLoad &ld)
{
  FpdContainerAction *cont;

  cont = upd_a->getContainerByName("constrain");
  if (!cont)
    cont = upd_a;
  cont->insertAction(clipAction);
}


void FpdClipper::getActions(Tab<FpdAction *> &actions) { actions.push_back(clipAction); }

int FpdClipper::findPoint(const char *point_name)
{
  for (int i = 0; i < pointNames.size(); ++i)
    if (stricmp(pointNames[i], point_name) == 0)
      return i;
  return -1;
}


void FpdClipper::addPoint(const char *point_name)
{
  if (findPoint(point_name) >= 0)
    return;

  pointNames.push_back() = point_name;
}


bool FpdClipper::removePoint(const char *point_name)
{
  int id = findPoint(point_name);
  if (id < 0)
    return false;

  erase_items(pointNames, id, 1);

  if (selectedPoint == id)
    selectedPoint = -1;
  else if (selectedPoint > id)
    selectedPoint--;

  return true;
}


int FpdClipper::findLine(const char *line_name)
{
  for (int i = 0; i < lineNames.size(); ++i)
    if (stricmp(lineNames[i], line_name) == 0)
      return i;
  return -1;
}


void FpdClipper::addLine(const char *line_name)
{
  if (findLine(line_name) >= 0)
    return;

  lineNames.push_back() = line_name;
}


bool FpdClipper::removeLine(const char *line_name)
{
  int id = findLine(line_name);
  if (id < 0)
    return false;

  erase_items(lineNames, id, 1);

  if (selectedLine == id)
    selectedLine = -1;
  else if (selectedLine > id)
    selectedLine--;

  return true;
}


FpdObject *FpdClipPointsAction::getObject() { return clipObj; }


void FpdClipPointsAction::exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp)
{
  // points list
  Tab<int> ptind(tmpmem);
  ptind.reserve(clipObj->pointNames.size());

  for (int i = 0; i < clipObj->pointNames.size(); ++i)
  {
    int id = exp.getPointIndex(exp.getPointByName(clipObj->pointNames[i]));

    if (id < 0)
      continue;
    ptind.push_back(id);
  }

  // lines list
  Tab<FastPhys::ClippedLine> lines(tmpmem);
  lines.reserve(clipObj->lineNames.size());

  for (int i = 0; i < clipObj->lineNames.size(); ++i)
  {
    FpdBone *bone = exp.getBoneByName(clipObj->lineNames[i]);
    if (!bone)
      continue;

    FpdPoint *p1, *p2;
    bone->getPoints(p1, p2, exp);
    if (!p1 || !p2)
      continue;

    int id1 = exp.getPointIndex(p1);
    int id2 = exp.getPointIndex(p2);
    if (id1 < 0 || id2 < 0)
      continue;

    lines.push_back(FastPhys::ClippedLine(id1, id2, clipObj->calcLineSegs(length(p2->getPos() - p1->getPos()))));
  }

  if (!ptind.size() && !lines.size())
    return;

  if (clipObj->clipType == FpdClipper::CLIP_SPHERICAL)
  {
    exp.countAction();
    cwr.writeInt32e(FastPhys::AID_CLIP_SPHERICAL);

    cwr.writeDwString(clipObj->node ? exp.getNodeName(clipObj->node) : NULL);

    Point3 lpos = clipObj->localTm.getcol(3);
    Point3 ldir = clipObj->localTm.getcol(1);

    cwr.write32ex(&lpos, sizeof(lpos));
    cwr.write32ex(&ldir, sizeof(ldir));

    cwr.writeReal(clipObj->radius);
    cwr.writeReal(DegToRad(90 - clipObj->angle / 2));

    cwr.writeInt32e(ptind.size());
    cwr.writeTabData32ex(ptind);

    cwr.writeInt32e(lines.size());
    cwr.writeTabData32ex(lines);
  }
  else if (clipObj->clipType == FpdClipper::CLIP_CYLINDRICAL)
  {
    exp.countAction();
    cwr.writeInt32e(FastPhys::AID_CLIP_CYLINDRICAL);

    cwr.writeDwString(clipObj->node ? exp.getNodeName(clipObj->node) : NULL);

    Point3 lpos = clipObj->localTm.getcol(3);
    Point3 ldir = clipObj->localTm.getcol(1);
    Point3 lside = clipObj->localTm.getcol(2);

    cwr.write32ex(&lpos, sizeof(lpos));
    cwr.write32ex(&ldir, sizeof(ldir));
    cwr.write32ex(&lside, sizeof(lside));

    cwr.writeReal(clipObj->radius);
    cwr.writeReal(DegToRad(90 - clipObj->angle / 2));

    cwr.writeInt32e(ptind.size());
    cwr.writeTabData32ex(ptind);

    cwr.writeInt32e(lines.size());
    cwr.writeTabData32ex(lines);
  }
  else
    G_ASSERT(false);
}
