// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "editModeRenderer.h"
#include <de3_interface.h>
#include <perfMon/dag_statDrv.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <de3_dynRenderService.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/visibility.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <render/dag_cur_view.h>

int EditModeRenderer::simple_tint_colorVarId = -1;
int EditModeRenderer::simple_tint_color_rtVarId = -1;
int EditModeRenderer::global_frame_block_id = -1;
int EditModeRenderer::rendinst_scene_block_id = -1;
const E3DCOLOR EditModeRenderer::default_tint_color = E3DCOLOR(255, 255, 255, 128);

EditModeRenderer::~EditModeRenderer()
{
  if (isInited())
  {
    rendinst::destroyRIGenVisibility(globalVisibility);
    rendinst::destroyRIGenVisibility(filteredVisibility);
  }
}

void EditModeRenderer::init()
{
  globalVisibility = rendinst::createRIGenVisibility(midmem);
  filteredVisibility = rendinst::createRIGenVisibility(midmem);

  const char *shaderName = "simple_tint_final_render";
  finalRender.init(shaderName);
  if (!finalRender.getMat())
    DAEDITOR3.conError("Shader \"%s\" cannot be found. Edit mode rendering won't work!", shaderName);

  simple_tint_colorVarId = get_shader_variable_id("simple_tint_color", true);
  simple_tint_color_rtVarId = get_shader_variable_id("simple_tint_color_rt", true);
  global_frame_block_id = ShaderGlobal::getBlockId("global_frame");
  rendinst_scene_block_id = ShaderGlobal::getBlockId("rendinst_scene");
}

void EditModeRenderer::initResolution(int width_, int height_)
{
  width = width_;
  height = height_;

  colorRt.close();
  colorRt.set(d3d::create_tex(NULL, width, height, TEXCF_RTARGET | TEXFMT_DEFAULT, 1, "simple_tint_color_rt"), "simple_tint_color_rt");
  depthRt = dag::create_tex(NULL, width, height, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "simple_tint_depth_rt");
}

void EditModeRenderer::render(IGenViewportWnd &wnd, const RIElementsCache &riElements,
  dag::ConstSpan<DynamicRenderableSceneInstance *> dynmodelElements, const RIElementsCache &occluderRiElements,
  dag::ConstSpan<DynamicRenderableSceneInstance *> occluderDynmodelElements, const E3DCOLOR outlineColor)
{
  TIME_D3D_PROFILE(render_tint_elements);

  int viewportWidth, viewportHeight;
  wnd.getViewportSize(viewportWidth, viewportHeight);

  if (!isInited())
    init();

  ShaderGlobal::set_float4(simple_tint_colorVarId, outlineColor);

  Driver3dRenderTarget prevRT;
  d3d::get_render_target(prevRT);

  if (width != viewportWidth || height != viewportHeight)
    initResolution(viewportWidth, viewportHeight);

  const int lastFrameBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(global_frame_block_id, ShaderGlobal::LAYER_FRAME);

  // rendinst
  const ViewportWindow &vpw = static_cast<ViewportWindow &>(wnd);
  TMatrix4 globtm4;
  d3d::calcglobtm(vpw.getViewTm(), vpw.getProjTm(), globtm4);
  mat44f globtm;
  v_mat44_make_from_44cu(globtm, globtm4.m[0]);
  rendinst::prepareRIGenExtraVisibility(globtm, ::grs_cur_view.pos, *globalVisibility, false, nullptr);
  rendinst::prepareRIGenVisibility(Frustum(globtm), ::grs_cur_view.pos, globalVisibility, false, nullptr);

  // Depth pre-pass: render the sub-composite (occluders) into the depth buffer so that
  // tinted sibling elements behind it are correctly hidden.
  d3d::set_render_target({depthRt.getTex2D(), 0, 0}, DepthAccess::RW, {});
  d3d::clearview(CLEAR_ZBUFFER, 0, 0, 0);
  if (!occluderRiElements.empty() || !occluderDynmodelElements.empty())
  {
    rendinst::VisibilityExternalIdFilter ri_occluder_filter = [&occluderRiElements](int ri_idx, const TMatrix &tm) -> bool {
      auto range = occluderRiElements.equal_range(ri_idx);
      for (auto it = range.first; it != range.second; ++it)
        if (it->second == tm)
          return true;
      return false;
    };
    rendinst::filterRIGenVisibilityById(globalVisibility, filteredVisibility, ri_occluder_filter);
    rendinst::filterRIGenExtraVisibilityById(globalVisibility, filteredVisibility, ri_occluder_filter);
    {
      SCENE_LAYER_GUARD(rendinst_scene_block_id);
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, filteredVisibility, ::grs_cur_view.itm,
        rendinst::LayerFlag::Opaque | rendinst::LayerFlag::NotExtra, rendinst::OptimizeDepthPass::Yes);
    }
    if (IDynRenderService *rs = EDITORCORE->queryEditorInterface<IDynRenderService>())
      for (auto *re : occluderDynmodelElements)
        rs->renderOneDynModelInstance(re, IRenderingService::Stage::STG_RENDER_DYNAMIC_OPAQUE);
  }

  // Color pass: render tinted siblings against the occluder depth.
  // Clear only color so the depth values from the pre-pass are preserved.
  d3d::set_render_target({depthRt.getTex2D(), 0, 0}, DepthAccess::RW, {{colorRt.getTex2D(), 0, 0}});
  d3d::clearview(CLEAR_TARGET, 0, 0, 0);

  rendinst::VisibilityExternalIdFilter ri_id_filter = [&riElements](int ri_idx, const TMatrix &tm) -> bool {
    auto range = riElements.equal_range(ri_idx);
    for (auto it = range.first; it != range.second; ++it)
      if (it->second == tm)
        return true;
    return false;
  };
  rendinst::filterRIGenVisibilityById(globalVisibility, filteredVisibility, ri_id_filter);
  rendinst::filterRIGenExtraVisibilityById(globalVisibility, filteredVisibility, ri_id_filter);
  {
    SCENE_LAYER_GUARD(rendinst_scene_block_id);
    rendinst::render::renderRIGen(rendinst::RenderPass::Normal, filteredVisibility, ::grs_cur_view.itm,
      rendinst::LayerFlag::Opaque | rendinst::LayerFlag::NotExtra, rendinst::OptimizeDepthPass::Yes);
  }

  // dynmodel
  if (IDynRenderService *rs = EDITORCORE->queryEditorInterface<IDynRenderService>())
  {
    for (auto *re : dynmodelElements)
      rs->renderOneDynModelInstance(re, IRenderingService::Stage::STG_RENDER_DYNAMIC_OPAQUE);
  }

  ShaderGlobal::setBlock(lastFrameBlockId, ShaderGlobal::LAYER_FRAME);

  d3d::set_render_target(prevRT);
  ShaderGlobal::set_texture(simple_tint_color_rtVarId, colorRt.getId());
  finalRender.render();
}
