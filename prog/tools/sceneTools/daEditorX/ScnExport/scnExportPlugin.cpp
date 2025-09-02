// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scnExportPlugin.h"

#include <oldEditor/de_util.h>
#include <oldEditor/exportToLightTool.h>
#include <oldEditor/de_workspace.h>

#include <EditorCore/ec_ObjectCreator.h>
#include <EditorCore/ec_colors.h>
#include <EditorCore/ec_wndPublic.h>

#include <libTools/dagFileRW/dagUtil.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>

#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>

#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/csgFaceRemover/geomContainerRemover.h>

#include <EditorCore/ec_IEditorCore.h>

#include <sceneBuilder/nsb_export_lt.h>
#include <sceneBuilder/nsb_LightmappedScene.h>
#include <sceneBuilder/nsb_LightingProvider.h>

#include <de3_entityFilter.h>
#include <de3_editorEvents.h>

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/menu.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <debug/dag_debug.h>
#include <oldEditor/de_cm.h>
#include <de3_huid.h>

using editorcore_extapi::dagTools;

enum
{
  PID_SPLIT_GRP = 600,
  PID_DO_COMBINE,
  PID_DO_SPLIT,
  PID_OPTIMIZE_FOR_CACHE,
  PID_TRIANGLES_CNT,
  PID_MIN_RAD,
  PID_MAX_SIZE,

  ID_PLUGIN_BASE,

  CM_BUILD_SCN = CM_PLUGIN_BASE,
};

extern IEditorService *create_built_scene_view_service();

ScnExportPlugin::ScnExportPlugin()
{
  bsSrv = create_built_scene_view_service();
  bsRendGeom = bsSrv ? bsSrv->queryInterface<IRenderingService>() : NULL;
  bsRendCube = bsSrv ? bsSrv->queryInterface<IRenderOnCubeTex>() : NULL;
}
ScnExportPlugin::~ScnExportPlugin()
{
  del_it(bsSrv);
  bsRendGeom = NULL;
  bsRendCube = NULL;
}

bool ScnExportPlugin::begin(int toolbar_id, unsigned menu_id)
{
  PropPanel::IMenu *mainMenu = DAGORED2->getMainMenu();
  mainMenu->addItem(menu_id, CM_BUILD_SCN, "Compile scene geom for export(PC)...");
  return true;
}

bool ScnExportPlugin::onPluginMenuClick(unsigned id)
{
  if (id == CM_BUILD_SCN)
  {
    PropPanel::DialogWindow *myDlg =
      DAGORED2->createDialog(hdpi::_pxScaled(300), hdpi::_pxScaled(560), "Compile scene geom for export(PC)");
    PropPanel::ContainerPropertyControl *myPanel = myDlg->getPanel();
    fillExportPanel(*myPanel);
    if (myDlg->showDialog() == PropPanel::DIALOG_ID_OK)
    {
      load_exp_shaders_for_target(_MAKE4C('PC'));
      if (!validateBuild(_MAKE4C('PC'), DAGORED2->getConsole(), myPanel))
        wingw::message_box(wingw::MBS_EXCL, "Compilation failed", "Failed to compile scene geom");
    }

    DAGORED2->invalidateViewportCache();
    DAGORED2->deleteDialog(myDlg);
    return true;
  }
  return false;
}

//==============================================================================
void *ScnExportPlugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IBinaryDataBuilder);
  if (bsRendGeom)
    RETURN_INTERFACE(huid, IRenderingService);
  if (bsRendCube)
    RETURN_INTERFACE(huid, IRenderOnCubeTex);
  return NULL;
}

bool ScnExportPlugin::buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *panel)
{
  String prj;
  DAGORED2->getProjectFolderPath(prj);

  if (check_built_scene_data_exist(prj + "/builtScene", cwr.getTarget()))
  {
    cwr.beginTaggedBlock(_MAKE4C('SCN'));
    store_built_scene_data(prj + "/builtScene", cwr, tn);
    cwr.align8();
    cwr.endBlock();
  }

  return true;
}


void ScnExportPlugin::fillExportPanel(PropPanel::ContainerPropertyControl &params)
{
  params.createStatic(0, "Scene export members:");

  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);

    if (check_geom_provider(plugin))
    {
      const char *plugName = plugin->getMenuCommandName();
      params.createCheckBox(ID_PLUGIN_BASE + i, plugName, disabledGamePlugins.getNameId(plugName) == -1);
    }
  }

  static bool falseBool = false;
  const int grPid = PID_SPLIT_GRP;

  PropPanel::ContainerPropertyControl *maxGroup = params.createGroup(grPid, "Split");
  G_ASSERT(maxGroup);

  maxGroup->createCheckBox(PID_DO_COMBINE, "Combine", splitter.combineObjects);

  maxGroup->createCheckBox(PID_DO_SPLIT, "Split", splitter.splitObjects);

  maxGroup->createCheckBox(PID_OPTIMIZE_FOR_CACHE, "Optimize for vertex cache", splitter.optimizeForVertexCache);

  maxGroup->createEditInt(PID_TRIANGLES_CNT, "Triangles per object:", splitter.trianglesPerObjectTargeted);

  maxGroup->createEditFloat(PID_MIN_RAD, "Min. split radius:", splitter.minRadiusToSplit);

  maxGroup->createEditFloat(PID_MAX_SIZE, "Max. combined size:", splitter.maxCombinedSize);

  params.setBool(grPid, true);
  params.setBoolValue(true);
}


bool ScnExportPlugin::validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *panel)
{
  disabledGamePlugins.reset();

  Tab<int> gamePlugs(tmpmem);

  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    if (!panel->getBool(ID_PLUGIN_BASE + i))
    {
      IGenEditorPlugin *plug = DAGORED2->getPlugin(i);

      if (plug)
        disabledGamePlugins.addNameId(plug->getMenuCommandName());
    }
    else
      gamePlugs.push_back(i);
  }

  splitter.combineObjects = panel->getBool(PID_DO_COMBINE);
  splitter.splitObjects = panel->getBool(PID_DO_SPLIT);
  splitter.optimizeForVertexCache = panel->getBool(PID_OPTIMIZE_FOR_CACHE);
  splitter.trianglesPerObjectTargeted = panel->getInt(PID_TRIANGLES_CNT);
  splitter.minRadiusToSplit = panel->getFloat(PID_MIN_RAD);
  splitter.maxCombinedSize = panel->getFloat(PID_MAX_SIZE);

  if (!gamePlugs.size())
    DAGORED2->getConsole().addMessage(ILogWriter::WARNING, "No one plugin selected to get geometry data.");

  return buildLmScene(gamePlugs, target);
}


bool ScnExportPlugin::buildLmScene(Tab<int> &plugs, unsigned target)
{
  String prj;
  DAGORED2->getProjectFolderPath(prj);

  const String lmsDat(DAGORED2->getPluginFilePath(this, "lms.dat"));
  const String lmDir = ::make_full_path(prj, "builtScene/");

  CoolConsole &con = DAGORED2->getConsole();
  con.startProgress();

  if (exportLms(lmsDat, con, plugs))
  {
    StaticSceneBuilder::LightmappedScene scene;

    if (scene.load(lmsDat, con))
    {
      if (!dd_dir_exist(lmDir))
      {
        ::dd_mkdir(lmDir);
        DAEDITOR3.conWarning("'%s' folder was created", ::make_full_path(prj, "builtScene").str());
      }

      StaticSceneBuilder::LightingProvider lighting;
      StaticSceneBuilder::StdTonemapper toneMapper;

      if (!build_and_save_scene1(scene, lighting, toneMapper, splitter, lmDir, StaticSceneBuilder::SCENE_FORMAT_LdrTgaDds, target, con,
            con, true))
      {
        con.endProgress();
        con.endLog();
        DAEDITOR3.conError("ScnExport: cannot build scene");
        return false;
      }
      con.endProgress();
      DAGORED2->spawnEvent(HUID_ReloadBuiltScene, NULL);
      return true;
    }
    else
      DAEDITOR3.conError("ScnExport: cannot load %s", lmsDat.str());
  }
  else
    DAEDITOR3.conError("ScnExport: cannot export scene to %s", lmsDat.str());
  con.endProgress();
  return false;
}

bool ScnExportPlugin::exportLms(const char *fname, CoolConsole &con, Tab<int> &plugs)
{
  debug(">> exporting LMS to %s", fname);

  DataBlock app_blk(DAGORED2->getWorkspace().getAppPath());
  const char *mgr_type = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("hmap")->getStr("type", NULL);
  bool removeLandMeshMat = (mgr_type && strcmp(mgr_type, "aces") == 0);

  int prev_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);

  IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT, 0);
  int i;
  for (i = 0; i < plugs.size(); ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(plugs[i]);
    if (plugin->queryInterface<IGatherStaticGeometry>())
      continue;

    IObjEntityFilter *filter = plugin->queryInterface<IObjEntityFilter>();

    if (filter && filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_EXPORT))
      filter->applyFiltering(IObjEntityFilter::STMASK_TYPE_EXPORT, true);
  }

  StaticGeometryContainer *cont = new (tmpmem) StaticGeometryContainer;

  for (i = 0; i < plugs.size(); ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(plugs[i]);

    IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();
    if (!geom)
      continue;

    geom->gatherStaticVisualGeometry(*cont);
  }

  if (removeLandMeshMat)
    for (int i = cont->nodes.size() - 1; i >= 0; --i)
    {
      bool landNode = false;
      for (int mi = 0; mi < cont->nodes[i]->mesh->mats.size(); ++mi)
      {
        if (cont->nodes[i]->mesh->mats[mi] && strstr(cont->nodes[i]->mesh->mats[mi]->className.str(), "land_mesh") &&
            !strstr(cont->nodes[i]->mesh->mats[mi]->className.str(), "land_mesh_clipmap"))
        {
          landNode = true;
          break;
        }
      }
      if (landNode)
        erase_items(cont->nodes, i, 1);
    }

  for (int ni = 0; ni < cont->nodes.size(); ++ni)
  {
    StaticGeometryNode *node = cont->nodes[ni];

    if (node && node->flags & StaticGeometryNode::FLG_AUTOMATIC_VISRANGE)
      node->visRange = -1;
  }

  bool removeInvisibleFacesCSG =
    app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("scnExport")->getBool("removeInsideFaces", true);
  if (removeInvisibleFacesCSG)
  {
    class RemoverNotificationCBConsole : public RemoverNotificationCB
    {
    public:
      void note(const char *s) override { DAEDITOR3.conNote(s); }
      void warning(const char *s) override { DAEDITOR3.conWarning(s); }
    };
    int time0 = dagTools->getTimeMsec();
    RemoverNotificationCBConsole notifyCb;
    remove_inside_faces(*cont, &notifyCb);
    DAEDITOR3.conNote("CSG face removal took %gsecs", (dagTools->getTimeMsec() - time0) / 1000.);
  }

  bool removeInvisibleFacesLand = false;
  if (mgr_type && strcmp(mgr_type, "aces") == 0)
    removeInvisibleFacesLand =
      app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("scnExport")->getBool("removeUndegroundFaces", true);
  if (removeInvisibleFacesLand)
  {
    // preprocessing
    static int lmeshObj = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("lmesh_obj");
    for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
    {
      IGenEditorPlugin *plug = DAGORED2->getPlugin(i);

      // int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
      if (/* (st_mask & lmeshObj) &&*/ plug && plug->queryInterfacePtr(HUID_IPostProcessGeometry))
        plug->queryInterface<IPostProcessGeometry>()->processGeometry(*cont);
    }
  }


  IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT, prev_mask);

  FullFileSaveCB cb(fname);
  if (!cb.fileHandle)
    return false;

  QuietProgressIndicator qpi;
  if (!StaticSceneBuilder::export_envi_to_LMS1(*cont, fname, qpi, con))
    return false;

  del_it(cont);
  return true;
}


bool ScnExportPlugin::addUsedTextures(ITextureNumerator &tn)
{
  String scenePath(DAGORED2->getPluginFilePath(this, "../builtScene"));
  ::add_built_scene_textures(scenePath, tn);
  return true;
}


void ScnExportPlugin::clearObjects()
{
  if (bsSrv)
    bsSrv->clearServiceData();
  DataBlock app_blk;
  if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
    DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());
  const DataBlock &scn_def = *app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("scnExport");

  disabledGamePlugins.reset();
  splitter.defaults();
  loadObjects(scn_def, DataBlock::emptyBlock, NULL);
}

void ScnExportPlugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path)
{
  DataBlock *disableBlk = blk.addNewBlock("disabled_geom_export");

  if (disableBlk)
    iterate_names(disabledGamePlugins, [&](int, const char *name) { disableBlk->addStr("name", name); });
  splitter.save(*blk.addBlock("splitter"));
}


//==============================================================================
void ScnExportPlugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
  const DataBlock *disableBlk = blk.getBlockByName("disabled_geom_export");

  if (disableBlk)
    for (int i = 0; i < disableBlk->paramCount(); ++i)
    {
      const char *name = disableBlk->getStr(i);
      if (name && *name)
        disabledGamePlugins.addNameId(name);
    }
  splitter.load(*blk.getBlockByNameEx("splitter"), false);
}


bool check_geom_provider(IGenEditorPlugin *plugin)
{
  if (!plugin)
    return false;

  IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();
  IObjEntityFilter *filter = NULL;

  if (!geom)
  {
    filter = plugin->queryInterface<IObjEntityFilter>();
    if (filter && !filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_EXPORT))
      filter = NULL;
  }

  return geom || filter;
}
