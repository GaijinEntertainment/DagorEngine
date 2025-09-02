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
  const char *__stdcall getRefProviderIdStr() const override { return "spline refs"; }
  const char *__stdcall getAssetType() const override { return TYPE; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override { processAssetBlk(a.props, a.getMgr(), refs); }

  static void processAssetBlk(DataBlock &blk, DagorAssetMgr &amgr, Tab<Ref> &refs)
  {
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
  }
};

class SplineClassRefProviderPlugin : public IDaBuildPlugin
{
public:
  bool __stdcall init(const DataBlock &appblk) override { return true; }
  void __stdcall destroy() override { delete this; }

  int __stdcall getExpCount() override { return 0; }
  const char *__stdcall getExpType(int) override { return nullptr; }
  IDagorAssetExporter *__stdcall getExp(int) override { return nullptr; }

  int __stdcall getRefProvCount() override { return 1; }
  const char *__stdcall getRefProvType(int idx) override { return TYPE; }
  IDagorAssetRefProvider *__stdcall getRefProv(int idx) override { return &ref; }

protected:
  SplineClassRefs ref;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) SplineClassRefProviderPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(spline)
REGISTER_DABUILD_PLUGIN(spline, nullptr)
