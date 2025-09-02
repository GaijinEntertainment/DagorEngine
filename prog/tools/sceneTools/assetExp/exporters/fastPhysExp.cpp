// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetPlugin.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <libTools/fastPhysData/fp_data.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <gameRes/dag_stdGameRes.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include "getSkeleton.h"

BEGIN_DABUILD_PLUGIN_NAMESPACE(fastPhys)

static const char *TYPE = "fastPhys";

static int skeleton_atype = -2;

static void init_atypes(DagorAssetMgr &mgr)
{
  static DagorAssetMgr *m = NULL;
  if (m != &mgr)
  {
    m = &mgr;
    skeleton_atype = m->getAssetTypeId("skeleton");
  }
}


class FastPhysExp : public IDagorAssetExporter
{
public:
  const char *__stdcall getExporterIdStr() const override { return "fastPhys exp"; }

  const char *__stdcall getAssetType() const override { return TYPE; }
  unsigned __stdcall getGameResClassId() const override { return FastPhysDataGameResClassId; }
  unsigned __stdcall getGameResVersion() const override { return 4; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    init_atypes(a.getMgr());
    files.clear();
    const char *gn_name = a.props.getStr("skeleton", NULL);
    DagorAsset *gn_a = gn_name ? a.getMgr().findAsset(gn_name, skeleton_atype) : NULL;
    if (gn_a)
      files.push_back() = gn_a->getTargetFilePath();
  }

  bool __stdcall isExportableAsset(DagorAsset &a) override { return true; }

  bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) override
  {
    FpdExporter exp;
    if (!getSkeleton(exp.nodeTree, a.getMgr(), a.props.getStr("skeleton", NULL), log))
      return false;

    if (!exp.load(a.props))
      return false;
    return exp.exportFastPhys(cwr);
  }
};

class FastPhysRefs : public IDagorAssetRefProvider
{
public:
  const char *__stdcall getRefProviderIdStr() const override { return "fastPhys refs"; }

  const char *__stdcall getAssetType() const override { return TYPE; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override
  {
    init_atypes(a.getMgr());

    refs.clear();

    const char *gn_name = a.props.getStr("skeleton", "");
    DagorAsset *gn_a = a.getMgr().findAsset(gn_name, skeleton_atype);
    {
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = 0;
      if (!gn_a)
        r.setBrokenRef(String(64, "%s:skeleton", gn_name));
      else
        r.refAsset = gn_a;
    }
  }
};

class FastPhysExporterPlugin : public IDaBuildPlugin
{
public:
  bool __stdcall init(const DataBlock &appblk) override { return true; }
  void __stdcall destroy() override { delete this; }

  int __stdcall getExpCount() override { return 1; }
  const char *__stdcall getExpType(int idx) override { return TYPE; }
  IDagorAssetExporter *__stdcall getExp(int idx) override { return &exp; }

  int __stdcall getRefProvCount() override { return 1; }
  const char *__stdcall getRefProvType(int idx) override { return TYPE; }
  IDagorAssetRefProvider *__stdcall getRefProv(int idx) override { return &ref; }

protected:
  FastPhysExp exp;
  FastPhysRefs ref;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) FastPhysExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(fastPhys)
REGISTER_DABUILD_PLUGIN(fastPhys, nullptr)
