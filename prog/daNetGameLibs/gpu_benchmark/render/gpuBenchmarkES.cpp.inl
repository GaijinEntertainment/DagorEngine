// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/graphicsAutodetect.h>
#include <render/gpuBenchmarkEntity.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/entityId.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/coreEvents.h>
#include <ioSys/dag_dataBlock.h>
#include <ecs/render/updateStageRender.h>
#include <render/renderer.h>
#include <ecs/render/resPtr.h>

using GraphicsAutodetectWrapper = eastl::unique_ptr<GraphicsAutodetect>;

ECS_DECLARE_RELOCATABLE_TYPE(GraphicsAutodetectWrapper);
ECS_REGISTER_RELOCATABLE_TYPE(GraphicsAutodetectWrapper, nullptr);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void graphics_autodetect_wrapper_on_appear_es_event_handler(const ecs::Event &, GraphicsAutodetectWrapper &graphics_auto_detect)
{
  const DataBlock *blk = ::dgs_get_settings()->getBlockByName("graphicsAutodetect");
  if (!blk)
    logerr("graphicsAutodetect block not found in dgs_get_settings() !");
  graphics_auto_detect = eastl::make_unique<GraphicsAutodetect>(blk ? *blk : DataBlock{}, 0);
}

template <typename Callable>
static void get_graphics_autodetect_ecs_query(Callable c);

template <typename Callable>
ECS_REQUIRE(GraphicsAutodetectWrapper &graphics_auto_detect)
static void delete_graphics_autodetect_ecs_query(Callable c);

GraphicsAutodetect *gpubenchmark::get_graphics_autodetect()
{
  GraphicsAutodetect *result = nullptr;
  get_graphics_autodetect_ecs_query([&](GraphicsAutodetectWrapper &graphics_auto_detect) { result = graphics_auto_detect.get(); });
  return result;
}
void gpubenchmark::make_graphics_autodetect_entity(gpubenchmark::Selfdestruct selfdestruct)
{
  ecs::ComponentsInitializer init;
  init[ECS_HASH("selfdestruct")] = selfdestruct == gpubenchmark::Selfdestruct::YES;
  g_entity_mgr->createEntitySync("graphics_autodetect", eastl::move(init));
}
void gpubenchmark::destroy_graphics_autodetect_entity()
{
  delete_graphics_autodetect_ecs_query([&](ecs::EntityId eid) { g_entity_mgr->destroyEntity(eid); });
}

ECS_TAG(render)
static void graphics_autodetect_before_render_es(const UpdateStageInfoBeforeRender &evt,
  GraphicsAutodetectWrapper &graphics_auto_detect,
  UniqueTex &graphics_auto_detect_tex,
  UniqueTex &graphics_auto_detect_depth,
  bool selfdestruct)
{
  if (!get_world_renderer() || (!graphics_auto_detect->isGpuBenchmarkRunning() && selfdestruct))
  {
    gpubenchmark::destroy_graphics_autodetect_entity();
    return;
  }

  if (!graphics_auto_detect_tex || !graphics_auto_detect_depth)
  {
    TextureInfo info;
    get_world_renderer()->getFinalTargetTex()->getinfo(info);
    uint32_t format = info.cflg & TEXFMT_MASK;
    int w, h;
    get_world_renderer()->getRenderingResolution(w, h);
    graphics_auto_detect_tex = dag::create_tex(nullptr, w, h, format | TEXCF_RTARGET, 1, "graphics_auto_detect_tex");
    graphics_auto_detect_depth = dag::create_tex(nullptr, w, h, TEXFMT_DEPTH32 | TEXCF_RTARGET, 1, "graphics_auto_detect_depth");
    debug("Created graphics_auto_detect textures with resolution %sx%s", w, h);
  }

  graphics_auto_detect->update(evt.dt);
  graphics_auto_detect->render(graphics_auto_detect_tex.getTex2D(), graphics_auto_detect_depth.getTex2D());
}