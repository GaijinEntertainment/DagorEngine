// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <ecs/anim/anim.h>
#include <ecs/render/resPtr.h>
#include <ecs/render/shaderVar.h>
#include <ecs/render/shaders.h>
#include <ecs/render/updateStageRender.h>
#include <render/collimatorMoaCore/render.h>

#include <3d/dag_resPtr.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <shaders/dag_dynSceneRes.h>


template <typename Callable>
inline void render_collimator_moa_lens_ecs_query(ecs::EntityManager &manager, Callable c);
template <typename Callable>
inline void setup_collimator_moa_render_ecs_query(ecs::EntityManager &manager, Callable c);
template <typename Callable>
inline void setup_collimator_moa_lens_ecs_query(ecs::EntityManager &manager, ecs::EntityId, Callable c);
template <typename Callable>
inline void update_collimator_moa_lens_axis_ecs_query(ecs::EntityManager &manager, ecs::EntityId, Callable c);
template <typename Callable>
inline void setup_collimator_moa_image_ecs_query(ecs::EntityManager &manager, ecs::EntityId, Callable c);

namespace
{
enum LensRenderMode
{
  DONT_RENDER = 0,
  RENDER_LENS = 1
};

struct LensRelemData
{
  const ShaderMesh::RElem *relem = nullptr;
  const TMatrix *wtm = nullptr;

  operator bool() const { return wtm != nullptr && relem != nullptr; }
};

} // namespace

namespace var
{
const static ShaderVariableInfo dcm_lens_up("dcm_lens_up");
const static ShaderVariableInfo dcm_lens_right("dcm_lens_right");
const static ShaderVariableInfo dcm_lens_forward("dcm_lens_forward");
const static ShaderVariableInfo dcm_eye_to_lens_aligned_origin("dcm_eye_to_lens_aligned_origin");
const static ShaderVariableInfo dcm_eye_to_plane_origin("dcm_eye_to_plane_origin");
const static ShaderVariableInfo dcm_color("dcm_color");
const static ShaderVariableInfo dcm_lens_color_tint("dcm_lens_color_tint");

const static ShaderVariableInfo dcm_shapes_buf("dcm_shapes_buf");
const static ShaderVariableInfo dcm_shapes_count("dcm_shapes_count");
const static ShaderVariableInfo dcm_shapes_buf_reg_count("dcm_shapes_buf_reg_count");

const static ShaderVariableInfo dcm_render_mode("dcm_render_mode");
const static ShaderVariableInfo dcm_inv_screen_size("dcm_inv_screen_size");

const static ShaderVariableInfo dcm_border_min("dcm_border_min");
const static ShaderVariableInfo dcm_border_scale("dcm_border_scale");
const static ShaderVariableInfo dcm_use_noise("dcm_use_noise");
const static ShaderVariableInfo dcm_noise_min_intensity("dcm_noise_min_intensity");
const static ShaderVariableInfo dcm_light_noise_thinness("dcm_light_noise_thinness");
const static ShaderVariableInfo dcm_light_noise_intensity_scale("dcm_light_noise_intensity_scale");
const static ShaderVariableInfo dcm_static_noise_uv_scale("dcm_static_noise_uv_scale");
const static ShaderVariableInfo dcm_static_noise_add("dcm_static_noise_add");
const static ShaderVariableInfo dcm_static_noise_scale("dcm_static_noise_scale");
const static ShaderVariableInfo dcm_dynamic_noise_uv_scale("dcm_dynamic_noise_uv_scale");
const static ShaderVariableInfo dcm_dynamic_noise_sub_scale("dcm_dynamic_noise_sub_scale");
const static ShaderVariableInfo dcm_dynamic_noise_scale("dcm_dynamic_noise_scale");
const static ShaderVariableInfo dcm_dynamic_noise_add("dcm_dynamic_noise_add");
const static ShaderVariableInfo dcm_dynamic_noise_speed("dcm_dynamic_noise_speed");
} // namespace var

static LensRelemData get_lens_relem_data(const AnimV20::AnimcharRendComponent &animchar_render, const int rigid_no, const int relem_id)
{
  const DynamicRenderableSceneInstance *sceneInstance = animchar_render.getSceneInstance();
  if (!sceneInstance || !sceneInstance->getLodsResource())
    return {};

  const DynamicRenderableSceneResource *sceneRes = sceneInstance->getLodsResource()->lods[0].scene;
  if (!sceneRes)
    return {};

  dag::ConstSpan<DynamicRenderableSceneResource::RigidObject> rigids = sceneRes->getRigidsConst();
  G_ASSERT_RETURN(rigid_no < rigids.size(), {});

  dag::ConstSpan<ShaderMesh::RElem> relems = rigids[rigid_no].mesh->getMesh()->getAllElems();
  G_ASSERT_RETURN(relem_id < relems.size(), {});

  const ShaderMesh::RElem &relem = relems[relem_id];
  if (relem.vertexData->isEmpty())
    return {};

  const DynamicRenderableSceneResource::RigidObject &rigid = rigids[rigid_no];

  LensRelemData res;
  res.relem = &relem;
  res.wtm = &sceneInstance->getNodeWtm(rigid.nodeId);

  return res;
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es, collimator_moa_update_lens_eids_es)
static void collimator_moa_update_lens_axis_es(const UpdateStageInfoBeforeRender &, ecs::EntityManager &manager,
  const ecs::EntityId collimator_moa_render__gun_mod_eid, const int collimator_moa_render__rigid_id,
  const int collimator_moa_render__relem_id)
{
  if (!collimator_moa_render__gun_mod_eid || collimator_moa_render__rigid_id < 0 || collimator_moa_render__relem_id < 0)
    return;

  update_collimator_moa_lens_axis_ecs_query(manager, collimator_moa_render__gun_mod_eid,
    [&](const AnimV20::AnimcharRendComponent &animchar_render, const Point3 &gunmod__collimator_moa_local_forward,
      Point3 &gunmod__collimator_moa_lens_axis) {
      const LensRelemData relemData =
        get_lens_relem_data(animchar_render, collimator_moa_render__rigid_id, collimator_moa_render__relem_id);
      if (relemData)
        gunmod__collimator_moa_lens_axis = normalize(*relemData.wtm % gunmod__collimator_moa_local_forward);
    });
}

namespace collimator_moa
{
static void setup_image_render(ecs::EntityId image_eid)
{
  setup_collimator_moa_image_ecs_query(*g_entity_mgr, image_eid,
    [&](const Point4 &collimator_moa_image__color, const float collimator_moa_image__border_min,
      const float collimator_moa_image__border_scale, const bool collimator_moa_image__use_noise,
      const float collimator_moa_image__noise_min_intensity, const float collimator_moa_image__light_noise_thinness,
      const float collimator_moa_image__light_noise_intensity_scale, const float collimator_moa_image__static_noise_uv_scale,
      const float collimator_moa_image__static_noise_add, const float collimator_moa_image__static_noise_scale,
      const float collimator_moa_image__dynamic_noise_uv_scale, const float collimator_moa_image__dynamic_noise_sub_scale,
      const float collimator_moa_image__dynamic_noise_scale, const float collimator_moa_image__dynamic_noise_add,
      const Point2 &collimator_moa_image__dynamic_noise_speed) {
      ShaderGlobal::set_float4(var::dcm_color, collimator_moa_image__color);
      ShaderGlobal::set_float(var::dcm_border_min, collimator_moa_image__border_min);
      ShaderGlobal::set_float(var::dcm_border_scale, collimator_moa_image__border_scale);

      ShaderGlobal::set_float(var::dcm_use_noise, collimator_moa_image__use_noise ? 1.0f : 0.0f);
      if (collimator_moa_image__use_noise)
      {
        ShaderGlobal::set_float(var::dcm_noise_min_intensity, collimator_moa_image__noise_min_intensity);

        ShaderGlobal::set_float(var::dcm_light_noise_thinness, collimator_moa_image__light_noise_thinness);
        ShaderGlobal::set_float(var::dcm_light_noise_intensity_scale, collimator_moa_image__light_noise_intensity_scale);

        ShaderGlobal::set_float(var::dcm_static_noise_uv_scale, collimator_moa_image__static_noise_uv_scale);
        ShaderGlobal::set_float(var::dcm_static_noise_add, collimator_moa_image__static_noise_add);
        ShaderGlobal::set_float(var::dcm_static_noise_scale, collimator_moa_image__static_noise_scale);

        ShaderGlobal::set_float(var::dcm_dynamic_noise_uv_scale, collimator_moa_image__dynamic_noise_uv_scale);
        ShaderGlobal::set_float(var::dcm_dynamic_noise_sub_scale, collimator_moa_image__dynamic_noise_sub_scale);
        ShaderGlobal::set_float(var::dcm_dynamic_noise_scale, collimator_moa_image__dynamic_noise_scale);
        ShaderGlobal::set_float(var::dcm_dynamic_noise_add, collimator_moa_image__dynamic_noise_add);
        ShaderGlobal::set_float4(var::dcm_dynamic_noise_speed, collimator_moa_image__dynamic_noise_speed);
      }
    });
}

static LensRelemData setup_render()
{
  LensRelemData relemData;
  setup_collimator_moa_render_ecs_query(*g_entity_mgr,
    [&](const ecs::EntityId &collimator_moa_render__gun_mod_eid, const int collimator_moa_render__rigid_id,
      const int collimator_moa_render__relem_id, const float collimator_moa_render__calibration_range_cm) {
      if (collimator_moa_render__gun_mod_eid)
      {
        setup_collimator_moa_lens_ecs_query(*g_entity_mgr, collimator_moa_render__gun_mod_eid,
          [&](const AnimV20::AnimcharRendComponent &animchar_render, const Point3 &gunmod__collimator_moa_local_forward,
            const Point3 &gunmod__collimator_moa_local_right, const Point3 &gunmod__collimator_moa_lens_axis,
            const float gunmod__collimator_moa_parallax_plane_dist, const Point4 &gunmod__collimator_lens_color) {
            relemData = get_lens_relem_data(animchar_render, collimator_moa_render__rigid_id, collimator_moa_render__relem_id);

            if (!relemData)
              return;

            const TMatrix &wtm = *relemData.wtm;
            const Point3 localUp = gunmod__collimator_moa_local_right % gunmod__collimator_moa_local_forward;
            const Point3 &lensForward = gunmod__collimator_moa_lens_axis;
            const Point3 lensRight = normalize(wtm % gunmod__collimator_moa_local_right);
            const Point3 lensUp = normalize(wtm % localUp);

            const Point3 &eyeToLensCenter = wtm.col[3];
            const float calibrationRangeMeters = collimator_moa_render__calibration_range_cm * 0.01;
            const Point3 eyeToLenseAlignedOrigin = eyeToLensCenter - calibrationRangeMeters * lensForward;

            const Point3 eyeToPlaneOrigin = eyeToLensCenter + lensForward * gunmod__collimator_moa_parallax_plane_dist;

            ShaderGlobal::set_float4(var::dcm_lens_right, lensRight);
            ShaderGlobal::set_float4(var::dcm_lens_up, lensUp);
            ShaderGlobal::set_float4(var::dcm_lens_forward, lensForward);
            ShaderGlobal::set_float4(var::dcm_eye_to_lens_aligned_origin, eyeToLenseAlignedOrigin);
            ShaderGlobal::set_float4(var::dcm_eye_to_plane_origin, eyeToPlaneOrigin);

            ShaderGlobal::set_float4(var::dcm_lens_color_tint, gunmod__collimator_lens_color);
          });
      }
    });
  return relemData;
}

void render(const IPoint2 &display_resolution)
{
  render_collimator_moa_lens_ecs_query(*g_entity_mgr,
    [&](const int collimator_moa_render__active_shapes_count, const int collimator_moa_render__shapes_buf_reg_count,
      const UniqueBufWithShaderVar &collimator_moa_render__current_shapes_buf,
      const ecs::EntityId &collimator_moa_render__active_image_eid, const ShadersECS &collimator_moa_render__lens_shader) {
      const LensRelemData relemData = setup_render();
      if (relemData && collimator_moa_render__current_shapes_buf && collimator_moa_render__lens_shader)
      {
        ShaderGlobal::set_int(var::dcm_render_mode, RENDER_LENS);

        ShaderGlobal::set_int(var::dcm_shapes_count, collimator_moa_render__active_shapes_count);
        ShaderGlobal::set_int(var::dcm_shapes_buf_reg_count, collimator_moa_render__shapes_buf_reg_count);
        ShaderGlobal::set_buffer(var::dcm_shapes_buf, collimator_moa_render__current_shapes_buf);
        setup_image_render(collimator_moa_render__active_image_eid);

        IPoint2 res = display_resolution;
        ShaderGlobal::set_float4(var::dcm_inv_screen_size, safeinv(float(res.x)), safeinv(float(res.y)), 0.0f, 0.0f);

        d3d::settm(TM_WORLD, *relemData.wtm);
        relemData.relem->vertexData->setToDriver();
        relemData.relem->renderWithElem(*collimator_moa_render__lens_shader.shElem);
        d3d::settm(TM_WORLD, TMatrix::IDENT);

        ShaderGlobal::set_int(var::dcm_render_mode, DONT_RENDER);
      }
    });
}

} // namespace collimator_moa
