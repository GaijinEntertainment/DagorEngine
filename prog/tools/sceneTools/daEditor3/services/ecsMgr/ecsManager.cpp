#include <ioSys/dag_dataBlock.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_workspace.h>
#include <de3_interface.h>
#include <EditorCore/ec_interface_ex.h>
#include <ecs/core/entityManager.h>
#include <ecs/io/blk.h>
#include <ecs/renderEvents.h>
#include <ecs/delayedAct/actInThread.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_drv3d.h>
#include <debug/dag_debug3d.h>

class EcsAdapter;

eastl::unique_ptr<EcsAdapter> ecsAdapter;

Tab<const char *> ecs_get_global_tags_context()
{
  Tab<const char *> globalTags(framemem_ptr());
  globalTags.push_back("render");
  globalTags.push_back("tools");
  return globalTags;
}

class EcsAdapter : public IEditorService, public IRenderingService
{
public:
  EcsAdapter(const char *entities_path, const char *scene_path)
  {
    if (!IDaEditor3Engine::get().registerService(this))
    {
      logerr("Failed to register ECS Manager service.");
    }

    ecs::TemplateRefs trefs;

    g_entity_mgr.demandInit();
    G_ASSERT(g_entity_mgr.get());
    g_entity_mgr->setFilterTags(ecs_get_global_tags_context());
    g_entity_mgr->getMutableTemplateDBInfo().resetMetaInfo();
    DataBlock entities;
    if (entities_path && entities.load(entities_path))
    {
      ecs::load_templates_blk_file(entities_path, entities, trefs, &g_entity_mgr->getMutableTemplateDBInfo());
      g_entity_mgr->addTemplates(trefs);
    }
    else if (entities_path)
      DAEDITOR3.conError("failed to read ECS tempates from %s", entities_path);

    DataBlock scene;
    if (scene_path && scene.load(scene_path))
    {
      ecs::create_entities_blk(scene, scene_path);
    }
    else if (scene_path)
      DAEDITOR3.conError("failed to create ECS scene from %s", scene_path);
  }

  ~EcsAdapter() { g_entity_mgr.demandDestroy(); }

  virtual const char *getServiceName() const override { return serviceName; };
  virtual const char *getServiceFriendlyName() const override { return friendlyServiceName; };

  virtual void setServiceVisible(bool vis) override { isVisible = true; };
  virtual bool getServiceVisible() const override { return isVisible; };

  virtual void actService(float dt) override
  {
    if (g_entity_mgr.get())
      g_entity_mgr->tick();
    g_entity_mgr->update(ecs::UpdateStageInfoAct(dt, applicationTime));
    g_entity_mgr->broadcastEventImmediate(ParallelUpdateFrameDelayed(dt, applicationTime));
  }

  virtual void beforeRenderService() override{};
  virtual void renderService() override{};
  virtual void renderTransService() override{};

  virtual void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IRenderingService);
    return nullptr;
  }
  virtual void renderGeometry(Stage stage)
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
  ecsAdapter = eastl::make_unique<EcsAdapter>(entitiesPath ? String(0, "%s%s", app_dir, entitiesPath).c_str() : nullptr,
    scenePath ? String(0, "%s%s", app_dir, scenePath).c_str() : nullptr);
}