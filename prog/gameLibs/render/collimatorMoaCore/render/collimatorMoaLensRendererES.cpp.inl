// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <ecs/anim/anim.h>
#include <ecs/render/resPtr.h>
#include <ecs/render/shaderVar.h>
#include <render/collimatorMoaCore/render.h>

#include <3d/dag_resPtr.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <shaders/dag_dynSceneRes.h>


template <typename Callable>
inline void render_collimator_moa_lens_ecs_query(Callable c);
template <typename Callable>
inline void setup_collimator_moa_render_ecs_query(Callable c);
template <typename Callable>
inline void setup_collimator_moa_lens_ecs_query(ecs::EntityId, Callable c);

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
const static ShaderVariableInfo dcm_dynamic_noise_intensity_scale("dcm_dynamic_noise_intensity_scale");
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

namespace collimator_moa
{
static LensRelemData setup_render()
{
  LensRelemData relemData;
  setup_collimator_moa_render_ecs_query(
    [&](const ecs::EntityId &collimator_moa_render__gun_mod_eid, const int collimator_moa_render__rigid_id,
      const int collimator_moa_render__relem_id, const float collimator_moa_render__calibration_range_cm) {
      if (collimator_moa_render__gun_mod_eid)
      {
        setup_collimator_moa_lens_ecs_query(collimator_moa_render__gun_mod_eid,
          [&](const AnimV20::AnimcharRendComponent &animchar_render, const float gunmod__collimator_moa_parallax_plane_dist,
            const Point4 &gunmod__collimator_moa_color, const float gunmod__collimator_moa_border_min,
            const float gunmod__collimator_moa_border_scale, const bool gunmod__collimator_moa_use_noise,
            const float gunmod__collimator_moa_noise_min_intensity, const float gunmod__collimator_moa_light_noise_thinness,
            const float gunmod__collimator_moa_light_noise_intensity_scale, const float gunmod__collimator_moa_static_noise_uv_scale,
            const float gunmod__collimator_moa_static_noise_add, const float gunmod__collimator_moa_static_noise_scale,
            const float gunmod__collimator_moa_dynamic_noise_uv_scale, const float gunmod__collimator_moa_dynamic_noise_sub_scale,
            const float gunmod__collimator_moa_dynamic_noise_scale, const float gunmod__collimator_moa_dynamic_noise_add,
            const float gunmod__collimator_moa_dynamic_noise_intensity_scale, const Point2 &gunmod__collimator_moa_dynamic_noise_speed,
            const float gunmod__collimator_moa_reticle_offset_y) {
            relemData = get_lens_relem_data(animchar_render, collimator_moa_render__rigid_id, collimator_moa_render__relem_id);

            if (!relemData)
              return;

            const TMatrix *wtm = relemData.wtm;
            const Point3 lensRight = normalize(-wtm->col[0]);
            const Point3 lensUp = normalize(wtm->col[2]);
            const Point3 lensForward = normalize(-wtm->col[1]);

            const Point3 &eyeToLensCenter = wtm->col[3] + gunmod__collimator_moa_reticle_offset_y * lensUp;
            const float calibrationRangeMeters = collimator_moa_render__calibration_range_cm * 0.01;
            const Point3 eyeToLenseAlignedOrigin = eyeToLensCenter - calibrationRangeMeters * lensForward;

            const Point3 eyeToPlaneOrigin = eyeToLensCenter + lensForward * gunmod__collimator_moa_parallax_plane_dist;

            ShaderGlobal::set_color4(var::dcm_lens_right, lensRight);
            ShaderGlobal::set_color4(var::dcm_lens_up, lensUp);
            ShaderGlobal::set_color4(var::dcm_lens_forward, lensForward);
            ShaderGlobal::set_color4(var::dcm_eye_to_lens_aligned_origin, eyeToLenseAlignedOrigin);
            ShaderGlobal::set_color4(var::dcm_eye_to_plane_origin, eyeToPlaneOrigin);

            ShaderGlobal::set_color4(var::dcm_color, gunmod__collimator_moa_color);
            ShaderGlobal::set_real(var::dcm_border_min, gunmod__collimator_moa_border_min);
            ShaderGlobal::set_real(var::dcm_border_scale, gunmod__collimator_moa_border_scale);

            ShaderGlobal::set_real(var::dcm_use_noise, gunmod__collimator_moa_use_noise ? 1.0f : 0.0f);
            if (gunmod__collimator_moa_use_noise)
            {
              ShaderGlobal::set_real(var::dcm_noise_min_intensity, gunmod__collimator_moa_noise_min_intensity);

              ShaderGlobal::set_real(var::dcm_light_noise_thinness, gunmod__collimator_moa_light_noise_thinness);
              ShaderGlobal::set_real(var::dcm_light_noise_intensity_scale, gunmod__collimator_moa_light_noise_intensity_scale);

              ShaderGlobal::set_real(var::dcm_static_noise_uv_scale, gunmod__collimator_moa_static_noise_uv_scale);
              ShaderGlobal::set_real(var::dcm_static_noise_add, gunmod__collimator_moa_static_noise_add);
              ShaderGlobal::set_real(var::dcm_static_noise_scale, gunmod__collimator_moa_static_noise_scale);

              ShaderGlobal::set_real(var::dcm_dynamic_noise_uv_scale, gunmod__collimator_moa_dynamic_noise_uv_scale);
              ShaderGlobal::set_real(var::dcm_dynamic_noise_sub_scale, gunmod__collimator_moa_dynamic_noise_sub_scale);
              ShaderGlobal::set_real(var::dcm_dynamic_noise_scale, gunmod__collimator_moa_dynamic_noise_scale);
              ShaderGlobal::set_real(var::dcm_dynamic_noise_add, gunmod__collimator_moa_dynamic_noise_add);
              ShaderGlobal::set_real(var::dcm_dynamic_noise_intensity_scale, gunmod__collimator_moa_dynamic_noise_intensity_scale);
              ShaderGlobal::set_color4(var::dcm_dynamic_noise_speed, gunmod__collimator_moa_dynamic_noise_speed);
            }
          });
      }
    });
  return relemData;
}

void render(const IPoint2 &display_resolution)
{
  render_collimator_moa_lens_ecs_query(
    [&](const int collimator_moa_render__active_shapes_count, const int collimator_moa_render__shapes_buf_reg_count,
      const UniqueBufHolder &collimator_moa_render__current_shapes_buf) {
      const LensRelemData relemData = setup_render();
      if (relemData && collimator_moa_render__current_shapes_buf)
      {
        ShaderGlobal::set_int(var::dcm_render_mode, RENDER_LENS);

        ShaderGlobal::set_int(var::dcm_shapes_count, collimator_moa_render__active_shapes_count);
        ShaderGlobal::set_int(var::dcm_shapes_buf_reg_count, collimator_moa_render__shapes_buf_reg_count);
        ShaderGlobal::set_buffer(var::dcm_shapes_buf, collimator_moa_render__current_shapes_buf);

        IPoint2 res = display_resolution;
        ShaderGlobal::set_color4(var::dcm_inv_screen_size, safeinv(float(res.x)), safeinv(float(res.y)), 0.0f, 0.0f);

        d3d::settm(TM_WORLD, *relemData.wtm);
        relemData.relem->vertexData->setToDriver();
        relemData.relem->render();
        d3d::settm(TM_WORLD, TMatrix::IDENT);

        ShaderGlobal::set_int(var::dcm_render_mode, DONT_RENDER);
      }
    });
}

} // namespace collimator_moa
