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
//  - simple resolve (static shadows + regular PBR stuff)

#include "satelliteRenderer.h"
#include <render/world/cameraParams.h>
#include <util/dag_convar.h>
#include <math/dag_mathUtils.h>
#include "global_vars.h"
#include <render/deferredRenderer.h>
#include "render/screencap.h"
#include "camera/sceneCam.h"
#include <render/viewVecs.h>
#include <rendInst/visibility.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <shaders/dag_shaderBlock.h>
#include <rendInst/rendInstGenRender.h>
#include <main/main.h>

namespace WorldRenderSatellite
{
// current height of camera
CONSOLE_FLOAT_VAL("render_sat", height, 40);
// fixed height of camera
CONSOLE_FLOAT_VAL("render_sat", height_fixed, 40);
// height offset from land if use_land_based_height_offset enabled
CONSOLE_FLOAT_VAL("render_sat", height_ofs, 25);
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

} // namespace WorldRenderSatellite
extern int rendinstDepthSceneBlockId;

using namespace WorldRenderSatellite;

void SatelliteRenderer::ensureTargets()
{
  if (targetTex && !size.pullValueChange())
    return;

  unsigned texFmt[3] = {TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, TEXFMT_A2B10G10R10, TEXFMT_A8R8G8B8};
  uint32_t expectedRtNum = countof(texFmt);

  renderTargetGbuf.reset();
  renderTargetGbuf = eastl::make_unique<DeferredRenderTarget>("satellite_render_resolve", "satellite_target_gbuf", size, size,
    DeferredRT::StereoMode::MonoOrMultipass, 0, expectedRtNum, texFmt, TEXFMT_DEPTH32);

  targetTex.close();
  targetTex = dag::create_tex(NULL, size, size, TEXCF_RTARGET | TEXFMT_DEFAULT, 1, "satellite_target");
  targetTex->disableSampler();

  if (!riGenVisibility)
  {
    riGenVisibility = rendinst::createRIGenVisibility(midmem);
    rendinst::setRIGenVisibilityMinLod(riGenVisibility, 0, 0);
    rendinst::setRIGenVisibilityDistMul(riGenVisibility, 0);
  }
}

void SatelliteRenderer::cleanupResources()
{
  if (!targetTex)
    return;

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
    bool htSuccess = callback_params.landmeshHeightGetter(Point2::xz(in_pos), ht);
    if (ht < 0)
      ht = 0;
    if (htSuccess)
      height.set(ht + height_ofs);
  }
  in_pos.y = height;
}

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

  ScopedShVars(Point3 in_pos)
  {
    originalZnZfar = ShaderGlobal::get_color4(zn_zfarVarId);
    originalWorldPos = ShaderGlobal::get_color4(world_view_posVarId);
    ShaderGlobal::set_color4(world_view_posVarId, in_pos.x, in_pos.y, in_pos.z, 1);
    ShaderGlobal::set_color4(zn_zfarVarId, zn, zf, 0, 0);
  }

  ~ScopedShVars()
  {
    ShaderGlobal::set_color4(zn_zfarVarId, originalZnZfar);
    ShaderGlobal::set_color4(world_view_posVarId, originalWorldPos);
  }
};

// todo: A copy from TiledMapContext, move to some common place
eastl::string tileXYToQuadKey(int tileX, int tileY, int zoom)
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

  BBox3 box;
  bbox3f boxCull;
  bbox3f actualBox;
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
    mat44f globtm;
    v_mat44_make_from_44cu(globtm, globtm_.m[0]);
    frustum = Frustum{globtm};
  }

  // apply matrixes
  d3d::settm(TM_PROJ, &proj);
  d3d::settm(TM_VIEW, view);
  set_viewvecs_to_shader(view, proj);
  set_inv_globtm_to_shader(view, proj, true);
  camera_params.jitterFrustum = frustum;
  camera_params.noJitterFrustum = frustum;

  static int use_satellite_renderingVarId = get_shader_variable_id("use_satellite_rendering");
  ShaderGlobal::set_int(use_satellite_renderingVarId, 1);

  // start deferred pass
  {
    renderTargetGbuf->setRt();
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0, 0, 0);
    FRAME_LAYER_GUARD(globalFrameBlockId);

    callback_params.renderLandmesh(globtm, frustum, in_pos);

    // RI+RIex visibility
    rendinst::prepareRIGenExtraVisibilityBox(boxCull, 0, 0, 0, *riGenVisibility, &actualBox);
    rendinst::prepareRIGenVisibility(frustum, in_pos, riGenVisibility, false, NULL);
    rendinst::render::before_draw(rendinst::RenderPass::Normal, riGenVisibility, frustum, nullptr);

    // RI prepass (cuz it is assumed in shaders)
    {
      SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);

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

  // gives shits about something, clear state
  ShaderElement::invalidate_cached_state_block();

  renderTargetGbuf->resolve(targetTex.getTex2D(), view, proj, nullptr, ShadingResolver::ClearTarget::Yes, globtm_);

  // render water on top of resolved content with simplified shader
  {
    ShaderGlobal::set_texture(downsampled_far_depth_texVarId, renderTargetGbuf->getDepthId());
    callback_params.renderWater(targetTex.getBaseTex(), view_itm, renderTargetGbuf->getDepth());
  }

  ShaderGlobal::set_int(use_satellite_renderingVarId, 0);

  // save if requested
  if (save)
  {
    // here we invert y, because we need to save images same as it looks in game:
    // game world coords are left-handed, but images are saved in right-handed coords
    int max_iy = (1 << zlevels);
    eastl::string quadkey = tileXYToQuadKey(scanIndex.x, (max_iy - scanIndex.y - 1), zlevels);
    screencap::make_screenshot(targetTex, quadkey.c_str());
    save.set(false);
  }
}

void SatelliteRenderer::scanSettleCheck(int vtex_updates)
{
  if (scanSettleFrameId < scan_settle_frames)
  {
    if (vtex_updates == 0)
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
  if (!scan && !draw_every_frame)
    return;

  if (scan_x1 - scan_x0 != scan_y1 - scan_y0 || scan_x1 - scan_x0 <= 0 || scan_y1 - scan_y0 <= 0)
  {
    logerr("map bbox is not square or empty set propper console vars scan_x0, scan_y0, scan_x1, scan_y1 directly, or setup correct "
           "`tiled_map` entity");
    scan.set(false);
    draw_every_frame.set(false);
    return;
  }

  CallbackParams callbackParams = getParamsCb();

  if (draw_every_frame)
  {
    renderFromPos(camera_params.viewItm.col[3], callbackParams, camera_params);
    return;
  }

  if (!scan)
  {
    scanIndex.x = 0;
    scanIndex.y = 0;
    return;
  }

  float w = (scan_x1 - scan_x0) / (1 << zlevels);
  float x = scan_x0 + w * scanIndex.x + 0.5 * w;
  float y = scan_y0 + w * scanIndex.y + 0.5 * w;

  // change camera pos to currently scanned position, to sync streaming and other stuff with ortho rendering
  TMatrix citm = get_cam_itm();
  citm.col[3] = {x, height * 2, y};
  set_cam_itm(citm);

  scanSettleCheck(callbackParams.clipmapGetLastUpdatedTileCount());
  renderFromPos({x, height, y}, callbackParams, camera_params);

  if (scanIndex.y >= (1 << zlevels))
  {
    scan.set(false);
    exit_game("satellite map scan done");
  }
}
