// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/fx/auroraBorealis.h>
#include <perfMon/dag_statDrv.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>

static inline Point4 point4(const Point3 &xyz, float w) { return Point4(xyz.x, xyz.y, xyz.z, w); }

void AuroraBorealis::setVars()
{
  ShaderGlobal::set_color4(aurora_borealis_bottomVarId, point4(params.bottom_color, params.bottom_height));
  ShaderGlobal::set_color4(aurora_borealis_topVarId, point4(params.top_color, fmax(params.top_height, params.bottom_height + 1.0)));
  ShaderGlobal::set_real(aurora_borealis_brightnessVarId, params.brightness);
  ShaderGlobal::set_real(aurora_borealis_luminanceVarId, params.luminance);
  ShaderGlobal::set_real(aurora_borealis_speedVarId, params.speed);
  ShaderGlobal::set_real(aurora_borealis_ripples_scaleVarId, params.ripples_scale);
  ShaderGlobal::set_real(aurora_borealis_ripples_speedVarId, params.ripples_speed);
  ShaderGlobal::set_real(aurora_borealis_ripples_strengthVarId, params.ripples_strength);
}

void AuroraBorealis::setParams(const AuroraBorealisParams &parameters, int targetW, int targetH)
{
  if (this->params != parameters)
  {
    this->params = parameters;
    setVars();
  }

  const int AURORA_BOREALIS_LEVEL = 4;

  if (auroraBorealisTex.getTex2D())
  {
    TextureInfo tinfo;
    auroraBorealisTex.getTex2D()->getinfo(tinfo);
    if (!params.enabled || targetW / AURORA_BOREALIS_LEVEL != tinfo.w || targetH / AURORA_BOREALIS_LEVEL != tinfo.h)
      auroraBorealisTex.close();
  }

  if (params.enabled && targetW > 0 && !auroraBorealisTex.getTex2D())
  {
    uint32_t noAlphaSkyHdrFmt = TEXFMT_R11G11B10F;
    if ((d3d::get_texformat_usage(noAlphaSkyHdrFmt) & (d3d::USAGE_RTARGET | d3d::USAGE_FILTER)) !=
        (d3d::USAGE_RTARGET | d3d::USAGE_FILTER))
      noAlphaSkyHdrFmt = TEXCF_SRGBREAD | TEXCF_SRGBWRITE;

    int skyW = targetW / AURORA_BOREALIS_LEVEL, skyH = targetH / AURORA_BOREALIS_LEVEL;
    auroraBorealisTex = dag::create_tex(NULL, skyW, skyH, TEXCF_RTARGET | noAlphaSkyHdrFmt, 1, "aurora_borealis_tex");
    auroraBorealisTex.setVar();
    auroraBorealisTex->disableSampler();
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    ShaderGlobal::set_sampler(get_shader_variable_id("aurora_borealis_tex_samplerstate"), d3d::request_sampler(smpInfo));
  }
}

void AuroraBorealis::beforeRender()
{
  if (params.enabled && auroraBorealisTex.getTex2D())
  {
    TIME_D3D_PROFILE(render_aurora_borealis);
    SCOPE_RENDER_TARGET;

    d3d::set_render_target(auroraBorealisTex.getTex2D(), 0);
    auroraBorealisFX.render();
    d3d::resource_barrier({auroraBorealisTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }
}

void AuroraBorealis::render()
{
  if (params.enabled && auroraBorealisTex.getTex2D())
  {
    TIME_D3D_PROFILE(apply_aurora_borealis);

    applyAuroraBorealis.render();
  }
}

AuroraBorealisParams AuroraBorealisParams::read(const DataBlock &blk)
{
  AuroraBorealisParams result; // default constructor

  result.enabled = blk.getBool("enabled", result.enabled);
  result.top_height = blk.getReal("top_height", result.top_height);
  result.top_color = blk.getPoint3("top_color", result.top_color);
  result.bottom_height = blk.getReal("bottom_height", result.bottom_height);
  result.bottom_color = blk.getPoint3("bottom_color", result.bottom_color);
  result.brightness = blk.getReal("brightness", result.brightness);
  result.luminance = blk.getReal("luminance", result.luminance);
  result.ripples_scale = blk.getReal("ripples_scale", result.ripples_scale);
  result.ripples_speed = blk.getReal("ripples_speed", result.ripples_speed);
  result.ripples_strength = blk.getReal("ripples_strength", result.ripples_strength);

  return result;
}

bool AuroraBorealisParams::operator!=(const AuroraBorealisParams &other) const
{
  return this->enabled != other.enabled || this->top_height != other.top_height || this->top_color != other.top_color ||
         this->bottom_height != other.bottom_height || this->bottom_color != other.bottom_color ||
         this->brightness != other.brightness || this->luminance != other.luminance || this->ripples_scale != other.ripples_scale ||
         this->ripples_speed != other.ripples_speed || this->ripples_strength != other.ripples_strength;
}

void AuroraBorealisParams::write(DataBlock &blk) const
{
  blk.setBool("enabled", enabled);
  blk.setReal("top_height", top_height);
  blk.setPoint3("top_color", top_color);
  blk.setReal("bottom_height", bottom_height);
  blk.setPoint3("bottom_color", bottom_color);
  blk.setReal("brightness", brightness);
  blk.setReal("luminance", luminance);
  blk.setReal("ripples_scale", ripples_scale);
  blk.setReal("ripples_speed", ripples_speed);
  blk.setReal("ripples_strength", ripples_strength);
}

AuroraBorealis::AuroraBorealis()
{
  auroraBorealisFX.init("aurora_borealis");
  applyAuroraBorealis.init("apply_aurora_borealis");
  aurora_borealis_bottomVarId = ::get_shader_variable_id("aurora_borealis_bottom", false);
  aurora_borealis_topVarId = ::get_shader_variable_id("aurora_borealis_top", false);
  aurora_borealis_brightnessVarId = ::get_shader_variable_id("aurora_borealis_brightness", false);
  aurora_borealis_luminanceVarId = ::get_shader_variable_id("aurora_borealis_luminance", false);
  aurora_borealis_speedVarId = ::get_shader_variable_id("aurora_borealis_speed", false);
  aurora_borealis_ripples_scaleVarId = ::get_shader_variable_id("aurora_borealis_ripples_scale", false);
  aurora_borealis_ripples_speedVarId = ::get_shader_variable_id("aurora_borealis_ripples_speed", false);
  aurora_borealis_ripples_strengthVarId = ::get_shader_variable_id("aurora_borealis_ripples_strength", false);

  setVars();
}

AuroraBorealis::~AuroraBorealis() { auroraBorealisTex.close(); }
