// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/fastPhysData/fp_data.h>
#include <libTools/fastPhysData/fp_edpoint.h>
#include <libTools/fastPhysData/fp_edbone.h>
#include <libTools/fastPhysData/fp_edclipper.h>

#include <generic/dag_sort.h>
#include <libTools/util/makeBindump.h>
#include <phys/dag_fastPhys.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_log.h>

FpdExporter::FpdExporter() : objects(tmpmem), points(tmpmem) {}
FpdExporter::~FpdExporter() {}

void FpdExporter::getPointGroup(int grp_id, int &first_pt, int &num_pts)
{
  for (int i = 0; i < points.size(); ++i)
    if (points[i]->groupId == grp_id)
    {
      first_pt = i;

      int e;
      for (e = i + 1; e < points.size(); ++e)
        if (points[e]->groupId != grp_id)
          break;

      num_pts = e - first_pt;
      return;
    }

  first_pt = 0;
  num_pts = 0;
}

FpdPoint *FpdExporter::getPointByName(const char *name)
{
  if (!name)
    return NULL;

  for (int i = 0; i < points.size(); ++i)
    if (stricmp(points[i]->getName(), name) == 0)
      return points[i];

  return NULL;
}


FpdBone *FpdExporter::getBoneByName(const char *name)
{
  if (!name)
    return NULL;

  for (int i = 0; i < objects.size(); ++i)
  {
    FpdBone *o = rtti_cast<FpdBone>(objects[i]);
    if (o && stricmp(o->getName(), name) == 0)
      return o;
  }

  return NULL;
}


int FpdExporter::getPointIndex(FpdObject *po)
{
  if (!po)
    return -1;

  for (int i = 0; i < points.size(); ++i)
    if (points[i] == po)
      return i;

  return -1;
}


static int comparePoints(const Ptr<FpdPoint> *a, const Ptr<FpdPoint> *b) { return (*a)->groupId - (*b)->groupId; }


void FpdExporter::exportAction(mkbindump::BinDumpSaveCB &cwr, FpdAction *action)
{
  int ofs = cwr.tell();
  cwr.writeInt32e(0);

  actionCounter = 0;
  if (action)
    action->exportAction(cwr, *this);

  cwr.writeInt32eAt(actionCounter, ofs);
}


bool FpdExporter::exportFastPhys(mkbindump::BinDumpSaveCB &cwr)
{
  if (nodeTree.empty())
    return false;

  sort(points, &comparePoints);

  // write file
  cwr.beginBlock();
  cwr.writeFourCC(FastPhys::FILE_ID);

  // write points
  cwr.writeInt32e(points.size());
  for (int i = 0; i < points.size(); ++i)
    points[i]->exportFastPhys(cwr);

  // write actions
  exportAction(cwr, initAction);
  exportAction(cwr, updateAction);
  cwr.endBlock();
  return true;
}


void FpdExporter::addObject(FpdObject *o)
{
  if (!o)
    return;
  objects.push_back(o);

  FpdPoint *po = rtti_cast<FpdPoint>(o);
  if (po)
    points.push_back(po);
}


FpdObject *FpdExporter::getEdObjectByName(const char *name)
{
  for (int i = 0; i < objects.size(); ++i)
    if (stricmp(objects[i]->getName(), name) == 0)
      return objects[i];
  return NULL;
}


bool FpdExporter::load(const DataBlock &blk)
{
  initAction = new FpdContainerAction("init");
  updateAction = new FpdContainerAction("update");

  // load objects
  int objNameId = blk.getNameId("obj");
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock &sb = *blk.getBlock(i);
    if (sb.getBlockNameId() != objNameId)
      continue;

    FpdObject *obj = IFpdLoad::create_object_by_class(sb.getStr("class", NULL));
    if (!obj)
    {
      logerr("Error loading FastPhys object");
      continue;
    }

    obj->setName(sb.getStr("name", "Object"));
    addObject(obj);

    if (!obj->load(sb, *this))
      return false;
  }

  // load actions
  if (!initAction->load(*blk.getBlockByNameEx("initAction"), *this))
    return false;
  if (!updateAction->load(*blk.getBlockByNameEx("updateAction"), *this))
    return false;
  return true;
}

FpdObject *IFpdLoad::create_object_by_class(const char *cn)
{
  if (!cn)
    return NULL;

  if (strcmp(cn, "point") == 0)
    return new FpdPoint();
  if (strcmp(cn, "bone") == 0)
    return new FpdBone();
  if (strcmp(cn, "clipper") == 0)
    return new FpdClipper();

  logerr("Unknow FastPhys object class '%s'", cn);
  return NULL;
}

FpdAction *IFpdLoad::load_action(const DataBlock &blk, IFpdLoad &loader)
{
  const char *cn = blk.getStr("class", NULL);
  if (!cn)
  {
    logerr("no FastPhys action class");
    return NULL;
  }

  FpdAction *action = NULL;

  if (strcmp(cn, "objectAction") == 0)
  {
    FpdObject *obj = loader.getEdObjectByName(blk.getStr("obj", NULL));
    if (!obj)
      return NULL;

    const char *name = blk.getStr("name", NULL);
    if (!name)
      return NULL;

    Tab<FpdAction *> actions(tmpmem);
    obj->getActions(actions);

    for (int i = 0; i < actions.size(); ++i)
      if (strcmp(actions[i]->actionName, name) == 0)
        return actions[i];

    return NULL;
  }
  else if (strcmp(cn, "container") == 0)
    action = new FpdContainerAction(NULL);
  else if (strcmp(cn, "integrator") == 0)
    action = new FpdIntegratorAction();

  if (!action)
  {
    logerr("unknown FastPhys action class '%s'", cn);
    return NULL;
  }

  if (!action->load(blk, loader))
  {
    delete action;
    return NULL;
  }
  return action;
}
