//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>
#include <util/dag_safeArg.h>
#include <util/dag_stdint.h>
#include <3d/dag_stereoIndex.h>

class DagorAsset;
class IAssetBaseViewClient;

class IEditorService;
class IObjEntity;
class IObjEntityMgr;
class ILogWriter;
class DataBlock;

namespace Outliner
{
class OutlinerWindow;
}

namespace PropPanel
{
class PanelWindowPropertyControl;
}

namespace ddsx
{
struct Buffer;
}


class IDaEditor3Engine
{
public:
  static constexpr unsigned HUID = 0x814F7714u; // IDaEditor3Engine
  static const int DAEDITOR3_VERSION = 0x107;

  // generic version compatibility check
  inline bool checkVersion() const { return (daEditor3InterfaceVer == DAEDITOR3_VERSION); }

  virtual bool initAssetBase(const char *app_dir) = 0;

  virtual const char *getBuildString() = 0;
  virtual bool registerService(IEditorService *srv) = 0;
  virtual bool unregisterService(IEditorService *srv) = 0;
  virtual IEditorService *findService(const char *internalName) const = 0;

  virtual bool registerEntityMgr(IObjEntityMgr *oemgr) = 0;
  virtual bool unregisterEntityMgr(IObjEntityMgr *oemgr) = 0;

  virtual int getAssetTypeId(const char *entity_name) const = 0;
  virtual const char *getAssetTypeName(int cls) const = 0;

  virtual dag::ConstSpan<int> getGenObjAssetTypes() const = 0;

  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent = false) = 0;
  virtual IObjEntity *createInvalidEntity(bool virtual_ent = false) = 0;
  virtual IObjEntity *cloneEntity(IObjEntity *origin) = 0;
  virtual int registerEntitySubTypeId(const char *subtype_str) = 0;

  virtual unsigned getEntitySubTypeMask(int mask_type) = 0;
  virtual void setEntitySubTypeMask(int mask_type, unsigned value) = 0;

  virtual uint64_t getEntityLayerHiddenMask() = 0;
  virtual void setEntityLayerHiddenMask(uint64_t value) = 0;

  virtual DagorAsset *getAssetByName(const char *asset_name, int asset_type = -1) = 0;
  virtual DagorAsset *getAssetByName(const char *asset_name, dag::ConstSpan<int> asset_types) = 0;
  virtual const DataBlock *getAssetProps(const DagorAsset &asset) = 0;
  virtual String getAssetTargetFilePath(const DagorAsset &asset) = 0;
  virtual const char *getAssetParentFolderName(const DagorAsset &asset) = 0;
  virtual const char *resolveTexAsset(const char *tex_asset_name) = 0;

  virtual const char *selectAsset(const char *asset, const char *caption, dag::ConstSpan<int> types, const char *filter_str = nullptr,
    bool open_all_grp = false) = 0;
  virtual void showAssetWindow(bool show, const char *caption, IAssetBaseViewClient *cli, dag::ConstSpan<int> types) = 0;
  virtual void addAssetToRecentlyUsed(const char *asset) = 0;

  //! simple console messages output
  virtual ILogWriter &getCon() = 0;
  virtual void conErrorV(const char *fmt, const DagorSafeArg *arg, int anum) = 0;
  virtual void conWarningV(const char *fmt, const DagorSafeArg *arg, int anum) = 0;
  virtual void conNoteV(const char *fmt, const DagorSafeArg *arg, int anum) = 0;
  virtual void conRemarkV(const char *fmt, const DagorSafeArg *arg, int anum) = 0;
  virtual void conShow(bool show_console_wnd) = 0;

  //! simple fatal handler interface
  virtual void setFatalHandler(bool quiet = false) = 0;
  virtual bool getFatalStatus() = 0;
  virtual void resetFatalStatus() = 0;
  virtual void popFatalHandler() = 0;

  //! proxy for texconvcache::get_tex_asset_built_ddsx
  virtual bool getTexAssetBuiltDDSx(DagorAsset &a, ddsx::Buffer &dest, unsigned target, const char *profile, ILogWriter *log) = 0;
  //! helper to build pseudo-texture-asset via properties BLK
  virtual bool getTexAssetBuiltDDSx(const char *a_name, const DataBlock &a_props, ddsx::Buffer &dest, unsigned target,
    const char *profile, ILogWriter *log) = 0;

  virtual void imguiBegin(const char *name, bool *open = nullptr, unsigned window_flags = 0) = 0;
  virtual void imguiBegin(PropPanel::PanelWindowPropertyControl &panel_window, bool *open = nullptr, unsigned window_flags = 0) = 0;
  virtual void imguiEnd() = 0;

  inline const char *selectAssetX(const char *asset, const char *caption, const char *type, const char *filter_str = nullptr,
    bool open_all_grp = false)
  {
    int t = getAssetTypeId(type);
    return selectAsset(asset, caption, make_span_const(&t, 1), filter_str, open_all_grp);
  }
  inline void hideAssetWindow() { showAssetWindow(false, NULL, NULL, {}); }

  inline void setStereoIndex(StereoIndex index) { stereoIndex = index; }
  inline StereoIndex getStereoIndex() const { return stereoIndex; }

  inline DagorAsset *getGenObjAssetByName(const char *asset_name) { return getAssetByName(asset_name, getGenObjAssetTypes()); }

  virtual Outliner::OutlinerWindow *createOutlinerWindow() = 0;

#define DSA_OVERLOADS_PARAM_DECL
#define DSA_OVERLOADS_PARAM_PASS
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void conError, conErrorV);
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void conWarning, conWarningV);
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void conNote, conNoteV);
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void conRemark, conRemarkV);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

public:
  static inline IDaEditor3Engine &get() { return *__daeditor3_global_instance; }
  static inline void set(IDaEditor3Engine *eng) { __daeditor3_global_instance = eng; }

protected:
  int daEditor3InterfaceVer = -1;

private:
  static IDaEditor3Engine *__daeditor3_global_instance;

  StereoIndex stereoIndex = StereoIndex::Mono;
};


class IEditorService
{
public:
  virtual ~IEditorService() {}

  virtual const char *getServiceName() const = 0;
  virtual const char *getServiceFriendlyName() const = 0;

  // used by editor to set/get visibility flag
  virtual void setServiceVisible(bool vis) = 0;
  virtual bool getServiceVisible() const = 0;

  // render/acting interface
  virtual void actService(float dt) = 0;
  virtual void beforeRenderService() = 0;
  virtual void renderService() = 0;
  virtual void renderTransService() = 0;

  virtual void onBeforeReset3dDevice() {}
  virtual void clearServiceData() {}

  virtual bool catchEvent([[maybe_unused]] unsigned event_huid, [[maybe_unused]] void *userData) { return false; }

  // COM-like facilities
  virtual void *queryInterfacePtr(unsigned huid) = 0;
  template <class T>
  inline T *queryInterface()
  {
    return (T *)queryInterfacePtr(T::HUID);
  }
};


#define DAEDITOR3 IDaEditor3Engine::get()

#ifndef RETURN_INTERFACE
#define RETURN_INTERFACE(huid, T) \
  if ((unsigned)huid == T::HUID)  \
  return static_cast<T *>(this)
#endif
