#pragma once


#include <assets/assetMgr.h>
#include <assets/assetMsgPipe.h>
#include <gameRes/dag_gameResSystem.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_fastStrMap.h>
#include <util/dag_string.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/progressInd.h>

static inline bool loadAssetBase(DagorAssetMgr &mgr, const char *app_dir, const DataBlock &appblk, IGenericProgressIndicator &pbar,
  ILogWriter &log)
{
  pbar.setActionDesc("Scanning asset base...");

  const DataBlock &blk = *appblk.getBlockByNameEx("assets");
  int nid = blk.getNameId("base");
  bool success = true;

  for (int i = 0; i < blk.paramCount(); i++)
    if (blk.getParamType(i) == DataBlock::TYPE_STRING && blk.getParamNameId(i) == nid)
    {
      String base(260, "%s/%s", app_dir, blk.getStr(i));
      if (!mgr.loadAssetsBase(base, "sample"))
      {
        debug("errors while loading base: %s", base.str());
        success = false;
      }
    }

  if (blk.getStr("gameRes", NULL) || blk.getStr("ddsxPacks", NULL))
  {
    ::reset_game_resources();
    DataBlock scannedResBlk;
    set_gameres_scan_recorder(&scannedResBlk, blk.getStr("gameRes", NULL), blk.getStr("ddsxPacks", NULL));

    nid = blk.getNameId("prebuiltGameResFolder");
    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamType(i) == DataBlock::TYPE_STRING && blk.getParamNameId(i) == nid)
      {
        String resDir(260, "%s/%s/", app_dir, blk.getStr(i));
        ::scan_for_game_resources(resDir, true, blk.getStr("ddsxPacks", NULL));
      }
    set_gameres_scan_recorder(NULL, NULL, NULL);

    const DataBlock *skipTypes = blk.getBlockByName("ignorePrebuiltAssetTypes");
    if (!skipTypes)
      skipTypes = blk.getBlockByNameEx("export")->getBlockByNameEx("types");

    mgr.mountBuiltGameResEx(scannedResBlk, *skipTypes);
  }

  auto mount_efx_assets = [&log](DagorAssetMgr &aMgr, const char *fx_blk_fname) {
    int atype = aMgr.getAssetTypeId("efx");
    if (atype < 0)
      return;

    DataBlock blk;
    if (!blk.load(fx_blk_fname))
    {
      log.addMessage(log.ERROR, "cannot load EFX blk: %s", fx_blk_fname);
      return;
    }
    if (!blk.blockCount())
      return;

    FastStrMap nm;
    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getStr("fx", NULL))
      {
        const char *fx_nm = blk.getBlock(i)->getBlockName();
        if (nm.addStrId(fx_nm, i) != i)
          log.addMessage(log.ERROR, "%s has duplicate FX <%s> at %d and %d block", fx_blk_fname, fx_nm, i, nm.getStrId(fx_nm));
      }

    aMgr.startMountAssetsDirect("ext.FX", 0);
    for (int i = 0; i < nm.getMapRaw().size(); i++)
      aMgr.makeAssetDirect(nm.getMapRaw()[i].name, *blk.getBlock(nm.getMapRaw()[i].id), atype);
    aMgr.stopMountAssetsDirect();
  };

  if (const char *efx_fname = appblk.getBlockByNameEx("assets")->getStr("efxBlk", NULL))
    mount_efx_assets(mgr, String(0, "%s/%s", app_dir, efx_fname));

  return success;
}
