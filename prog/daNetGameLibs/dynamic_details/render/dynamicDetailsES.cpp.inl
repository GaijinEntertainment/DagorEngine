// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_resetDevice.h>
#include <math/random/dag_random.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <ecs/anim/anim.h>
#include <shaders/dag_dynSceneRes.h>
#include <ecs/render/animCharUtils.h>
#include <util/dag_console.h>
#include "render/renderEvent.h"


#define TEXTURE_LIST                              \
  VAR(dynamic_details_diff_tex, diffuse_textures) \
  VAR(dynamic_details_norm_tex, norm_textures)

#define VAR(a, b) static TEXTUREID a##TexId;
TEXTURE_LIST
#undef VAR

#define D_CPT (4)
// Details capacity, 4 because of IPoint4 and float4
G_STATIC_ASSERT(D_CPT <= 4); // Code highly optimized for that condition.

struct DynamicDetailsTextureManager
{
  Tab<SimpleString> diffuseTexNames;
  Tab<SimpleString> normTexNames;
  FastNameMap details_names; // Names of loaded details.
  void saveTextures()
  {
    clear_and_shrink(diffuseTexNames);
    clear_and_shrink(normTexNames);

    char tmps[256];
    iterate_names(details_names, [&](int, const char *name) {
      G_ASSERT(name != nullptr);

      SNPRINTF(tmps, sizeof(tmps), "%s_d", name);
      diffuseTexNames.push_back() = tmps;

      SNPRINTF(tmps, sizeof(tmps), "%s_n", name);
      normTexNames.push_back() = tmps;
    });
  }

  void initTextures()
  {
    G_ASSERT(diffuseTexNames.size() == normTexNames.size());
    G_ASSERT(!diffuseTexNames.empty());

    const auto &diffuse_textures = diffuseTexNames;
    const auto &norm_textures = normTexNames;

#define VAR(a, b)                                                                             \
  if (!b.empty())                                                                             \
  {                                                                                           \
    a##TexId = ::add_managed_array_texture(#a, make_span((const char **)b.data(), b.size())); \
    int a##VarId = ::get_shader_variable_id(#a);                                              \
    ShaderGlobal::set_texture(a##VarId, a##TexId);                                            \
    int a##_samplerstateVarId = ::get_shader_variable_id(#a "_samplerstate");                 \
    ShaderGlobal::set_sampler(a##_samplerstateVarId, d3d::request_sampler({}));               \
  }
    TEXTURE_LIST
#undef VAR
  }

  DynamicDetailsTextureManager() = default;
  DynamicDetailsTextureManager(DynamicDetailsTextureManager &&) = default;
  ~DynamicDetailsTextureManager()
  {
#define VAR(a, b) ShaderGlobal::reset_from_vars(a##TexId);
    TEXTURE_LIST
#undef VAR
  }

  void afterReset() { initTextures(); }
};

ECS_DECLARE_RELOCATABLE_TYPE(DynamicDetailsTextureManager);
ECS_REGISTER_RELOCATABLE_TYPE(DynamicDetailsTextureManager, nullptr);

template <typename Callable>
static void create_dynamic_details_ecs_query(ecs::EntityId, Callable);

ECS_TAG(render)
static void create_dynamic_details_es(const BeforeLoadLevel &, ecs::EntityManager &manager)
{
  if (!manager.getTemplateDB().getTemplateByName("dynamic_details"))
    return;

  create_dynamic_details_ecs_query(manager.getOrCreateSingletonEntity(ECS_HASH("dynamic_details")),
    [](DynamicDetailsTextureManager &dynamic_details_mgr, const ecs::StringList &dynamicDetails) {
      for (const ecs::string &texName : dynamicDetails)
        dynamic_details_mgr.details_names.addNameId(texName.c_str());

      dynamic_details_mgr.saveTextures();
      dynamic_details_mgr.initTextures();
    });
}


// efficiently selects min(cnt, 4) unique numbers in range [0, n)
static IPoint4 select_uniques(int &seed, int cnt, int n)
{
  G_ASSERT(cnt <= n + 1);
  int x1, x2, x3, x4;
  x1 = x2 = x3 = x4 = -1;
  if (cnt > 0)
  {
    x1 = _rnd_int(seed, 0, n - 1);
    if (cnt > 1)
    {
      x2 = (x1 + _rnd_int(seed, 1, n - 1)) % n;
      if (cnt > 2)
      {
        x3 = (x1 + _rnd_int(seed, 2, n - 1)) % n;
        x3 = (x3 == x2) ? (x1 + 1) % n : x3;
        if (cnt > 3)
        {
          x4 = (x1 + _rnd_int(seed, 3, n - 1)) % n;
          x4 = (x4 == x3 || x4 == x2) ? (x1 + 1) % n : x4;
          x4 = (x4 == x3 || x4 == x2) ? (x1 + 2) % n : x4;
        }
      }
    }
  }
  return IPoint4(x1, x2, x3, x4);
}

// get random value with exp distribution
static int _poisson_rnd(int &seed, int max, int exp)
{
  // return log(exp, rnd(pow(exp, max)));
  if (max == 0)
    return 0;
  bool a;
  int ans = 0;
  do
  {
    a = (_rnd_int(seed, 1, exp) == 1);
    ans += a;
  } while (a && ans < max);
  return ans;
}

static bool is_exists4(const Color4 &c, float v) { return c.r == v || c.g == v || c.b == v || c.a == v; }

static const ecs::Array *get_group_by_name(const ecs::Object &dynamic_details__groups, const char *name)
{
  const ecs::Array *group = dynamic_details__groups.getNullable<ecs::Array>(ECS_HASH_SLOW(name));
  if (!group)
    logerr("dynamic_details: No group found with the name: \"%s\"", name);
  return group;
}

static int get_detail_id_by_name(const char *name, const FastNameMap &details_names)
{
  int dId = details_names.getNameId(name);
  if (dId < 0)
    logerr("dynamic_details: No dynamic detail found with the name: \"%s\"", name);
  return dId;
}

template <typename Callable>
static void get_dynamic_details_indices_ecs_query(Callable c);

struct DynamicDetails
{
  Color4 indexes = Color4(-1, -1, -1, -1);
  // translate f2, rotation f, scale f
  Color4 details_transform[4] = {Color4(0, 0, 0, 1), Color4(0, 0, 0, 1), Color4(0, 0, 0, 1), Color4(0, 0, 0, 1)};
};

DynamicDetails get_dynamic_details_indices(
  int seed, const ecs::Object &dynamic_details__groups, const ecs::Array &dynamic_details__presets)
{
  DynamicDetails result;
  get_dynamic_details_indices_ecs_query([&](const DynamicDetailsTextureManager &dynamic_details_mgr) {
    if (dynamic_details__presets.empty())
      return;

    int presetId = _rnd_int(seed, 0, dynamic_details__presets.size() - 1);
    const ecs::Object &preset = dynamic_details__presets[presetId].get<ecs::Object>();

    int genDetails = 0;
    // Generate essential details
    for (ecs::Object::const_iterator gIt = preset.begin(); gIt != preset.end(); ++gIt)
    {
      const ecs::Object &gParams = gIt->second.get<ecs::Object>();
      const eastl::string &gName = gIt->first;
      int dMin = gParams.getMemberOr("details_min", 0);
      if (!dMin)
        continue;

      const ecs::Array *group = get_group_by_name(dynamic_details__groups, gName.c_str());
      if (!group)
        continue;

      IPoint4 dIds = select_uniques(seed, dMin, group->size());
      for (int j = 0; j < dMin; j++)
      {
        const ecs::string &dName = (*group)[dIds[j]].get<ecs::string>();
        int dId = get_detail_id_by_name(dName.c_str(), dynamic_details_mgr.details_names);
        if (dId < 0)
          continue;

        G_ASSERT_LOG(genDetails < D_CPT, "Summ of details_min in preset should be no more than %d.", D_CPT);
        result.indexes[genDetails] = dId;
        genDetails++;
      }
    }

    // Generate optional details
    int groupsCnt = min<int>(preset.size(), _rnd_int(seed, 0, D_CPT));
    IPoint4 groupsIds = select_uniques(seed, groupsCnt, preset.size());

    for (int i = 0; i < groupsCnt && genDetails < D_CPT; i++)
    {
      int gId = groupsIds[i];
      ecs::Object::const_iterator gIt = preset.begin() + gId;
      const eastl::string &gName = gIt->first;
      const ecs::Object &gParams = gIt->second.get<ecs::Object>();

      int dMin = gParams.getMemberOr("details_min", 0);
      int dMax = gParams.getMemberOr("details_max", D_CPT);
      int dCnt = min(D_CPT - genDetails, dMax - dMin);
      if (!dCnt)
        continue;

      // It's not linear distribution between count in one group
      int dRnd = 1 + _poisson_rnd(seed, dCnt - 1, preset.size());
      // This distribution is equal to selecting few groups,
      // may be with same id, with one random detail in each of them.
      // This way was chosen because it can select up to 4*dCnt details,
      // that better copes with overlapping.

      const ecs::Array *group = get_group_by_name(dynamic_details__groups, gName.c_str());
      if (!group)
        continue;

      int oCnt = min<int>(D_CPT, group->size());
      IPoint4 dIds = select_uniques(seed, oCnt, group->size());
      // We select more than dRnd count to fix overlapping with essential details

      int genNew = genDetails + dRnd;
      for (int j = 0; j < oCnt && genDetails < genNew; j++)
      {
        const ecs::string &dName = (*group)[dIds[j]].get<ecs::string>();
        int dId = get_detail_id_by_name(dName.c_str(), dynamic_details_mgr.details_names);
        if (dId < 0)
          continue;

        // Check for overlapping
        if (is_exists4(result.indexes, dId))
          continue;

        result.indexes[genDetails] = dId;
        genDetails++;
      }
    }
  });
  return result;
}

static DynamicDetails apply_dynamic_details_preset(const ecs::Array &dynamic_details_preset)
{
  DynamicDetails result;
  get_dynamic_details_indices_ecs_query([&](const DynamicDetailsTextureManager &dynamic_details_mgr) {
    int count = dynamic_details_preset.size();
    if (count > D_CPT)
    {
      logerr("dynamic details support <= 4 textures in dynamic_details_preset");
      count = D_CPT;
    }
    int genDetails = 0;
    for (int i = 0; i < count; i++)
    {
      const ecs::Object &texture = dynamic_details_preset[i].get<ecs::Object>();
      const char *textureName = texture.getMemberOr(ECS_HASH("texture"), "");
      int dId = get_detail_id_by_name(textureName, dynamic_details_mgr.details_names);
      if (dId < 0)
        continue;
      result.indexes[genDetails] = dId;
      Point2 uvOffset = texture.getMemberOr(ECS_HASH("uv_offset"), Point2(0, 0));
      float uvRotation = texture.getMemberOr(ECS_HASH("uv_rotation"), 0.f);
      float uvScale = texture.getMemberOr(ECS_HASH("uv_scale"), 1.f);
      result.details_transform[genDetails] = Color4(uvOffset.x, uvOffset.y, uvRotation * DEG_TO_RAD, safeinv(uvScale));
      genDetails++;
    }
  });
  return result;
}


ECS_TAG(render)
ECS_REQUIRE(bool skeleton_attach__attached)
ECS_TRACK(animchar_attach__attachedTo)
ECS_ON_EVENT(on_appear)
void dynamic_details_es_event_handler(const ecs::Event &,
  AnimV20::AnimcharRendComponent &animchar_render,
  const ecs::Object &dynamic_details__groups,
  const ecs::Array &dynamic_details__presets,
  const ecs::Array *dynamic_details_preset,
  ecs::EntityId animchar_attach__attachedTo)
{
  static int detailVarId = get_shader_variable_id("dynamic_details");
  static int detailTransformVarId[4] = {get_shader_variable_id("dynamic_details_transform_0"),
    get_shader_variable_id("dynamic_details_transform_1"), get_shader_variable_id("dynamic_details_transform_2"),
    get_shader_variable_id("dynamic_details_transform_3")};

  DynamicDetails details;
  if (dynamic_details_preset)
  {
    details = apply_dynamic_details_preset(*dynamic_details_preset);
  }
  else
  {
    int seed = ECS_GET_OR(animchar_attach__attachedTo, appearance__rndSeed, 0);
    if (!seed)
      seed = grnd();
    details = get_dynamic_details_indices(seed, dynamic_details__groups, dynamic_details__presets);
  }

  recreate_material_with_new_params(animchar_render, [&](ShaderMaterial *mat) {
    mat->set_color4_param(detailVarId, details.indexes);
    for (int i = 0; i < 4; i++)
      mat->set_color4_param(detailTransformVarId[i], details.details_transform[i]);
  });
}


static void dynamic_detials_after_reset_es(const AfterDeviceReset &, DynamicDetailsTextureManager &dynamic_details_mgr)
{
  dynamic_details_mgr.afterReset();
}

template <typename Callable>
static void reset_dynamic_details_all_ecs_query(Callable c);

template <typename Callable>
static void reset_dynamic_details_selected_ecs_query(Callable c);

template <typename Callable>
static void info_dynamic_details_selected_ecs_query(Callable c);


static bool dynamic_details_write_debug_info(
  ecs::EntityId eid, AnimV20::AnimcharRendComponent &animchar_render, const DynamicDetailsTextureManager &dynamic_details_mgr)
{
  const int varId = get_shader_variable_id("dynamic_details", true);

  if (!VariableMap::isGlobVariablePresent(varId))
    return false;

  DynamicRenderableSceneInstance *instance = animchar_render.getSceneInstance();
  if (instance == nullptr)
    return false;
  DynamicRenderableSceneLodsResource *newRes = instance->getLodsResource()->clone();
  if (newRes == nullptr)
    return false;
  DynamicRenderableSceneResource *nRes = newRes->getLod(0);
  if (nRes == nullptr)
    return false;

  Color4 details_indices;

  Tab<ShaderMaterial *> matList;
  nRes->gatherUsedMat(matList);
  if (matList.empty())
    return false;

  matList[0]->getColor4Variable(varId, details_indices);

  console::print_d("Dynamic details info for eid: %u", (uint32_t)eid);
  for (int i = 0; i < 4; i++)
  {
    const char *detail_name = dynamic_details_mgr.details_names.getName(int(details_indices[i]));
    console::print_d("  Detail %d, value %f, name: %s", i, details_indices[i], detail_name ? detail_name : "Unknown");
  }
  console::print_d("");

  return true;
}

static bool dynamic_details_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("dynamic_details", "info", 1, 1)
  {
    get_dynamic_details_indices_ecs_query([&](const DynamicDetailsTextureManager &dynamic_details_mgr) {
      info_dynamic_details_selected_ecs_query(
        [&](ecs::EntityId eid,
          AnimV20::AnimcharRendComponent &animchar_render ECS_REQUIRE(const AnimV20::AnimcharBaseComponent &animchar,
            const ecs::Object &dynamic_details__groups, const ecs::Array &dynamic_details__presets, bool skeleton_attach__attached,
            ecs::Tag daeditor__selected)) { dynamic_details_write_debug_info(eid, animchar_render, dynamic_details_mgr); });
    });
  }

  return found;
}

REGISTER_CONSOLE_HANDLER(dynamic_details_console_handler);
