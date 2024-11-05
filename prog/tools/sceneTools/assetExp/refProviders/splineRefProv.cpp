// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetPlugin.h>
#include "../asset_ref_hlp.h"
#include <debug/dag_debug.h>

static const char *TYPE = "spline";

BEGIN_DABUILD_PLUGIN_NAMESPACE(spline)

class SplineClassRefs : public IDagorAssetRefProvider
{
public:
  virtual const char *__stdcall getRefProviderIdStr() const { return "spline refs"; }
  virtual const char *__stdcall getAssetType() const { return TYPE; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  dag::ConstSpan<Ref> __stdcall getAssetRefs(DagorAsset &a) override { return processAssetBlk(a.props, a.getMgr()); }

  static dag::ConstSpan<Ref> processAssetBlk(DataBlock &blk, DagorAssetMgr &amgr)
  {
    static Tab<IDagorAssetRefProvider::Ref> refs(tmpmem);
    int mat_atype = amgr.getAssetTypeId("mat");

    refs.clear();
    if (const DataBlock *bRoad = blk.getBlockByName("road"))
      dblk::iterate_child_blocks_by_name(*bRoad, "lamp",
        [&](const DataBlock &bLamp) { add_asset_ref(refs, amgr, bLamp.getStr("name", nullptr), false, false, -1, true); });
    dblk::iterate_child_blocks_by_name(blk, "obj_generate", [&](const DataBlock &bGen) {
      dblk::iterate_child_blocks_by_name(bGen, "object",
        [&](const DataBlock &bObj) { add_asset_ref(refs, amgr, bObj.getStr("name", nullptr), false, false, -1, true); });
    });

    dblk::iterate_child_blocks_by_name(blk, "loft",
      [&](const DataBlock &bLoft) { add_asset_ref(refs, amgr, bLoft.getStr("matName", nullptr), false, false, mat_atype, true); });

    return refs;
  }
};

class SplineClassRefProviderPlugin : public IDaBuildPlugin
{
public:
  virtual bool __stdcall init(const DataBlock &appblk) { return true; }
  virtual void __stdcall destroy() { delete this; }

  virtual int __stdcall getExpCount() { return 0; }
  virtual const char *__stdcall getExpType(int) { return nullptr; }
  virtual IDagorAssetExporter *__stdcall getExp(int) { return nullptr; }

  virtual int __stdcall getRefProvCount() { return 1; }
  virtual const char *__stdcall getRefProvType(int idx) { return TYPE; }
  virtual IDagorAssetRefProvider *__stdcall getRefProv(int idx) { return &ref; }

protected:
  SplineClassRefs ref;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) SplineClassRefProviderPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(spline)
REGISTER_DABUILD_PLUGIN(spline, nullptr)
