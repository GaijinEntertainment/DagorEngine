// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorRenderer.h"

#include <shaders/dag_dynSceneRes.h>
#include <util/dag_threadPool.h>
#include <rendInst/visibility.h>
#include <rendInst/rendInstGenRender.h>
#include <ecs/render/updateStageRender.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/dynModelRenderer.h>
#include <render/world/global_vars.h>
#include <shaders/dag_shaderBlock.h>
#include <render/skies.h>
#include <drv/3d/dag_matricesAndPerspective.h>

#define VAR(a) ShaderVariableInfo a##VarId(#a);
MIRROR_VARS_LIST
#undef VAR

DynamicMirrorRenderer::DynamicMirrorRenderer() {}

void DynamicMirrorRenderer::init()
{
  riVisibility = rendinst::createRIGenVisibility(midmem);
  const bool hqReflection = false;
  const int riLod = hqReflection ? 1 : rendinst::render::MAX_LOD_COUNT_WITH_ALPHA;
  rendinst::setRIGenVisibilityMinLod(riVisibility, rendinst::render::MAX_LOD_COUNT_WITH_ALPHA, riLod);

  auto skies = get_daskies();
  if (skies != nullptr)
  {
    PreparedSkiesParams mirrorSkiesParams;
    mirrorSkiesParams.panoramic = PreparedSkiesParams::Panoramic::OFF;
    mirrorSkiesParams.reprojection = PreparedSkiesParams::Reprojection::ON;
    mirrorSkiesParams.skiesLUTQuality = 1;
    mirrorSkiesParams.scatteringScreenQuality = 1;
    mirrorSkiesParams.scatteringDepthSlices = 8;
    mirrorSkiesParams.transmittanceColorQuality = 1;
    mirrorSkiesParams.scatteringRangeScale = 1;
    mirrorSkiesParams.minScatteringRange = 80000;
    skiesData = skies->createSkiesData("mirror_bounding", mirrorSkiesParams);
  }
  initialized = true;
}

bool DynamicMirrorRenderer::isInitialized() const { return initialized; }

DynamicMirrorRenderer::~DynamicMirrorRenderer()
{
  initialized = false;
  if (riVisibility != nullptr)
    rendinst::destroyRIGenVisibility(riVisibility);
  riVisibility = nullptr;
  if (skiesData != nullptr)
    get_daskies()->destroy_skies_data(skiesData);
  skiesData = nullptr;
}

void DynamicMirrorRenderer::setMirror(ecs::EntityId eid,
  DynamicRenderableSceneInstance *scene_instance,
  const ecs::Point4List *additional_data,
  bool animchar_render_priority)
{
  G_ASSERTF(mirrorEntityId == ecs::EntityId{}, "There is already an active mirror");
  mirrorEntityId = eid;

  animcharMirrorData.needsHighRenderPriority = animchar_render_priority;
  animcharMirrorData.sceneInstance = scene_instance;
  animcharMirrorData.hasAdditionalData = additional_data != nullptr;
  animcharMirrorData.additionalData.resize(additional_data != nullptr ? additional_data->size() : 0);
  if (additional_data != nullptr)
    eastl::copy(additional_data->begin(), additional_data->end(), animcharMirrorData.additionalData.begin());

  int pathFilterSize = scene_instance->getNodeCount();
  mirrors.clear();
  animcharMirrorData.nodeFilter.resize(pathFilterSize);
  eastl::fill(animcharMirrorData.nodeFilter.begin(), animcharMirrorData.nodeFilter.end(), 0);
  const char *mirrorMatName = "dynamic_mirror";
  for (const DynamicRenderableSceneLodsResource::Lod &dynResLod : scene_instance->getLodsResource()->lods)
  {
    DynamicRenderableSceneResource *dynRes = dynResLod.scene;
    G_ASSERT_CONTINUE(dynRes);

    Tab<ShaderMeshResource *> meshesList(tmpmem);
    Tab<int> nodeIdsList(tmpmem);
    Tab<BSphere3> rigidSpheres;
    dynRes->getMeshes(meshesList, nodeIdsList, &rigidSpheres);

    dynRes->findRigidNodesWithMaterials(make_span_const(&mirrorMatName, 1), [&](int nodeId) {
      if (nodeId < 0)
        return;
      MirrorData mirror;
      mirror.rendAnimcharNodeId = nodeId;
      animcharMirrorData.nodeFilter[nodeId] = eastl::numeric_limits<uint8_t>::max();
      bool found = false;
      for (int i = 0; i < nodeIdsList.size(); ++i)
      {
        if (nodeIdsList[i] == nodeId)
        {
          found = true;
          mirror.boundingSphere = rigidSpheres[i];
        }
      }
      if (found)
        mirrors.push_back(eastl::move(mirror));
    });
  }
}

void DynamicMirrorRenderer::unsetMirror(ecs::EntityId eid)
{
  G_ASSERT(mirrorEntityId == eid);
  G_UNUSED(eid);
  mirrorEntityId = ecs::EntityId{};
}

bool DynamicMirrorRenderer::hasMirrorSet() const { return mirrorEntityId != ecs::EntityId{}; }

static class MirrorVisibilityJob final : public cpujobs::IJob
{
public:
  void start(const mat44f &globtm_, const Frustum &frustum_, const Point3 &view_pos, RiGenVisibility *ri_visibility, int height_)
  {
    threadpool::wait(this);
    globtm = globtm_;
    frustum = frustum_;
    viewPos = view_pos;
    visibility = ri_visibility;
    height = height_;
    threadpool::add(this, threadpool::PRIO_LOW);
  }

  const char *getJobName(bool &) const override { return "MirrorVisibilityJob"; }

  void doJob() override
  {
    const float MIN_HEIGHT_IN_PIXELS = 10.f;
    const vec4f threshold = v_div_x(v_set_x(MIN_HEIGHT_IN_PIXELS), v_cvt_vec4f(v_seti_x(height)));
    const vec4f camPosAndThresSq = v_perm_xyzd(v_ldu(&viewPos.x), v_rot_1(v_mul_x(threshold, threshold)));
    const auto angleSizeFilter = [=](vec4f bbmin, vec4f bbmax) {
      bbox3f bbox = {bbmin, bbmax};
      const vec4f to_box = v_length3_sq_x(v_sub(v_bbox3_center(bbox), camPosAndThresSq));
      const vec4f dia = v_bbox3_inner_diameter(bbox);
      return v_test_vec_x_gt(v_mul_x(dia, dia), v_mul_x(to_box, v_perm_wxyz(camPosAndThresSq))) != 0;
    };

    rendinst::prepareRIGenExtraVisibility(globtm, viewPos, *visibility, false, nullptr, {}, rendinst::RiExtraCullIntention::MAIN,
      false, false, false, angleSizeFilter);
    rendinst::prepareRIGenVisibility(frustum, viewPos, visibility, false, nullptr);
  }

private:
  mat44f globtm;
  Frustum frustum;
  Point3 viewPos;
  RiGenVisibility *visibility;
  int height;
} dynamic_mirror_visibility_job;

void DynamicMirrorRenderer::prepareMirrorRIVisibilityAsync(int texture_height)
{
  G_ASSERTF(hasCameraData, "prepareMirrorRIVisibilityAsync() called without calling prepareCameraData() first");
  dynamic_mirror_visibility_job.start(reinterpret_cast<const mat44f &>(currentCameraData.globTm), currentCameraData.frustum,
    currentCameraData.viewPos, riVisibility, texture_height);
}

void DynamicMirrorRenderer::waitStaticsVisibility()
{
  TIME_PROFILE(wait_mirror_visibility);
  threadpool::wait(&dynamic_mirror_visibility_job);
}

void DynamicMirrorRenderer::prepareCameraData(const TMatrix &view_itm, const Driver3dPerspective &persp, const Plane3 &mirror_plane)
{
  hasCameraData = true;
  currentCameraData = get_common_camera_data(view_itm, persp, mirror_plane);
}

void DynamicMirrorRenderer::clearCameraData() { hasCameraData = false; }

DynamicMirrorRenderer::CameraData DynamicMirrorRenderer::get_common_camera_data(
  const TMatrix &view_itm, const Driver3dPerspective &persp, const Plane3 &mirror_plane)
{
  TMatrix mirrorRenderViewItm = view_itm;
  // Original view, but flipped on Y
  mirrorRenderViewItm.setcol(1, -mirrorRenderViewItm.getcol(1));

  auto reflectionTm4 = matrix_reflect(mirror_plane);
  TMatrix reflectionTm;
  for (int i = 0; i < 4; ++i)
    reflectionTm.setcol(i, Point3::xyz(reflectionTm4.getrow(i)));
  auto mirroredViewItm = reflectionTm * view_itm;
  // mirrored globtm won't produce a viable Frustum, so let's flip Y to fix the handedness
  mirroredViewItm.setcol(1, -mirroredViewItm.getcol(1));

  auto mirroredCameraPos = mirroredViewItm.getcol(3);

  const float zNearBias = 0.3f;
  float cameraDist = fabsf(mirror_plane.distance(mirroredCameraPos)) + zNearBias;

  Driver3dPerspective mirrorPersp = Driver3dPerspective(persp.wk, persp.hk, max(persp.zn, cameraDist), persp.zf, persp.ox, persp.oy);
  // Mirrors cannot be in the [mirrorPersp.zn, mirrorPersp.zf] range; Z range is flipped for the special prepass
  Driver3dPerspective prepassPersp = Driver3dPerspective(persp.wk, persp.hk, mirrorPersp.zf, mirrorPersp.zn, persp.ox, persp.oy);
  TMatrix mirroredViewTm;
  mirroredViewTm = orthonormalized_inverse(mirroredViewItm);
  TMatrix4 mirrorProjTm;
  G_ASSERT(d3d::calcproj(mirrorPersp, mirrorProjTm));
  TMatrix4 mirrorGlobTm = TMatrix4(mirroredViewTm) * mirrorProjTm;
  TMatrix4 prepassProjTm;
  G_ASSERT(d3d::calcproj(prepassPersp, prepassProjTm));
  return {mirrorGlobTm, mirrorProjTm, prepassProjTm, mirrorRenderViewItm, mirroredViewItm, mirroredViewTm, Frustum(mirrorGlobTm),
    mirrorPersp, prepassPersp, mirroredCameraPos};
}

void render_dynamic_mirrors(const DPoint3 &main_camera_pos,
  const AnimcharMirrorData &animchar_mirror_data,
  int render_pass,
  bool render_depth,
  TMatrix view_itm,
  const TexStreamingContext &tex_streaming_ctx)
{
  view_itm.setcol(3, view_itm.getcol(3) - main_camera_pos);
  TMatrix viewTm = inverse(view_itm);

  int sceneBlockId = render_depth ? dynamicDepthSceneBlockId : dynamicSceneBlockId;

  ShaderGlobal::set_int(dyn_model_render_passVarId,
    render_depth ? eastl::to_underlying(dynmodel::RenderPass::Depth) : eastl::to_underlying(dynmodel::RenderPass::Color));

  uint8_t renderMask = render_depth ? UpdateStageInfoRender::RENDER_DEPTH : UpdateStageInfoRender::RENDER_MAIN;
  auto &state = dynmodel_renderer::get_immediate_state();

  auto additionalData = animchar_mirror_data.getAdditionalData();
  auto filter = dynmodel_renderer::PathFilterView(animchar_mirror_data.nodeFilter.data(), animchar_mirror_data.nodeFilter.size());
  state.process_animchar(ShaderMesh::STG_opaque, ShaderMesh::STG_opaque, animchar_mirror_data.sceneInstance,
    animchar_additional_data::get_optional_data(additionalData), dynmodel_renderer::NeedPreviousMatrices::No, {}, filter, renderMask,
    animchar_mirror_data.needsHighRenderPriority ? dynmodel_renderer::RenderPriority::HIGH
                                                 : dynmodel_renderer::RenderPriority::DEFAULT,
    nullptr, tex_streaming_ctx);

  if (state.multidrawList.empty() && state.list.empty())
    return;

  const auto *buffer = state.requestBuffer(dynmodel_renderer::get_buffer_type_from_render_pass(render_pass));
  if (buffer != nullptr)
  {
    SCOPE_VIEW_PROJ_MATRIX;
    d3d::settm(TM_VIEW, viewTm);
    state.setVars(buffer->buffer.getBufId());
    SCENE_LAYER_GUARD(sceneBlockId);
    state.render(buffer->curOffset);
  }
}
