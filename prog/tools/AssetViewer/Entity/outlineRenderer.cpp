// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "outlineRenderer.h"
#include <de3_interface.h>
#include <perfMon/dag_statDrv.h>
#include <EditorCore/ec_interface.h>
#include <de3_dynRenderService.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/visibility.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <render/dag_cur_view.h>

int OutlineRenderer::simple_outline_colorVarId = -1;
int OutlineRenderer::simple_outline_color_rtVarId = -1;
int OutlineRenderer::global_frame_block_id = -1;
int OutlineRenderer::rendinst_scene_block_id = -1;
const E3DCOLOR OutlineRenderer::outline_color = E3DCOLOR(255, 255, 0);

OutlineRenderer::~OutlineRenderer()
{
  if (isInited())
  {
    rendinst::destroyRIGenVisibility(globalVisibility);
    rendinst::destroyRIGenVisibility(filteredVisibility);
  }
}

void OutlineRenderer::init()
{
  globalVisibility = rendinst::createRIGenVisibility(midmem);
  filteredVisibility = rendinst::createRIGenVisibility(midmem);

  const char *shaderName = "simple_outline_final_render";
  finalRender.init(shaderName);
  if (!finalRender.getMat())
    DAEDITOR3.conError("Shader \"%s\" cannot be found. Outline rendering won't work!", shaderName);

  simple_outline_colorVarId = get_shader_variable_id("simple_outline_color", true);
  simple_outline_color_rtVarId = get_shader_variable_id("simple_outline_color_rt", true);
  global_frame_block_id = ShaderGlobal::getBlockId("global_frame");
  rendinst_scene_block_id = ShaderGlobal::getBlockId("rendinst_scene");

  ShaderGlobal::set_color4(simple_outline_colorVarId, outline_color);
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  ShaderGlobal::set_sampler(get_shader_variable_id("simple_outline_color_rt_samplerstate", true), d3d::request_sampler(smpInfo));
}

void OutlineRenderer::initResolution(int width_, int height_)
{
  width = width_;
  height = height_;

  colorRt.close();
  colorRt.set(d3d::create_tex(NULL, width, height, TEXCF_RTARGET | TEXFMT_DEFAULT, 1, "simple_outline_color_rt"),
    "simple_outline_color_rt");
}

void OutlineRenderer::render(IGenViewportWnd &wnd, const RIElementsCache &riElements,
  dag::ConstSpan<DynamicRenderableSceneInstance *> dynmodelElements)
{
  TIME_D3D_PROFILE(render_outline_elements);

  int viewportWidth, viewportHeight;
  wnd.getViewportSize(viewportWidth, viewportHeight);

  if (!isInited())
    init();

  if (width != viewportWidth || height != viewportHeight)
    initResolution(viewportWidth, viewportHeight);

  Driver3dRenderTarget prevRT;
  d3d::get_render_target(prevRT);

  d3d::set_render_target(colorRt.getTex2D(), 0);
  d3d::clearview(CLEAR_TARGET, 0, 1, 0);

  const int lastFrameBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(global_frame_block_id, ShaderGlobal::LAYER_FRAME);

  // rendinst
  mat44f globtm;
  d3d::getglobtm(globtm);
  rendinst::prepareRIGenVisibility(Frustum(globtm), ::grs_cur_view.pos, globalVisibility, false, nullptr);

  rendinst::VisibilityExternalIdFilter ri_id_filter = [&riElements](int ri_idx, const TMatrix &tm) -> bool {
    auto range = riElements.equal_range(ri_idx);
    for (auto it = range.first; it != range.second; ++it)
      if (it->second == tm)
        return true;
    return false;
  };
  rendinst::filterRIGenVisibilityById(globalVisibility, filteredVisibility, ri_id_filter);
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
  ShaderGlobal::set_texture(simple_outline_color_rtVarId, colorRt.getId());
  finalRender.render();
}
