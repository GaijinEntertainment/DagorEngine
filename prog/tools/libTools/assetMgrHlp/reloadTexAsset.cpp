// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetMsgPipe.h>
#include <util/dag_string.h>
#include <assets/texAssetBuilderTextureFactory.h>
#include <3d/dag_texMgr.h>

bool reload_changed_texture_asset(const DagorAsset &a)
{
  String texName;
  texName.setStrCat(a.getName(), "*");

  TEXTUREID texAssetId = get_managed_texture_id(texName);
  TEXTUREID texId = get_managed_texture_id(a.getSrcFilePath());

  if (texAssetId == BAD_TEXTUREID && texId == BAD_TEXTUREID)
    return false;

  bool ld = texconvcache::build_on_demand_tex_factory_cease_loading(false);
  discard_unused_managed_texture(texAssetId);
  discard_unused_managed_texture(texId);

  if (downgrade_managed_tex_to_lev(texAssetId, 1) || downgrade_managed_tex_to_lev(texId, 1))
    a.getMgr().getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::NOTE,
      String(256, "texture '%s' is discarded and to be reloaded", a.getName()), NULL, NULL);
  if (ld)
    texconvcache::build_on_demand_tex_factory_cease_loading(ld);

  if (int arrtex_changed = reload_managed_array_textures_for_changed_slice(texName))
    a.getMgr().getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::NOTE,
      String(256, "reloaded %d arrtex due to asset '%s' changed", arrtex_changed, a.getName()), NULL, NULL);
  return true;
}
