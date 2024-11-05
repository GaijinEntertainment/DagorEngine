// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/fastPhysData/fp_data.h>

#include <libTools/util/makeBindump.h>
#include <phys/dag_fastPhys.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>


void FpdContainerAction::exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp)
{
  for (int i = 0; i < subActions.size(); ++i)
    subActions[i]->exportAction(cwr, exp);
}


void FpdContainerAction::save(DataBlock &blk, const GeomNodeTree &tree)
{
  blk.setStr("class", "container");

  blk.setStr("name", actionName);

  for (int i = 0; i < subActions.size(); ++i)
  {
    DataBlock &sb = *blk.addNewBlock("action");

    FpdObject *obj = subActions[i]->getObject();

    if (obj)
    {
      sb.setStr("class", "objectAction");
      sb.setStr("obj", obj->getName());
      sb.setStr("name", subActions[i]->actionName);
    }
    else
      subActions[i]->save(sb, tree);
  }
}


bool FpdContainerAction::load(const DataBlock &blk, IFpdLoad &loader)
{
  String oldName(actionName);
  actionName = blk.getStr("name", oldName);

  int nameId = blk.getNameId("action");

  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock &sb = *blk.getBlock(i);
    if (sb.getBlockNameId() != nameId)
      continue;

    insertAction(loader.loadAction(sb));
  }
  return true;
}


void FpdContainerAction::insertAction(FpdAction *action, int at)
{
  if (!action)
    return;

  if (at < 0 || at >= subActions.size())
    at = subActions.size();

  insert_items(subActions, at, 1, &action);
}


void FpdContainerAction::removeActions(int at, int num)
{
  if (at < 0)
  {
    num += at;
    at = 0;
  }
  if (at + num > subActions.size())
    num = subActions.size() - at;
  if (num <= 0)
    return;

  erase_items(subActions, at, num);
}


int FpdContainerAction::getActionIndexByName(const char *name)
{
  for (int i = 0; i < subActions.size(); ++i)
    if (stricmp(subActions[i]->actionName, name) == 0)
      return i;

  return -1;
}


void FpdIntegratorAction::exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp)
{
  int firstPt, numPts;
  exp.getPointGroup(1, firstPt, numPts);

  if (!numPts)
    return;

  exp.countAction();
  cwr.writeInt32e(FastPhys::AID_INTEGRATE);
  cwr.writeInt32e(firstPt);
  cwr.writeInt32e(firstPt + numPts);
}


void FpdIntegratorAction::save(DataBlock &blk, const GeomNodeTree &tree) { blk.setStr("class", "integrator"); }


bool FpdIntegratorAction::load(const DataBlock &blk, IFpdLoad &loader) { return true; }
