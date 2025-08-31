// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "deformHeightmap.h"
#include "shaders/deform_hmap.hlsli"

#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_bounds3.h>
#include <math/dag_e3dColor.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <render/renderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_overrideStates.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <landMesh/biomeQuery.h>
#include <scene/dag_physMat.h>

// for FPS cam check, TODO: not really
#include <ecs/core/entityManager.h>
#include "camera/sceneCam.h"
#include <game/player.h>
#include <game/gameEvents.h>
#include <ioSys/dag_dataBlock.h>
#include "landMesh/biome_query.hlsli"
#include <landMesh/lmeshManager.h>
#include <main/level.h>

// This does not release resources, only for profiling and debugging purposes
CONSOLE_BOOL_VAL("deform_hmap", deform_enabled, true);
CONSOLE_INT_VAL("deform_hmap", edge_raise_forced, -1, -1, 1);
CONSOLE_FLOAT_VAL_MINMAX("deform_hmap", cam_dist_forced, 0, 0, 2.0f);

#define GLOBAL_VARS_LIST                  \
  VAR(deform_hmap_enabled)                \
  VAR(deform_hmap_tex_size)               \
  VAR(deform_hmap_postfx_source_tex)      \
  VAR(deform_hmap_info_tex)               \
  VAR(deform_hmap_reproject_uv_offset)    \
  VAR(deform_hmap_tex)                    \
  VAR(deform_hmap_world_to_uv_scale_bias) \
  VAR(deform_hmap_zn_zf)                  \
  VAR(deform_fps_cam_dist)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

// For console commands
static DeformHeightmap *instance = nullptr;

DeformHeightmap::DeformHeightmap(const DeformHeightmapDesc &desc) :
  texSize(desc.texSize),
  maskTexSize(texSize / DEFORM_HMAP_THREADGROUP_WIDTH),
  numThreadGroupsOnTexXY((texSize + DEFORM_HMAP_THREADGROUP_WIDTH - 1) / DEFORM_HMAP_THREADGROUP_WIDTH),
  numThreadGroupsOnMaskXY((maskTexSize + DEFORM_HMAP_THREADGROUP_WIDTH - 1) / DEFORM_HMAP_THREADGROUP_WIDTH),
  boxSize(desc.rectSize),
  zn(desc.minTerrainHeight),
  zf(desc.maxTerrainHeight),
  raiseEdges(desc.raiseEdges),
  needClearDepth(true),
  prevRtIdx(0),
  currentRtIdx(1),
  prevMaskIdx(0),
  currentMaskIdx(1),
  postFxSrcIdx(0),
  postFxDstIdx(1),
  currentCamPosXZ(0, 0),
  prevCamPosXZ(0, 0),
  fpsCamDist(desc.fpsCamDist)
{
  G_ASSERT(instance == nullptr);
  if (texSize % DEFORM_HMAP_THREADGROUP_WIDTH != 0)
  {
    logerr("DeformHeightmap: 'texSize' (%d) is not divisible by 'DEFORM_HMAP_THREADGROUP_WIDTH' (%d)! "
           "The shaders are intentionally not handling this situation explicitly!",
      texSize, DEFORM_HMAP_THREADGROUP_WIDTH);
  }
  if (maskTexSize % DEFORM_HMAP_THREADGROUP_WIDTH != 0)
  {
    logerr("DeformHeightmap: 'maskTexSize' (%d) is not divisible by 'DEFORM_HMAP_THREADGROUP_WIDTH' (%d)! "
           "The shaders are intentionally not handling this situation explicitly!",
      texSize, DEFORM_HMAP_THREADGROUP_WIDTH);
  }

  for (size_t i = 0; i < depthTextures.size(); i++)
  {
    String texName(64, "deform_hmap_depth_tex_%d", i);
    depthTextures[i] = dag::create_tex(nullptr, texSize, texSize, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, texName);
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
    smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
    d3d::SamplerHandle smp = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(get_shader_variable_id("deform_hmap_postfx_source_tex_samplerstate", true), smp);
    ShaderGlobal::set_sampler(get_shader_variable_id("deform_hmap_tex_samplerstate", true), smp);
    ShaderGlobal::set_sampler(get_shader_variable_id("deform_hmap_info_tex_samplerstate", true), smp);
  }
  for (size_t i = 0; i < postFxTextures.size(); i++)
  {
    String texName(64, "deform_hmap_postfx_tex_%d", i);
    const int flags = TEXCF_UNORDERED | TEXFMT_R8;
    postFxTextures[i] = dag::create_tex(nullptr, texSize, texSize, flags, 1, texName);
    // The correct addr mode would be border, with border color of E3DCOLOR_MAKE(127, 0, 0, 0), but arbitrary border
    // colors are not supported on XB1. This is not a problem since we intentionally never sample out-of-bounds uvs.
  }
  for (size_t i = 0; i < maskGridTextures.size(); i++)
  {
    String texName(64, "deform_hmap_mask_grid_tex_%d", i);
    const int flags = TEXCF_UNORDERED | TEXFMT_R8;
    maskGridTextures[i] = dag::create_tex(nullptr, maskTexSize, maskTexSize, flags, 1, texName);
  }
  for (size_t i = 0; i < deformInfoTextures.size(); i++)
  {
    String texName(64, "deform_hmap_info_tex_%d", i);
    deformInfoTextures[i] = dag::create_tex(nullptr, texSize, texSize, TEXCF_RTARGET | TEXFMT_R8, 1, texName);
  }
  {
    const char *bufName = "deform_hmap_postfx_cell_buf";
    postFxCellBuffer = dag::buffers::create_ua_structured(sizeof(uint32_t), maskTexSize * maskTexSize, bufName);
    bufName = "deform_hmap_clear_cell_buf";
    clearCellBuffer = dag::buffers::create_ua_structured(sizeof(uint32_t), maskTexSize * maskTexSize, bufName);
  }
  {
    const char *bufName = "deform_hmap_indirect_buf";
    // Buffer for 2 sets of indirect args. 1st set is for postfx, 2nd set is for clear
    indirectBuffer = dag::buffers::create_ua_indirect(dag::buffers::Indirect::Dispatch, 2, bufName);
  }

  reprojectPostFx = eastl::make_unique<PostFxRenderer>("deform_hmap_reproject");
  clearMaskCs.reset(new_compute_shader("deform_hmap_clearmask_cs", true));
  deformCs.reset(new_compute_shader("deform_hmap_deform_cs", true));
  clearIndirect.reset(new_compute_shader("deform_hmap_clearindirect_cs", true));
  dispatcherCs.reset(new_compute_shader("deform_hmap_dispatcher_cs", true));
  edgeDetectCs.reset(new_compute_shader("deform_hmap_edge_detect_cs", true));
  blurCs.reset(new_compute_shader("deform_hmap_blur_cs", true));
  clearerCs.reset(new_compute_shader("deform_hmap_clearer_cs", true));

  shaders::OverrideState state = shaders::OverrideState();
  state.set(shaders::OverrideState::Z_FUNC);
  state.zFunc = CMPF_ALWAYS;
  zFuncAlwaysStateId = shaders::overrides::create(state);

#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR

  ShaderGlobal::set_int(deform_hmap_tex_sizeVarId, texSize);
  ShaderGlobal::set_color4(deform_hmap_zn_zfVarId, zn, zf, 0, 0);

  iViewTm.identity();
  viewTm.identity();
  projTm.identity();

#define VAR(a) &&VariableMap::isGlobVariablePresent(a##VarId)
  bool isValid = reprojectPostFx && reprojectPostFx->getElem() != nullptr && clearMaskCs && deformCs && clearIndirect &&
                 dispatcherCs && edgeDetectCs && blurCs && clearerCs GLOBAL_VARS_LIST;
#undef VAR

  if (isValid)
    instance = this;
  else
    logerr("DeformHeightmap: Not all deform hmap related shaders and shadervars were found, initialization failed.");

  const int physMatCount = PhysMat::physMatCount();
  const PhysMat::MaterialData &defaultPhysMat = PhysMat::getMaterial("default");
  const Point2 &defaultVehicleHmapDeformParams = defaultPhysMat.vehicleHeightmapDeformation;
  dag::Vector<Point4, framemem_allocator> deformParams(physMatCount,
    {defaultVehicleHmapDeformParams.x, defaultVehicleHmapDeformParams.y, defaultPhysMat.humanHeightmapDeformation, 0});
  for (int physmatId = 0; physmatId < physMatCount; physmatId++)
  {
    const PhysMat::MaterialData &currentPhysMat = PhysMat::getMaterial(physmatId);
    const Point2 &vehicleHmapDeformParams = currentPhysMat.vehicleHeightmapDeformation;
    deformParams[physmatId] = {vehicleHmapDeformParams.x, vehicleHmapDeformParams.y, currentPhysMat.humanHeightmapDeformation, 0};
  }

  hmapDeformParamsBuffer = dag::buffers::create_persistent_sr_structured(sizeof(Point4), physMatCount, "hmap_deform_params_buffer");
  hmapDeformParamsBuffer.getBuf()->updateData(0, (uint32_t)(physMatCount * sizeof(Point4)), deformParams.data(), VBLOCK_WRITEONLY);
}

DeformHeightmap::~DeformHeightmap() { instance = nullptr; }

bool DeformHeightmap::isValid() const { return instance != nullptr; }

bool DeformHeightmap::isEnabled() const { return deform_enabled.get(); }

void DeformHeightmap::clear()
{
  for (size_t i = 0; i < depthTextures.size(); i++)
  {
    d3d::clear_rt({deformInfoTextures[i].getTex2D()});
    d3d::clear_rt({depthTextures[i].getTex2D()});
  }

  for (size_t i = 0; i < depthTextures.size(); i++)
    if (currentRtIdx != i)
      d3d::resource_barrier({depthTextures[i].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  float clearVal[4] = {0.5f, 0.f, 0.f, 0.f};
  for (size_t i = 0; i < postFxTextures.size(); i++)
  {
    d3d::clear_rwtexf(postFxTextures[i].getTex2D(), clearVal, 0, 0);
    d3d::resource_barrier({postFxTextures[i].getTex2D(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
  }

  clearVal[0] = 1.f;
  for (size_t i = 0; i < maskGridTextures.size(); i++)
  {
    d3d::clear_rwtexf(maskGridTextures[i].getTex2D(), clearVal, 0, 0);
    d3d::resource_barrier({maskGridTextures[i].getTex2D(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
  }
}

Point4 DeformHeightmap::getDeformRect() const
{
  float halfBoxSize = boxSize * 0.5f;
  return Point4(currentCamPosXZ.x - halfBoxSize, currentCamPosXZ.y - halfBoxSize, currentCamPosXZ.x + halfBoxSize,
    currentCamPosXZ.y + halfBoxSize);
}

bool DeformHeightmap::isCameraInFpsView() const
{
  if (is_free_camera_enabled())
    return false;

  if (!g_entity_mgr->getOr(get_cur_cam_entity(), ECS_HASH("isTpsView"), false))
    return true;

  // TODO: not correct (eg. moving in crawled position while zooming/aiming gives false negative results, technically it is not aiming)
  bool is_fps_cam = false;
  if (ecs::EntityId localPlayerEid = game::get_controlled_hero()) // or game::get_local_player_eid()
    g_entity_mgr->sendEventImmediate(localPlayerEid, CmdGetActorUsesFpsCam(&is_fps_cam));
  return is_fps_cam;
}

void DeformHeightmap::beforeRenderWorld(const Point3 &cam_pos)
{
  ShaderGlobal::set_int(deform_hmap_enabledVarId, deform_enabled.get() ? 1 : 0);
  if (!deform_enabled.get())
    return;

  Point2 deltaCamPos = Point2::xz(cam_pos) - prevCamPosXZ;
  const float texelSize = boxSize / texSize;
  IPoint2 deltaCamPosInTexels = ipoint2(deltaCamPos / texelSize);
  Point2 deltaCamPosRoundedToTexels = point2(deltaCamPosInTexels) * texelSize;
  currentCamPosXZ = prevCamPosXZ + deltaCamPosRoundedToTexels;

  ShaderGlobal::set_color4(deform_hmap_world_to_uv_scale_biasVarId, 1.0f / boxSize, 1.0f / boxSize,
    -currentCamPosXZ.x / boxSize + 0.5f, -currentCamPosXZ.y / boxSize + 0.5f);

  iViewTm.setcol(0, Point3(1, 0, 0));
  iViewTm.setcol(1, Point3(0, 0, -1));
  iViewTm.setcol(2, Point3(0, 1, 0));
  iViewTm.setcol(3, Point3::x0y(currentCamPosXZ));
  viewTm = inverse(iViewTm);

  projTm = matrix_ortho_lh_reverse(boxSize, boxSize, zn, zf);

  float camDist = cam_dist_forced.get() < 0.00001 ? fpsCamDist : cam_dist_forced.get();
  ShaderGlobal::set_real(deform_fps_cam_distVarId, isCameraInFpsView() ? camDist : 0);
}

void DeformHeightmap::beforeRenderDepth()
{
  if (!deform_enabled.get())
    return;

  saveStates();

  if (needClearDepth)
  {
    clear();
    needClearDepth = false;
  }

  TIME_D3D_PROFILE(deform_hmap_beforeRenderDepth);

  d3d::set_render_target(deformInfoTextures[currentRtIdx].getTex2D(), 0);
  d3d::set_depth(depthTextures[currentRtIdx].getTex2D(), DepthAccess::RW);

  Point2 deltaCamXZ = currentCamPosXZ - prevCamPosXZ;
  Point2 uvOffset = deltaCamXZ / boxSize;
  ShaderGlobal::set_texture(deform_hmap_postfx_source_texVarId, depthTextures[prevRtIdx]);
  ShaderGlobal::set_texture(deform_hmap_info_texVarId, deformInfoTextures[prevRtIdx]);
  ShaderGlobal::set_color4(deform_hmap_reproject_uv_offsetVarId, uvOffset.x, uvOffset.y, 0, 0);
  shaders::overrides::set(zFuncAlwaysStateId);
  reprojectPostFx->render();
  shaders::overrides::reset();

  d3d::settm(TM_VIEW, viewTm);
  d3d::settm(TM_PROJ, &projTm);
}

void DeformHeightmap::afterRenderDepth()
{
  if (!deform_enabled.get())
    return;

  TIME_D3D_PROFILE(deform_hmap_afterRenderDepth);

  // So rtarget will be available for set as deform_hmap_postfx_source_tex
  d3d::set_depth(nullptr, DepthAccess::RW);
  d3d::resource_barrier({depthTextures[currentRtIdx].getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});

  d3d::set_render_target(nullptr, 0);
  d3d::resource_barrier({deformInfoTextures[currentRtIdx].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});

  // Clear mask in advance in a separate pass because deform pass needs to fill it in a way threadgroups write
  // each-other's mask data
  {
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), maskGridTextures[currentMaskIdx].getTex2D());
    clearMaskCs->dispatch(numThreadGroupsOnMaskXY, numThreadGroupsOnMaskXY, 1);
  }

  d3d::resource_barrier(
    {maskGridTextures[currentMaskIdx].getTex2D(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});

  // Calculate deform values and mask grid from depth data
  {
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), postFxTextures[postFxDstIdx].getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), maskGridTextures[currentMaskIdx].getTex2D());
    ShaderGlobal::set_texture(deform_hmap_postfx_source_texVarId, depthTextures[currentRtIdx]);
    ShaderGlobal::set_texture(deform_hmap_info_texVarId, deformInfoTextures[currentRtIdx]);
    deformCs->dispatch(numThreadGroupsOnTexXY, numThreadGroupsOnTexXY, 1);
    d3d::resource_barrier(
      {maskGridTextures[currentMaskIdx].getBaseTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
    eastl::swap(postFxDstIdx, postFxSrcIdx);
  }

  // Clear indirect buffers
  {
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), indirectBuffer.getBuf());
    clearIndirect->dispatch(1, 1, 1);
  }

  d3d::resource_barrier({indirectBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  // Fill indirect buffers and cell buffers based on what needs to be postprocessed and what needs to be cleared
  {
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), maskGridTextures[currentMaskIdx].getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), maskGridTextures[prevMaskIdx].getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 2, VALUE), indirectBuffer.getBuf());
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 3, VALUE), postFxCellBuffer.getBuf());
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 4, VALUE), clearCellBuffer.getBuf());
    dispatcherCs->dispatch(numThreadGroupsOnMaskXY, numThreadGroupsOnMaskXY, 1);
  }

  d3d::resource_barrier({postFxCellBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::resource_barrier({clearCellBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  // Edge detection postfx pass
  bool needEdgeDetect = edge_raise_forced.get() < 0 ? raiseEdges : (bool)edge_raise_forced.get();
  if (needEdgeDetect)
  {
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), postFxTextures[postFxDstIdx].getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 1, VALUE), postFxCellBuffer.getBuf());
    d3d::resource_barrier({postFxTextures[postFxSrcIdx].getBaseTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    ShaderGlobal::set_texture(deform_hmap_postfx_source_texVarId, postFxTextures[postFxSrcIdx]);
    d3d::resource_barrier({indirectBuffer.getBuf(), RB_RO_INDIRECT_BUFFER});
    edgeDetectCs->dispatch_indirect(indirectBuffer.getBuf(), 0);
    eastl::swap(postFxSrcIdx, postFxDstIdx);
    d3d::resource_barrier({postFxCellBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  }

  d3d::resource_barrier({postFxCellBuffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  // Blur postfx pass
  {
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), postFxTextures[postFxDstIdx].getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 1, VALUE), postFxCellBuffer.getBuf());
    d3d::resource_barrier({postFxTextures[postFxSrcIdx].getBaseTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    ShaderGlobal::set_texture(deform_hmap_postfx_source_texVarId, postFxTextures[postFxSrcIdx]);
    blurCs->dispatch_indirect(indirectBuffer.getBuf(), 0);
  }

  d3d::resource_barrier({postFxTextures[postFxDstIdx].getTex2D(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});

  // Clear cells that were written in previous frame but weren't in current frame
  {
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), postFxTextures[postFxDstIdx].getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), postFxTextures[postFxSrcIdx].getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 2, VALUE), clearCellBuffer.getBuf());
    int byteOffset = DISPATCH_INDIRECT_NUM_ARGS * INDIRECT_BUFFER_ELEMENT_SIZE;
    clearerCs->dispatch_indirect(indirectBuffer.getBuf(), byteOffset);
    d3d::resource_barrier(
      {postFxTextures[postFxDstIdx].getBaseTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
  }

  restoreStates();

  ShaderGlobal::set_texture(deform_hmap_texVarId, postFxTextures[postFxDstIdx]);
  ShaderGlobal::set_texture(deform_hmap_info_texVarId, deformInfoTextures[currentRtIdx]);

  prevCamPosXZ = currentCamPosXZ;
  eastl::swap(prevRtIdx, currentRtIdx);
  eastl::swap(currentMaskIdx, prevMaskIdx);
  postFxSrcIdx = 0;
  postFxDstIdx = 1;
}

void DeformHeightmap::saveStates()
{
  d3d_get_render_target(origRt);
  d3d_get_view_proj(origViewProj);
}

void DeformHeightmap::restoreStates()
{
  d3d::set_depth(nullptr, DepthAccess::RW);
  d3d_set_view_proj(origViewProj);
  d3d_set_render_target(origRt);
}

static bool deform_hmap_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("deform_hmap", "clear", 1, 1)
  {
    if (instance)
      instance->requestClear();
  }
  return found;
}

static void deform_hmap_after_device_reset(bool full_reset)
{
  if (full_reset && instance != nullptr)
    instance->requestClear();
}

REGISTER_CONSOLE_HANDLER(deform_hmap_console_handler);
REGISTER_D3D_AFTER_RESET_FUNC(deform_hmap_after_device_reset);