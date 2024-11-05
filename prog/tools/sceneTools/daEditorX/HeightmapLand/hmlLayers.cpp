// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlObjectsEditor.h"
#include "hmlPlugin.h"
#include <de3_interface.h>
#include <de3_hmapService.h>
#include "hmlCm.h"
#include <math/dag_adjpow2.h>

FastNameMapEx EditLayerProps::layerNames;
Tab<EditLayerProps> EditLayerProps::layerProps;
int EditLayerProps::activeLayerIdx[EditLayerProps::TYPENUM];

void EditLayerProps::resetLayersToDefauls()
{
  layerNames.reset();
  layerNames.addNameId("default");

  layerProps.resize(EditLayerProps::TYPENUM);
  mem_set_0(layerProps);

  for (int i = 0; i < EditLayerProps::TYPENUM; i++)
  {
    layerProps[i].nameId = 0;
    layerProps[i].type = i;
    layerProps[i].renderToMask = 1;
    layerProps[i].exp = 1;
    layerProps[i].renameable = false;

    activeLayerIdx[i] = i;
  }
}

static const char *lptype_name[] = {"ent", "spl", "plg"};
void EditLayerProps::loadLayersConfig(const DataBlock &blk, const DataBlock &local_data)
{
  const DataBlock *layersBlk = blk.getBlockByName("layers");
  const DataBlock &layersBlk_local = *local_data.getBlockByNameEx("layers");
  for (int i = 0; i < layersBlk->blockCount(); i++)
  {
    const DataBlock &b = *layersBlk->getBlock(i);
    int nid = layerNames.addNameId(b.getBlockName());
    if (nid < 0)
    {
      DAEDITOR3.conError("bad layer name <%s> in %s", b.getBlockName(), "heightmapLand.plugin.blk");
      continue;
    }
    int lidx = findLayerOrCreate(b.getInt("type", 0), nid);

    const DataBlock &b_local =
      *layersBlk_local.getBlockByNameEx(lptype_name[layerProps[lidx].type])->getBlockByNameEx(b.getBlockName());
    layerProps[lidx].lock = b_local.getBool("lock", false);
    layerProps[lidx].hide = b_local.getBool("hide", false);
    layerProps[lidx].renderToMask = b_local.getBool("renderToMask", true);
    layerProps[lidx].exp = b.getBool("export", true);
    layerProps[lidx].renameable = false;
  }

  activeLayerIdx[0] = findLayer(ENT, layerNames.getNameId(layersBlk_local.getStr("ENT_active", "default")));
  activeLayerIdx[1] = findLayer(SPL, layerNames.getNameId(layersBlk_local.getStr("SPL_active", "default")));
  activeLayerIdx[2] = findLayer(PLG, layerNames.getNameId(layersBlk_local.getStr("PLG_active", "default")));
  for (int i = 0; i < EditLayerProps::TYPENUM; i++)
    if (activeLayerIdx[i] < 0)
      activeLayerIdx[i] = i;

  uint64_t lh_mask = 0;
  for (int i = 0; i < layerProps.size(); i++)
    if (layerProps[i].hide)
      lh_mask |= 1ull << i;
  DAEDITOR3.setEntityLayerHiddenMask(lh_mask);
}
void EditLayerProps::saveLayersConfig(DataBlock &blk, DataBlock &local_data)
{
  if (layerProps.size() != EditLayerProps::TYPENUM ||
      (layerProps[0].renderToMask & layerProps[1].renderToMask & layerProps[2].renderToMask) != 1 ||
      (layerProps[0].lock | layerProps[1].lock | layerProps[2].lock) != 0 ||
      (layerProps[0].hide | layerProps[1].hide | layerProps[2].hide) != 0)
  {
    DataBlock &layersBlk = *blk.addBlock("layers");
    DataBlock &layersBlk_local = *local_data.addBlock("layers");
    for (int i = 0; i < layerProps.size(); i++)
    {
      DataBlock &b = *layersBlk.addNewBlock(layerProps[i].name());
      DataBlock &b_local = *layersBlk_local.addBlock(lptype_name[layerProps[i].type])->addBlock(b.getBlockName());

      b.setInt("type", layerProps[i].type);
      b_local.setBool("lock", layerProps[i].lock);
      b_local.setBool("hide", layerProps[i].hide);
      b_local.setBool("renderToMask", layerProps[i].renderToMask);
      if (!layerProps[i].exp)
        b.setBool("export", layerProps[i].exp);
    }

    layersBlk_local.setStr("ENT_active", layerProps[activeLayerIdx[0]].name());
    layersBlk_local.setStr("SPL_active", layerProps[activeLayerIdx[1]].name());
    layersBlk_local.setStr("PLG_active", layerProps[activeLayerIdx[2]].name());
  }

  // After saving the layer config the newly created layers are no longer renameable.
  for (EditLayerProps &layerProp : layerProps)
    layerProp.renameable = false;
}

void EditLayerProps::renameLayer(int idx, const char *new_name)
{
  if (!layerProps[idx].renameable)
  {
    DAEDITOR3.conError("Cannot rename layer <%s>! Only new and not-yet saved layers can be renamed!", new_name);
    return;
  }

  const int nid = layerNames.addNameId(new_name);
  if (nid < 0)
  {
    DAEDITOR3.conError("Bad layer name <%s>!", new_name);
    return;
  }

  const int existingIdx = findLayer(layerProps[idx].type, nid);
  if (existingIdx >= 0)
  {
    DAEDITOR3.conError("Layer name <%s> is not unique within the type!", new_name);
    return;
  }

  layerProps[idx].nameId = nid;
}
