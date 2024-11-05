// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetHlp.h>
#include <gameRes/dag_gameResources.h>

class EffectsPluginSaveLoadParamsDataCB : public ScriptHelpers::SaveDataCB, public BaseParamScriptLoadCB
{
public:
  struct RefSlot
  {
    String name;
    void *ref;

    RefSlot() : ref(NULL) {}
  };

  Tab<RefSlot> refs;

  Tab<GameResource *> resRefs;
  int selfGameResId = 0;

  Tab<const char *> brokenRefs;

  EffectsPluginSaveLoadParamsDataCB() : refs(tmpmem), resRefs(tmpmem) {}


  int findRefSlot(const char *name) const
  {
    for (int i = 0; i < refs.size(); ++i)
      if (stricmp(refs[i].name, name) == 0)
        return i;
    return -1;
  }


  virtual int getRefSlotId(const char *name, bool make_unique)
  {
    String slotName(name);

    for (;;)
    {
      int id = findRefSlot(slotName);
      if (id < 0)
      {
        id = append_items(refs, 1);

        RefSlot &r = refs[id];
        r.name = slotName;

        return id;
      }

      if (!make_unique)
        return id;

      ::make_next_numbered_name(slotName, 2);
    }
  }


  int getSelfGameResId() override { return selfGameResId; }
  void *getReference(int id) override
  {
    if (id < 0 || id >= refs.size())
      return NULL;
    return refs[id].ref;
  }

  const char *getBrokenRefName(int id) override
  {
    if (id < 0 || id >= brokenRefs.size())
      return nullptr;
    return brokenRefs[id];
  }

  void createRefs(DagorAsset &asset, dag::ConstSpan<IDagorAssetRefProvider::Ref> ref_list)
  {
    const DataBlock &main_blk = asset.props;
    int refNameId = main_blk.getNameId("ref");
    int tex_atype = asset.getMgr().getTexAssetTypeId();
    int fx_atype = asset.getMgr().getAssetTypeId("fx");
    int ordId = 0;
    Tab<int> remap(tmpmem);

    selfGameResId = dabuildcache::get_asset_res_id(asset);
    remap.resize(refs.size());
    mem_set_ff(remap);
    brokenRefs.resize(refs.size());
    mem_set_0(brokenRefs);
    for (int bi = 0; bi < main_blk.blockCount(); ++bi)
    {
      const DataBlock &blk = *main_blk.getBlock(bi);
      if (blk.getBlockNameId() != refNameId)
        continue;

      const char *name = blk.getStr("slot", NULL);
      if (!name)
        continue;

      int id = findRefSlot(name);
      if (id >= 0)
        remap[id] = ordId;
      ordId++;
    }

    for (int ri = 0; ri < refs.size(); ++ri)
    {
      int i = remap[ri];
      if (i < 0 || i >= ref_list.size())
        continue;
      if (ref_list[i].flags & IDagorAssetRefProvider::RFLG_BROKEN)
      {
        brokenRefs[ri] = ref_list[i].getBrokenRef();
        if (strstr(brokenRefs[ri], ":tex"))
          refs[ri].ref = (void *)(uintptr_t)D3DRESID::INVALID_ID;
        get_app().getConsole().addMessage(ILogWriter::ERROR, "broken %s", brokenRefs[ri]);
        continue;
      }

      if (!ref_list[i].getAsset())
        continue;

      if (ref_list[i].getAsset()->getType() == tex_atype)
      {
        add_managed_texture(String(260, "%s*", ref_list[i].getAsset()->getName()));
        TEXTUREID texId = get_tex_gameres(ref_list[i].getAsset()->getName());
        refs[ri].ref = (void *)(uintptr_t) unsigned(texId);
        resRefs.push_back((GameResource *)(uintptr_t) unsigned(texId));
      }
      else if (ref_list[i].getAsset()->getType() == fx_atype)
      {
        GameResource *res = get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(ref_list[i].getAsset()->getName()), EffectGameResClassId);
        refs[ri].ref = res;
        resRefs.push_back(res);
      }
    }
  }
};
