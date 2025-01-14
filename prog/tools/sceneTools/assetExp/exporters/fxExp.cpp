// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetPlugin.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <scriptHelpers/scriptHelpers.h>
#include <fx/effectClassTools.h>
#include <fx/dag_commonFx.h>
#include <fx/commonFxTools.h>
#include <gameRes/dag_stdGameRes.h>
#include <debug/dag_debug.h>

static const char *TYPE = "fx";
extern String fx_devres_base_path;

BEGIN_DABUILD_PLUGIN_NAMESPACE(fx)

class EffectExporter : public IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const { return "fx exp"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }
  virtual unsigned __stdcall getGameResClassId() const { return EffectGameResClassId; }
  virtual unsigned __stdcall getGameResVersion() const { return 135; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override { files.clear(); }

  virtual bool __stdcall isExportableAsset(DagorAsset &a) { return true; }

  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
    String className(a.props.getStr("className", NULL));

    cwr.writeInt16e(className.length());
    cwr.writeRaw(className.data(), className.length());


    IEffectClassTools *toolsIface = ::get_effect_class_tools_interface(className);
    if (!toolsIface)
    {
      log.addMessage(ILogWriter::ERROR, "effect class '%s' is not registered for tools", className);
      return false;
    }

    ScriptHelpers::set_tuned_element(toolsIface->createTunedElement());

    ScriptHelpers::load_helpers(*a.props.getBlockByNameEx("params"));

    class SaveCB : public ScriptHelpers::SaveDataCB
    {
    public:
      Tab<const char *> refs;

      SaveCB() : refs(tmpmem) {}

      virtual int getRefSlotId(const char *name, bool make_unique)
      {
        for (int i = 0; i < refs.size(); ++i)
          if (stricmp(refs[i], name) == 0)
            return i;
        return -1;
      }

      void loadRefs(const DataBlock &main_blk)
      {
        int refNameId = main_blk.getNameId("ref");

        for (int bi = 0; bi < main_blk.blockCount(); ++bi)
        {
          const DataBlock &blk = *main_blk.getBlock(bi);
          if (blk.getBlockNameId() != refNameId)
            continue;

          const char *name = blk.getStr("slot", NULL);
          if (!name)
            continue;

          refs.push_back(name);
        }
      }
    } saveCb;

    saveCb.loadRefs(a.props);

    cwr.beginBlock();
    ScriptHelpers::save_params_data(cwr, &saveCb);
    cwr.align8();
    cwr.endBlock();

    return true;
  }
};

class EffectRefs : public IDagorAssetRefProvider
{
public:
  virtual const char *__stdcall getRefProviderIdStr() const { return "fx refs"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  dag::ConstSpan<Ref> __stdcall getAssetRefs(DagorAsset &a) override
  {
    static Tab<Ref> refs(tmpmem);
    refs.clear();

    int refNameId = a.props.getNameId("ref");
    for (int bi = 0; bi < a.props.blockCount(); ++bi)
    {
      const DataBlock &blk = *a.props.getBlock(bi);
      if (blk.getBlockNameId() != refNameId)
        continue;

      const char *name = blk.getStr("ref", NULL);
      const char *typeName = blk.getStr("type", NULL);

      Ref &r = refs.push_back();
      r.flags = RFLG_EXTERNAL;

      if (!name || !typeName)
      {
        r.flags |= RFLG_OPTIONAL;
        r.refAsset = NULL;
        // debug("%d: %s %s -> optional", refs.size(), name, typeName);
        continue;
      }

      r.refAsset = a.getMgr().findAsset(name, a.getMgr().getAssetTypeId(typeName));
      if (!r.refAsset)
        r.setBrokenRef(String(128, "%s:%s", name, typeName));
      // debug("%d: %s %s -> %p", refs.size(), name, typeName, r.refAsset);
    }

    return refs;
  }
};

class EffectExporterPlugin : public IDaBuildPlugin
{
public:
  virtual bool __stdcall init(const DataBlock &appblk)
  {
    const char *fx_nut = appblk.getBlockByNameEx("assets")->getStr("fxScriptsDir", NULL);
    if (fx_nut)
      fx_devres_base_path.printf(260, "%s/%s/", appblk.getStr("appDir", "."), fx_nut);

    register_all_common_fx_tools();
    return true;
  }
  virtual void __stdcall destroy() { delete this; }

  virtual int __stdcall getExpCount() { return 1; }
  virtual const char *__stdcall getExpType(int idx) { return TYPE; }
  virtual IDagorAssetExporter *__stdcall getExp(int idx) { return &exp; }

  virtual int __stdcall getRefProvCount() { return 1; }
  virtual const char *__stdcall getRefProvType(int idx) { return TYPE; }
  virtual IDagorAssetRefProvider *__stdcall getRefProv(int idx) { return &ref; }

protected:
  EffectExporter exp;
  EffectRefs ref;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) EffectExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(fx)
REGISTER_DABUILD_PLUGIN(fx, nullptr)
