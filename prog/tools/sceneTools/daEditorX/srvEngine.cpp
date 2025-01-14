// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityFilter.h>
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMgr.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetMsgPipe.h>
#include <assets/assetHlp.h>
#include <assets/assetExpCache.h>
#include <assetsGui/av_selObjDlg.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_outliner.h>
#include <libTools/util/strUtil.h>
#include <libTools/dagFileRW/textureNameResolver.h>
#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <libTools/shaderResBuilder/matSubst.h>
#include <oldEditor/de_interface.h>
#include <propPanel/constants.h>
#include <propPanel/control/panelWindow.h>
#include <sepGui/wndGlobal.h>
#include <coolConsole/coolConsole.h>
#include <workCycle/dag_workCycle.h>
#include <gameRes/dag_gameResSystem.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_texMgr.h>
#include <util/dag_texMetaData.h>
#include <util/dag_oaHashNameMap.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>

#include <sepGui/wndPublic.h>
#include "de_batch_log.h"

#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <winGuiWrapper/wgw_input.h>

namespace environment
{
bool on_asset_changed(const DagorAsset &asset);
}

extern BatchLogCB *bl_callback;
extern DataBlock de3scannedResBlk;
extern bool dabuild_usage_allowed;
extern bool minimize_dabuild_usage;
extern bool src_assets_scan_allowed;

extern void mount_efx_assets(DagorAssetMgr &aMgr, const char *fx_blk_fname);

class GenServicePlugin : public IGenEditorPlugin
{
public:
  GenServicePlugin(IEditorService *_srv) : srv(_srv) {}
  virtual ~GenServicePlugin() {}

  // internal name (C idenitifier name restrictons); used save/load data and logging
  virtual const char *getInternalName() const { return srv->getServiceName(); }
  // user-friendly plugin name to be displayed in menu
  virtual const char *getMenuCommandName() const { return srv->getServiceFriendlyName(); }

  // returns desired order number; by default plugins are sorted by this number (increasing order)
  virtual int getRenderOrder() const { return 0; }
  // returns desired order number for binary data build process;
  // data is build and saved to dump in increasion order
  virtual int getBuildOrder() const { return 10; }

  // non-pure virtual functions
  //  help link in CHM file
  virtual const char *getHelpUrl() const { return NULL; }
  // show this plugin in menu
  virtual bool showInMenu() const { return false; }
  // show this plugin in Tab Bar
  virtual bool showInTabs() const { return false; }
  // show in menu "Select all" command for this plugin
  virtual bool showSelectAll() const { return false; }


  // notification about register/unregister
  virtual void registered() {}
  virtual void unregistered()
  {
    DAEDITOR3.unregisterService(srv);
    del_it(srv);
  }

  // called before enter in main loop when all plugins loaded and initialized
  virtual void beforeMainLoop() {}

  // called when user requests switch to this plugin; returns true on success
  virtual bool begin(int toolbar_id, unsigned menu_id) { return false; }
  // called when user requests switch from this plugin; returns true on success
  virtual bool end() { return false; }
  // called by editor AFTER begin() returns true; can return NULL when no event handling is needed
  virtual IGenEventHandler *getEventHandler() { return NULL; }

  // used by editor to set/get visibility flag
  virtual void setVisible(bool vis) { srv->setServiceVisible(vis); }
  virtual bool getVisible() const { return srv->getServiceVisible(); }

  virtual bool getSelectionBox(BBox3 &box) const { return false; }
  virtual bool getStatusBarPos(Point3 &pos) const { return false; }

  // clears all objects contained in this plugin (on NEW or before LOAD command)
  virtual void clearObjects() { srv->clearServiceData(); }
  // notify plugin about new project creation
  virtual void onNewProject() {}
  // saves all objects contained in this plugin to data block
  // and/or to base_path folder (with trailing slash)
  virtual void saveObjects(DataBlock &blk, DataBlock &l_data, const char *base_path) {}
  // loads all objects for this plugin from data block
  // and/or from base_path folder (with trailing slash)
  virtual void loadObjects(const DataBlock &blk, const DataBlock &l_data, const char *base_path) {}
  virtual bool acceptSaveLoad() const { return false; }

  // selects all objects
  virtual void selectAll() {}
  virtual void deselectAll() {}

  // render/acting interface
  virtual void actObjects(float dt) { srv->actService(dt); }
  virtual void beforeRenderObjects(IGenViewportWnd *vp)
  {
    if (vp)
      srv->beforeRenderService();
  }
  virtual void renderObjects() { srv->renderService(); }
  virtual void renderTransObjects() { srv->renderTransService(); }

  virtual void onBeforeReset3dDevice() { srv->onBeforeReset3dDevice(); }

  // COM-like facilities
  virtual void *queryInterfacePtr(unsigned huid) { return srv->queryInterfacePtr(huid); }
  // return true if stop catching
  virtual bool catchEvent(unsigned ev_huid, void *data) { return srv->catchEvent(ev_huid, data); }

  virtual bool onPluginMenuClick(unsigned id) { return false; }

public:
  IEditorService *srv;
};


static Tab<GenServicePlugin *> srvPlugins(inimem);
static Tab<IEditorService *> services(inimem);
static Tab<IObjEntityMgr *> entityMgrs(inimem);
static Tab<IObjEntityMgr *> entityClsMap(inimem);
static IObjEntityMgr *invEntMgr = NULL;
static Tab<int> genObjTypes(inimem);
static void *ppHwnd = NULL;


class DaEditor3Updater : public IDagorAssetChangeNotify
{
public:
  virtual void onAssetRemoved(int asset_name_id, int asset_type) {}

  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
  {
    bool changed = reload_changed_texture_asset(asset);
    changed |= environment::on_asset_changed(asset);

    if (changed)
    {
      EDITORCORE->updateViewports();
      EDITORCORE->invalidateViewportCache();
    }
  }
};

static DaEditor3Updater updater;

struct FatalHandlerCtx
{
  bool (*handler)(const char *msg, const char *call_stack, const char *file, int line);
  bool status, quiet;
};
static Tab<FatalHandlerCtx> fhCtx(inimem);
static bool fatalStatus = false, fatalQuiet = false;
static bool de3_gen_fatal_handler(const char *msg, const char *call_stack, const char *file, int line)
{
  fatalStatus = true;
  if (!fatalQuiet)
  {
    DAEDITOR3.conError("FATAL: %s,%d:\n    %s", file, line, msg);
    DAEDITOR3.conShow(true);
  }
  return false;
}

class DaEditor3Engine : public IDaEditor3Engine, public NullMsgPipe, public ITextureNameResolver
{
public:
  DaEditor3Engine() : m_FilterTypeId(-1)
  {
    daEditor3InterfaceVer = DAEDITOR3_VERSION;
    assetDlg = 0;
  }
  ~DaEditor3Engine() { resetInterface(); }

  Tab<String> objFilterList;

  virtual const char *getBuildString() { return "1.0"; }
  virtual bool registerService(IEditorService *srv)
  {
    if (srv->getServiceFriendlyName())
    {
      debug("register service: %s", srv->getServiceName());
      srvPlugins.push_back(new GenServicePlugin(srv));
      if (!DAGORED2->registerPlugin(srvPlugins.back()))
      {
        delete srvPlugins.back();
        srvPlugins.pop_back();
        delete srv;
        debug("-- register service failed");
        return false;
      }
    }
    else
    {
      debug("register hidden service: %s", srv->getServiceName());
      services.push_back(srv);
    }

    IObjEntityMgr *oemgr = srv->queryInterface<IObjEntityMgr>();
    if (oemgr)
      registerEntityMgr(oemgr);

    return true;
  }
  virtual bool unregisterService(IEditorService *srv)
  {
    debug("unregister service: %s", srv->getServiceName());
    for (int i = srvPlugins.size() - 1; i >= 0; i--)
      if (srvPlugins[i]->srv == srv)
      {
        IObjEntityMgr *oemgr = srv->queryInterface<IObjEntityMgr>();
        if (oemgr)
          unregisterEntityMgr(oemgr);

        erase_items(srvPlugins, i, 1);
        return true;
      }
    for (int i = services.size() - 1; i >= 0; i--)
      if (services[i] == srv)
      {
        erase_items(services, i, 1);
        return true;
      }
    return false;
  }

  virtual bool registerEntityMgr(IObjEntityMgr *oemgr)
  {
    for (int i = entityMgrs.size() - 1; i >= 0; i--)
      if (entityMgrs[i] == oemgr)
        return false;

    entityMgrs.push_back(oemgr);
    if (!invEntMgr && oemgr->canSupportEntityClass(-1))
      invEntMgr = oemgr;
    return true;
  }
  virtual bool unregisterEntityMgr(IObjEntityMgr *oemgr)
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

  virtual int getAssetTypeId(const char *entity_name) const { return assetMgr.getAssetTypeId(entity_name); }
  virtual const char *getAssetTypeName(int cls) const { return assetMgr.getAssetTypeName(cls); }
  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent) { return IObjEntity::create(asset, virtual_ent); }
  virtual IObjEntity *createInvalidEntity(bool virtual_ent)
  {
    const DagorAsset *null = NULL;
    return invEntMgr ? invEntMgr->createEntity(*null, virtual_ent) : NULL;
  }
  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    if (origin->getAssetTypeId() == -1)
      return invEntMgr ? invEntMgr->cloneEntity(origin) : NULL;

    return IObjEntity::clone(origin);
  }
  virtual int registerEntitySubTypeId(const char *subtype_str) { return IObjEntity::registerSubTypeId(subtype_str); }
  virtual unsigned getEntitySubTypeMask(int mask_type) { return IObjEntityFilter::getSubTypeMask(mask_type); }
  virtual void setEntitySubTypeMask(int mask_type, unsigned value) { IObjEntityFilter::setSubTypeMask(mask_type, value); }
  virtual uint64_t getEntityLayerHiddenMask() { return IObjEntityFilter::getLayerHiddenMask(); }
  virtual void setEntityLayerHiddenMask(uint64_t value) { IObjEntityFilter::setLayerHiddenMask(value); }

  virtual dag::ConstSpan<int> getGenObjAssetTypes() const { return genObjTypes; }

  virtual DagorAsset *getAssetByName(const char *_name, int asset_type)
  {
    if (!_name || !_name[0])
      return NULL;

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

  virtual DagorAsset *getAssetByName(const char *_name, dag::ConstSpan<int> asset_types)
  {
    if (!_name || !_name[0])
      return NULL;

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

      if (asset_types.data() == genObjTypes.data())
        return assetMgr.findAsset(String::mk_sub_str(name, type), asset_type);
      conError("not-allowed type of asset: %s", name.str());
      return NULL;
    }
    return assetMgr.findAsset(name, asset_types);
  }

  virtual const DataBlock *getAssetProps(const DagorAsset &asset) override { return &asset.props; }

  virtual String getAssetTargetFilePath(const DagorAsset &asset) override { return asset.getTargetFilePath(); }

  virtual const char *getAssetParentFolderName(const DagorAsset &asset) override
  {
    const int folderIndex = asset.getFolderIndex();
    const DagorAssetFolder &folder = asset.getMgr().getFolder(folderIndex);
    return folder.folderName;
  }

  virtual const char *resolveTexAsset(const char *tex_asset_name)
  {
    static String tmp;
    if (resolveTextureName(tex_asset_name, tmp))
      return tmp;
    return NULL;
  }

  void setupGenObjTypes(const DataBlock *blk)
  {
    clear_and_shrink(objFilterList);
    if (!blk)
    {
      const char *genobj_asset_type[] = {"prefab", "rendInst", "fx", "physObj", "composit", "efx"};
      for (int i = 0; i < sizeof(genobj_asset_type) / sizeof(genobj_asset_type[0]); i++)
        objFilterList.push_back() = genobj_asset_type[i];
      return;
    }
    int nid = blk->getNameId("type");
    for (int i = 0; i < blk->paramCount(); i++)
      if (blk->getParamType(i) == DataBlock::TYPE_STRING && blk->getParamNameId(i) == nid)
        objFilterList.push_back() = blk->getStr(i);
  }

  virtual bool initAssetBase(const char *app_dir)
  {
    String fname(260, "%sapplication.blk", app_dir);
    DataBlock appblk;

    assetMgr.clear();
    assetMgr.setMsgPipe(this);

    if (!appblk.load(fname))
    {
      debug("cannot load %s", fname.str());
      return false;
    }
    appblk.setStr("appDir", app_dir);
    ::dagor_set_game_act_rate(appblk.getInt("work_cycle_act_rate", 100));
    DataBlock::setRootIncludeResolver(app_dir);

    // load asset base
    const DataBlock &blk = *appblk.getBlockByNameEx("assets");
    int base_nid = blk.getNameId("base");

    CoolConsole &con = DAGORED2->getConsole();
    con.showConsole();
    con.startLog();
    con.startProgress();

    assetMgr.setupAllowedTypes(*blk.getBlockByNameEx("types"), blk.getBlockByName("export"));
    for (int i = 0; src_assets_scan_allowed && i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == base_nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
      {
        fname.printf(260, "%s%s", app_dir, blk.getStr(i));
        assetMgr.loadAssetsBase(fname, "global");
      }
    if (!minimize_dabuild_usage) // prefer real texture assets to gameres loaded from *.dxp.bin
    {
      dag::ConstSpan<DagorAsset *> assets = assetMgr.getAssets();
      for (int i = 0; i < assets.size(); i++)
        if (assets[i]->getType() == assetMgr.getTexAssetTypeId())
        {
          TEXTUREID tid = get_managed_texture_id(String(0, "%s*", assets[i]->getName()));
          if (tid != BAD_TEXTUREID)
          {
            DAEDITOR3.conWarning("using texture %s (tid=%d) from asset, not from *.DxP.bin", assets[i]->getName(), tid);
            evict_managed_tex_id(tid);
          }
        }
    }

    const DataBlock *skipTypes = blk.getBlockByName("ignorePrebuiltAssetTypes");
    if (!skipTypes)
      skipTypes = blk.getBlockByNameEx("export")->getBlockByNameEx("types");
    if (blk.getStr("gameRes", NULL) || blk.getStr("ddsxPacks", NULL))
      assetMgr.mountBuiltGameResEx(de3scannedResBlk, *skipTypes);

    if (blk.getStr("fmodEvents", NULL))
      assetMgr.mountFmodEvents(blk.getStr("fmodEvents", NULL));

    if (const char *efx_fname = appblk.getBlockByNameEx("assets")->getStr("efxBlk", NULL))
      mount_efx_assets(assetMgr, String(0, "%s/%s", app_dir, efx_fname));

    if (0)
    {
      dag::ConstSpan<DagorAsset *> assets = assetMgr.getAssets();
      debug("%d assets", assets.size());

      for (int i = 0; i < assets.size(); i++)
      {
        const DagorAsset &a = *assets[i];
        debug("  [%d] <%s> name=%s nspace=%s(%d) isVirtual=%d folder=%d srcPath=%s", i, assetMgr.getAssetTypeName(a.getType()),
          a.getName(), a.getNameSpace(), a.isGloballyUnique(), a.isVirtual(), a.getFolderIndex(), a.getSrcFilePath());
      }
    }

    set_global_tex_name_resolver(this);
    assetMgr.enableChangesTracker(true);
    assetMgr.subscribeUpdateNotify(&updater, -1, assetMgr.getTexAssetTypeId());

    // prepare genObjTypes
    setupGenObjTypes(appblk.getBlockByName("genObjTypes"));
    genObjTypes.clear();
    for (int i = 0; i < objFilterList.size(); i++)
    {
      int atype = getAssetTypeId(objFilterList[i]);
      if (atype != -1)
        genObjTypes.push_back(atype);
    }

    // add texture assets to texmgr
    if (get_max_managed_texture_id())
      debug("tex/res registered before AssetBase: %d", get_max_managed_texture_id().index());
    if (::get_gameres_sys_ver() == 2)
    {
      int atype = assetMgr.getTexAssetTypeId();
      dag::ConstSpan<DagorAsset *> assets = assetMgr.getAssets();
      int hqtex_ns = assetMgr.getAssetNameSpaceId("texHQ");
      int tqtex_ns = assetMgr.getAssetNameSpaceId("texTQ");
      for (int i = 0; i < assets.size(); i++)
        if (assets[i]->getType() == atype && assets[i]->getNameSpaceId() != hqtex_ns && assets[i]->getNameSpaceId() != tqtex_ns &&
            !strstr(assets[i]->getName(), "$uhq")) // we skip UHQ here since we don't need separate TID for them in editor
        {
          TextureMetaData tmd;
          tmd.read(assets[i]->props, "PC");
          SimpleString nm(tmd.encode(String(128, "%s*", assets[i]->getName())));
          if (get_managed_texture_id(nm) == BAD_TEXTUREID)
            add_managed_texture(nm);
        }
      if (get_max_managed_texture_id())
        debug("tex/res registered with   AssetBase: %d", get_max_managed_texture_id().index());
    }
    assetlocalprops::init(app_dir, "develop/.asset-local");
    AssetExportCache::createSharedData(assetlocalprops::makePath("assets-hash.bin"));

    if (dabuild_usage_allowed)
      dabuild_usage_allowed = appblk.getBool("dagored_use_dabuild", true);
    if (dabuild_usage_allowed)
    {
      if (!minimize_dabuild_usage)
      {
        dabuildcache::init(sgg::get_exe_path_full(), &getCon());
        dabuildcache::bind_with_mgr(assetMgr, appblk, app_dir);
      }
      if (texconvcache::init(assetMgr, appblk, sgg::get_exe_path_full(), false, true))
        conNote("texture conversion cache inited");
      if (assetrefs::load_plugins(assetMgr, appblk, sgg::get_exe_path_full(), !minimize_dabuild_usage))
        conNote("asset refs plugins inited");
    }
    const DataBlock &projDefBlk = *appblk.getBlockByNameEx("projectDefaults");
    if (bool zstd = projDefBlk.getBool("preferZSTD", false))
    {
      ShaderMeshData::preferZstdPacking = zstd;
      ShaderMeshData::zstdMaxWindowLog = projDefBlk.getInt("zstdMaxWindowLog", 0);
      ShaderMeshData::zstdCompressionLevel = projDefBlk.getInt("zstdCompressionLevel", 18);
      debug("ShaderMesh prefers ZSTD (compressionLev=%d %s)", ShaderMeshData::zstdCompressionLevel,
        ShaderMeshData::zstdMaxWindowLog ? String(0, "maxWindow=%u", 1 << ShaderMeshData::zstdMaxWindowLog).c_str() : "defaultWindow");
    }
    if (bool oodle = projDefBlk.getBool("allowOODLE", false))
    {
      ShaderMeshData::allowOodlePacking = oodle;
      debug("ShaderMesh allows OODLE");
    }
    if (bool zlib = projDefBlk.getBool("preferZLIB", false))
    {
      ShaderMeshData::forceZlibPacking = zlib;
      if (!ShaderMeshData::preferZstdPacking)
        debug("ShaderMesh prefers ZLIB");
    }

    const DataBlock &b = *appblk.getBlockByNameEx("tex_shader_globvar");
    for (int i = 0; i < b.paramCount(); i++)
      if (b.getParamType(i) == DataBlock::TYPE_STRING)
      {
        int gvid = get_shader_glob_var_id(b.getParamName(i), true);
        if (gvid < 0)
          conWarning("cannot set tex <%s> to var <%s> - shader globvar is missing", b.getStr(i), b.getParamName(i));
        else
        {
          TEXTUREID tid = get_managed_texture_id(b.getStr(i));
          if (tid == BAD_TEXTUREID)
            conWarning("cannot set tex <%s> to var <%s> - texture is missing", b.getStr(i), b.getParamName(i));
          else
          {
            ShaderGlobal::set_texture_fast(gvid, tid);
            conNote("set tex <%s> to globvar <%s>", b.getStr(i), b.getParamName(i));
          }
        }
      }

    if (const DataBlock *remap =
          appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("prefab")->getBlockByName("remapShaders"))
      static_geom_mat_subst.setupMatSubst(*remap);
    static_geom_mat_subst.setMatProcessor(
      new GenericTexSubstProcessMaterialData(nullptr, DataBlock::emptyBlock, &assetMgr, &DAGORED2->getConsole()));
    return true;
  }


  //! simple console messages output
  virtual ILogWriter &getCon() { return DAGORED2->getConsole(); }
  virtual void conErrorV(const char *fmt, const DagorSafeArg *arg, int anum) { conMessageV(ILogWriter::ERROR, fmt, arg, anum); }
  virtual void conWarningV(const char *fmt, const DagorSafeArg *arg, int anum) { conMessageV(ILogWriter::WARNING, fmt, arg, anum); }
  virtual void conNoteV(const char *fmt, const DagorSafeArg *arg, int anum) { conMessageV(ILogWriter::NOTE, fmt, arg, anum); }
  virtual void conRemarkV(const char *fmt, const DagorSafeArg *arg, int anum) { conMessageV(ILogWriter::REMARK, fmt, arg, anum); }
  virtual void conShow(bool show_console_wnd)
  {
    if (show_console_wnd)
      DAGORED2->getConsole().showConsole(true);
    else
      DAGORED2->getConsole().hideConsole();
  }

#define DSA_OVERLOADS_PARAM_DECL ILogWriter::MessageType type,
#define DSA_OVERLOADS_PARAM_PASS type,
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void conMessage, conMessageV);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
  void conMessageV(ILogWriter::MessageType type, const char *msg, const DagorSafeArg *arg, int anum)
  {
    static String s1, s2;
    String &s = is_main_thread() ? s1 : s2;

    s.vprintf(1024, msg, arg, anum);
    DAGORED2->getConsole().addMessage(type, s);

    if (is_main_thread() && bl_callback && (type == ILogWriter::WARNING || type == ILogWriter::ERROR))
      bl_callback->write(s.str());
  }

  virtual void onAssetMgrMessage(int msg_type, const char *msg, DagorAsset *asset, const char *asset_src_fpath)
  {
    G_ASSERT(msg_type >= NOTE && msg_type <= REMARK);
    ILogWriter::MessageType t = (ILogWriter::MessageType)msg_type;
    updateErrCount(msg_type);

    if (asset_src_fpath)
      conMessage(t, "%s (file %s)", msg, asset_src_fpath);
    else if (asset)
      conMessage(t, "%s (file %s)", msg, asset->getSrcFilePath());
    else
      conMessage(t, "%s", msg);
  }

  //! simple fatal handler interface
  virtual void setFatalHandler(bool quiet)
  {
    FatalHandlerCtx &ctx = fhCtx.push_back();
    ctx.handler = dgs_fatal_handler;
    ctx.status = fatalStatus;
    ctx.quiet = fatalQuiet;
    dgs_fatal_handler = de3_gen_fatal_handler;
    fatalStatus = false;
    fatalQuiet = quiet;
  }
  virtual bool getFatalStatus() { return fatalStatus; }
  virtual void resetFatalStatus() { fatalStatus = false; }
  virtual void popFatalHandler()
  {
    if (fhCtx.size() < 1)
      return;
    dgs_fatal_handler = fhCtx.back().handler;
    if (fatalStatus)
      conShow(true);
    fatalStatus = fhCtx.back().status | fatalStatus;
    fatalQuiet = fhCtx.back().quiet;
    fhCtx.pop_back();
  }

  // asset GUI
  virtual const char *selectAsset(const char *asset, const char *caption, dag::ConstSpan<int> types, const char *filter_str,
    bool open_all_grp)
  {
    static const int DLG_W = 320;
    static String buf;
    int _x = 0, _y = 0, _w = 0, _h = 600;
    IWndManager *_manager = DAGORED2->getWndManager();

    IGenViewportWnd *viewport = DAGORED2->getCurrentViewport();
    int vp_w = 0, vp_h = 0;
    if (viewport)
      viewport->getViewportSize(vp_w, vp_h);
    _w = (vp_w > 1600) ? DLG_W * 7 / 4 : DLG_W;
    _h = vp_h;

    unsigned pp_w = 0, pp_h = 0;
    if (_manager->getWindowPosSize(ppHwnd, _x, _y, pp_w, pp_h))
    {
      _manager->clientToScreen(_x, _y);
      _x = ((int)(_x - _w - 5) < 0) ? _x + pp_w + 5 : _x - _w - 5;
      _h = pp_h;
    }
    else if (viewport)
    {
      viewport->clientToScreen(_x, _y);
      _x += vp_w - _w - 5;
    }
    SelectAssetDlg dlg(_manager->getMainWindow(), &assetMgr, caption, "Select asset", "Reset asset", types, _x, _y, _w, _h);

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

    Tab<bool> tree_exps(midmem);
    assetTreeStateCache.getState(tree_exps, types);
    dlg.setTreeNodesExpand(tree_exps);

    int ret = dlg.showDialog();

    dlg.getTreeNodesExpand(tree_exps);
    assetTreeStateCache.setState(tree_exps, types);

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

  virtual void showAssetWindow(bool show, const char *caption, IAssetBaseViewClient *cli, dag::ConstSpan<int> types)
  {
    static const int DLG_W = 250;
    static const int DLG_H = 450;
    Tab<bool> tree_exps(midmem);
    static Tab<int> last_filter(midmem);

    if (assetDlg)
    {
      if (m_FilterTypeId != -1)
      {
        if (DAEDITOR3.getAssetTypeId("spline") == m_FilterTypeId)
          mSelectedSplineName = assetDlg->getSelObjName();
        else if (DAEDITOR3.getAssetTypeId("land") == m_FilterTypeId)
          mSelectedPolygonName = assetDlg->getSelObjName();
        else
          mSelectedEntityName = assetDlg->getSelObjName();
      }

      assetDlg->getTreeNodesExpand(tree_exps);
      assetTreeStateCache.setState(tree_exps, last_filter);
      del_it(assetDlg);
    }

    m_FilterTypeId = (types.size()) ? types[0] : -1;

    if (show)
    {
      IWndManager *_manager = DAGORED2->getWndManager();
      IGenViewportWnd *viewport = DAGORED2->getCurrentViewport();
      int _x = 0, _y = 0, _w = DLG_W, _h = DLG_H;

      assetDlg = new (uimem) SelectAssetDlg(_manager->getMainWindow(), &assetMgr, cli, caption, types, _x, _y, _w, _h);

      if (DAEDITOR3.getAssetTypeId("spline") == m_FilterTypeId && !mSelectedSplineName.empty())
        assetDlg->selectObj(mSelectedSplineName.str());
      else if (DAEDITOR3.getAssetTypeId("land") == m_FilterTypeId && !mSelectedPolygonName.empty())
        assetDlg->selectObj(mSelectedPolygonName.str());
      else if (!mSelectedEntityName.empty())
        assetDlg->selectObj(mSelectedEntityName.str());

      if (cli)
        cli->onAvSelectAsset(assetDlg->getSelectedAsset(), assetDlg->getSelObjName());

      last_filter = types;
      assetTreeStateCache.getState(tree_exps, types);
      assetDlg->setTreeNodesExpand(tree_exps);
      assetDlg->dockTo(DAGORED2->getLeftDockNodeId());
      assetDlg->show();
    }
  }

  virtual void addAssetToRecentlyUsed(const char *name) override
  {
    if (assetDlg)
      assetDlg->addAssetToRecentlyUsed(name);
  }

  virtual bool getTexAssetBuiltDDSx(DagorAsset &a, ddsx::Buffer &dest, unsigned target, const char *profile, ILogWriter *log)
  {
    return texconvcache::get_tex_asset_built_ddsx(a, dest, target, profile, log);
  }
  virtual bool getTexAssetBuiltDDSx(const char *a_name, const DataBlock &a_props, ddsx::Buffer &dest, unsigned target,
    const char *profile, ILogWriter *log)
  {
    class DagorAssetX : public DagorAsset
    {
    public:
      DagorAssetX(DagorAssetMgr &m, const char *name) : DagorAsset(m)
      {
        struct OpenDagorAssetMgr : public DagorAssetMgr
        {
          int addNameId(const char *nm) { return assetNames.addNameId(nm); }
        };

        nameId = static_cast<OpenDagorAssetMgr &>(m).addNameId(name);
        nspaceId = -1;
        virtualBlk = false;
        folderIdx = 0;
        fileNameId = -1;
        assetType = m.getTexAssetTypeId();
        globUnique = false;
      }
    };

    DagorAssetX a(assetMgr, a_name);
    a.props = a_props;
    return texconvcache::get_tex_asset_built_ddsx(a, dest, target, profile, log);
  }

  virtual void imguiBegin(const char *name, bool *open, unsigned window_flags) override
  {
    editor_core_imgui_begin(name, open, window_flags);
  }

  virtual void imguiBegin(PropPanel::PanelWindowPropertyControl &panel_window, bool *open, unsigned window_flags) override
  {
    panel_window.beforeImguiBegin();
    imguiBegin(panel_window.getStringCaption(), open, window_flags);
  }

  virtual void imguiEnd() override { ImGui::End(); }

  void resetInterface()
  {
    static_geom_mat_subst.setMatProcessor(nullptr);
    AssetExportCache::saveSharedData();
    dabuildcache::term();
    texconvcache::term();
    set_global_tex_name_resolver(this);
    del_it(assetDlg);
    assetMgr.enableChangesTracker(false);
    assetMgr.unsubscribeUpdateNotify(&updater);
    assetMgr.clear();
    assetrefs::unload_plugins(assetMgr);

    srvPlugins.clear();
    services.clear();
    entityMgrs.clear();
    entityClsMap.clear();
    invEntMgr = NULL;
  }
  void update() { assetMgr.trackChangesContinuous(-1); }

  virtual bool resolveTextureName(const char *src_name, String &out_str)
  {
    String tmp_stor;
    out_str = DagorAsset::fpath2asset(TextureMetaData::decodeFileName(src_name, &tmp_stor));
    if (out_str.length() > 0 && out_str[out_str.length() - 1] == '*')
      erase_items(out_str, out_str.length() - 1, 1);
    DagorAsset *tex_a = assetMgr.findAsset(out_str, assetMgr.getTexAssetTypeId());

    if (tex_a)
    {
      out_str.printf(64, "%s*", tex_a->getName());
      if (get_managed_texture_id(out_str) != BAD_TEXTUREID)
        return true;
      TextureMetaData tmd;
      tmd.read(tex_a->props, "PC");
      out_str = tmd.encode(tex_a->getTargetFilePath(), &tmp_stor);
    }
    else
    {
      if (!out_str.empty())
        DAEDITOR3.conError("tex %s for %s not found (src tex is %s)", out_str.str(), getCurrentDagName(), src_name);
      out_str.printf(260, "%s/../commonData/tex/missing.dds", sgg::get_exe_path_full());
    }
    return true;
  }

  virtual Outliner::OutlinerWindow *createOutlinerWindow() override { return new Outliner::OutlinerWindow(); }

protected:
  DagorAssetMgr assetMgr;
  SelectAssetDlg *assetDlg;
  SimpleString mSelectedEntityName;
  SimpleString mSelectedSplineName;
  SimpleString mSelectedPolygonName;
  int m_FilterTypeId;

  class AssetTreeStateCache
  {
  public:
    AssetTreeStateCache() : treeStates(midmem) {}
    ~AssetTreeStateCache() { clear_all_ptr_items(treeStates); }

    void getState(Tab<bool> &exps, dag::ConstSpan<int> types)
    {
      clear_and_shrink(exps);
      for (int i = 0; i < treeStates.size(); ++i)
        if (treeStates[i]->checkTypes(types))
        {
          exps = treeStates[i]->states;
          return;
        }
    }

    void setState(dag::ConstSpan<bool> exps, dag::ConstSpan<int> types)
    {
      for (int i = 0; i < treeStates.size(); ++i)
        if (treeStates[i]->checkTypes(types))
        {
          treeStates[i]->states = exps;
          return;
        }

      treeStates.push_back(new ATSInfo(exps, types));
    }

  private:
    struct ATSInfo
    {
      ATSInfo(dag::ConstSpan<bool> exps, dag::ConstSpan<int> types) : states(midmem), filter(midmem)
      {
        states = exps;
        filter = types;
      }

      bool checkTypes(dag::ConstSpan<int> types)
      {
        if (filter.size() != types.size())
          return false;
        for (int i = 0; i < filter.size(); ++i)
          if (filter[i] != types[i])
            return false;
        return true;
      }

      Tab<bool> states;
      Tab<int> filter;
    };

    Tab<ATSInfo *> treeStates;
  } assetTreeStateCache;
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
static bool showInvalidAssets = false;
static uint64_t entLayerHiddenMask = (1ull << 63); // mark layer=63 as hidden

int IObjEntity::registerSubTypeId(const char *subtype_str)
{
  if (entSubType.nameCount() < 32)
    return entSubType.addNameId(subtype_str);
  return entSubType.getNameId(subtype_str);
}
void IObjEntityFilter::setSubTypeMask(int mask_type, unsigned mask) { entSubTypeMask[mask_type] = mask; }
unsigned IObjEntityFilter::getSubTypeMask(int mask_type) { return entSubTypeMask[mask_type]; }
void IObjEntityFilter::setLayerHiddenMask(uint64_t lhmask) { entLayerHiddenMask = lhmask | (1ull << 63); }
uint64_t IObjEntityFilter::getLayerHiddenMask() { return entLayerHiddenMask; }
void IObjEntityFilter::setShowInvalidAsset(bool show) { showInvalidAssets = show; }
bool IObjEntityFilter::getShowInvalidAsset() { return showInvalidAssets; }

void terminate_interface_de3()
{
  engine_impl.resetInterface();
  IDaEditor3Engine::set(NULL);
}
void regular_update_interface_de3() { engine_impl.update(); }
void daeditor3_set_plugin_prop_panel(void *h) { ppHwnd = h; }

void services_act(float dt)
{
  for (int i = 0; i < services.size(); i++)
    services[i]->actService(dt);
}
void services_before_render()
{
  for (int i = 0; i < services.size(); i++)
    services[i]->beforeRenderService();
}
void services_render()
{
  for (int i = 0; i < services.size(); i++)
    services[i]->renderService();
}
void services_render_trans()
{
  for (int i = 0; i < services.size(); i++)
    services[i]->renderTransService();
}

bool services_catch_event(unsigned event_huid, void *user_data)
{
  for (IEditorService *s : services)
    if (s->catchEvent(event_huid, user_data))
      return true;
  return false;
}
