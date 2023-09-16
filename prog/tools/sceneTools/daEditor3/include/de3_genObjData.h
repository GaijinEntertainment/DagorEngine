//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <EditorCore/ec_interface.h>
#include <de3_landClassData.h>
#include <de3_assetService.h>


namespace objgenerator
{
class LandClassData
{
public:
  LandClassData(const char *asset_name) : data(NULL), poolTiled(midmem), poolPlanted(midmem)
  {
    IAssetService *srv = EDITORCORE->queryEditorInterface<IAssetService>();
    if (srv)
      data = srv->getLandClassData(asset_name);
    if (data && data->tiled)
      poolTiled.resize(data->tiled->data.size());
    if (data && data->planted)
      poolPlanted.resize(data->planted->ent.size());
  }
  ~LandClassData()
  {
    IAssetService *srv = EDITORCORE->queryEditorInterface<IAssetService>();
    if (srv && data)
      srv->releaseLandClassData(data);
    data = NULL;
    clear_and_shrink(poolTiled);
    clear_and_shrink(poolPlanted);
  }

  void beginGenerate()
  {
    for (int gi = 0; gi < poolTiled.size(); gi++)
      poolTiled[gi].resetUsedEntities();
    for (int gi = 0; gi < poolPlanted.size(); gi++)
      poolPlanted[gi].resetUsedEntities();
  }

  void endGenerate()
  {
    for (int gi = 0; gi < poolTiled.size(); gi++)
      poolTiled[gi].deleteUnusedEntities();
    for (int gi = 0; gi < poolPlanted.size(); gi++)
      poolPlanted[gi].deleteUnusedEntities();
  }

  void clearObjects()
  {
    beginGenerate();
    endGenerate();
  }

public:
  landclass::AssetData *data;
  Tab<landclass::SingleEntityPool> poolTiled;
  Tab<landclass::SingleEntityPool> poolPlanted;
};
} // namespace objgenerator
