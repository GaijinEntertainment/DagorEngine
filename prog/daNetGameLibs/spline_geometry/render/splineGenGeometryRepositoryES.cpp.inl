// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/world/frameGraphHelpers.h"
#include "render/world/frameGraphNodes/prevFrameTexRequests.h"
#include "splineGenGeometryRepository.h"
#include "splineGenGeometryShaderVar.h"
#include <shaders/dag_shaders.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityId.h>
#include <triangleSizeDebug/triangleSizeDebug.h>

namespace var
{
static ShaderVariableInfo rendinst_transparent_triangle_size_debug("rendinst_transparent_triangle_size_debug", true);
}

SplineGenGeometryRepository::SplineGenGeometryRepository() {}

SplineGenGeometryIbPtr SplineGenGeometryRepository::getOrMakeIb(uint32_t slices, uint32_t stripes)
{
  auto it = ibs.find(slices);
  if (it != ibs.end())
  {
    it->second->resize(stripes);
    return it->second;
  }

  return ibs.emplace(slices, eastl::make_shared<SplineGenGeometryIb>(slices, stripes)).first->second;
}

SplineGenGeometryAssetPtr SplineGenGeometryRepository::getOrMakeAsset(const eastl::string &asset_name)
{
  auto it = assets.find(asset_name);
  if (it != assets.end())
  {
    return it->second;
  }

  return assets.emplace(asset_name, eastl::make_shared<SplineGenGeometryAsset>(asset_name)).first->second;
}

dafg::NodeHandle createTransparentSplineGenNodeImpl(bool is_triangle_debug)
{
  auto nodeNs = is_triangle_debug ? dafg::root() / "tringle_size_debug" / "transparent" : dafg::root() / "transparent" / "close";
  return nodeNs.registerNode("spline_gen", DAFG_PP_NODE_SRC, [is_triangle_debug](dafg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    read_prev_frame_tex(registry);

    if (is_triangle_debug)
    {
      auto ns = registry.root() / "transparent" / "close";
      registry.requestRenderPass().color({"triangle_size_tex"}).depthRo(ns.readTexture("depth_for_transparency"));
      registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
      use_camera_in_camera(registry);
    }
    else
      request_common_published_transparent_state(registry, true);

    return [is_triangle_debug](const dafg::multiplexing::Index &multiplexing_index) {
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      STATE_GUARD_0(ShaderGlobal::set_int(var::rendinst_transparent_triangle_size_debug, VALUE), is_triangle_debug);
      for (auto &managerPair : get_spline_gen_repository().getManagers())
        managerPair.second->renderTransparent();
    };
  });
}


void SplineGenGeometryRepository::createTransparentSplineGenNode()
{
  if (transparentSplineGenNode)
    return;

  transparentSplineGenNode = createTransparentSplineGenNodeImpl(false);
}

template <typename Callable>
static void load_spline_gen_template_params_ecs_query(ecs::EntityManager &manager, Callable c);

SplineGenGeometryManagerPtr SplineGenGeometryRepository::getOrMakeManager(const eastl::string &template_name)
{
  auto it = managers.find(template_name);
  if (it != managers.end())
  {
    return it->second;
  }

  bool found = false;
  uint32_t slices = 0;
  uint32_t stripes = 0;
  eastl::string diffuseName;
  eastl::string normalName;
  eastl::string assetName;
  eastl::string emissiveMaskName;
  eastl::string aoTexName;
  eastl::string shaderType;
  uint32_t assetLod;
  load_spline_gen_template_params_ecs_query(*g_entity_mgr,
    [&](const ecs::string &spline_gen_template__template_name, int spline_gen_template__slices, int spline_gen_template__stripes,
      const ecs::string &spline_gen_template__diffuse_name, const ecs::string &spline_gen_template__normal_name,
      const ecs::string &spline_gen_template__asset_name, const ecs::string &spline_gen_template__shader_type,
      const ecs::string &spline_gen_template__emissive_mask_name, const ecs::string &spline_gen_template__skin_ao_tex_name,
      int spline_gen_template__asset_lod) {
      if (spline_gen_template__template_name == template_name)
      {
        G_ASSERTF(!found, "Duplicate spline_gen_template definition for %s", template_name.c_str());
        found = true;
        slices = spline_gen_template__slices;
        stripes = spline_gen_template__stripes;
        diffuseName = spline_gen_template__diffuse_name;
        normalName = spline_gen_template__normal_name;
        emissiveMaskName = spline_gen_template__emissive_mask_name;
        aoTexName = spline_gen_template__skin_ao_tex_name;
        assetName = spline_gen_template__asset_name;
        assetLod = spline_gen_template__asset_lod;
        shaderType = spline_gen_template__shader_type;
      }
    });
  G_ASSERTF(found, "spline_gen_template %s not found!", template_name.c_str());
  return managers
    .emplace(template_name, eastl::make_shared<SplineGenGeometryManager>(template_name, slices, stripes, diffuseName, normalName,
                              emissiveMaskName, aoTexName, assetName, shaderType, assetLod))
    .first->second;
}

template <typename Callable>
static void load_spline_gen_shapes_ecs_query(ecs::EntityManager &manager, Callable c);

SplineGenGeometryShapeManagerPtr SplineGenGeometryRepository::getOrMakeShapeManager()
{
  if (!shapeManager)
  {
    shapeManager = eastl::make_unique<SplineGenGeometryShapeManager>();
    load_spline_gen_shapes_ecs_query(*g_entity_mgr,
      [&](ecs::Point4List &spline_gen_shape__points, const ecs::string &spline_gen_shape__shape_name) {
        if (spline_gen_shape__shape_name.empty())
        {
          logerr("spline_gen_shape__shape_name is empty. Unnamed shape?");
          return;
        }

        shapeManager->addShape(spline_gen_shape__points, spline_gen_shape__shape_name);
      });
    shapeManager->createAndFillBuffer();
  }
  return shapeManager;
}

void SplineGenGeometryRepository::reset()
{
  ibs.clear();
  assets.clear();
  managers.clear();
}

ska::flat_hash_map<uint32_t, eastl::shared_ptr<SplineGenGeometryIb>> &SplineGenGeometryRepository::getIbs() { return ibs; }

ska::flat_hash_map<eastl::string, eastl::shared_ptr<SplineGenGeometryAsset>> &SplineGenGeometryRepository::getAssets()
{
  return assets;
}

ska::flat_hash_map<eastl::string, eastl::shared_ptr<SplineGenGeometryManager>> &SplineGenGeometryRepository::getManagers()
{
  return managers;
}

ECS_REGISTER_BOXED_TYPE(SplineGenGeometryRepository, nullptr);
ECS_AUTO_REGISTER_COMPONENT(SplineGenGeometryRepository, "spline_gen_repository", nullptr, 0);

template <typename Callable>
void get_spline_gen_ecs_query(ecs::EntityManager &manager, Callable);

SplineGenGeometryRepository &get_spline_gen_repository()
{
  SplineGenGeometryRepository *spline_instance = nullptr;
  get_spline_gen_ecs_query(*g_entity_mgr,
    [&](SplineGenGeometryRepository &spline_gen_repository) { spline_instance = &spline_gen_repository; });
  G_ASSERT_EX(spline_instance, "need spline_gen_repository entity");
  return *spline_instance;
}

ECS_TAG(render, dev)
static void create_transparent_spline_triangle_debug_es(const CreateTriangleDebugNodes &evt)
{
  if (!evt.systems.isTransparent)
    return;

  evt.nodes->push_back(createTransparentSplineGenNodeImpl(true));
}