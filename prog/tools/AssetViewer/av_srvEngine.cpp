// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "av_appwnd.h"

#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityFilter.h>
#include <de3_baseInterfaces.h>
#include <de3_skiesService.h>
#include <de3_dynRenderService.h>
#include <de3_lightService.h>
#include <de3_skiesService.h>
#include <de3_editorEvents.h>
#include <de3_splineGenSrv.h>
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMgr.h>
#include <assets/assetMsgPipe.h>
#include <assets/texAssetBuilderTextureFactory.h>
#include <assetsGui/av_selObjDlg.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_interface.h>
#include <libTools/util/strUtil.h>
#include <shaders/dag_shaders.h>
#include <perfMon/dag_graphStat.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_texPackMgr2.h>
#include <gui/dag_stdGuiRender.h>
#include <util/dag_oaHashNameMap.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <propPanel/constants.h>
#include <util/dag_fastIntList.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <regExp/regExp.h>
#include <imgui/imgui.h>

static Tab<IEditorService *> srvPlugins(inimem);
static Tab<IObjEntityMgr *> entityMgrs(inimem);
static Tab<IObjEntityMgr *> entityClsMap(inimem);
static Tab<int> genObjTypes(inimem);
static IObjEntityMgr *invEntMgr = NULL;
static bool genObjTypesInited = false;

struct FatalHandlerCtx
{
  bool (*handler)(const char *msg, const char *call_stack, const char *, int);
  bool status, quiet;
};

static Tab<FatalHandlerCtx> fhCtx(inimem);
static bool fatalStatus = false, fatalQuiet = false;


static bool de3_gen_fatal_handler(const char *msg, [[maybe_unused]] const char *call_stack, const char *file, int line)
{
  fatalStatus = true;
  if (!fatalQuiet)
  {
    DAEDITOR3.conError("FATAL: %s,%d:\n    %s", file, line, msg);
    EDITORCORE->getConsole().showConsole();
  }
  return false;
}

static debug_log_callback_t orig_debug_log = nullptr;
static RegExp *logerr_to_console_re_include = nullptr;
static RegExp *logerr_to_console_re_exclude = nullptr;
static int logerr_to_console_callback(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  static bool entrance_guard = false;
  if (is_main_thread() && lev_tag == LOGLEVEL_ERR && !entrance_guard)
  {
    entrance_guard = true;
    if (logerr_to_console_re_include || logerr_to_console_re_exclude)
    {
      String msg;
      msg.vprintf(0, fmt, (const DagorSafeArg *)arg, anum);
      if ((!logerr_to_console_re_include || logerr_to_console_re_include->test(msg)) &&
          (!logerr_to_console_re_exclude || !logerr_to_console_re_exclude->test(msg)))
      {
        DAEDITOR3.getCon().addMessageFmt(ILogWriter::ERROR, msg, nullptr, 0);
        EDITORCORE->getConsole().showConsole();
      }
    }
    else
    {
      DAEDITOR3.getCon().addMessageFmt(ILogWriter::ERROR, fmt, (const DagorSafeArg *)arg, anum);
      EDITORCORE->getConsole().showConsole();
    }
    entrance_guard = false;
    return 1;
  }
  return entrance_guard ? 0 : (orig_debug_log ? orig_debug_log(lev_tag, fmt, arg, anum, ctx_file, ctx_line) : 1);
}

class DaEditor3Engine : public IDaEditor3Engine
{
public:
  DaEditor3Engine() { daEditor3InterfaceVer = DAEDITOR3_VERSION; }
  ~DaEditor3Engine() {}

  const char *getBuildString() override { return "1.0"; }

  bool registerService(IEditorService *srv) override
  {
    if (!disabledSrvNames.nameCount())
    {
      DataBlock appblk(::get_app().getWorkspace().getAppPath());
      const DataBlock &b = *appblk.getBlockByNameEx("dagored_disabled_plugins");
      int nid = b.getNameId("disable");
      if (nid >= 0)
        for (int i = 0; i < b.paramCount(); i++)
          if (b.getParamNameId(i) == nid && b.getParamType(i) == b.TYPE_STRING)
            if (strncmp(b.getStr(i), "(srv)", 5) == 0)
              disabledSrvNames.addNameId(b.getStr(i));

      if (!disabledSrvNames.nameCount())
        disabledSrvNames.addNameId(".");
    }
    srvPlugins.push_back(srv);
    if (srv->getServiceFriendlyName() && disabledSrvNames.getNameId(srv->getServiceFriendlyName()) >= 0)
    {
      debug("-- service %s not loaded, disable in application.blk", srv->getServiceFriendlyName());
      srvPlugins.pop_back();
      delete srv;
      return false;
    }

    IObjEntityMgr *oemgr = srv->queryInterface<IObjEntityMgr>();
    if (oemgr)
      registerEntityMgr(oemgr);

    return true;
  }

  bool unregisterService(IEditorService *srv) override
  {
    for (int i = srvPlugins.size() - 1; i >= 0; i--)
      if (srvPlugins[i] == srv)
      {
        IObjEntityMgr *oemgr = srv->queryInterface<IObjEntityMgr>();
        if (oemgr)
          unregisterEntityMgr(oemgr);

        erase_items(srvPlugins, i, 1);
        return true;
      }
    return false;
  }

  IEditorService *findService(const char *internalName) const override
  {
    for (int i = 0; i < srvPlugins.size(); i++)
      if (strcmp(srvPlugins[i]->getServiceName(), internalName) == 0)
        return srvPlugins[i];
    return nullptr;
  }

  bool registerEntityMgr(IObjEntityMgr *oemgr) override
  {
    for (int i = entityMgrs.size() - 1; i >= 0; i--)
      if (entityMgrs[i] == oemgr)
        return false;

    if (!invEntMgr && oemgr->canSupportEntityClass(-1))
      invEntMgr = oemgr;
    entityMgrs.push_back(oemgr);
    return true;
  }

  bool unregisterEntityMgr(IObjEntityMgr *oemgr) override
  {
    for (int i = entityMgrs.size() - 1; i >= 0; i--)
      if (entityMgrs[i] == oemgr)
      {
        for (int j = entityClsMap.size() - 1; j >= 0; j--)
          if (entityClsMap[j] == oemgr)
            entityClsMap[j] = NULL;
        erase_items(entityMgrs, i, 1);
        if (invEntMgr == oemgr)
          invEntMgr = NULL;
        return true;
      }
    return false;
  }

  int getAssetTypeId(const char *entity_name) const override { return get_app().getAssetMgr().getAssetTypeId(entity_name); }

  const char *getAssetTypeName(int cls) const override { return get_app().getAssetMgr().getAssetTypeName(cls); }

  IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent) override { return IObjEntity::create(asset, virtual_ent); }

  IObjEntity *createInvalidEntity(bool virtual_ent) override
  {
    const DagorAsset *null = NULL;
    return invEntMgr ? invEntMgr->createEntity(*null, virtual_ent) : NULL;
  }

  IObjEntity *cloneEntity(IObjEntity *origin) override
  {
    if (origin->getAssetTypeId() == -1)
      return invEntMgr ? invEntMgr->cloneEntity(origin) : NULL;
    return IObjEntity::clone(origin);
  }

  int registerEntitySubTypeId(const char *subtype_str) override { return IObjEntity::registerSubTypeId(subtype_str); }

  unsigned getEntitySubTypeMask(int mask_type) override { return IObjEntityFilter::getSubTypeMask(mask_type); }

  void setEntitySubTypeMask(int mask_type, unsigned value) override { IObjEntityFilter::setSubTypeMask(mask_type, value); }
  uint64_t getEntityLayerHiddenMask() override { return 1ull << 63; }
  void setEntityLayerHiddenMask(uint64_t /*value*/) override {}

  dag::ConstSpan<int> getGenObjAssetTypes() const override
  {
    if (!genObjTypesInited)
    {
      // prepare genObjTypes
      const char *genobj_asset_type[] = {"prefab", "rendInst", "fx", "physObj", "composit", "dynModel", "animChar"};

      Tab<const char *> obj_types;
      obj_types = make_span_const(genobj_asset_type, sizeof(genobj_asset_type) / sizeof(genobj_asset_type[0]));

      DataBlock appblk(::get_app().getWorkspace().getAppPath());
      if (const DataBlock *b = appblk.getBlockByName("genObjTypes"))
      {
        int nid = b->getNameId("type");
        for (int i = 0; i < b->paramCount(); i++)
          if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
            obj_types.push_back(b->getStr(i));
      }

      genObjTypes.clear();
      for (int i = 0; i < obj_types.size(); i++)
      {
        int atype = getAssetTypeId(obj_types[i]);
        if (atype != -1 && find_value_idx(genObjTypes, atype) < 0)
          genObjTypes.push_back(atype);
      }
      genObjTypesInited = true;
    }

    return genObjTypes;
  }

  DagorAsset *getAssetByName(const char *_name, int asset_type) override
  {
    if (!_name || !_name[0])
      return NULL;
    const DagorAssetMgr &assetMgr = get_app().getAssetMgr();

    String name(dd_get_fname(_name));
    remove_trailing_string(name, ".res.blk");
    name = DagorAsset::fpath2asset(name);

    const char *type = strchr(name, ':');
    if (type)
    {
      asset_type = assetMgr.getAssetTypeId(type + 1);
      if (asset_type == -1)
      {
        conError("invalid type of asset: %s", name.str());
        return NULL;
      }
      return assetMgr.findAsset(String::mk_sub_str(name, type), asset_type);
    }
    else if (asset_type == -1)
      return assetMgr.findAsset(name);
    else
      return assetMgr.findAsset(name, asset_type);
  }

  DagorAsset *getAssetByName(const char *_name, dag::ConstSpan<int> asset_types) override
  {
    if (!_name || !_name[0])
      return NULL;

    const DagorAssetMgr &assetMgr = get_app().getAssetMgr();
    String name(dd_get_fname(_name));
    remove_trailing_string(name, ".res.blk");
    name = DagorAsset::fpath2asset(name);

    const char *type = strchr(name, ':');
    if (type)
    {
      int asset_type = assetMgr.getAssetTypeId(type + 1);
      if (asset_type == -1)
      {
        conError("invalid type of asset: %s", name.str());
        return NULL;
      }

      for (int i = 0; i < asset_types.size(); i++)
        if (asset_type == asset_types[i])
          return assetMgr.findAsset(String::mk_sub_str(name, type), asset_type);

      conError("not-allowed type of asset: %s", name.str());
      return NULL;
    }
    return assetMgr.findAsset(name, asset_types);
  }

  const DataBlock *getAssetProps(const DagorAsset &asset) override { return &asset.props; }

  String getAssetTargetFilePath(const DagorAsset &asset) override { return asset.getTargetFilePath(); }

  const char *getAssetParentFolderName(const DagorAsset &asset) override
  {
    const int folderIndex = asset.getFolderIndex();
    const DagorAssetFolder &folder = asset.getMgr().getFolder(folderIndex);
    return folder.folderName;
  }

  const char *resolveTexAsset(const char *tex_asset_name) override
  {
    static String tmp;
    if (get_app().resolveTextureName(tex_asset_name, tmp))
      return tmp;
    return NULL;
  }

  bool initAssetBase(const char *) override { return false; }


  //! simple console messages output
  ILogWriter &getCon() override { return EDITORCORE->getConsole(); }

  void conErrorV(const char *fmt, const DagorSafeArg *arg, int anum) override { conMessageV(ILogWriter::ERROR, fmt, arg, anum); }
  void conWarningV(const char *fmt, const DagorSafeArg *arg, int anum) override { conMessageV(ILogWriter::WARNING, fmt, arg, anum); }
  void conNoteV(const char *fmt, const DagorSafeArg *arg, int anum) override { conMessageV(ILogWriter::NOTE, fmt, arg, anum); }
  void conRemarkV(const char *fmt, const DagorSafeArg *arg, int anum) override { conMessageV(ILogWriter::REMARK, fmt, arg, anum); }
  void conShow(bool show_console_wnd) override
  {
    if (show_console_wnd)
      EDITORCORE->getConsole().showConsole(true);
    else
      EDITORCORE->getConsole().hideConsole();
  }

#define DSA_OVERLOADS_PARAM_DECL ILogWriter::MessageType type,
#define DSA_OVERLOADS_PARAM_PASS type,
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void conMessage, conMessageV);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
  void conMessageV(ILogWriter::MessageType type, const char *msg, const DagorSafeArg *arg, int anum)
  {
    static String s1;
    String s2;
    String &s = is_main_thread() ? s1 : s2;

    s.vprintf(1024, msg, arg, anum);
    EDITORCORE->getConsole().addMessage(type, s);
  }

  //! simple fatal handler interface
  void setFatalHandler(bool quiet) override
  {
    FatalHandlerCtx &ctx = fhCtx.push_back();
    ctx.handler = dgs_fatal_handler;
    ctx.status = fatalStatus;
    ctx.quiet = fatalQuiet;
    dgs_fatal_handler = de3_gen_fatal_handler;
    fatalStatus = false;
    fatalQuiet = quiet;
  }

  bool getFatalStatus() override { return fatalStatus; }
  void resetFatalStatus() override { fatalStatus = false; }

  void popFatalHandler() override
  {
    if (fhCtx.size() < 1)
      return;
    dgs_fatal_handler = fhCtx.back().handler;
    fatalStatus = fhCtx.back().status | fatalStatus;
    fatalQuiet = fhCtx.back().quiet;
    fhCtx.pop_back();
  }

  // asset GUI
  const char *selectAsset(const char *asset, const char *caption, dag::ConstSpan<int> types, const char *filter_str,
    bool open_all_grp) override
  {
    static String buf;
    IWndManager &mgr = *get_app().getWndManager();

    SelectAssetDlg dlg(mgr.getMainWindow(), const_cast<DagorAssetMgr *>(&get_app().getAssetMgr()), caption, "Select asset",
      "Reset asset", types);
    dlg.setManualModalSizingEnabled();

    dlg.selectObj(asset);
    if (open_all_grp)
    {
      Tab<bool> gr;
      dlg.getTreeNodesExpand(gr);
      mem_set_ff(gr);
      dlg.setTreeNodesExpand(gr);
    }
    if (filter_str)
      dlg.setFilterStr(filter_str);

    int ret = dlg.showDialog();

    if (ret == PropPanel::DIALOG_ID_CLOSE)
      return NULL;

    if (ret == PropPanel::DIALOG_ID_OK)
    {
      buf = dlg.getSelObjName();
      return buf;
    }

    // on reset asset, return empty string
    return "";
  }

  void showAssetWindow(bool, const char *, IAssetBaseViewClient *, dag::ConstSpan<int>) override {}
  void addAssetToRecentlyUsed(const char *) override {}
  bool getTexAssetBuiltDDSx(DagorAsset &, ddsx::Buffer &, unsigned, const char *, ILogWriter *) override { return false; }
  bool getTexAssetBuiltDDSx(const char *, const DataBlock &, ddsx::Buffer &, unsigned, const char *, ILogWriter *) override
  {
    return false;
  }

  void imguiBegin(const char *name, bool *open, unsigned window_flags) override { editor_core_imgui_begin(name, open, window_flags); }

  void imguiBegin(PropPanel::PanelWindowPropertyControl &panel_window, bool *open, unsigned window_flags) override
  {
    panel_window.beforeImguiBegin();
    imguiBegin(panel_window.getStringCaption(), open, window_flags);
  }

  void imguiEnd() override { ImGui::End(); }

  Outliner::OutlinerWindow *createOutlinerWindow() override
  {
    G_ASSERT(false);
    return nullptr;
  }

  FastNameMap disabledSrvNames;
};

static DaEditor3Engine engine_impl;
IDaEditor3Engine *IDaEditor3Engine::__daeditor3_global_instance = &engine_impl;


IObjEntity *IObjEntity::create(const DagorAsset &asset, bool virtual_ent)
{
  int entity_class = asset.getType();
  if (entity_class < entityClsMap.size() && entityClsMap[entity_class])
    return entityClsMap[entity_class]->createEntity(asset, virtual_ent);

  if (entity_class >= entityClsMap.size())
  {
    int from = entityClsMap.size();
    entityClsMap.resize(entity_class + 1);
    while (from <= entity_class)
      entityClsMap[from++] = NULL;
  }

  for (int i = 0; i < entityMgrs.size(); i++)
    if (entityMgrs[i]->canSupportEntityClass(entity_class))
    {
      entityClsMap[entity_class] = entityMgrs[i];
      return entityClsMap[entity_class]->createEntity(asset, virtual_ent);
    }
  return NULL;
}


IObjEntity *IObjEntity::clone(IObjEntity *origin)
{
  int entity_class = origin->getAssetTypeId();

  if (entity_class < 0)
    return NULL;
  if (entity_class < entityClsMap.size() && entityClsMap[entity_class])
  {
    IObjEntity *ent = entityClsMap[entity_class]->cloneEntity(origin);
    if (ent)
      ent->setSubtype(origin->getSubtype());
    return ent;
  }
  return NULL;
}


static OAHashNameMap<true> entSubType;
static unsigned entSubTypeMask[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
static bool showInvalidAssets = true;


int IObjEntity::registerSubTypeId(const char *subtype_str)
{
  if (entSubType.nameCount() < 32)
    return entSubType.addNameId(subtype_str);
  return entSubType.getNameId(subtype_str);
}


void IObjEntityFilter::setSubTypeMask(int mask_type, unsigned mask) { entSubTypeMask[mask_type] = mask; }


unsigned IObjEntityFilter::getSubTypeMask(int mask_type) { return entSubTypeMask[mask_type]; }
uint64_t IObjEntityFilter::getLayerHiddenMask() { return (1ull << 63); }


void IObjEntityFilter::setShowInvalidAsset(bool show) { showInvalidAssets = show; }
bool IObjEntityFilter::getShowInvalidAsset() { return showInvalidAssets; }


#include "av_plugin.h"
#include <shaders/dag_shaderMesh.h>
extern ISkiesService *av_skies_srv;
extern SimpleString av_skies_preset, av_skies_env, av_skies_wtype;
static bool use_skies = false;

class ServicesRenderPlugin : public IGenEditorPlugin, public ILightingChangeClient
{
  bool getVisible() const override { return true; }
  void *queryInterfacePtr(unsigned huid) override
  {
    if (huid == ILightingChangeClient::HUID)
      return static_cast<ILightingChangeClient *>(this);
    return IGenEditorPlugin::queryInterfacePtr(huid);
  }

  const char *getInternalName() const override { return "srvRender"; }

  void registered() override {}
  void unregistered() override {}

  bool begin(DagorAsset *) override { return false; }
  bool end() override { return true; }

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &) const override { return false; }

  void actObjects(float dt) override
  {
    use_skies = av_skies_srv && (!av_skies_preset.empty() || get_app().dngBasedSceneRenderUsed()) &&
                !EDITORCORE->queryEditorInterface<IDynRenderService>()->hasEnvironmentSnapshot();
    if (use_skies)
      av_skies_srv->updateSkies(dt);
    for (int i = 0; i < srvPlugins.size(); i++)
      srvPlugins[i]->actService(dt);
  }
  void beforeRenderObjects() override
  {
    static int water_level_gvid = get_shader_variable_id("water_level", true);
    ShaderGlobal::set_real(water_level_gvid, -3000);

    ddsx::tex_pack2_perform_delayed_data_loading();
    for (int i = 0; i < srvPlugins.size(); i++)
      srvPlugins[i]->beforeRenderService();
  }
  void renderObjects() override
  {
    for (int i = 0; i < srvPlugins.size(); i++)
      srvPlugins[i]->renderService();
  }
  void renderTransObjects() override
  {
    for (int i = 0; i < srvPlugins.size(); i++)
      srvPlugins[i]->renderTransService();
  }

  void renderGeometry(Stage stage) override
  {
    if (!get_app().canRenderEnvi() && (stage == STG_RENDER_ENVI || stage == STG_RENDER_CLOUDS))
      return;

    bool ortho = false;
    if (IGenViewportWnd *vp = EDITORCORE->getRenderViewport())
      ortho = vp->isOrthogonal();

    if (use_skies && stage == STG_BEFORE_RENDER)
      if (ISceneLightService *ltSrv = EDITORCORE->queryEditorInterface<ISceneLightService>())
        ltSrv->updateShaderVars();
    if (use_skies && !ortho)
    {
      if (stage == STG_BEFORE_RENDER)
        av_skies_srv->beforeRenderSky();
      else if (stage == STG_RENDER_ENVI)
        av_skies_srv->renderSky();
      else if (stage == STG_RENDER_CLOUDS)
        av_skies_srv->renderClouds();
    }
    else if (use_skies && ortho && stage == STG_BEFORE_RENDER)
      av_skies_srv->beforeRenderSkyOrtho();

    if (stage == STG_RENDER_ENVI && !use_skies)
      environment::renderEnvironment(ortho);

    for (int i = 0; i < srvPlugins.size(); i++)
      if (IRenderingService *srv = srvPlugins[i]->queryInterface<IRenderingService>())
        srv->renderGeometry(stage);

    if (ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>())
    {
      bool opaque = true;
      if (stage == STG_RENDER_STATIC_TRANS)
        opaque = false;
      else if (stage != STG_RENDER_STATIC_OPAQUE)
        return;

      FastIntList loft_layers;
      loft_layers.addInt(0);
      splSrv->gatherLoftLayers(loft_layers, true);

      d3d::settm(TM_WORLD, TMatrix::IDENT);
      TMatrix4 globtm;
      d3d::getglobtm(globtm);
      Frustum frustum;
      frustum.construct(globtm);

      for (int lli = 0; lli < loft_layers.size(); lli++)
        splSrv->renderLoftGeom(loft_layers.getList()[lli], opaque, frustum, -1);
    }
  }

  void renderUI() override
  {
    for (int i = 0; i < srvPlugins.size(); i++)
      if (IRenderingService *srv = srvPlugins[i]->queryInterface<IRenderingService>())
        srv->renderUI();
  }

  void onLightingChanged() override
  {
    for (int i = 0; i < srvPlugins.size(); i++)
      if (ILightingChangeClient *srv = srvPlugins[i]->queryInterface<ILightingChangeClient>())
        srv->onLightingChanged();
  }
  void onLightingSettingsChanged() override
  {
    for (int i = 0; i < srvPlugins.size(); i++)
      if (ILightingChangeClient *srv = srvPlugins[i]->queryInterface<ILightingChangeClient>())
        srv->onLightingSettingsChanged();
  }

  bool supportAssetType(const DagorAsset &) const override { return false; }

  void fillPropPanel(PropPanel::ContainerPropertyControl &) override {}
  void postFillPropPanel() override {}
};
static ServicesRenderPlugin *srvPlugin = NULL;

static struct DagorEdReset3DCallback : public IDrv3DResetCB
{
  void beforeReset([[maybe_unused]] bool full_reset) override
  {
    if (IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>())
      drSrv->beforeD3DReset(full_reset);
    for (int i = 0; i < srvPlugins.size(); i++)
      srvPlugins[i]->onBeforeReset3dDevice();
  }

  void afterReset(bool full_reset) override
  {
    extern void rebuild_shaders_stateblocks();

    DAEDITOR3.conWarning("GPU driver restarted...");
    rebuild_shaders_stateblocks();
    StdGuiRender::after_device_reset();

    if (IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>())
      drSrv->afterD3DReset(full_reset);
    if (ISkiesService *skiesSrv = EDITORCORE->queryEditorInterface<ISkiesService>())
      skiesSrv->afterD3DReset(full_reset);

    if (full_reset)
    {
      DAEDITOR3.conNote("reloading textures...");
      ddsx::reload_active_textures(0);
    }
    DAEDITOR3.conNote("notifying services...");
    for (int i = 0; i < srvPlugins.size(); i++)
      srvPlugins[i]->catchEvent(HUID_AfterD3DReset, NULL);
  }
} drv_3d_reset_cb;

void init_interface_de3()
{
  IEditorCoreEngine::get()->registerPlugin(srvPlugin = new ServicesRenderPlugin);
  ::set_3d_device_reset_callback(&drv_3d_reset_cb);

  DataBlock appblk(::get_app().getWorkspace().getAppPath());
  const char *incl_re_str = nullptr;
  const char *excl_re_str = nullptr;
  if (const DataBlock *b = appblk.getBlockByNameEx("logerr_to_con")->getBlockByName("AssetViewer"))
  {
    incl_re_str = b->getStr("include_re", nullptr);
    excl_re_str = b->getStr("exclude_re", nullptr);
    if (incl_re_str)
    {
      logerr_to_console_re_include = new RegExp;
      if (!logerr_to_console_re_include->compile(incl_re_str, ""))
      {
        logerr("bad regexp str for %s/%s/%s: '%s'", "logerr_to_con", "AssetViewer", "include_re", incl_re_str);
        del_it(logerr_to_console_re_include);
        incl_re_str = nullptr;
      }
    }
    if (excl_re_str)
    {
      logerr_to_console_re_exclude = new RegExp;
      if (!logerr_to_console_re_exclude->compile(excl_re_str, ""))
      {
        logerr("bad regexp str for %s/%s/%s: '%s'", "logerr_to_con", "AssetViewer", "exclude_re", excl_re_str);
        del_it(logerr_to_console_re_exclude);
        excl_re_str = nullptr;
      }
    }
  }
  if (!excl_re_str || *excl_re_str) // if exclude_re=="" then we filter out all errors, so callback may be not installed
    orig_debug_log = debug_set_log_callback(&logerr_to_console_callback);
  DAEDITOR3.conNote("installed logerr->CON reporter, filtering: include=%s  exclude=%s", incl_re_str ? incl_re_str : "<ANY>",
    excl_re_str ? (*excl_re_str ? excl_re_str : "<ALL>") : "<NONE>");
}

void terminate_interface_de3()
{
  debug_set_log_callback(orig_debug_log);
  del_it(logerr_to_console_re_include);
  del_it(logerr_to_console_re_exclude);
  debug("reset logerr->CON reporter");
  clear_all_ptr_items(srvPlugins);
  texconvcache::term_build_on_demand_tex_factory();
  IDaEditor3Engine::set(NULL);
  srvPlugin = NULL;
}

bool de3_spawn_event(unsigned event_huid, void *user_data)
{
  for (int i = 0; i < srvPlugins.size(); ++i)
    if (srvPlugins[i]->catchEvent(event_huid, user_data))
      return true;
  return false;
}
