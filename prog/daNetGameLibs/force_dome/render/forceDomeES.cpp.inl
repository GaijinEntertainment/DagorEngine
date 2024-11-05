// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/render/updateStageRender.h>
#include <daECS/core/coreEvents.h>

#include <scene/dag_occlusion.h>
#include <render/primitiveObjects.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_lockSbuffer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <force_dome/shaders/force_dome_const.hlsli>
#include <math/dag_mathUtils.h>
#include "render/renderEvent.h"
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>


struct ForceDomeResources
{
  DynamicShaderHelper forceDomeShader;
  UniqueBuf sphereVb;
  UniqueBuf sphereIb;

  uint32_t vertexCount, faceCount;

  shaders::OverrideStateId insideSphereState, outsideSphereState;
  UniqueBufHolder spheresBuf;

  enum
  {
    SLICES = 50
  };

  ForceDomeResources()
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::Z_CLAMP_ENABLED);
    insideSphereState = shaders::overrides::create(state);
    state.set(shaders::OverrideState::FLIP_CULL);
    outsideSphereState = shaders::overrides::create(state);

    spheresBuf = dag::buffers::create_one_frame_cb(FORCE_DOME_MAX_INSTANCE, "forcedome_spheres");

    forceDomeShader.init("forcedome", nullptr, 0, "forcedome", true);

    calc_sphere_vertex_face_count(SLICES, SLICES, false, vertexCount, faceCount);

    sphereVb = dag::create_vb(vertexCount * sizeof(Point3), 0, "sphere");
    sphereIb = dag::create_ib(faceCount * 6, 0, "sphereIb");

    fillBuffers();
  }

  void fillBuffers()
  {
    auto indices = lock_sbuffer<uint8_t>(sphereIb.getBuf(), 0, 6 * faceCount, VBLOCK_WRITEONLY);
    auto vertices = lock_sbuffer<uint8_t>(sphereVb.getBuf(), 0, vertexCount * sizeof(Point3), VBLOCK_WRITEONLY);
    if (!indices || !vertices)
      return;

    create_sphere_mesh(dag::Span<uint8_t>(vertices.get(), vertexCount * sizeof(Point3)),
      dag::Span<uint8_t>(indices.get(), faceCount * 6), 1.0f, SLICES, SLICES, sizeof(Point3), false, false, false, false);
  }

  ~ForceDomeResources()
  {
    shaders::overrides::destroy(insideSphereState);
    shaders::overrides::destroy(outsideSphereState);
  }

  void setInsideSphereState() { shaders::overrides::set(insideSphereState); }

  void setOutsideSphereState()
  {
    shaders::overrides::reset();
    shaders::overrides::set(outsideSphereState);
  }

  void resetState() { shaders::overrides::reset(); }

  void render(const Point4 *sph, int count)
  {
    for (int i = 0; i < count; i += FORCE_DOME_MAX_INSTANCE, sph += FORCE_DOME_MAX_INSTANCE)
    {
      int renderCount = min(count - i, (int)FORCE_DOME_MAX_INSTANCE);
      spheresBuf->updateData(0, sizeof(Point4) * renderCount, &sph->x, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
      forceDomeShader.shader->setStates(0, true);
      d3d::setind(sphereIb.getBuf());
      d3d::setvsrc(0, sphereVb.getBuf(), sizeof(Point3));
      d3d::drawind_instanced(PRIM_TRILIST, 0, faceCount, 0, renderCount);
    }
  }

  void setParams(const Point4 &color, float texture_scale, float brightness, float edge_thinness, float edge_intensity)
  {
    ShaderGlobal::set_color4(get_shader_variable_id("forcedome_color", true), color);
    ShaderGlobal::set_real(get_shader_variable_id("forcedome_texture_scale", true), texture_scale);
    ShaderGlobal::set_real(get_shader_variable_id("forcedome_brightness", true), brightness);
    ShaderGlobal::set_real(get_shader_variable_id("forcedome_edge_thinness", true), edge_thinness);
    ShaderGlobal::set_real(get_shader_variable_id("forcedome_edge_intensity", true), edge_intensity);
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(ForceDomeResources);
ECS_REGISTER_RELOCATABLE_TYPE(ForceDomeResources, nullptr);

ECS_ON_EVENT(on_appear)
void force_dome_resources_created_es(const ecs::Event &,
  ForceDomeResources &force_dome_resources,
  const Point4 &force_dome__color,
  float force_dome__texture_scale,
  float force_dome__brightness,
  float force_dome__edge_thinness,
  float force_dome__edge_intensity)
{
  force_dome_resources.setParams(force_dome__color, force_dome__texture_scale, force_dome__brightness, force_dome__edge_thinness,
    force_dome__edge_intensity);
}

ECS_TAG(render)
ECS_TRACK(*)
void force_dome_resources_updated_es(const ecs::Event &,
  ForceDomeResources &force_dome_resources,
  const Point4 &force_dome__color,
  float force_dome__texture_scale,
  float force_dome__brightness,
  float force_dome__edge_thinness,
  float force_dome__edge_intensity)
{
  force_dome_resources.setParams(force_dome__color, force_dome__texture_scale, force_dome__brightness, force_dome__edge_thinness,
    force_dome__edge_intensity);
}

// Calculates radius from which we start to draw back side of the polygonal sphere.
bool force_dome_is_camera_outside_sphere_mesh(
  Point3 relCamPos, float sphR, uint32_t slices /* count of meridians */, uint32_t stacks /* count of parallels + 1 */)
{
  float stackAngle = PI / stacks;
  float camH = relCamPos.y;
  float camDistSq = lengthSq(relCamPos);
  float camDist = sqrtf(camDistSq);
  // Check if camera closer than the radius of the inscribed sphere of an octahedron (very a low polygon sphere)
  // Some check is required anyway to avoid division by 0
  if (camDist < sphR * 0.408f)
    return false;
  float camAngleH = safe_acos(camH / camDist); // Angle between cam and positive Y.
  float camAngleHTrunc = fmod(camAngleH, stackAngle);
  // Flat radius at cam height.
  float halfStackAngle = stackAngle * 0.5f;
  float rH = sphR * cos(halfStackAngle) / cos(camAngleHTrunc - halfStackAngle) * sin(camAngleH);

  float sliceAngle = 2.0f * PI / slices;
  Point2 camPosFlat = Point2(relCamPos.x, relCamPos.z);
  float camRad = max(1e-6f, length(camPosFlat));         // to avoid division by 0
  float camAngleFlat = safe_acos(camPosFlat.x / camRad); // Angle between cam and positive XY quad.
  float camAngleTrunc = fmod(camAngleFlat, sliceAngle);
  float halfSliceAngle = sliceAngle * 0.5f;
  float r = rH * cos(halfSliceAngle) / cos(camAngleTrunc - halfSliceAngle);

  float trSq = r * r + camH * camH;
  return camDistSq > trSq;
}

template <typename Callable>
static void force_dome_render_ecs_query(Callable fn);

ECS_BEFORE(animchar_render_trans_es)
void force_dome_field_es(const UpdateStageInfoRenderTrans &stage, ForceDomeResources &force_dome_resources)
{
  TIME_D3D_PROFILE(force_dome);
  eastl::vector<Point4, framemem_allocator> allSpheres, spheresOutside;

  Frustum frustum = stage.loadGlobTm();

  force_dome_render_ecs_query([&](const Point3 &force_dome__position, float force_dome__radius) {
    Point3 center = force_dome__position;
    if (!frustum.testSphereB(center, force_dome__radius))
      return;
    Point4 currSphere = Point4(center.x, center.y, center.z, force_dome__radius);
    bool isCameraOutsideMesh = force_dome_is_camera_outside_sphere_mesh(stage.viewItm.getcol(3) - center, force_dome__radius,
      force_dome_resources.SLICES, force_dome_resources.SLICES);

    const float radiusOffset = 0.02f;
    if (isCameraOutsideMesh)
    {
      currSphere.w -= radiusOffset;
      if (stage.occlusion && !stage.occlusion->isVisibleSphere(v_ldu(&center.x), v_splats(force_dome__radius)))
        return;
      spheresOutside.push_back(currSphere);
    }
    else
    {
      currSphere.w += radiusOffset;
    }
    allSpheres.push_back(currSphere);
  });

  if (allSpheres.empty())
    return;

  {
    force_dome_resources.setInsideSphereState();
    force_dome_resources.render(allSpheres.begin(), allSpheres.size());
    force_dome_resources.resetState();
  }

  if (!spheresOutside.empty())
  {
    force_dome_resources.setOutsideSphereState();
    force_dome_resources.render(spheresOutside.begin(), spheresOutside.size());
    force_dome_resources.resetState();
  }
}

ECS_ON_EVENT(on_appear)
ECS_REQUIRE_NOT(ecs::Tag force_dome__disableChangeTransform)
void force_dome_created_es(const ecs::Event &, float force_dome__radius, const Point3 &force_dome__position, TMatrix &transform)
{
  const float colliderRadius = 150.0f;
  const float r = force_dome__radius / colliderRadius;
  transform.setcol(0, r, 0, 0);
  transform.setcol(1, 0, r, 0);
  transform.setcol(2, 0, 0, r);
  transform.setcol(3, force_dome__position);
}

ECS_ON_EVENT(AfterDeviceReset)
void force_dome_after_reset_es(const ecs::Event &, ForceDomeResources &force_dome_resources) { force_dome_resources.fillBuffers(); }
