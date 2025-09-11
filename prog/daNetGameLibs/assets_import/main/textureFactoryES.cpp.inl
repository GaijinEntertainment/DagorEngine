// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texMgr.h>
#include <drv/3d/dag_driver.h>
#include <osApiWrappers/dag_miscApi.h>
#include <assets/asset.h>
#include <memory/dag_framemem.h>

#include <util/dag_string.h>
#include "assetStatus.h"

#include "../lib3d/texMgrData.h"
#include "3d/dag_buildOnDemandTexFactory.h"


AssetLoadingStatus get_texture_status(const char *name)
{
  if (build_on_demand_tex_factory::get())
  {
    D3DRESID tid = get_managed_texture_id(String(0, framemem_ptr(), "%s*", name));
    ManagedTexEntryDesc desc;
    if (::get_managed_tex_entry_desc(tid, desc))
    {
      if (desc.isLoading)
        return AssetLoadingStatus::Loading;
      if (desc.isLoadedWithErrors)
        return AssetLoadingStatus::LoadedWithErrors;
      return AssetLoadingStatus::Loaded;
    }
  }
  return AssetLoadingStatus::NotLoaded;
}

void reregister_texture_in_build_on_demand_factory(const DagorAsset &a)
{
  using texmgr_internal::RMGR;
  TextureFactory *factory = build_on_demand_tex_factory::get();
  G_ASSERT_RETURN(factory, );
  D3DRESID tid = get_managed_texture_id(String(0, "%s*", a.getName()));
  if (!tid || !RMGR.isValidID(tid, nullptr) || RMGR.getFactory(tid.index()) == factory)
    return;
  texmgr_internal::TexMgrAutoLock autoLock;
  unsigned idx = tid.index();

  // see getAssetNameIdFromPackRecIdx for details
  for (auto &r : RMGR.texDesc[idx].packRecIdx)
  {
    r.pack = 0x7E00u | (uint32_t(a.getNameId()) >> 16u);
    r.rec = uint32_t(a.getNameId()) & 0xffffu;
  }

  SimpleString nm(RMGR.getName(idx));
  RMGR.setName(idx, nullptr, RMGR.getFactory(idx));
  RMGR.setName(idx, nm, factory);
  RMGR.setFactory(idx, factory);
  RMGR.dumpTexState(idx);
}
