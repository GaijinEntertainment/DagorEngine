#include <streaming/dag_streamingCtrl.h>
#include <streaming/dag_streamingMgr.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>

StreamingSceneController::StreamingSceneController(IStreamingSceneManager &_mgr, const DataBlock &blk, const char *folder_path) :
  mgr(_mgr), actionSph(inimem), curObserverPos(0, 0, 0)
{

  const DataBlock *cb = blk.getBlockByNameEx("loadAtStart");
  int nid_stream = blk.getNameId("stream");
  int nid_load = blk.getNameId("load");
  int nid_sphere = blk.getNameId("sphere");
  int i;

  for (i = 0; i < cb->paramCount(); i++)
    if (cb->getParamType(i) == DataBlock::TYPE_STRING)
    {
      if (cb->getParamNameId(i) == nid_load)
      {
        int bindump_id = sceneBin.addNameId(String(0, "%s/%s", folder_path, cb->getStr(i)));
        mgr.loadBinDump(sceneBin.getName(bindump_id));
      }
      else if (cb->getParamNameId(i) == nid_stream)
      {
        int bindump_id = sceneBin.addNameId(String(0, "%s/%s", folder_path, cb->getStr(i)));
        mgr.loadBinDumpAsync(sceneBin.getName(bindump_id));
      }
    }


  real def_loadrad = blk.getReal("def_loadrad", 0);
  real def_unloadrad = blk.getReal("def_unloadrad", 100);


  for (i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid_sphere)
    {
      cb = blk.getBlock(i);
      ActionSphere as;
      const char *name;

      as.center = cb->getPoint3("center", Point3(0, 0, 0));
      as.rad = cb->getReal("rad", 1);
      as.loadRad2 = cb->getReal("loadRad", def_loadrad) + as.rad;
      as.unloadRad2 = cb->getReal("unloadRad", def_unloadrad) + as.rad;
      as.loadRad2 *= as.loadRad2;
      as.unloadRad2 *= as.unloadRad2;
      as.bindumpId = -1;

      if ((name = cb->getStr("stream", NULL)) == 0)
      {
        debug_ctx("unknown \"stream\" not found");
        continue;
      }

      as.sceneBinId = sceneBin.addNameId(String(0, "%s/%s", folder_path, name));

      actionSph.push_back(as);
    }

  debug_ctx("%d actionspehers", actionSph.size());
}

StreamingSceneController::~StreamingSceneController() {}

void StreamingSceneController::preloadAtPos(const Point3 &p, real overlap_rad)
{
  curObserverPos = p;
  curObserverPos.y = 0;

  for (int i = actionSph.size() - 1; i >= 0; i--)
  {
    real rad2 = lengthSq(curObserverPos - actionSph[i].center);
    real eff_rad2 = actionSph[i].rad - overlap_rad;
    eff_rad2 *= eff_rad2;

    if (rad2 < eff_rad2 && actionSph[i].bindumpId == -1)
      actionSph[i].bindumpId = mgr.loadBinDump(sceneBin.getName(actionSph[i].sceneBinId));
  }
}
void StreamingSceneController::setObserverPos(const Point3 &p)
{
  curObserverPos = p;
  curObserverPos.y = 0;

  for (int i = actionSph.size() - 1; i >= 0; i--)
  {
    real rad2 = lengthSq(curObserverPos - actionSph[i].center);
    if (rad2 < actionSph[i].loadRad2 && actionSph[i].bindumpId == -1)
      actionSph[i].bindumpId = mgr.loadBinDumpAsync(sceneBin.getName(actionSph[i].sceneBinId));
    else if (rad2 > actionSph[i].unloadRad2 && actionSph[i].bindumpId != -1)
    {
      mgr.unloadBinDump(sceneBin.getName(actionSph[i].sceneBinId), false); // async
      actionSph[i].bindumpId = -1;
    }
  }
}
float StreamingSceneController::getBinDumpOptima(unsigned bindump_id)
{
  real optima = MAX_REAL;
  for (int i = actionSph.size() - 1; i >= 0; i--)
    if (actionSph[i].bindumpId == bindump_id)
    {
      real rad2 = lengthSq(curObserverPos - actionSph[i].center);
      if (rad2 < actionSph[i].loadRad2 && rad2 < optima)
        optima = rad2;
    }
  return optima;
}
