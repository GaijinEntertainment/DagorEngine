// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_workspace.h>
#include <de3_interface.h>
#include <EditorCore/ec_interface_ex.h>
#include <ecs/core/entityManager.h>
#include <ecs/io/blk.h>
#include <ecs/renderEvents.h>
#include <ecs/delayedAct/actInThread.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <debug/dag_debug3d.h>

static Tab<const char *> ecs_tools_get_global_tags_context()
{
  Tab<const char *> globalTags(framemem_ptr());
  globalTags.push_back("render");
  globalTags.push_back("tools");
  return globalTags;
}

class EcsAdapter : public IEditorService, public IRenderingService
{
public:
  EcsAdapter(const char *entities_path, const char *scene_path, const DataBlock &app_blk)
  {
    ecs::TemplateRefs trefs(*g_entity_mgr);
    ecs::TemplateRefs::NameMap ignoreTypeNm, ignoreCompNm;
    if (const DataBlock *ecs_blk = app_blk.getBlockByName("ecs"))
    {
      bool ignore_all_bad_types = ecs_blk->getBool("ignoreAllBadComponentTypes", false);
      bool ignore_all_bad_comps = ecs_blk->getBool("ignoreAllBadComponents", false);
      if (const DataBlock *ignore_types_blk = ignore_all_bad_types ? nullptr : ecs_blk->getBlockByName("ignoreComponentTypes"))
        dblk::iterate_params_by_name_and_type(*ignore_types_blk, "type", DataBlock::TYPE_STRING,
          [ignore_types_blk, &ignoreTypeNm](int idx) { ignoreTypeNm.addNameId(ignore_types_blk->getStr(idx)); });
      if (const DataBlock *ignore_comps_blk = ignore_all_bad_comps ? nullptr : ecs_blk->getBlockByName("ignoreComponents"))
        dblk::iterate_params_by_name_and_type(*ignore_comps_blk, "comp", DataBlock::TYPE_STRING,
          [ignore_comps_blk, &ignoreCompNm](int idx) { ignoreCompNm.addNameId(ignore_comps_blk->getStr(idx)); });
      trefs.setIgnoreNames((ignore_all_bad_types || ignoreTypeNm.nameCount()) ? &ignoreTypeNm : nullptr,
        (ignore_all_bad_comps || ignoreCompNm.nameCount()) ? &ignoreCompNm : nullptr);

      if (ignore_all_bad_types)
        debug("ecs will ignore all unknown types");
      else if (ignoreTypeNm.nameCount())
        debug("ecs will ignore %d types specified in %s", ignoreTypeNm.nameCount(), "application.blk");
      if (ignore_all_bad_comps)
        debug("ecs will ignore all components with unknown type");
      else if (ignoreCompNm.nameCount())
        debug("ecs will ignore %d component names specified in %s", ignoreCompNm.nameCount(), "application.blk");
    }

    g_entity_mgr.demandInit();
    G_ASSERT(g_entity_mgr.get());
    g_entity_mgr->setFilterTags(ecs_tools_get_global_tags_context());
    g_entity_mgr->getMutableTemplateDBInfo().resetMetaInfo();
    DataBlock entities;
    if (entities_path && entities.load(entities_path))
    {
      ecs::load_templates_blk_file(*g_entity_mgr, entities_path, entities, trefs, &g_entity_mgr->getMutableTemplateDBInfo());
      g_entity_mgr->addTemplates(trefs);
    }
    else if (entities_path)
      DAEDITOR3.conError("failed to read ECS tempates from %s", entities_path);

    DataBlock scene;
    if (scene_path && scene.load(scene_path))
    {
      ecs::create_entities_blk(*g_entity_mgr, scene, scene_path);
    }
    else if (scene_path)
      DAEDITOR3.conError("failed to create ECS scene from %s", scene_path);
  }

  ~EcsAdapter() override { g_entity_mgr.demandDestroy(); }

  const char *getServiceName() const override { return serviceName; };
  const char *getServiceFriendlyName() const override { return friendlyServiceName; };

  void setServiceVisible(bool vis) override { isVisible = true; };
  bool getServiceVisible() const override { return isVisible; };

  void actService(float dt) override
  {
    if (g_entity_mgr.get())
      g_entity_mgr->tick();
    g_entity_mgr->update(ecs::UpdateStageInfoAct(dt, applicationTime));
    g_entity_mgr->broadcastEventImmediate(ParallelUpdateFrameDelayed(dt, applicationTime));
  }

  void beforeRenderService() override {}
  void renderService() override {}
  void renderTransService() override {}

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IRenderingService);
    return nullptr;
  }
  void renderGeometry(Stage stage) override
  {
    // get matrices for ecs stages
    mat44f globtm;
    d3d::getglobtm(globtm);
    TMatrix viewtm;
    d3d::gettm(TM_VIEW, viewtm);

    switch (stage)
    {
      case STG_BEFORE_RENDER: g_entity_mgr->broadcastEventImmediate(BeforeRender(globtm, viewtm, nullptr)); break;
      case STG_RENDER_SHADOWS: g_entity_mgr->broadcastEventImmediate(RenderStage(RenderStage::Shadow, globtm, viewtm, nullptr)); break;
      case STG_RENDER_DYNAMIC_OPAQUE:
        g_entity_mgr->broadcastEventImmediate(RenderStage(RenderStage::Color, globtm, viewtm, nullptr));
        break;
      case STG_RENDER_DYNAMIC_TRANS:
        g_entity_mgr->broadcastEventImmediate(RenderStage(RenderStage::Transparent, globtm, viewtm, nullptr));
        g_entity_mgr->update(ecs::UpdateStageInfoRenderDebug());
        ::flush_buffered_debug_lines();
        break;
      default: break;
    }
  }

private:
  static constexpr const char *serviceName = "ecsManager";
  static constexpr const char *friendlyServiceName = nullptr; //"(srv) ECS Manager"; // hidden service

  bool isVisible = false;
  float applicationTime = 0;
};

extern size_t ecs_manager_dummy_ecs_sum;

void init_ecs_mgr_service(const DataBlock &app_blk, const char *app_dir)
{
  G_ASSERT(app_dir);

  ecs_manager_dummy_ecs_sum++; // just to anchor the symbol to this function

  const DataBlock *game = app_blk.getBlockByNameEx("game");
  const char *entitiesPath = game->getStr("entities", nullptr);
  const char *scenePath = game->getStr("scene", nullptr);

  String entities(0, "%s%s", app_dir, entitiesPath);
  String scene(0, "%s%s", app_dir, scenePath);
  EcsAdapter *ecsAdapter = new (inimem) EcsAdapter(entitiesPath ? String(0, "%s%s", app_dir, entitiesPath).c_str() : nullptr,
    scenePath ? String(0, "%s%s", app_dir, scenePath).c_str() : nullptr, app_blk);
  if (!IDaEditor3Engine::get().registerService(ecsAdapter))
    logerr("Failed to register ECS Manager service.");
}

#include <assets/assetMgr.h>
static int derive_last_folder_idx(const DagorAssetMgr &aMgr)
{
  int folder_idx = -1;
  while (aMgr.getFolderPtr(folder_idx + 1))
    folder_idx++;
  return folder_idx;
}

void mount_ecs_template_assets(DagorAssetMgr &aMgr)
{
  int atype = aMgr.getAssetTypeId("ecs_template");
  if (atype < 0)
    return;

  const ecs::TemplateDB &templates = g_entity_mgr->getTemplateDB();
  FastNameMap folders;
  for (auto &temp : templates)
  {
    const char *path = temp.getPath();
    if (path && *path)
      folders.addNameId(path);
  }

  aMgr.startMountAssetsDirect("ECS templates", 0);
  for (auto &temp : templates)
    if (!temp.getPath() || !*temp.getPath())
    {
      DataBlock props;
      aMgr.makeAssetDirect(temp.getName(), props, atype);
    }
  aMgr.stopMountAssetsDirect();

  int parent_folder_idx = derive_last_folder_idx(aMgr);
  iterate_names(folders, [&](int id, const char *name) {
    String fname(name);
    simplify_fname(fname);
    aMgr.startMountAssetsDirect(fname, parent_folder_idx);
    for (auto &temp : templates)
      if (folders.getNameId(temp.getPath()) == id)
      {
        DataBlock props;
        aMgr.makeAssetDirect(temp.getName(), props, atype);
      }
    aMgr.stopMountAssetsDirect();
  });
}
