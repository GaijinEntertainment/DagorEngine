// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetPlugin.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <gameRes/dag_stdGameRes.h>
#include <assets/assetMgr.h>
#include <debug/dag_debug.h>
#include <3d/dag_materialData.h>
#include <math/dag_Point4.h>
#include <util/dag_string.h>

BEGIN_DABUILD_PLUGIN_NAMESPACE(mat)

static const char *TYPE = "mat";

class MaterialExporter : public IDagorAssetExporter
{
public:
  const char *__stdcall getExporterIdStr() const override { return "material exp"; }

  const char *__stdcall getAssetType() const override { return TYPE; }
  unsigned __stdcall getGameResClassId() const override { return MaterialGameResClassId; }
  unsigned __stdcall getGameResVersion() const override { return 2; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override { files.clear(); }

  bool __stdcall isExportableAsset(DagorAsset &a) override { return true; }

  void writePoint4(const Point4 &point, mkbindump::BinDumpSaveCB &cwr)
  {
    cwr.writeReal(point.x);
    cwr.writeReal(point.y);
    cwr.writeReal(point.z);
    cwr.writeReal(point.w);
  }

  bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) override
  {
    cwr.writeDwString(a.props.getStr("class_name", "simple"));
    cwr.writeDwString(a.props.getStr("script", ""));
    cwr.writeInt32e(a.props.getInt("flags", 0));

    cwr.writeReal(a.props.getReal("power", 0));
    writePoint4(a.props.getPoint4("diff", Point4(1, 1, 1, 1)), cwr);
    writePoint4(a.props.getPoint4("amb", Point4(1, 1, 1, 1)), cwr);
    writePoint4(a.props.getPoint4("spec", Point4(1, 1, 1, 1)), cwr);
    writePoint4(a.props.getPoint4("emis", Point4(1, 1, 1, 1)), cwr);

    return true;
  }
};

class MaterialRefs : public IDagorAssetRefProvider
{
public:
  const char *__stdcall getRefProviderIdStr() const override { return "material refs"; }

  const char *__stdcall getAssetType() const override { return TYPE; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override
  {
    refs.clear();
    if (a.getMgr().getTexAssetTypeId() < 0)
      return;

    const DataBlock *texturesBlk = a.props.getBlockByNameEx("textures");
    String texBuf;

    for (int i = 0; i < MAXMATTEXNUM; i++)
    {
      texBuf.printf(16, "tex%d", i);
      const char *texName = texturesBlk->getStr(texBuf, NULL);
      if (!texName)
        continue;

      DagorAsset *texA = a.getMgr().findAsset(texName, a.getMgr().getTexAssetTypeId());
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = RFLG_EXTERNAL;

      if (!texA)
        r.setBrokenRef(String(64, "%s:tex", texName));
      else
        r.refAsset = texA;
    }
  }
};

class MaterialExporterPlugin : public IDaBuildPlugin
{
public:
  bool __stdcall init(const DataBlock &appblk) override { return true; }
  void __stdcall destroy() override { delete this; }

  int __stdcall getExpCount() override { return 1; }
  const char *__stdcall getExpType(int idx) override
  {
    switch (idx)
    {
      case 0: return "mat";
      default: return NULL;
    }
  }
  IDagorAssetExporter *__stdcall getExp(int idx) override
  {
    switch (idx)
    {
      case 0: return &expMat;
      default: return NULL;
    }
  }

  int __stdcall getRefProvCount() override { return 1; }
  const char *__stdcall getRefProvType(int idx) override
  {
    switch (idx)
    {
      case 0: return "mat";
      default: return NULL;
    }
  }
  IDagorAssetRefProvider *__stdcall getRefProv(int idx) override
  {
    switch (idx)
    {
      case 0: return &refsMat;
      default: return NULL;
    }
  }

protected:
  MaterialExporter expMat;
  MaterialRefs refsMat;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) MaterialExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(mat)
REGISTER_DABUILD_PLUGIN(mat, nullptr)
