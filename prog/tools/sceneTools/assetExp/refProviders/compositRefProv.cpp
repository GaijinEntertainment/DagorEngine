#include <assets/daBuildExpPluginChain.h>
#include <assets/assetPlugin.h>
#include "../asset_ref_hlp.h"
#include <debug/dag_debug.h>

static const char *TYPE = "composit";

BEGIN_DABUILD_PLUGIN_NAMESPACE(composit)

class CompositClassRefs : public IDagorAssetRefProvider
{
public:
  virtual const char *__stdcall getRefProviderIdStr() const { return "composit refs"; }
  virtual const char *__stdcall getAssetType() const { return TYPE; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  dag::ConstSpan<Ref> __stdcall getAssetRefs(DagorAsset &a) override { return processAssetBlk(a.props, a.getMgr()); }

  static dag::ConstSpan<Ref> processAssetBlk(DataBlock &blk, DagorAssetMgr &amgr)
  {
    static Tab<IDagorAssetRefProvider::Ref> refs(tmpmem);
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
    return refs;
  }
};

class CompositClassRefProviderPlugin : public IDaBuildPlugin
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
  CompositClassRefs ref;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) CompositClassRefProviderPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(composit)
REGISTER_DABUILD_PLUGIN(composit, nullptr)
