#include <render/genericLUT/genericLUT.h>
#include <3d/dag_drv3d.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaders.h>

GenericTonemapLUT::~GenericTonemapLUT() = default;
GenericTonemapLUT::GenericTonemapLUT() = default;

static int lut_sizeVarId = -1, first_lut_rsliceVarId = -1, tonemap_sim_rtVarId = -1;

static bool check_compute_support()
{
  auto &drvDesc = d3d::get_driver_desc();
  return drvDesc.shaderModel >= 5.0_sm && !drvDesc.issues.hasComputeCanNotWrite3DTex;
}

static uint32_t get_color_format(GenericTonemapLUT::HDROutput hdr)
{
  if (hdr == GenericTonemapLUT::HDROutput::HDR && d3d::get_texformat_usage(TEXFMT_A16B16G16R16F, RES3D_VOLTEX) & d3d::USAGE_RTARGET)
    return TEXFMT_A16B16G16R16F;
  if (d3d::get_texformat_usage(TEXFMT_A2B10G10R10, RES3D_VOLTEX) & d3d::USAGE_RTARGET)
    return TEXFMT_A2B10G10R10;
  return TEXFMT_DEFAULT;
}

bool GenericTonemapLUT::init(const char *lut_name, const char *render_shader_name, const char *compute_shader_name, HDROutput hdr,
  int lut_size)
{
  lutSize = lut_size;

  uint32_t flag = get_color_format(hdr);

  if (ComputeShaderElement * computeShader;
      check_compute_support() && (d3d::get_texformat_usage(flag, RES3D_VOLTEX) & d3d::USAGE_UNORDERED) == d3d::USAGE_UNORDERED &&
      (computeShader = new_compute_shader(compute_shader_name, true)) != nullptr)
  {
    computeLUT.reset(computeShader);
    flag |= TEXCF_UNORDERED;
  }
  else
  {
    if (!render_shader_name)
      return false;

    renderLUT = eastl::make_unique<PostFxRenderer>();
    renderLUT->init(render_shader_name);
    flag |= TEXCF_RTARGET;
    first_lut_rsliceVarId = get_shader_variable_id("first_lut_rslice", true);
    tonemap_sim_rtVarId = get_shader_variable_id("tonemap_sim_rt", true);
  }

  lut_sizeVarId = get_shader_variable_id("lut_size", true);

  lut = dag::create_voltex(lutSize, lutSize, lutSize, flag, 1, lut_name);
  if (auto voltex = lut.getVolTex())
  {
    voltex->texaddr(TEXADDR_CLAMP);
    return true;
  }

  return false;
}

bool GenericTonemapLUT::perform()
{
  ShaderGlobal::set_int(lut_sizeVarId, lutSize);
  if (computeLUT)
  {
    d3d::set_rwtex(STAGE_CS, 0, lut.getVolTex(), 0, 0);
    computeLUT->dispatchThreads(lutSize + 3, lutSize + 3, lutSize + 3);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    d3d::resource_barrier({lut.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    return true;
  }

  if (renderLUT)
  {
    SCOPE_RENDER_TARGET;
    int maxSimRT = d3d::get_driver_desc().maxSimRT; // assume we support up to 8 render targets, and it is known per-platform
    maxSimRT = (maxSimRT >= 8) ? 8 : 4;
    ShaderGlobal::set_int(tonemap_sim_rtVarId, maxSimRT);
    d3d::set_render_target();
    for (int i = 0; i < lutSize; i += maxSimRT) // with geom shaders we can render all targets, but geom shaders are not supported in
                                                // metal
    {
      for (int j = 0; j < maxSimRT; ++j)
        d3d::set_render_target(j, lut.getVolTex(), i + j, 0);
      ShaderGlobal::set_int(first_lut_rsliceVarId, i);
      renderLUT->render();
    }
    d3d::resource_barrier({lut.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    return true;
  }

  return false;
}
