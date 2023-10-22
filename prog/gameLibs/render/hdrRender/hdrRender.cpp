#include <render/hdrRender.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_resPtr.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <ioSys/dag_dataBlock.h>

namespace
{
namespace defaults
{
#include "../shaders/hdr/hdr_render_defaults.sh"
}
} // namespace

#define GLOBAL_VARS_LIST  \
  VAR(hdr_ps_output_mode) \
  VAR(paper_white_nits)   \
  VAR(hdr_brightness)     \
  VAR(hdr_shadows)        \
  VAR(hdr_enabled)        \
  VAR(hdr_source_offset)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

// Same as in render_options.nut
constexpr float MIN_PAPER_WHITE_NITS = 100;
constexpr float MAX_PAPER_WHITE_NITS = 1000;

static UniqueTexHolder float_rt_tex;
eastl::unique_ptr<PostFxRenderer> encode_hdr_renderer;

bool hdrrender::is_hdr_enabled() { return !!d3d::driver_command(DRV3D_COMMAND_IS_HDR_ENABLED, NULL, NULL, NULL); }

bool hdrrender::int10_hdr_buffer() { return d3d::driver_command(DRV3D_COMMAND_INT10_HDR_BUFFER, NULL, NULL, NULL); }

bool hdrrender::is_hdr_available() { return d3d::driver_command(DRV3D_COMMAND_IS_HDR_AVAILABLE, NULL, NULL, NULL); }

bool hdrrender::is_active() { return !!encode_hdr_renderer; }

HdrOutputMode hdrrender::get_hdr_output_mode()
{
  return static_cast<HdrOutputMode>(d3d::driver_command(DRV3D_COMMAND_HDR_OUTPUT_MODE, NULL, NULL, NULL));
}

void hdrrender::init_globals(const DataBlock &videoCfg)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR

  update_globals();

  float paper_white_nits = videoCfg.getInt("paperWhiteNits", int(defaults::paper_white_nits));
  float hdr_brightness = videoCfg.getReal("hdr_brightness", defaults::hdr_brightness);
  float hdr_shadows = videoCfg.getReal("hdr_shadows", defaults::hdr_shadows);

  paper_white_nits = eastl::clamp(paper_white_nits, MIN_PAPER_WHITE_NITS, MAX_PAPER_WHITE_NITS);

  update_paper_white_nits(paper_white_nits);
  update_hdr_brightness(hdr_brightness);
  update_hdr_shadows(hdr_shadows);
}

static void create_hdr_fp_tex(int width, int height, int fp_format, bool uav_usage)
{
  const int flg = fp_format | TEXCF_RTARGET | (uav_usage ? TEXCF_UNORDERED : 0);
  if (auto tex = float_rt_tex.getTex2D())
  {
    TextureInfo info;
    tex->getinfo(info);
    if (info.w == width && info.h == height && info.cflg == flg)
      return;
    float_rt_tex.close();
  }

  float_rt_tex = dag::create_tex(NULL, width, height, flg, 1, "hdr_fp_tex");
  G_ASSERT(float_rt_tex);
}

static void init(int width, int height, int fp_format, bool uav_usage, const char *shader_name)
{
  debug("[HDR] init called with %d, %d, %s", fp_format, (int)uav_usage, shader_name);

  ::create_hdr_fp_tex(width, height, fp_format, uav_usage);

  encode_hdr_renderer.reset(new PostFxRenderer);
  encode_hdr_renderer->init(shader_name);
}

void hdrrender::init(int width, int height, int fp_format, bool uav_usage)
{
  debug("[HDR] hdrrender::init called with %d, %d", fp_format, (int)uav_usage);

  ShaderGlobal::set_int(hdr_enabledVarId, is_hdr_enabled() ? 1 : 0);

  if (!is_hdr_enabled())
    return;

  auto shaderName = int10_hdr_buffer() ? "encode_hdr_to_int10" : "encode_hdr_to_float";
  ::init(width, height, fp_format, uav_usage, shaderName);
}

void hdrrender::init(int width, int height, bool linear_input, bool uav_usage)
{
  debug("[HDR] hdrrender::init (linear_input) called with %d, %d", (int)linear_input, (int)uav_usage);

  if (!linear_input)
    return init((int)TEXFMT_R11G11B10F, uav_usage);

  if (!is_hdr_enabled())
  {
    shutdown();
    return;
  }

  int fpFormat = int10_hdr_buffer() ? TEXFMT_R11G11B10F : TEXFMT_A16B16G16R16F;
  ::init(width, height, fpFormat, uav_usage, "encode_hdr_from_linear");
}

void hdrrender::set_resolution(int width, int height, bool uav_usage)
{
  int fpFormat = int10_hdr_buffer() ? TEXFMT_R11G11B10F : TEXFMT_A16B16G16R16F;
  ::create_hdr_fp_tex(width, height, fpFormat, uav_usage);
}

void hdrrender::shutdown()
{
  ShaderGlobal::set_int(hdr_enabledVarId, 0);

  float_rt_tex.close();
  encode_hdr_renderer.reset();
}

Texture *hdrrender::get_render_target_tex() { return float_rt_tex.getTex2D(); }

TEXTUREID hdrrender::get_render_target_tex_id() { return float_rt_tex.getTexId(); }

const ManagedTex &hdrrender::get_render_target() { return float_rt_tex; }

void hdrrender::set_render_target()
{
  if (is_hdr_enabled())
  {
    d3d::set_render_target(float_rt_tex.getTex2D(), 0);
    d3d::set_backbuf_depth();
  }
  else
    d3d::set_render_target();
}

bool hdrrender::encode(int x_offset, int y_offset)
{
  if (is_hdr_enabled())
  {
    G_ASSERT(encode_hdr_renderer);

    TIME_D3D_PROFILE(encode_hdr);
    ShaderGlobal::set_color4(hdr_source_offsetVarId, x_offset, y_offset, 0, 0);
    float_rt_tex.setVar();
    encode_hdr_renderer->render();
    return true;
  }
  return false;
}

void hdrrender::update_globals()
{
  int outputMode = d3d::driver_command(DRV3D_COMMAND_HDR_OUTPUT_MODE, NULL, NULL, NULL);
  ShaderGlobal::set_int(hdr_ps_output_modeVarId, outputMode);
}

void hdrrender::update_paper_white_nits(float value) { ShaderGlobal::set_real(paper_white_nitsVarId, value); }

void hdrrender::update_hdr_brightness(float value) { ShaderGlobal::set_real(hdr_brightnessVarId, value); }

void hdrrender::update_hdr_shadows(float value) { ShaderGlobal::set_real(hdr_shadowsVarId, value); }

float hdrrender::get_default_paper_white_nits() { return defaults::paper_white_nits; }

float hdrrender::get_default_hdr_brightness() { return defaults::hdr_brightness; }

float hdrrender::get_default_hdr_shadows() { return defaults::hdr_shadows; }

float hdrrender::get_paper_white_nits() { return ShaderGlobal::get_real(paper_white_nitsVarId); }

float hdrrender::get_hdr_brightness() { return ShaderGlobal::get_real(hdr_brightnessVarId); }

float hdrrender::get_hdr_shadows() { return ShaderGlobal::get_real(hdr_shadowsVarId); }
