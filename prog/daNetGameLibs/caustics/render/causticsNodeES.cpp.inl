// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "caustics.h"

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <ecs/core/entityManager.h>
#include <ecs/render/shaders.h>
#include <ecs/render/resPtr.h>
#include <ecs/render/updateStageRender.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/world/wrDispatcher.h>
#include <scene/dag_tiledScene.h>
#include <math/dag_mathUtils.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <util/dag_console.h>
#include <debug/dag_log.h>

#define CAUSTICS_VARS VAR(combined_shadows_has_caustics)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
CAUSTICS_VARS
#undef VAR

template <typename Callable>
static void water_quality_medium_or_high_ecs_query(Callable c);
template <typename Callable>
static void water_caustics_node_exists_ecs_query(Callable c);
template <typename Callable>
static void water_caustics_update_settings_ecs_query(Callable c);
template <typename Callable>
static void water_caustics_get_settings_ecs_query(ecs::EntityId e, Callable c);

void queryCausticsSettings(CausticsSetting &settings)
{
  ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH("caustics_settings"));
  if (!eid)
  {
    LOGERR_ONCE("Water caustic is enabled but entity with \"caustics_settings\" template is missing on the scene.!");
    return;
  }

  water_caustics_get_settings_ecs_query(eid, [&settings](float &water_caustics_scroll_speed, float &water_caustics_world_scale) {
    settings.causticsScrollSpeed = water_caustics_scroll_speed;
    settings.causticsWorldScale = water_caustics_world_scale;
  });
}

bool use_water_caustics()
{
  bool waterCausticsEntityExists = false;
  water_caustics_node_exists_ecs_query([&waterCausticsEntityExists](const dafg::NodeHandle &caustics__renderNode) {
    G_UNUSED(caustics__renderNode);
    waterCausticsEntityExists = true;
  });
  bool waterHighOrMedium = false;
  water_quality_medium_or_high_ecs_query([&waterHighOrMedium](const ecs::string &render_settings__waterQuality) {
    waterHighOrMedium = render_settings__waterQuality == "high" || render_settings__waterQuality == "medium";
  });
  return waterCausticsEntityExists && waterHighOrMedium && renderer_has_feature(FeatureRenderFlags::FULL_DEFERRED) &&
         renderer_has_feature(FeatureRenderFlags::COMBINED_SHADOWS);
}

static void update_caustics_entity(dafg::NodeHandle &caustics__perCameraResNode,
  dafg::NodeHandle &caustics__renderNode,
  UniqueTexHolder &caustics__indoor_probe_mask,
  bool &needs_water_heightmap,
  bool &combined_shadows__use_additional_textures)
{
  caustics__perCameraResNode = {};
  caustics__renderNode = {};
  caustics__indoor_probe_mask.close();
  if (use_water_caustics())
  {
    g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("caustics_settings"));
    needs_water_heightmap = true;
    combined_shadows__use_additional_textures = true;
    ShaderGlobal::set_int(combined_shadows_has_causticsVarId, 1);

    auto [perCameraResNode, renderNode] = makeCausticsNode();
    caustics__perCameraResNode = eastl::move(perCameraResNode);
    caustics__renderNode = eastl::move(renderNode);
  }
  else
  {
    needs_water_heightmap = false;
    combined_shadows__use_additional_textures = false;
    ShaderGlobal::set_int(combined_shadows_has_causticsVarId, 0);
  }
}

template <typename Callable>
static void create_caustics_node_ecs_query(Callable c);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__waterQuality)
static void caustics_water_quality_changed_es(const ecs::Event &, const ecs::string &render_settings__waterQuality)
{
  G_UNUSED(render_settings__waterQuality);
  create_caustics_node_ecs_query(
    [](dafg::NodeHandle &caustics__perCameraResNode, dafg::NodeHandle &caustics__renderNode,
      UniqueTexHolder &caustics__indoor_probe_mask, bool &needs_water_heightmap, bool &combined_shadows__use_additional_textures) {
      update_caustics_entity(caustics__perCameraResNode, caustics__renderNode, caustics__indoor_probe_mask, needs_water_heightmap,
        combined_shadows__use_additional_textures);
    });
}

ECS_TAG(render)
ECS_ON_EVENT(ChangeRenderFeatures)
static void caustics_render_features_changed_es(const ecs::Event &evt,
  dafg::NodeHandle &caustics__perCameraResNode,
  dafg::NodeHandle &caustics__renderNode,
  UniqueTexHolder &caustics__indoor_probe_mask,
  bool &needs_water_heightmap,
  bool &combined_shadows__use_additional_textures)
{
  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
  {
    if (!changedFeatures->isFeatureChanged(FeatureRenderFlags::FULL_DEFERRED) &&
        !changedFeatures->isFeatureChanged(FeatureRenderFlags::COMBINED_SHADOWS) &&
        !changedFeatures->isFeatureChanged(CAMERA_IN_CAMERA))
      return;
  }
  update_caustics_entity(caustics__perCameraResNode, caustics__renderNode, caustics__indoor_probe_mask, needs_water_heightmap,
    combined_shadows__use_additional_textures);
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void caustics_before_render_es(const UpdateStageInfoBeforeRender &,
  dafg::NodeHandle &caustics__perCameraResNode,
  dafg::NodeHandle &caustics__renderNode,
  UniqueTexHolder &caustics__indoor_probe_mask,
  const ShadersECS &caustics__indoor_probe_shader)
{
  if (!caustics__indoor_probe_shader)
    return;
  auto *scene = WRDispatcher::getEnviProbeBoxesScene();
  if (!scene)
    return;

  if (caustics__perCameraResNode && caustics__renderNode && !caustics__indoor_probe_mask)
  {
    TIME_D3D_PROFILE(CausticsIndoorProbeMask);

    dag::Vector<Point3> vertices;
    vertices.reserve(scene->getNodesCount() * 6);
    bbox3f probeSceneBoxVec;
    v_bbox3_init_empty(probeSceneBoxVec);
    auto makeBottomQuad = [&vertices, &probeSceneBoxVec](const mat44f &worldtm) {
      vec4f lb = v_mat44_mul_vec4(worldtm, v_make_vec4f(-0.5, -0.5, -0.5, 1));
      vec4f lt = v_mat44_mul_vec4(worldtm, v_make_vec4f(-0.5, -0.5, 0.5, 1));
      vec4f rb = v_mat44_mul_vec4(worldtm, v_make_vec4f(0.5, -0.5, -0.5, 1));
      vec4f rt = v_mat44_mul_vec4(worldtm, v_make_vec4f(0.5, -0.5, 0.5, 1));
      auto convert = [&probeSceneBoxVec](const vec4f &v) -> Point3 {
        v_bbox3_add_pt(probeSceneBoxVec, v);
        Point3_vec4 vec;
        v_st(&vec, v);
        return vec;
      };
      Point3 lb3 = convert(lb);
      Point3 lt3 = convert(lt);
      Point3 rb3 = convert(rb);
      Point3 rt3 = convert(rt);
      vertices.push_back(lb3);
      vertices.push_back(lt3);
      vertices.push_back(rt3);
      vertices.push_back(rt3);
      vertices.push_back(rb3);
      vertices.push_back(lb3);
    };
    for (const scene::node_index &nodeIdx : *scene)
    {
      const mat44f &matrix4x4 = scene->getNode(nodeIdx);
      makeBottomQuad(matrix4x4);
    }
    if (vertices.empty())
      return;

    BBox3 probeSceneBox;
    v_stu_bbox3(probeSceneBox, probeSceneBoxVec);
    probeSceneBox.inflate(1); // Just to have some padding everywhere

    static constexpr int TEX_SIZE_MAX = 1024;
    static constexpr float meter_per_texel = 0.1;
    IPoint2 texSize = ipoint2(ceil(Point2::xz(probeSceneBox.width()) / meter_per_texel));
    texSize.x = min(TEX_SIZE_MAX, get_bigger_pow2(texSize.x));
    texSize.y = min(TEX_SIZE_MAX, get_bigger_pow2(texSize.y));
    caustics__indoor_probe_mask =
      dag::create_tex(nullptr, texSize.x, texSize.y, TEXFMT_DEPTH16 | TEXCF_RTARGET, 1, "caustics__indoor_probe_mask");
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      smpInfo.filter_mode = d3d::FilterMode::Compare;
      smpInfo.mip_map_mode = d3d::MipMapMode::Point;
      smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
      ShaderGlobal::set_sampler(get_shader_variable_id("caustics__indoor_probe_mask_samplerstate", true),
        d3d::request_sampler(smpInfo));
    }
    debug("caustics__indoor_probe_mask created with resolution %d x %d", texSize.x, texSize.y);

    TMatrix viewItm;
    viewItm.setcol(0, {1, 0, 0});
    viewItm.setcol(1, {0, 0, 1});
    viewItm.setcol(2, {0, 1, 0});
    viewItm.setcol(3, {probeSceneBox.center().x, probeSceneBox.boxMin().y, probeSceneBox.center().z});
    TMatrix viewTm = inverse(viewItm);
    TMatrix4 projMat = matrix_ortho_lh_reverse(probeSceneBox.width().x, probeSceneBox.width().z, 0, probeSceneBox.width().y);
    TMatrix4 globtm = TMatrix4(viewTm) * projMat;
    ShaderGlobal::set_float4x4(get_shader_variable_id("caustics__indoor_probe_mask_matrix"), globtm.transpose());

    d3d::settm(TM_VIEW, viewTm);
    d3d::settm(TM_PROJ, &projMat);
    d3d::set_render_target({caustics__indoor_probe_mask.getTex2D(), 0}, DepthAccess::RW, {});
    d3d::clearview(CLEAR_ZBUFFER, 0, 0, 0);
    caustics__indoor_probe_shader.shElem->setStates();
    d3d::draw_up(PRIM_TRILIST, vertices.size() / 3, vertices.data(), sizeof(Point3));
  }
}

static bool caustic_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("render", "causticsWorldScale", 1, 2)
  {
    float scale = argc > 1 ? atof(argv[1]) : -1;
    water_caustics_update_settings_ecs_query([scale](float &water_caustics_scroll_speed, float &water_caustics_world_scale) {
      G_UNUSED(water_caustics_scroll_speed);
      water_caustics_world_scale = scale;
    });
  }
  CONSOLE_CHECK_NAME("postfx", "causticsScrollSpeed", 1, 1)
  {
    float scroll = argc > 1 ? atof(argv[1]) : -1;
    water_caustics_update_settings_ecs_query([scroll](float &water_caustics_scroll_speed, float &water_caustics_world_scale) {
      water_caustics_scroll_speed = scroll;
      G_UNUSED(water_caustics_world_scale);
    });
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(caustic_console_handler);