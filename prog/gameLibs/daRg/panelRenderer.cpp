// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/dag_panelRenderer.h>
#include <daRg/dag_guiScene.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_overrideStates.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_mathAng.h>
#include <math/dag_mathUtils.h>
#include <math/dag_bounds3.h>
#include <memory/dag_framemem.h>
#include <util/dag_finally.h>
#include "guiScene.h"
#include "panelRenderer.h"
#include <EASTL/vector_multimap.h>
#include <EASTL/vector_map.h>

namespace darg_panel_renderer
{

enum ShaderTarget
{
  FrameBuffer,
  FrameBufferWithoutDepth,
  Depth,
  GBuffer,

  Count
};

static ShaderTarget to_shader_target(RenderPass render_pass)
{
  switch (render_pass)
  {
    case RenderPass::Translucent: return ShaderTarget::FrameBuffer;

    case RenderPass::TranslucentWithoutDepth: return ShaderTarget::FrameBufferWithoutDepth;

    case RenderPass::Shadow: return ShaderTarget::Depth;

    case RenderPass::GBuffer: return ShaderTarget::GBuffer;

    default:
      G_ASSERTF(false, "Implement finding the shader for this render pass! %d", int(render_pass));
      return ShaderTarget::FrameBuffer;
  }
}

static DynamicShaderHelper panel_shaders[ShaderTarget::Count];

static int panel_textureVarId = -1;
static int panel_brightnessVarId = -1;
static int panel_pointer_texture1VarId = -1;
static int panel_pointer_uv_tl_iwih1VarId = -1;
static int panel_pointer_color1VarId = -1;
static int panel_pointer_texture2VarId = -1;
static int panel_pointer_uv_tl_iwih2VarId = -1;
static int panel_pointer_color2VarId = -1;

static int panel_smoothnessVarId = -1;
static int panel_reflectanceVarId = -1;
static int panel_metalnessVarId = -1;

static int panel_worldVarId = -1;
static int panel_world_normalVarId = -1;

static VDECL panel_vertex_declaration = BAD_VDECL;
static int panel_vertex_stride = -1;
static UniqueBuf panel_vertex_buffers[1];
static UniqueBuf panel_index_buffers[1];
static int panel_triangle_counts[1] = {0};

static eastl::vector_map<shaders::OverrideStateId, shaders::OverrideStateId> zOverrides;

struct PanelVertex
{
  Point3 position;
  Point3 normal;
  Point2 texcoord;
};

static const uint16_t rectIndices[] = {0, 1, 2, 1, 3, 2};
static const PanelVertex rectVertices[4] = {
  PanelVertex{Point3(0.5f, 0.5f, 0), Point3(0, 0, 1), Point2(1, 0)},
  PanelVertex{Point3(-0.5f, 0.5f, 0), Point3(0, 0, 1), Point2(0, 0)},
  PanelVertex{Point3(0.5f, -0.5f, 0), Point3(0, 0, 1), Point2(1, 1)},
  PanelVertex{Point3(-0.5f, -0.5f, 0), Point3(0, 0, 1), Point2(0, 1)},
};

template <typename Func, typename T, size_t count>
static bool buildBuffer(Func createFunc, UniqueBuf &buffer, const T (&data)[count], const char *name)
{
  buffer = createFunc(sizeof(data), SBCF_CPU_ACCESS_WRITE, name);
  G_ASSERT_RETURN(buffer.getBuf(), false);
  G_ASSERT_RETURN(buffer.getBuf()->updateData(0, sizeof(data), data, VBLOCK_WRITEONLY), false);
  return true;
}

static bool initialize_rendering_resources()
{
  static CompiledShaderChannelId channels[] = {
    {SCTYPE_FLOAT3, SCUSAGE_POS, 0, 0}, {SCTYPE_FLOAT3, SCUSAGE_NORM, 0, 0}, {SCTYPE_FLOAT2, SCUSAGE_TC, 0, 0}};

  if (!panel_shaders[0].shader)
  {
    panel_shaders[ShaderTarget::FrameBuffer].init("darg_panel_frame_buffer", channels, countof(channels), "darg_panel_frame_buffer",
      true);
    panel_shaders[ShaderTarget::FrameBufferWithoutDepth].init("darg_panel_frame_buffer_no_depth", channels, countof(channels),
      "darg_panel_frame_buffer_no_depth", true);
    panel_shaders[ShaderTarget::Depth].init("darg_panel_depth", channels, countof(channels), "darg_panel_depth", true);
    panel_shaders[ShaderTarget::GBuffer].init("darg_panel_gbuffer", channels, countof(channels), "darg_panel_gbuffer", true);

    panel_textureVarId = ::get_shader_glob_var_id("panel_texture", true);
    panel_brightnessVarId = ::get_shader_glob_var_id("panel_brightness", true);
    panel_pointer_texture1VarId = ::get_shader_glob_var_id("panel_pointer_texture1", true);
    panel_pointer_uv_tl_iwih1VarId = ::get_shader_glob_var_id("panel_pointer_uv_tl_iwih1", true);
    panel_pointer_color1VarId = ::get_shader_glob_var_id("panel_pointer_color1", true);
    panel_pointer_texture2VarId = ::get_shader_glob_var_id("panel_pointer_texture2", true);
    panel_pointer_uv_tl_iwih2VarId = ::get_shader_glob_var_id("panel_pointer_uv_tl_iwih2", true);
    panel_pointer_color2VarId = ::get_shader_glob_var_id("panel_pointer_color2", true);
    panel_smoothnessVarId = ::get_shader_glob_var_id("panel_smoothness", true);
    panel_reflectanceVarId = ::get_shader_glob_var_id("panel_reflectance", true);
    panel_metalnessVarId = ::get_shader_glob_var_id("panel_metalness", true);
    panel_worldVarId = ::get_shader_glob_var_id("panel_world", true);
    panel_world_normalVarId = ::get_shader_glob_var_id("panel_world_normal", true);
  }

  if (panel_vertex_declaration == BAD_VDECL)
  {
    panel_vertex_declaration = dynrender::addShaderVdecl(channels, countof(channels));
    panel_vertex_stride = dynrender::getStride(channels, countof(channels));
    G_ASSERT_RETURN(panel_vertex_declaration != BAD_VDECL, false);
  }

  int geomIndex = int(darg::PanelGeometry::Rectangle);
  if (!panel_vertex_buffers[geomIndex].getBuf())
    G_ASSERT_RETURN(buildBuffer(dag::create_vb, panel_vertex_buffers[geomIndex], rectVertices, "darg_panel_rect_vb"), false);
  if (!panel_index_buffers[geomIndex].getBuf())
    G_ASSERT_RETURN(buildBuffer(dag::create_ib, panel_index_buffers[geomIndex], rectIndices, "darg_panel_rect_ib"), false);
  panel_triangle_counts[geomIndex] = countof(rectIndices) / 3;

  return true;
}

static TMatrix build_panel_transform(const darg::PanelSpatialInfo &info)
{
  Quat rotation;
  euler_to_quat(DegToRad(info.angles.y), DegToRad(info.angles.x), DegToRad(info.angles.z), rotation);

  TMatrix matRotation = quat_to_matrix(rotation);
  TMatrix matScale = TMatrix::IDENT;
  matScale[0][0] = info.size.x;
  matScale[1][1] = info.size.y;
  matScale[2][2] = info.size.z;
  TMatrix matTranslation = TMatrix::IDENT;
  matTranslation.setcol(3, info.position);

  return matTranslation * matRotation * matScale;
}

TMatrix orient_toward(const Point3 &point, const Point3 &size, const Point3 &position, bool lockY = false)
{
  TMatrix result;

  Point3 x, y, z;

  if (lockY)
  {
    y = Point3(0, 1, 0);
    z = point - position;
    z.normalize();
    x = z % y;
    x.normalize();
    z = y % x;
  }
  else
  {
    z = point - position;
    z.normalize();
    x = z % Point3(0, 1, 0);
    x.normalize();
    y = x % z;
  }

  result.setcol(0, x * size.x);
  result.setcol(1, y * size.y);
  result.setcol(2, z * size.z);
  result.setcol(3, position);

  return result;
};

static BBox3 calculate_aabb(darg::PanelSpatialInfo &info, const TMatrix &transform)
{
  switch (info.geometry)
  {
    case darg::PanelGeometry::Rectangle:
    {
      Point3 a = rectVertices[0].position * transform;
      Point3 b = rectVertices[1].position * transform;
      Point3 c = rectVertices[2].position * transform;
      Point3 d = rectVertices[3].position * transform;
      return BBox3(min(min(a, b), min(c, d)), max(max(a, b), max(c, d)));
    }
    case darg::PanelGeometry::None: return BBox3();

    default:
      G_ASSERTF(false, "Implement calculating the bounding box for this type of geometry! %d", int(info.geometry));
      return BBox3();
  }
}

static void clip_panel(const Frustum &camera_frustum, darg::PanelSpatialInfo &info, const TMatrix &transform)
{
  auto bounding = calculate_aabb(info, transform);
  info.visible = bounding.width().length() > 0.001 && camera_frustum.testBox(bounding) != Frustum::OUTSIDE;
}

void panel_spatial_resolver(const TMatrix &vr_space, const TMatrix &camera, const TMatrix &left_hand_transform,
  const TMatrix &right_hand_transform, const Frustum &camera_frustum, darg::PanelSpatialInfo &info, EntityResolver entity_resolver,
  TMatrix &out_transform)
{
  FINALLY([&]() { clip_panel(camera_frustum, info, out_transform); });

  TMatrix panelTransform = build_panel_transform(info);

  switch (info.anchor)
  {
    case darg::PanelAnchor::Scene: break;

    case darg::PanelAnchor::VRSpace: panelTransform = vr_space * panelTransform; break;

    case darg::PanelAnchor::Head: panelTransform = camera * panelTransform; break;

    case darg::PanelAnchor::LeftHand: panelTransform = left_hand_transform * panelTransform; break;

    case darg::PanelAnchor::RightHand: panelTransform = right_hand_transform * panelTransform; break;

    case darg::PanelAnchor::Entity:
      panelTransform = entity_resolver(info.anchorEntityId, info.anchorNodeName.data()) * panelTransform;
      break;

    default: DAG_FATAL("This PanelAnchor type should be handled here. %d", int(info.anchor)); return;
  }

  // If all axes are locked, it means that the rotation of the panel is locked, and its orientation
  // is controlled entirely by its rotation angles
  if (info.constraint == darg::PanelRotationConstraint::None)
  {
    out_transform = panelTransform;
    return;
  }

  Point3 panelPosition = panelTransform.getcol(3);

  TMatrix orientTransform;
  switch (info.constraint)
  {
    case darg::PanelRotationConstraint::FaceLeftHand:
      orientTransform = orient_toward(left_hand_transform.getcol(3), info.size, panelPosition);
      break;

    case darg::PanelRotationConstraint::FaceRightHand:
      orientTransform = orient_toward(right_hand_transform.getcol(3), info.size, panelPosition);
      break;

    case darg::PanelRotationConstraint::FaceHead: orientTransform = orient_toward(camera.getcol(3), info.size, panelPosition); break;

    case darg::PanelRotationConstraint::FaceHeadLockY:
      orientTransform = orient_toward(camera.getcol(3), info.size, panelPosition, true);
      break;

    case darg::PanelRotationConstraint::FaceEntity:
      orientTransform = orient_toward(entity_resolver(info.facingEntityId, nullptr).getcol(3), info.size, panelPosition, true);
      break;

    default: DAG_FATAL("This PanelRotationConstraint type should be handled here. %d", int(info.constraint)); return;
  }

  out_transform = orientTransform;
}

void render_panels_in_world(const darg::IGuiScene &scene_, const Point3 &view_point, RenderPass render_pass)
{
  using namespace darg;

  const darg::GuiScene &scene = static_cast<const darg::GuiScene &>(scene_);
  const auto &panels = scene.getPanels();

  if (panels.empty())
    return;

  TIME_D3D_PROFILE(render_panels_in_world);

  eastl::vector_multimap<float, const PanelData *, eastl::less<float>, framemem_allocator> sortedPanels;

  // sort panels and calculate their transforms
  for (const auto &itPanelData : panels)
  {
    const PanelData *panelData = itPanelData.second.get();
    if (panelData->panel->spatialInfo.visible && panelData->isInThisPass(render_pass) && panelData->panel->renderInfo.isValid)
    {
      float distSq = (view_point - panelData->panel->renderInfo.transform.getcol(3)).lengthSq();
      sortedPanels.insert({distSq, panelData});
    }
  }

  G_ASSERT_RETURN(initialize_rendering_resources(), );

  if (!panel_shaders[to_shader_target(render_pass)].shader)
    return;

  d3d::setvdecl(panel_vertex_declaration);

  TMatrix oldWorldTm, oldViewTm;
  d3d::gettm(TM_WORLD, oldWorldTm);
  d3d::gettm(TM_VIEW, oldViewTm);

  TMatrix adjustedViewTm{oldViewTm};
  adjustedViewTm.setcol(3, {0.f, 0.f, 0.f});
  d3d::settm(TM_VIEW, adjustedViewTm);

  int oldFrameBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

  // In case of shadow rendering, we take the override which is already set, and if
  // there is one, we double the depth bias values. This is needed as the panels are
  // thin, flat surfaces, usually close to the camera, and constantly in motion. As
  // a result, they have banding all over them.
  shaders::OverrideStateId oldOverride;
  if (render_pass == RenderPass::Shadow)
  {
    oldOverride = shaders::overrides::get_current();
    if (oldOverride)
    {
      shaders::OverrideStateId modifiedOverride;

      auto iter = zOverrides.find(oldOverride);
      if (iter == zOverrides.end())
      {
        shaders::OverrideState oldState = shaders::overrides::get(oldOverride);
        oldState.zBias *= 2;
        oldState.slopeZBias *= 2;
        modifiedOverride = shaders::overrides::create(oldState);
        zOverrides[oldOverride] = modifiedOverride;
      }
      else
        modifiedOverride = iter->second;

      shaders::overrides::reset();
      shaders::overrides::set(modifiedOverride);
    }
  }

  for (auto iter = sortedPanels.rbegin(); iter != sortedPanels.rend(); ++iter)
  {
    TIME_D3D_PROFILE(render_one_panel);

    const PanelData &panelData = *iter->second;
    const PanelSpatialInfo &spatialInfo = panelData.panel->spatialInfo;
    const PanelRenderInfo &renderInfo = panelData.panel->renderInfo;
    int geomIndex = int(spatialInfo.geometry);

    G_ASSERTF_CONTINUE(geomIndex >= 0 && geomIndex < countof(panel_vertex_buffers),
      "This PanelGeometry type should be handled here. %d", int(spatialInfo.geometry));

    d3d::setvsrc(0, panel_vertex_buffers[geomIndex].getBuf(), panel_vertex_stride);
    d3d::setind(panel_index_buffers[geomIndex].getBuf());

    TMatrix adjustedTm{renderInfo.transform};
    adjustedTm.setcol(3, renderInfo.transform.getcol(3) - view_point);
    d3d::settm(TM_WORLD, adjustedTm);

    if (render_pass == RenderPass::GBuffer)
      spatialInfo.lastTransform = renderInfo.transform;

    TMatrix adjustedNormalTm{renderInfo.transform};
    adjustedNormalTm.setcol(0, normalize(adjustedNormalTm.getcol(0)));
    adjustedNormalTm.setcol(1, normalize(adjustedNormalTm.getcol(1)));
    adjustedNormalTm.setcol(2, normalize(adjustedNormalTm.getcol(2)));

    ShaderGlobal::set_texture(panel_textureVarId, renderInfo.texture);
    ShaderGlobal::set_real(panel_brightnessVarId, renderInfo.brightness);
    ShaderGlobal::set_texture(panel_pointer_texture1VarId, renderInfo.pointerTexture[0]);
    ShaderGlobal::set_color4(panel_pointer_color1VarId, renderInfo.pointerColor[0]);
    ShaderGlobal::set_color4(panel_pointer_uv_tl_iwih1VarId, renderInfo.pointerUVLeftTop[0].x, renderInfo.pointerUVLeftTop[0].y,
      renderInfo.pointerUVInvSize[0].x, renderInfo.pointerUVInvSize[0].y);
    ShaderGlobal::set_texture(panel_pointer_texture2VarId, renderInfo.pointerTexture[1]);
    ShaderGlobal::set_color4(panel_pointer_color2VarId, renderInfo.pointerColor[1]);
    ShaderGlobal::set_color4(panel_pointer_uv_tl_iwih2VarId, renderInfo.pointerUVLeftTop[1].x, renderInfo.pointerUVLeftTop[1].y,
      renderInfo.pointerUVInvSize[1].x, renderInfo.pointerUVInvSize[1].y);

    TMatrix4 worldTm(adjustedTm);
    TMatrix4 worldNormalTm(adjustedNormalTm);

    ShaderGlobal::set_real(panel_smoothnessVarId, renderInfo.smoothness);
    ShaderGlobal::set_real(panel_reflectanceVarId, renderInfo.reflectance);
    ShaderGlobal::set_real(panel_metalnessVarId, renderInfo.metalness);
    ShaderGlobal::set_float4x4(panel_worldVarId, worldTm);
    ShaderGlobal::set_float4x4(panel_world_normalVarId, worldNormalTm);

    panel_shaders[to_shader_target(render_pass)].shader->setStates();

    d3d::drawind(PRIM_TRILIST, 0, panel_triangle_counts[geomIndex], 0);
  }

  ShaderGlobal::setBlock(oldFrameBlock, ShaderGlobal::LAYER_FRAME);

  d3d::settm(TM_VIEW, oldViewTm);
  d3d::settm(TM_WORLD, oldWorldTm);

  if (render_pass == RenderPass::Shadow)
  {
    shaders::overrides::reset();
    if (oldOverride)
      shaders::overrides::set(oldOverride);
  }
}

void clean_up()
{
  for (auto &shader : panel_shaders)
    shader.close();
  for (auto &buffer : panel_vertex_buffers)
    buffer.close();
  for (auto &buffer : panel_index_buffers)
    buffer.close();
  for (auto &tc : panel_triangle_counts)
    tc = 0;
  for (auto &iter : zOverrides)
    shaders::overrides::destroy(iter.second);
  zOverrides.clear();

  panel_vertex_declaration = BAD_VDECL;
  panel_vertex_stride = -1;
}

static void darg_panels_after_reset_device(bool)
{
  int geomIndex = int(darg::PanelGeometry::Rectangle);
  panel_vertex_buffers[geomIndex].close();
  panel_index_buffers[geomIndex].close();
}

REGISTER_D3D_AFTER_RESET_FUNC(darg_panels_after_reset_device);

} // namespace darg_panel_renderer
