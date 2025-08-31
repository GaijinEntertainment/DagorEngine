// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetPlugin.h>
#include "../asset_ref_hlp.h"
#include <debug/dag_debug.h>

static const char *TYPE = "composit";

BEGIN_DABUILD_PLUGIN_NAMESPACE(composit)

class CompositClassRefs : public IDagorAssetRefProvider
{
public:
  const char *__stdcall getRefProviderIdStr() const override { return "composit refs"; }
  const char *__stdcall getAssetType() const override { return TYPE; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override { processAssetBlk(a.props, a.getMgr(), refs); }

  static void processAssetBlk(DataBlock &blk, DagorAssetMgr &amgr, Tab<IDagorAssetRefProvider::Ref> &refs)
  {
    int node_nid = blk.getNameId("node");
    int name_nid = blk.getNameId("name");
    int ent_nid = blk.getNameId("ent");
    int spline_nid = blk.getNameId("spline");
    int polygon_nid = blk.getNameId("polygon");
    int point_nid = blk.getNameId("point");

    refs.clear();
    dblk::iterate_blocks_by_name(blk, "node", [&](const DataBlock &b) {
      dblk::iterate_params_by_name_id_and_type(b, name_nid, DataBlock::TYPE_STRING,
        [&](int param_idx) { add_asset_ref(refs, amgr, b.getStr(param_idx), false, false, -1, true); });

      dblk::iterate_child_blocks(b, [&](const DataBlock &b2) {
        if (b2.getBlockNameId() == ent_nid)
          add_asset_ref(refs, amgr, b2.getStr("name", nullptr), false, false, -1, true);
        else if (b2.getBlockNameId() == spline_nid || b2.getBlockNameId() == polygon_nid)
        {
          const char *splAssetName = b2.getStr("blkGenName", "");
          dblk::iterate_child_blocks_by_name_id(b2, point_nid, [&](const DataBlock &b3) {
            const char *splname = b3.getBool("useDefSplineGen", true) ? splAssetName : b3.getStr("blkGenName", "");
            if (*splname)
              add_asset_ref(refs, amgr, splname, false, false, amgr.getAssetTypeId("spline"), true);
          });
        }
      });
    });
  }
};

class CompositClassRefProviderPlugin : public IDaBuildPlugin
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
  CompositClassRefs ref;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) CompositClassRefProviderPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(composit)
REGISTER_DABUILD_PLUGIN(composit, nullptr)
