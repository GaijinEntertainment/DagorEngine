#include <render/upscale/upscaleSampling.h>

#include <3d/dag_drv3d.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/upscale_sampling_weights.hlsli>
#include <shaders/dag_shaderBlock.h>

namespace
{

static const float UPSCALE_WEIGHTS_DATA[UPSCALE_WEIGHTS_COUNT * 4] = {
  // Bottom Right
  0.75 * 0.25, 0.25 * 0.25, 0.75 * 0.25, 0.75 * 0.75, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0.25, 0.75, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0.25, 0.75,
  0, 0, 0, 0.25, 0.75, 1, 0, 0, 0, 0.25, 0, 0, 0.75, 1, 0, 0, 0, 0.25, 0, 0, 0.75, 0.75, 0.25, 0, 0, 0.25, 0, 0, 0.75, 0.75, 0.25, 0,
  0, 0.75 * 0.25, 0.25 * 0.25, 0.75 * 0.25, 0.75 * 0.75,
  // Bottom Left
  0.25 * 0.25, 0.75 * 0.25, 0.75 * 0.75, 0.75 * 0.25, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0.75, 0.25, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0.25, 0.75,
  0, 0, 0.25, 0.75, 0, 1, 0, 0, 0, 0.25, 0, 0, 0.75, 0, 0, 1, 0, 0, 0, 0.75, 0.25, 0.25, 0.75, 0, 0, 0.25, 0.75, 0, 0, 0, 0.25, 0.75,
  0, 0.25 * 0.25, 0.75 * 0.25, 0.75 * 0.75, 0.75 * 0.25,
  // Top Right
  0.75 * 0.75, 0.75 * 0.25, 0.25 * 0.25, 0.75 * 0.25, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0.25, 0.75, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0.75, 0.25,
  0, 0, 0, 0.25, 0.75, 1, 0, 0, 0, 0.75, 0, 0, 0.25, 1, 0, 0, 0, 0.75, 0, 0, 0.25, 0.75, 0.25, 0, 0, 0.75, 0, 0, 0.25, 0.75, 0.25, 0,
  0, 0.75 * 0.75, 0.75 * 0.25, 0.25 * 0.25, 0.75 * 0.25,
  // Top Left
  0.75 * 0.25, 0.75 * 0.75, 0.75 * 0.25, 0.25 * 0.25, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0.75, 0.25, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0.75, 0.25,
  0, 0, 0.75, 0.25, 0, 1, 0, 0, 0, 0.75, 0, 0, 0.25, 0, 0, 1, 0, 0, 0, 0.75, 0.25, 0.25, 0.75, 0, 0, 0.25, 0.75, 0, 0, 0, 0.75, 0.25,
  0, 0.75 * 0.25, 0.75 * 0.75, 0.75 * 0.25, 0.25 * 0.25};

}; // anonymous namespace

UpscaleSamplingTex::UpscaleSamplingTex(uint32_t w, uint32_t h)
{
  upscaleWeightsBuffer = dag::buffers::create_persistent_cb(UPSCALE_WEIGHTS_COUNT, "upscale_sampling_weights");

  upscaleRenderer.init("upscale_sampling");
  upscaleRendererCS.reset(new_compute_shader("upscale_sampling_cs", true));

  bool hasComputeSupport = upscaleRendererCS && d3d::should_use_compute_for_image_processing({TEXFMT_R8});

  uint32_t flags = TEXFMT_R8 | (hasComputeSupport ? TEXCF_UNORDERED : TEXCF_RTARGET);
  upscaleTex = dag::create_tex(NULL, w, h, flags, 1, "upscale_sampling_tex");

  uploadWeights();
}

void UpscaleSamplingTex::onReset() { uploadWeights(); }

void UpscaleSamplingTex::uploadWeights()
{
  upscaleWeightsBuffer.getBuf()->updateData(0, sizeof(UPSCALE_WEIGHTS_DATA), (void *)UPSCALE_WEIGHTS_DATA,
    VBLOCK_WRITEONLY | VBLOCK_DISCARD);
}

void UpscaleSamplingTex::render()
{
  TIME_D3D_PROFILE(upscale_sampling_tex);

  SCOPE_RESET_SHADER_BLOCKS;

  TextureInfo ti;
  if (upscaleTex)
    upscaleTex->getinfo(ti);

  if (upscaleTex && !!(ti.cflg & TEXCF_UNORDERED))
  {
    static int upscale_target_sizeVarId = ::get_shader_variable_id("upscale_target_size", true);
    ShaderGlobal::set_color4(upscale_target_sizeVarId, ti.w, ti.h);

    d3d::set_rwtex(STAGE_CS, 0, upscaleTex.getTex2D(), 0, 0);
    upscaleRendererCS->dispatchThreads(ti.w, ti.h, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    d3d::resource_barrier({upscaleTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  }
  else
  {
    SCOPE_RENDER_TARGET;

    auto target = upscaleTex.getTex2D();

    d3d::set_render_target(target, 0);
    upscaleRenderer.render();
    d3d::resource_barrier({target, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  }
}
