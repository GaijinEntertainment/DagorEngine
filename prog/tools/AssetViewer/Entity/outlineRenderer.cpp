#include "outlineRenderer.h"
#include <de3_interface.h>
#include <EditorCore/ec_interface.h>
#include <render/dynModelRenderer.h>
#include <rendInst/rendInstExtraRender.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_shaderBlock.h>

int OutlineRenderer::simple_outline_colorVarId = -1;
int OutlineRenderer::simple_outline_color_rtVarId = -1;
int OutlineRenderer::global_frame_block_id = -1;
int OutlineRenderer::rendinst_scene_block_id = -1;
const E3DCOLOR OutlineRenderer::outline_color = E3DCOLOR(255, 255, 0);

void OutlineRenderer::init()
{
  const char *shaderName = "simple_outline_final_render";
  finalRender.init(shaderName);
  if (!finalRender.getMat())
    DAEDITOR3.conError("Shader \"%s\" cannot be found. Outline rendering won't work!", shaderName);

  simple_outline_colorVarId = get_shader_variable_id("simple_outline_color", true);
  simple_outline_color_rtVarId = get_shader_variable_id("simple_outline_color_rt", true);
  global_frame_block_id = ShaderGlobal::getBlockId("global_frame");
  rendinst_scene_block_id = ShaderGlobal::getBlockId("rendinst_scene");

  rendinstMatrixBuffer.reset(
    d3d::create_sbuffer(sizeof(Point4), 4U, SBCF_BIND_SHADER_RES, TEXFMT_A32B32G32R32F, "simple_outline_matrix_buffer"));

  ShaderGlobal::set_color4(simple_outline_colorVarId, outline_color);
}

void OutlineRenderer::initResolution(int width_, int height_)
{
  width = width_;
  height = height_;

  colorRt.close();
  colorRt.set(d3d::create_tex(NULL, width, height, TEXCF_RTARGET | TEXFMT_DEFAULT, 1, "simple_outline_color_rt"),
    "simple_outline_color_rt");
  colorRt.getTex2D()->texaddr(TEXADDR_CLAMP);
}

void OutlineRenderer::render(IGenViewportWnd &wnd, dag::ConstSpan<RenderElement> elements)
{
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

  for (const RenderElement &re : elements)
  {
    if (re.rendInstExtraResourceIndex >= 0)
    {
      Point4 data[4];
      data[0] = Point4(re.transform.getcol(0).x, re.transform.getcol(1).x, re.transform.getcol(2).x, re.transform.getcol(3).x),
      data[1] = Point4(re.transform.getcol(0).y, re.transform.getcol(1).y, re.transform.getcol(2).y, re.transform.getcol(3).y),
      data[2] = Point4(re.transform.getcol(0).z, re.transform.getcol(1).z, re.transform.getcol(2).z, re.transform.getcol(3).z);
      data[3] = Point4(0, 0, 0, 0);
      rendinstMatrixBuffer.get()->updateData(0, sizeof(Point4) * 4U, &data, 0);

      IPoint2 offsAndCnt(0, 1);
      uint16_t riIdx = re.rendInstExtraResourceIndex;
      uint32_t zeroLodOffset = 0;

      SCENE_LAYER_GUARD(rendinst_scene_block_id);

      rendinst::render::ensureElemsRebuiltRIGenExtra(/*gpu_instancing = */ false);

      rendinst::render::renderRIGenExtraFromBuffer(rendinstMatrixBuffer.get(), dag::ConstSpan<IPoint2>(&offsAndCnt, 1),
        dag::ConstSpan<uint16_t>(&riIdx, 1), dag::ConstSpan<uint32_t>(&zeroLodOffset, 1), rendinst::RenderPass::Normal,
        rendinst::OptimizeDepthPass::Yes, rendinst::OptimizeDepthPrepass::No, rendinst::IgnoreOptimizationLimits::No,
        rendinst::LayerFlag::Opaque);
    }
    else
    {
      G_ASSERT(re.sceneInstance);
      if (!dynrend::render_in_tools(re.sceneInstance, dynrend::RenderMode::Opaque))
        re.sceneInstance->render();
    }
  }

  ShaderGlobal::setBlock(lastFrameBlockId, ShaderGlobal::LAYER_FRAME);

  d3d::set_render_target(prevRT);
  ShaderGlobal::set_texture(simple_outline_color_rtVarId, colorRt.getId());
  finalRender.render();
}
