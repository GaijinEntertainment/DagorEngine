// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_string.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/sharedComponent.h>

#include <webui/nodeBasedShaderType.h>
#define INSIDE_RENDERER
#include <render/world/private_worldRenderer.h>
#include <render/renderEvent.h>
#include <gameRes/dag_stdGameResId.h>
#include <gameRes/dag_gameResSystem.h>
#include <ecs/gameres/commonLoadingJobMgr.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>


extern const char *const EMPTY_LEVEL_NAME;
extern const char *const DEFAULT_WR_LEVEL_NAME;

template <typename Callable>
static void volfog_optional_graphs_ecs_query(Callable c);

void add_volfog_optional_graphs()
{
  if (WorldRenderer *wr = (WorldRenderer *)get_world_renderer())
  {
    volfog_optional_graphs_ecs_query(
      [wr](const ecs::string &volfog) { wr->enableVolumeFogOptionalShader(String(volfog.c_str()), true); });
  }
}

ECS_ON_EVENT(on_appear, on_disappear)
static void add_volfog_optional_graph_es_event_handler(const ecs::Event &evt, const ecs::string &volfog)
{
  if (WorldRenderer *wr = (WorldRenderer *)get_world_renderer())
  {
    bool dest = evt.is<ecs::EventEntityDestroyed>() || evt.is<ecs::EventComponentsDisappear>();
    wr->enableVolumeFogOptionalShader(String(volfog.c_str()), !dest);
  }
}

template <typename Callable>
static void envi_cover_optional_graphs_ecs_query(Callable c);

void add_envi_cover_optional_graphs()
{
  if (WorldRenderer *wr = (WorldRenderer *)get_world_renderer())
  {
    envi_cover_optional_graphs_ecs_query([wr](const ecs::string &envi_cover_optional_graph) {
      wr->enableEnviCoverOptionalShader(String(envi_cover_optional_graph.c_str()), true);
    });
  }
}

ECS_ON_EVENT(on_appear, on_disappear)
static void add_envi_cover_optional_graph_es_event_handler(const ecs::Event &evt, const ecs::string &envi_cover_optional_graph)
{
  if (WorldRenderer *wr = (WorldRenderer *)get_world_renderer())
  {
    bool dest = evt.is<ecs::EventEntityDestroyed>() || evt.is<ecs::EventComponentsDisappear>();
    wr->enableEnviCoverOptionalShader(String(envi_cover_optional_graph.c_str()), !dest);
  }
}


struct LoadNbsVolfogJob final : public cpujobs::IJob
{
  eastl::string resName;
  String rootGraph;
  float lowRange;
  float highRange;
  float lowHeight;
  float highHeight;

  LoadNbsVolfogJob(
    eastl::string &&res_name, const char *root_graph, float low_range, float high_range, float low_height, float high_height) :
    resName(res_name),
    rootGraph(root_graph),
    lowRange(low_range),
    highRange(high_range),
    lowHeight(low_height),
    highHeight(high_height)
  {}

  const char *getJobName(bool &) const override { return "LoadNbsVolfogJob"; }

  void doJob() override
  {
    if (auto res = get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(resName.c_str()), LShaderGameResClassId))
      release_game_resource(res);
  }

  void releaseJob() override
  {
    if (auto wr = static_cast<WorldRenderer *>(get_world_renderer()))
      wr->loadFogNodes(rootGraph, lowRange, highRange, lowHeight, highHeight);

    delete this;
  }
};

template <typename Callable>
inline void get_volfog_nbs_ecs_query(Callable c);

ECS_ON_EVENT(OnLevelLoaded)
static void nbs_volfog_init_es(const OnLevelLoaded &evt)
{
  bool volFogLoaded = false;

  get_volfog_nbs_ecs_query(
    [&volFogLoaded](const ecs::string &volfog_nbs__rootGraph, const float volfog_nbs__low_range, const float volfog_nbs__high_range,
      const float volfog_nbs__low_height, const float volfog_nbs__high_height) {
      eastl::string fullName = node_based_shader_get_resource_name(volfog_nbs__rootGraph.c_str());
      if (get_resource_type_id(fullName.c_str()) == LShaderGameResClassId)
      {
        volFogLoaded = true;
        auto job = new LoadNbsVolfogJob(eastl::move(fullName), volfog_nbs__rootGraph.c_str(), volfog_nbs__low_range,
          volfog_nbs__high_range, volfog_nbs__low_height, volfog_nbs__high_height);
        G_VERIFY(cpujobs::add_job(ecs::get_common_loading_job_mgr(), job));
      }
    });

  if (volFogLoaded)
    return;

  // Fallback
  const DataBlock *volFogBlk = evt.level_blk.getBlockByNameEx("volFog");
  if (ecs::string rootGraph = volFogBlk->getStr("rootFogGraph", ""); !rootGraph.empty())
  {
    eastl::string fullName = node_based_shader_get_resource_name(rootGraph.c_str());
    if (get_resource_type_id(fullName.c_str()) == LShaderGameResClassId)
    {
      logerr("rootFogGraph definition in level.blk is deprecated, please avoid using it!");
      float lowRange = volFogBlk->getReal("low_range", 256.0);
      float highRange = volFogBlk->getReal("high_range", lowRange * 4);
      float lowHeight = volFogBlk->getReal("low_height", 8.0);
      float highHeight = volFogBlk->getReal("high_height", 200.0);

      auto job = new LoadNbsVolfogJob(eastl::move(fullName), rootGraph.c_str(), lowRange, highRange, lowHeight, highHeight);
      G_VERIFY(cpujobs::add_job(ecs::get_common_loading_job_mgr(), job));
    }
  }
}


struct LoadNbsEnviCoverJob final : public cpujobs::IJob
{
  eastl::string resName;
  String rootGraph;

  LoadNbsEnviCoverJob(eastl::string &&res_name, const char *root_graph) : resName(res_name), rootGraph(root_graph) {}

  const char *getJobName(bool &) const override { return "LoadNbsEnviCoverJob"; }

  void doJob() override
  {
    if (auto res = get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(resName.c_str()), LShaderGameResClassId))
      release_game_resource(res);
  }
  void releaseJob() override
  {
    if (auto wr = static_cast<WorldRenderer *>(get_world_renderer()))
      wr->loadEnviCoverNodes(rootGraph);

    delete this;
  }
};

template <typename Callable>
inline void get_envicover_nbs_ecs_query(Callable c);

ECS_ON_EVENT(OnLevelLoaded)
static void nbs_envi_cover_init_es(const OnLevelLoaded &evt)
{
  bool enviCoverLoaded = false;

  get_envicover_nbs_ecs_query([&enviCoverLoaded](const ecs::string &envi_cover_nbs__rootGraph) {
    eastl::string fullName = node_based_shader_get_resource_name(envi_cover_nbs__rootGraph.c_str());
    if (get_resource_type_id(fullName.c_str()) == LShaderGameResClassId)
    {
      enviCoverLoaded = true;
      auto job = new LoadNbsEnviCoverJob(eastl::move(fullName), envi_cover_nbs__rootGraph.c_str());
      G_VERIFY(cpujobs::add_job(ecs::get_common_loading_job_mgr(), job));
    }
  });

  if (enviCoverLoaded)
    return;

  // Fallback
  const DataBlock *volFogBlk = evt.level_blk.getBlockByNameEx("enviCover");
  if (ecs::string rootGraph = volFogBlk->getStr("rootEnviCoverGraph", ""); !rootGraph.empty())
  {
    logerr("rootEnviCoverGraph definition in level.blk is deprecated, please avoid using it!");
    eastl::string fullName = node_based_shader_get_resource_name(rootGraph.c_str());
    if (get_resource_type_id(fullName.c_str()) == LShaderGameResClassId)
    {
      auto job = new LoadNbsEnviCoverJob(eastl::move(fullName), rootGraph.c_str());
      G_VERIFY(cpujobs::add_job(ecs::get_common_loading_job_mgr(), job));
    }
  }
}