#pragma once

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetRefs.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>

static DagorAsset *get_asset_by_name(DagorAssetMgr &amgr, const char *_name, int asset_type)
{
  if (!_name || !_name[0])
    return NULL;

  String name(dd_get_fname(_name));
  remove_trailing_string(name, ".res.blk");
  name = DagorAsset::fpath2asset(name);

  const char *type = strchr(name, ':');
  if (type)
  {
    asset_type = amgr.getAssetTypeId(type + 1);
    if (asset_type == -1)
    {
      logerr("invalid type of asset: %s", name.str());
      return NULL;
    }
    return amgr.findAsset(String::mk_sub_str(name, type), asset_type);
  }
  else if (asset_type == -1)
    return amgr.findAsset(name);
  return amgr.findAsset(name, asset_type);
}
static bool is_explicit_asset_name_non_exportable(DagorAssetMgr &amgr, const char *_name)
{
  if (!_name || !_name[0])
    return false;

  String name(dd_get_fname(_name));
  remove_trailing_string(name, ".res.blk");
  name = DagorAsset::fpath2asset(name);

  const char *type = strchr(name, ':');
  if (type)
  {
    int asset_type = amgr.getAssetTypeId(type + 1);
    if (asset_type == -1)
      return false;
    if (!amgr.getAssetExporter(asset_type))
      return true;
  }
  return false;
}

static bool add_asset_ref(Tab<IDagorAssetRefProvider::Ref> &refs, DagorAssetMgr &amgr, const char *nm, bool optional, bool external,
  int atype, bool skip_dup)
{
  if (!nm)
    return false;
  IDagorAssetRefProvider::Ref r1;
  r1.flags = 0;
  if (DagorAsset *a = get_asset_by_name(amgr, nm, atype))
    r1.refAsset = a;
  else
    r1.setBrokenRef(nm);
  if (optional)
    r1.flags |= IDagorAssetRefProvider::RFLG_OPTIONAL;
  if (external)
    r1.flags |= IDagorAssetRefProvider::RFLG_EXTERNAL;

  if (skip_dup)
    for (auto &r : refs)
      if (r.flags == r1.flags && r.refAsset == r1.refAsset)
        return false;
  refs.push_back(r1);
  return true;
}
