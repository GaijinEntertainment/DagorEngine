// Copyright (C) Gaijin Games KFT.  All rights reserved.

// provides top-down (above) rendering used for satellite like imagering (3d ingame based render minimaps)
// two main methods include render at will on XYZ coords
// and scripted rendering that generates tiles for whole level
//
// render is custom as we don't support non-perspective camera at all
// (optimized stripped persp stuff and assumes in all code base)
// included features
//  - opaq RI
//  - opaq RIex
//  - terrain
//  - water
//  - resolve with the game shader (TargetParams::resolveShaderName)

#include <render/satelliteRenderer.h>
#include <render/cameraParams.h>
#include <util/dag_convar.h>
#include <math/dag_mathUtils.h>
#include <render/deferredRenderer.h>
#include <render/viewVecs.h>
#include <rendInst/visibility.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_renderTarget.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <rendInst/rendInstGenRender.h>
#include <memory/dag_memBase.h>
#include <debug/dag_log.h>
#include <debug/dag_assert.h>
#include <EASTL/string.h>


namespace var
{
static ShaderVariableInfo use_satellite_rendering{"use_satellite_rendering", true};
static ShaderVariableInfo zn_zfar{"zn_zfar"};
static ShaderVariableInfo world_view_pos{"world_view_pos"};
static ShaderVariableInfo downsampled_far_depth_tex{"downsampled_far_depth_tex", true};
static ShaderVariableInfo globtm_inv{"globtm_inv", true};
static ShaderVariableInfo view_vecLT{"view_vecLT", true};
static ShaderVariableInfo view_vecRT{"view_vecRT", true};
static ShaderVariableInfo view_vecLB{"view_vecLB", true};
static ShaderVariableInfo view_vecRB{"view_vecRB", true};
} // namespace var
static ShaderBlockIdHolder global_frame_block_id{"global_frame"};
static ShaderBlockIdHolder rendinst_depth_scene_block_id{"rendinst_depth_scene"};

namespace WorldRenderSatellite
{
// current height of camera
CONSOLE_FLOAT_VAL("render_sat", height, 40);
// fixed height of camera
CONSOLE_FLOAT_VAL("render_sat", height_fixed, 40);
// height offset from land if use_land_based_height_offset enabled
CONSOLE_FLOAT_VAL("render_sat", height_ofs, 80);
// use offset from land height instead of constant height
CONSOLE_BOOL_VAL("render_sat", use_land_based_height_offset, true);
// size of targets in pixels
CONSOLE_INT_VAL("render_sat", size, 1024, 64, 4096);
CONSOLE_FLOAT_VAL("render_sat", zn, 0.01);
CONSOLE_FLOAT_VAL("render_sat", zf, 1000);
// update target every frame
CONSOLE_BOOL_VAL("render_sat", draw_every_frame, false);
// save current XYZ tile being rendered
CONSOLE_BOOL_VAL("render_sat", save, false);
// stars map scan. moves camera to cover area in x0,y0,x1,y1 square and saves each tile as screenshot
CONSOLE_BOOL_VAL("render_sat", scan, false);
// size of square scan will be happening in
// The bbox of the map. It is assumed, that map is square, with square tiles.
CONSOLE_FLOAT_VAL("render_sat", scan_x0, -2048.0f);
CONSOLE_FLOAT_VAL("render_sat", scan_y0, -2048.0f);
CONSOLE_FLOAT_VAL("render_sat", scan_x1, 2048.0f);
CONSOLE_FLOAT_VAL("render_sat", scan_y1, 2048.0f);
// the depth of quadtree See more: https://learn.microsoft.com/en-us/azure/azure-maps/zoom-levels-and-tile-grid
CONSOLE_INT_VAL("render_sat", zlevels, 5, 1, 10);
// before saving result, wait for some frames for streaming to finish its nasty job
CONSOLE_INT_VAL("render_sat", scan_settle_frames, 20, 3, 1000);

struct ScopedCameraParams
{
  CameraParams originalCamera;
  CameraParams &cameraRef;

  ScopedCameraParams(CameraParams &orig) : cameraRef(orig), originalCamera(orig) {}

  ~ScopedCameraParams() { cameraRef = originalCamera; }
};

struct ScopedShVars
{
  Color4 originalZnZfar;
  Color4 originalWorldPos;
  Color4 originalViewVecs[4];
  TMatrix4 originalGlobTmInv;
  TEXTUREID originalFarDepthTex;

  ScopedShVars(Point3 in_pos)
  {
    originalZnZfar = ShaderGlobal::get_float4(var::zn_zfar);
    originalWorldPos = ShaderGlobal::get_float4(var::world_view_pos);
    originalViewVecs[0] = ShaderGlobal::get_float4(var::view_vecLT);
    originalViewVecs[1] = ShaderGlobal::get_float4(var::view_vecRT);
    originalViewVecs[2] = ShaderGlobal::get_float4(var::view_vecLB);
    originalViewVecs[3] = ShaderGlobal::get_float4(var::view_vecRB);
    originalGlobTmInv = ShaderGlobal::get_float4x4(var::globtm_inv);
    originalFarDepthTex = ShaderGlobal::get_texture(var::downsampled_far_depth_tex);
    ShaderGlobal::set_float4(var::world_view_pos, in_pos.x, in_pos.y, in_pos.z, 1);
    ShaderGlobal::set_float4(var::zn_zfar, zn, zf, 0, 0);
  }

  ~ScopedShVars()
  {
    ShaderGlobal::set_float4(var::zn_zfar, originalZnZfar);
    ShaderGlobal::set_float4(var::world_view_pos, originalWorldPos);
    ShaderGlobal::set_float4(var::view_vecLT, originalViewVecs[0]);
    ShaderGlobal::set_float4(var::view_vecRT, originalViewVecs[1]);
    ShaderGlobal::set_float4(var::view_vecLB, originalViewVecs[2]);
    ShaderGlobal::set_float4(var::view_vecRB, originalViewVecs[3]);
    ShaderGlobal::set_float4x4(var::globtm_inv, originalGlobTmInv);
    ShaderGlobal::set_texture(var::downsampled_far_depth_tex, originalFarDepthTex);
  }
};
} // namespace WorldRenderSatellite

using namespace WorldRenderSatellite;

static bool is_square_rect(float x0, float y0, float x1, float y1) { return x1 - x0 > 0 && is_equal_float(x1 - x0, y1 - y0, 0.01f); }

SatelliteRenderer::SatelliteRenderer() = default;
SatelliteRenderer::~SatelliteRenderer() { cleanupResources(); }

void SatelliteRenderer::init(const TargetParams &target_params, const CallbackParams &callback_params)
{
  G_ASSERT(!target_params.gbufFormats.empty());
  G_ASSERT(target_params.gbufFormats.size() <= DeferredRT::MAX_NUM_MRT);
  G_ASSERT(!target_params.resolveShaderName.empty());
  G_ASSERT(callback_params.landmeshHeightGetter);
  G_ASSERT(callback_params.renderWater);
  G_ASSERT(callback_params.renderLandmesh);
  G_ASSERT(callback_params.clipmapGetLastUpdatedTileCount);
  G_ASSERT(callback_params.texStreamingGetPendingCount);
  G_ASSERT(callback_params.saveTile);
  G_ASSERT(callback_params.setSceneCameraPos);
  G_ASSERT(callback_params.onScanDone);
  cleanupResources();
  targetParams = target_params;
  callbackParams = callback_params;
}

bool SatelliteRenderer::isActive() const { return scan || draw_every_frame; }
bool SatelliteRenderer::isScanning() const { return scan; }

bool SatelliteRenderer::startScan(const ScanParams &params)
{
  if (!is_square_rect(params.leftTop.x, params.leftTop.y, params.rightBottom.x, params.rightBottom.y))
  {
    logerr("satelliteRenderer: scan rect (%g, %g) - (%g, %g) is not square or empty, no scan", params.leftTop.x, params.leftTop.y,
      params.rightBottom.x, params.rightBottom.y);
    return false;
  }
  scan_x0.set(params.leftTop.x);
  scan_y0.set(params.leftTop.y);
  scan_x1.set(params.rightBottom.x);
  scan_y1.set(params.rightBottom.y);
  zlevels.set(params.zlevels);
  size.set(params.tileWidth);
  use_land_based_height_offset.set(params.fixedHeight <= 0.f);
  if (params.fixedHeight > 0.f)
    height_fixed.set(params.fixedHeight);
  if (params.heightOffset > 0.f)
    height_ofs.set(params.heightOffset);
  stopScan();
  scan.set(true);
  return true;
}

void SatelliteRenderer::stopScan()
{
  scan.set(false);
  save.set(false);
  scanIndex = IPoint2::ZERO;
  scanSettleFrameId = 0;
}

void SatelliteRenderer::ensureTargets()
{
  if (targetTex && !size.pullValueChange())
    return;

  renderTargetGbuf.reset();
  renderTargetGbuf = eastl::make_unique<DeferredRenderTarget>(targetParams.resolveShaderName.c_str(), "satellite_target_gbuf", size,
    size, DeferredRT::StereoMode::MonoOrMultipass, 0, targetParams.gbufFormats.size(), targetParams.gbufFormats.data(),
    targetParams.depthFormat);

  targetTex.close();
  targetTex = dag::create_tex(NULL, size, size, TEXCF_RTARGET | targetParams.targetFormat, 1, "satellite_target");

  if (!riGenVisibility)
  {
    riGenVisibility = rendinst::createRIGenVisibility(midmem);
    rendinst::setRIGenVisibilityMinLod(riGenVisibility, 0, 0);
    rendinst::setRIGenVisibilityDistMul(riGenVisibility, 10);
  }
}

void SatelliteRenderer::cleanupResources()
{
  targetTex.close();
  renderTargetGbuf.reset();

  rendinst::destroyRIGenVisibility(riGenVisibility);
  riGenVisibility = nullptr;
}

void SatelliteRenderer::adjustRenderHeight(Point3 &in_pos, const CallbackParams &callback_params)
{
  height.set(height_fixed);
  if (use_land_based_height_offset)
  {
    real ht;
    if (callback_params.landmeshHeightGetter(Point2::xz(in_pos), ht))
      height.set(max(ht, 0.f) + height_ofs);
  }
  in_pos.y = height;
}

// todo: A copy from TiledMapContext, move to some common place
static eastl::string tileXYToQuadKey(int tileX, int tileY, int zoom)
{
  eastl::string quadKey(eastl::string::CtorDoNotInitialize{}, zoom);
  for (int i = zoom; i > 0; i--)
  {
    char digit = '0';
    int mask = 1 << (i - 1);
    if ((tileX & mask) != 0)
    {
      digit++;
    }
    if ((tileY & mask) != 0)
    {
      digit++;
      digit++;
    }
    quadKey.push_back(digit);
  }
  return quadKey;
}

void SatelliteRenderer::renderFromPos(Point3 in_pos, const CallbackParams &callback_params, CameraParams &camera_params)
{
  ensureTargets();
  adjustRenderHeight(in_pos, callback_params);

  // setup scope based restores
  ScopedShVars shVarsOverrides(in_pos);
  ScopedCameraParams cameraRestore(camera_params);
  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;

  BBox3 box;
  bbox3f boxCull;
  // setup bbox
  {
    box.makecube(in_pos, (scan_x1 - scan_x0) / (1 << zlevels));
    // make sure that we don't cut RIs when range is smaller than height
    BBox3 heightAdjustedBox = box;
    heightAdjustedBox += in_pos + Point3(0, 2.0 * height, 0);
    heightAdjustedBox += in_pos - Point3(0, 2.0 * height, 0);
    boxCull = v_ldu_bbox3(heightAdjustedBox);
  }

  TMatrix view_itm;
  TMatrix view;
  TMatrix4 proj;
  TMatrix4 globtm_;
  mat44f globtm;
  Frustum frustum;
  // fill matrixes
  {
    view_itm = camera_params.viewItm;
    view_itm.col[3] = in_pos;
    proj = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[0].z, box[1].z, zn, zf);

    view.setcol(0, 1, 0, 0);
    view.setcol(1, 0, 0, 1);
    view.setcol(2, 0, 1, 0);
    view.setcol(3, 0, 0, 1);

    globtm_ = TMatrix4(view) * proj;
    v_mat44_make_from_44cu(globtm, globtm_.m[0]);
    frustum = Frustum{globtm};
  }

  // apply matrixes
  d3d::settm(TM_PROJ, &proj);
  d3d::settm(TM_VIEW, view);
  set_viewvecs_to_shader(view, proj);
  set_inv_globtm_to_shader(view, proj, true);
  camera_params.viewTm = view;
  camera_params.viewItm = view_itm;
  camera_params.jitterFrustum = frustum;
  camera_params.noJitterFrustum = frustum;

  float halfW = (box[1].x - box[0].x) * 0.5f;
  float halfH = (box[1].z - box[0].z) * 0.5f;
  Driver3dPerspective orthoPersp(1.0f / halfW, 1.0f / halfH, zn, zf);
  camera_params.jitterPersp = orthoPersp;
  camera_params.noJitterPersp = orthoPersp;

  STATE_GUARD_0(ShaderGlobal::set_int(var::use_satellite_rendering, VALUE), 1);

  // start deferred pass
  {
    renderTargetGbuf->setRt();
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0, 0, 0);
    FRAME_LAYER_GUARD(global_frame_block_id);

    callback_params.renderLandmesh(globtm, proj, frustum, in_pos);

    // RI+RIex visibility
    rendinst::prepareRIGenExtraVisibilityBox(boxCull, 0, 0, 0, *riGenVisibility);
    rendinst::prepareRIGenVisibility(frustum, in_pos, riGenVisibility, false, NULL);
    rendinst::render::before_draw(rendinst::RenderPass::Normal, riGenVisibility, frustum, nullptr);

    // RI prepass (cuz it is assumed in shaders)
    {
      SCENE_LAYER_GUARD(rendinst_depth_scene_block_id);

      rendinst::render::renderRIGenOptimizationDepth(rendinst::RenderPass::Depth, riGenVisibility, view_itm,
        rendinst::IgnoreOptimizationLimits::No, rendinst::SkipTrees::No, rendinst::RenderGpuObjects::No, 1U);
    }

    // RI+RIex opaq pass
    {
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, riGenVisibility, view_itm, rendinst::LayerFlag::Opaque,
        rendinst::OptimizeDepthPass::No, /*count_multiply*/ 1, rendinst::AtestStage::All);
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, riGenVisibility, view_itm, rendinst::LayerFlag::NotExtra,
        rendinst::OptimizeDepthPass::Yes, /*count_multiply*/ 1, rendinst::AtestStage::All);
    }
  }

  ShaderElement::invalidate_cached_state_block();

  renderTargetGbuf->resolve(targetTex.getTex2D(), view, proj, nullptr, ShadingResolver::ClearTarget::Yes);

  // render water on top of resolved content with simplified shader
  {
    ShaderGlobal::set_texture(var::downsampled_far_depth_tex, renderTargetGbuf->getDepthId());
    callback_params.renderWater(targetTex.getBaseTex(), camera_params, renderTargetGbuf->getDepth());
  }

  // save if requested
  if (save)
  {
    // here we invert y, because we need to save images same as it looks in game:
    // game world coords are left-handed, but images are saved in right-handed coords
    int max_iy = (1 << zlevels);
    eastl::string quadkey = tileXYToQuadKey(scanIndex.x, (max_iy - scanIndex.y - 1), zlevels);
    callback_params.saveTile(targetTex, quadkey.c_str());
    save.set(false);
  }
}

void SatelliteRenderer::scanSettleCheck(int vtex_updates, int tex_streaming_pending)
{
  if (scanSettleFrameId < scan_settle_frames)
  {
    if (vtex_updates == 0 && tex_streaming_pending == 0)
      ++scanSettleFrameId;
  }
  else if (scanSettleFrameId == scan_settle_frames)
  {
    save.set(true);
    ++scanSettleFrameId;
  }
  else
  {
    scanSettleFrameId = 0;
    scanIndex.x += 1;
    if (scanIndex.x >= (1 << zlevels))
    {
      scanIndex.x = 0;
      scanIndex.y += 1;
    }
  }
}

void SatelliteRenderer::renderScripted(CameraParams &camera_params)
{
  if (!isActive() || !callbackParams.renderLandmesh)
    return;

  if (!is_square_rect(scan_x0, scan_y0, scan_x1, scan_y1))
  {
    logerr("map bbox is not square or empty set propper console vars scan_x0, scan_y0, scan_x1, scan_y1 directly, or setup correct "
           "`tiled_map` entity");
    const bool wasScan = scan;
    stopScan();
    draw_every_frame.set(false);
    if (wasScan)
      callbackParams.onScanDone(false);
    return;
  }

  if (draw_every_frame)
  {
    renderFromPos(camera_params.viewItm.col[3], callbackParams, camera_params);
    return;
  }

  float w = (scan_x1 - scan_x0) / (1 << zlevels);
  float x = scan_x0 + w * scanIndex.x + 0.5 * w;
  float y = scan_y0 + w * scanIndex.y + 0.5 * w;

  callbackParams.setSceneCameraPos({x, height * 1.1f, y});

  scanSettleCheck(callbackParams.clipmapGetLastUpdatedTileCount(), callbackParams.texStreamingGetPendingCount());
  renderFromPos({x, height, y}, callbackParams, camera_params);

  if (scanIndex.y >= (1 << zlevels))
  {
    stopScan();
    callbackParams.onScanDone(true);
  }
}
