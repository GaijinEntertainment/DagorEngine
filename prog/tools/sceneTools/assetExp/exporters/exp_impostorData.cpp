#include <gameRes/dag_stdGameRes.h>
#include <assets/asset.h>
#include <assets/assetExporter.h>
#include <assets/assetMgr.h>
#include <assets/assetRefs.h>
#include <libTools/util/binDumpUtil.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/makeBindump.h>
#include "fatalHandler.h"

#include "dabuild_exp_plugin_chain.h"

static const char *TYPE = "impostorData";

BEGIN_DABUILD_PLUGIN_NAMESPACE(impostor)

class ImpostorExporter final : public IDagorAssetExporter
{
public:
  const char *__stdcall getExporterIdStr() const override { return "impostorData exp"; }

  const char *__stdcall getAssetType() const override { return TYPE; }
  unsigned __stdcall getGameResClassId() const override { return ImpostorDataGameResClassId; }
  unsigned __stdcall getGameResVersion() const override
  {
    const int base_ver = 1;
    return base_ver;
  }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();
    files.push_back() = a.getTargetFilePath();
  }

  bool __stdcall isExportableAsset(DagorAsset &a) override { return true; }

  bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) override
  {
    DataBlock impostorDataBlk;
    impostorDataBlk.load(a.getTargetFilePath());
    int blockCount = impostorDataBlk.blockCount();
    DataBlock exportBlk;
    for (int i = 0; i < blockCount; ++i)
    {
      const DataBlock *assetBlock = impostorDataBlk.getBlock(i);
      const DataBlock *contentBlk = assetBlock->getBlockByName("content");
      if (contentBlk == nullptr)
      {
        log.addMessage(ILogWriter::ERROR, "Impostor data is missing the 'content' block <%s>", assetBlock->getBlockName());
        return false;
      }
      DataBlock *targetBlock = exportBlk.addBlock(assetBlock->getBlockName());
      targetBlock->appendParamsFrom(contentBlk);
    }
    cwr.beginBlock();
    write_ro_datablock(cwr, exportBlk);
    cwr.endBlock();
    return true;
  }
};

class ImpostorClassExporterPlugin final : public IDaBuildPlugin
{
public:
  bool __stdcall init(const DataBlock &appblk) override { return true; }
  void __stdcall destroy() override { delete this; }

  int __stdcall getExpCount() override { return 1; }
  const char *__stdcall getExpType(int idx) override
  {
    switch (idx)
    {
      case 0: return TYPE;
      default: return nullptr;
    }
  }
  IDagorAssetExporter *__stdcall getExp(int idx) override
  {
    switch (idx)
    {
      case 0: return &exp;
      default: return nullptr;
    }
  }

  int __stdcall getRefProvCount() override { return 0; }
  const char *__stdcall getRefProvType(int idx) override { return nullptr; }
  IDagorAssetRefProvider *__stdcall getRefProv(int idx) override { return nullptr; }

protected:
  ImpostorExporter exp;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) ImpostorClassExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(impostor)
REGISTER_DABUILD_PLUGIN(impostor, nullptr)
