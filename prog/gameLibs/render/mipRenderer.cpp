// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/mipRenderer.h>
#include <3d/dag_textureIDHolder.h>
#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <math/integer/dag_IPoint2.h>

#define GLOBAL_VARS_LIST \
  VAR(mip_target_size)   \
  VAR(gaussian_mipchain_srgb)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

static int prev_mip_tex_register_const_no = -1;

MipRenderer::MipRenderer() {}

MipRenderer::~MipRenderer() { close(); }

bool MipRenderer::init(const char *shader)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
  int tmp = get_shader_variable_id("prev_mip_tex_register_const_no", true);
  prev_mip_tex_register_const_no = ShaderGlobal::get_int_fast(tmp);
  if (prev_mip_tex_register_const_no <= 0)
    return false;
  mipRenderer.init(shader);

  auto cshader = String(shader) += "_cs";
  mipRendererCS.reset(new_compute_shader(cshader.data(), true));

  return true;
}

void MipRenderer::close()
{
  mipRenderer.clear();
  mipRendererCS.reset();
}

void MipRenderer::renderTo(BaseTexture *src, BaseTexture *dst, const IPoint2 &target_size) const
{
  if (prev_mip_tex_register_const_no < 0)
    return;
  SCOPE_RENDER_TARGET;

  TextureInfo texInfo;

  src->getinfo(texInfo);
  d3d::set_render_target(dst, 0);
  ShaderGlobal::set_color4(mip_target_sizeVarId, target_size.x, target_size.y, 1.0f / target_size.x, 1.0f / target_size.y);
  src->texmiplevel(0, 0);
  d3d::resource_barrier({src, RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
  d3d::set_tex(STAGE_PS, prev_mip_tex_register_const_no, src);
  d3d::set_sampler(STAGE_PS, prev_mip_tex_register_const_no, d3d::request_sampler({}));
  mipRenderer.render();
  src->texmiplevel(-1, -1);
}

void MipRenderer::render(BaseTexture *tex, uint8_t max_level) const
{
  if (prev_mip_tex_register_const_no < 0)
  {
    tex->generateMips();
    return;
  }

  TextureInfo texInfo;
  tex->getinfo(texInfo);

  bool isCS = mipRendererCS && ((texInfo.cflg & TEXCF_UNORDERED) != 0);

  SCOPE_RENDER_TARGET;

  const int levelCount = min(tex->level_count(), int(max_level) + 1);

  d3d::set_sampler(isCS ? STAGE_CS : STAGE_PS, prev_mip_tex_register_const_no, d3d::request_sampler({}));
  for (int i = 1; i < levelCount; ++i)
  {
    int w = max(texInfo.w >> i, 1), h = max(texInfo.h >> i, 1);
    ShaderGlobal::set_color4(mip_target_sizeVarId, w, h, 1.0f / w, 1.0f / h);
    tex->texmiplevel(i - 1, i - 1);
    d3d::set_tex(isCS ? STAGE_CS : STAGE_PS, prev_mip_tex_register_const_no, tex);

    if (isCS)
    {
      bool isSRGB = !!(texInfo.cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE));
      ShaderGlobal::set_int(gaussian_mipchain_srgbVarId, isSRGB ? 1 : 0);

      d3d::resource_barrier(
        {{tex, tex}, {RB_RO_SRV | RB_STAGE_COMPUTE, RB_RW_UAV | RB_STAGE_COMPUTE}, {unsigned(i - 1), unsigned(i)}, {1U, 1U}});
      d3d::set_rwtex(STAGE_CS, 0, tex, 0, i);
      mipRendererCS->dispatchThreads(w, h, 1);
    }
    else
    {
      d3d::resource_barrier({tex, RB_RO_SRV | RB_STAGE_PIXEL, unsigned(i - 1), 1});
      d3d::set_render_target(tex, i);
      mipRenderer.render();
    }
  }

  if (isCS)
    d3d::resource_barrier({tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, unsigned(levelCount)});
  else
    d3d::resource_barrier({tex, RB_RO_SRV | RB_STAGE_PIXEL, unsigned(levelCount - 1), 1});

  tex->texmiplevel(-1, -1);
}