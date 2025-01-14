// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineGenGeometryRepository.h"
#include "splineGenGeometryShaderVar.h"
#include <shaders/dag_shaders.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/entityId.h>

#define VAR(a) int a##VarId = -1;
SPLINE_GEN_VARS_LIST
#undef VAR

SplineGenGeometryRepository::SplineGenGeometryRepository()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  SPLINE_GEN_VARS_LIST
#undef VAR
}

SplineGenGeometryIb &SplineGenGeometryRepository::getOrMakeIb(uint32_t slices, uint32_t stripes)
{
  auto it = ibs.find(slices);
  if (it != ibs.end())
  {
    it->second->resize(stripes);
    return *it->second;
  }

  return *ibs.emplace(slices, eastl::make_unique<SplineGenGeometryIb>(slices, stripes)).first->second;
}

SplineGenGeometryAsset &SplineGenGeometryRepository::getOrMakeAsset(const eastl::string &asset_name)
{
  auto it = assets.find(asset_name);
  if (it != assets.end())
  {
    return *it->second;
  }

  return *assets.emplace(asset_name, eastl::make_unique<SplineGenGeometryAsset>(asset_name)).first->second;
}

template <typename Callable>
static void load_spline_gen_template_params_ecs_query(Callable c);

SplineGenGeometryManager &SplineGenGeometryRepository::getOrMakeManager(const eastl::string &template_name)
{
  auto it = managers.find(template_name);
  if (it != managers.end())
  {
    return *it->second;
  }

  bool found = false;
  uint32_t slices = 0;
  uint32_t stripes = 0;
  eastl::string diffuseName;
  eastl::string normalName;
  eastl::string assetName;
  uint32_t assetLod;
  load_spline_gen_template_params_ecs_query(
    [&](const ecs::string &spline_gen_template__template_name, int spline_gen_template__slices, int spline_gen_template__stripes,
      const ecs::string &spline_gen_template__diffuse_name, const ecs::string &spline_gen_template__normal_name,
      const ecs::string &spline_gen_template__asset_name, int spline_gen_template__asset_lod) {
      if (spline_gen_template__template_name == template_name)
      {
        G_ASSERTF(!found, "Duplicate spline_gen_template definition for %s", template_name.c_str());
        found = true;
        slices = spline_gen_template__slices;
        stripes = spline_gen_template__stripes;
        diffuseName = spline_gen_template__diffuse_name;
        normalName = spline_gen_template__normal_name;
        assetName = spline_gen_template__asset_name;
        assetLod = spline_gen_template__asset_lod;
      }
    });
  G_ASSERTF(found, "spline_gen_template %s not found!", template_name.c_str());
  return *managers
            .emplace(template_name, eastl::make_unique<SplineGenGeometryManager>(template_name, slices, stripes, diffuseName,
                                      normalName, assetName, assetLod))
            .first->second;
}

void SplineGenGeometryRepository::reset()
{
  ibs.clear();
  assets.clear();
  managers.clear();
}

ska::flat_hash_map<uint32_t, eastl::unique_ptr<SplineGenGeometryIb>> &SplineGenGeometryRepository::getIbs() { return ibs; }

ska::flat_hash_map<eastl::string, eastl::unique_ptr<SplineGenGeometryAsset>> &SplineGenGeometryRepository::getAssets()
{
  return assets;
}

ska::flat_hash_map<eastl::string, eastl::unique_ptr<SplineGenGeometryManager>> &SplineGenGeometryRepository::getManagers()
{
  return managers;
}

ECS_REGISTER_BOXED_TYPE(SplineGenGeometryRepository, nullptr);
ECS_AUTO_REGISTER_COMPONENT(SplineGenGeometryRepository, "spline_gen_repository", nullptr, 0);

template <typename Callable>
void get_spline_gen_ecs_query(Callable);

SplineGenGeometryRepository &get_spline_gen_repository()
{
  SplineGenGeometryRepository *spline_instance = nullptr;
  get_spline_gen_ecs_query([&](SplineGenGeometryRepository &spline_gen_repository) { spline_instance = &spline_gen_repository; });
  G_ASSERT_EX(spline_instance, "need spline_gen_repository entity");
  return *spline_instance;
}
