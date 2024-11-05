// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetMsgPipe.h>
#include <de3_interface.h>
#include <libTools/util/conLogWriter.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>

#define DUMMY_IMPL              G_ASSERTF(false, "Unimplemented")
#define DUMMY_IMPL_VALUE(value) G_ASSERTF_RETURN(false, value, "Unimplemented")
#define DUMMY_IMPL_0            G_ASSERTF_RETURN(false, 0, "Unimplemented")

class MessagePipe final : public IDagorAssetMsgPipe
{
public:
  enum
  {
    NOTE,
    WARNING,
    ERROR,
    FATAL,
    REMARK
  };

  void onAssetMgrMessage(int msg_type, const char *msg, DagorAsset *asset, const char *asset_src_fpath) final
  {
    G_UNUSED(asset);
    G_UNUSED(asset_src_fpath);
    switch (msg_type)
    {
      case NOTE: debug("Note: %d", msg); break;
      case WARNING: logwarn("%d", msg); break;
      case ERROR:
        logerr("%d", msg);
        errorCount++;
        break;
      case FATAL:
        DAG_FATAL("%d", msg);
        errorCount++;
        break;
      case REMARK: debug("Remark: %d", msg); break;
    }
  }
  int getErrorCount() final { return errorCount; }
  void resetErrorCount() final { errorCount = 0; }

private:
  unsigned int errorCount = 0;
};

class DaEditor3Engine : public IDaEditor3Engine
{
public:
  DaEditor3Engine();
  ~DaEditor3Engine();

  dag::ConstSpan<DagorAsset *> getAssets() const;

  const char *getBuildString() override { return "1.0"; }

  bool initAssetBase(const char *app_dir) override;

  bool registerService(IEditorService *srv) override { DUMMY_IMPL_0; }
  bool unregisterService(IEditorService *srv) override { DUMMY_IMPL_0; }

  bool registerEntityMgr(IObjEntityMgr *oemgr) override { DUMMY_IMPL_0; }
  bool unregisterEntityMgr(IObjEntityMgr *oemgr) override { DUMMY_IMPL_0; }

  int getAssetTypeId(const char *entity_name) const override;
  const char *getAssetTypeName(int cls) const override { DUMMY_IMPL_0; }

  dag::ConstSpan<int> getGenObjAssetTypes() const override { DUMMY_IMPL_VALUE({}); }

  IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent = false) override { DUMMY_IMPL_0; }
  IObjEntity *createInvalidEntity(bool virtual_ent = false) override { DUMMY_IMPL_0; }
  IObjEntity *cloneEntity(IObjEntity *origin) override { DUMMY_IMPL_0; }
  int registerEntitySubTypeId(const char *subtype_str) override { DUMMY_IMPL_VALUE(-1); }

  unsigned getEntitySubTypeMask(int mask_type) override { DUMMY_IMPL_0; }
  void setEntitySubTypeMask(int mask_type, unsigned value) override { DUMMY_IMPL; }

  uint64_t getEntityLayerHiddenMask() override { DUMMY_IMPL_0; }
  void setEntityLayerHiddenMask(uint64_t value) override { DUMMY_IMPL; }

  DagorAsset *getAssetByName(const char *asset_name, int asset_type = -1) override;
  DagorAsset *getAssetByName(const char *asset_name, dag::ConstSpan<int> asset_types) override;
  const DataBlock *getAssetProps(const DagorAsset &asset) override { DUMMY_IMPL_0; }
  String getAssetTargetFilePath(const DagorAsset &asset) override { DUMMY_IMPL_VALUE(String()); }
  const char *getAssetParentFolderName(const DagorAsset &asset) override { DUMMY_IMPL_0; }
  const char *resolveTexAsset(const char *tex_asset_name) override { DUMMY_IMPL_0; }

  const char *selectAsset(const char *asset, const char *caption, dag::ConstSpan<int> types, const char *filter_str = nullptr,
    bool open_all_grp = false) override
  {
    DUMMY_IMPL_0;
  }
  void showAssetWindow(bool show, const char *caption, IAssetBaseViewClient *cli, dag::ConstSpan<int> types) override { DUMMY_IMPL; }
  void addAssetToRecentlyUsed(const char *asset) override { DUMMY_IMPL; }

  //! simple console messages output
  ILogWriter &getCon() override { return console; }
  void conErrorV(const char *fmt, const DagorSafeArg *arg, int anum) override;
  void conWarningV(const char *fmt, const DagorSafeArg *arg, int anum) override;
  void conNoteV(const char *fmt, const DagorSafeArg *arg, int anum) override;
  void conRemarkV(const char *fmt, const DagorSafeArg *arg, int anum) override;
  void conShow(bool show_console_wnd) override { DUMMY_IMPL; }

  void setFatalHandler(bool quiet = false) override { DUMMY_IMPL; }
  bool getFatalStatus() override { DUMMY_IMPL_0; }
  void resetFatalStatus() override { DUMMY_IMPL; }
  void popFatalHandler() override { DUMMY_IMPL; }

  bool getTexAssetBuiltDDSx(DagorAsset &a, ddsx::Buffer &dest, unsigned target, const char *profile, ILogWriter *log) override
  {
    DUMMY_IMPL_0;
  }
  bool getTexAssetBuiltDDSx(const char *a_name, const DataBlock &a_props, ddsx::Buffer &dest, unsigned target, const char *profile,
    ILogWriter *log) override
  {
    DUMMY_IMPL_0;
  }

  DagorAssetMgr &getAssetManager();
  const DagorAssetMgr &getAssetManager() const;

  ConsoleLogWriter *getConsoleLogWriter() { return &console; }

  void imguiBegin(const char *name, bool *open, unsigned window_flags) override {}
  void imguiBegin(PropPanel::PanelWindowPropertyControl &panel_window, bool *open, unsigned window_flags) override {}
  void imguiEnd() override {}

  virtual Outliner::OutlinerWindow *createOutlinerWindow() override
  {
    G_ASSERT(false);
    return nullptr;
  }

private:
  ConsoleLogWriter console;
  DagorAssetMgr assetMgr;
  MessagePipe msgPipe;

  bool minimizeDabuildUsage = false;
  bool srcAssetsScanAllowed = true;
  bool dabuildUsageAllowed = true;

  void conMessageV(ILogWriter::MessageType type, const char *msg, const DagorSafeArg *arg, int anum);
  void addMessage(ILogWriter::MessageType type, const char *s);
};

#undef DUMMY_IMPL
#undef DUMMY_IMPL_VALUE
#undef DUMMY_IMPL_0
