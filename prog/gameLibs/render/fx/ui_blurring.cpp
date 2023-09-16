#include <render/fx/ui_blurring.h>

#include <3d/dag_drv3d.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IBBox2.h>
#include <math/dag_hlsl_floatx.h>
#include <shaders/dag_shaders.h>
#include <perfMon/dag_statDrv.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>

#define GLOBAL_VARS_LIST   \
  VAR(blur_src_tex)        \
  VAR(blur_pixel_offset)   \
  VAR(blur_src_tex_size)   \
  VAR(blur_dst_tex_size)   \
  VAR(blur_background_tex) \
  VAR(blur_bw)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

const int FIRST_GAUSS_SIZE = 3, SECOND_GAUSS_SIZE = 3 * 7 + 1;
constexpr int FIRST_REG = 16; // same as in shader
constexpr int CB_SIZE = 4096 - FIRST_REG;

static void init_blur_shader_vars()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
}

static inline int calc_all_secondary_gauss_sizes(int max_ui_mip)
{
  int allSecondaryGaussSizes = 1;
  for (int i = 1; i < max_ui_mip; ++i)
    allSecondaryGaussSizes *= SECOND_GAUSS_SIZE;

  return allSecondaryGaussSizes;
}

static void blur_mip(int tex_mip, TextureIDHolder &tex, TextureIDHolder &interm_texture,
  int interm_texture_mip, // target_mip-bloom_last_mip_start_mip
  PostFxRenderer &blur, const IBBox2 *begin, const IBBox2 *end, int4 *quad_buffer)
{
  TextureInfo info;
  const int levels = tex.getTex2D()->level_count();
  G_ASSERTF((uint32_t)tex_mip < levels, "%d < %d", tex_mip, levels);
  G_ASSERTF((uint32_t)interm_texture_mip < interm_texture.getTex2D()->level_count(), "%d < %d", interm_texture_mip,
    interm_texture.getTex2D()->level_count());
  tex_mip = min(levels - 1, tex_mip);
  tex.getTex2D()->getinfo(info, tex_mip);
  const int w = info.w, h = info.h;

  interm_texture.getTex2D()->getinfo(info, interm_texture_mip);
  G_ASSERTF(info.w == w && info.h == h, "%dx%d (%d) != %dx%d (mip = %d)", w, h, tex_mip, info.w, info.h, interm_texture_mip);
  d3d::set_render_target(interm_texture.getTex2D(), interm_texture_mip);
  interm_texture.getTex2D()->texmiplevel(interm_texture_mip, interm_texture_mip);
  ShaderGlobal::set_texture(blur_src_texVarId, tex.getId());
  ShaderGlobal::set_color4(blur_pixel_offsetVarId, 1.0f / w, 0, 0, 0);
  tex.getTex2D()->texmiplevel(tex_mip, tex_mip);

  int4 *quad = quad_buffer;
  for (auto s = begin; s != end; ++s)
  {
    if (s->lim[1].x <= s->lim[0].x || s->lim[1].y <= s->lim[0].y)
      continue;

    *(quad++) = int4(s->lim[0].x, s->lim[0].y, (s->lim[1].x - s->lim[0].x), (s->lim[1].y - s->lim[0].y));
  }

  blur.getElem()->setStates();
  const uint32_t quadCount = quad - quad_buffer;
  d3d::set_vs_constbuffer_size(quadCount + FIRST_REG);
  d3d::set_vs_const(FIRST_REG, quad_buffer, quadCount);
  d3d::draw_instanced(PRIM_TRISTRIP, 0, 2, quadCount);

  d3d::set_render_target(tex.getTex2D(), tex_mip);
  d3d::resource_barrier({interm_texture.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(interm_texture_mip), 1});
  ShaderGlobal::set_texture(blur_src_texVarId, interm_texture.getId());
  ShaderGlobal::set_color4(blur_pixel_offsetVarId, 0, 1.0f / h, 0, 0);

  blur.getElem()->setStates();
  d3d::draw_instanced(PRIM_TRISTRIP, 0, 2, quadCount);

  interm_texture.getTex2D()->texmiplevel(-1, -1);
  tex.getTex2D()->texmiplevel(-1, -1);
}

void update_blurred_from(const TextureIDPair &src, const IBBox2 *begin, const IBBox2 *end, const TextureIDPair &downsampled_frame,
  TextureIDHolder &ui_mip, const int max_ui_mip, PostFxRenderer &downsample_first_step, PostFxRenderer &downsample_4,
  PostFxRenderer &blur, TextureIDHolder &bloom_last_mip, bool bw)
{
  update_blurred_from(nullptr, src, begin, end, downsampled_frame, ui_mip, max_ui_mip, downsample_first_step, downsample_4, blur,
    bloom_last_mip, bw);
}

void update_blurred_from(const TextureIDPair &src, const TextureIDPair &background, const IBBox2 *begin, const IBBox2 *end,
  const TextureIDPair &intermediate, TextureIDHolder &ui_mip, const int max_ui_mip_, PostFxRenderer &downsample_first_step,
  PostFxRenderer &downsample_4, PostFxRenderer &blur, TextureIDHolder &intermediate2, bool bw)
{
  if (begin == end)
    return;
  if (blur_src_texVarId < 0)
    init_blur_shader_vars();
  TIME_D3D_PROFILE(updateBlurredScene);
  SCOPE_RENDER_TARGET;
  d3d::set_render_target();
  const int max_ui_mip = min((uint32_t)max_ui_mip_, (uint32_t)ui_mip.getTex2D()->level_count());
  G_ASSERT(intermediate2.getTex() || max_ui_mip < 2);
  int allSecondaryGaussSizes = calc_all_secondary_gauss_sizes(max_ui_mip);
  const int totalGaussSize = FIRST_GAUSS_SIZE * allSecondaryGaussSizes;
  TextureInfo texInfo;
  background.getTex2D()->getinfo(texInfo);
  TextureInfo src_info;
  (src.getTex() ? src.getTex2D() : background.getTex2D())->getinfo(src_info, 0);
  const Point2 screenToTextureCoords((float)src_info.w / float(texInfo.w), (float)src_info.h / float(texInfo.h));
  TextureInfo ui_info;
  ui_mip.getTex2D()->getinfo(ui_info, 0);
  TextureInfo dinfo;
  if (end - begin > CB_SIZE)
  {
    logerr("Too many ui elements for bluring, count: %d, max: %d", end - begin, CB_SIZE);
    end = begin + CB_SIZE;
  }
  int4 quadBuffer[CB_SIZE];
  int downsampled_frame_mip = intermediate.getTex2D()->level_count() - 1;
  for (int l = 0, e = intermediate.getTex2D()->level_count(); l < e; ++l)
  {
    intermediate.getTex2D()->getinfo(dinfo, l);
    if (dinfo.w <= ui_info.w * 2 || dinfo.h <= ui_info.h * 2) // we downsample additionally while blurring, by 2x2
    {
      downsampled_frame_mip = l;
      break;
    }
  }
  intermediate.getTex2D()->getinfo(dinfo, downsampled_frame_mip);
  const float div = static_cast<float>(src_info.w) / dinfo.w;
  {
    TIME_D3D_PROFILE(downsample_and_blend);

    d3d::set_render_target(intermediate.getTex2D(), downsampled_frame_mip);
    ShaderGlobal::set_color4(blur_src_tex_sizeVarId, 1.0f / src_info.w, 1.0f / src_info.h, 0, 0);
    ShaderGlobal::set_texture(blur_src_texVarId, src.getTex() ? src.getId() : background.getId());
    ShaderGlobal::set_color4(blur_dst_tex_sizeVarId, 1.0f / dinfo.w, 1.0f / dinfo.h, 0, 0);
    ShaderGlobal::set_texture(blur_background_texVarId, background.getId());
    ShaderGlobal::set_real(blur_bwVarId, bw ? 1.f : 0.f);
    if (!src.getTex())
      d3d::resource_barrier({background.getTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});

    int4 *quad = quadBuffer;
    for (auto s = begin; s != end; ++s)
    {
      const IPoint2 lt_ = s->lim[0], rb_ = s->lim[1];
      IPoint2 fLt = IPoint2(lt_.x * screenToTextureCoords.x, lt_.y * screenToTextureCoords.y);
      IPoint2 fRb = IPoint2(rb_.x * screenToTextureCoords.x, rb_.y * screenToTextureCoords.y);
      IPoint2 lt = ipoint2(floor(point2(fLt) / div)) - IPoint2(totalGaussSize + 2, totalGaussSize + 2);
      IPoint2 rb = ipoint2(ceil(point2(fRb) / div)) + IPoint2(totalGaussSize + 1, totalGaussSize + 1);

      lt = max(lt, IPoint2(0, 0));
      rb = min(rb, IPoint2(dinfo.w, dinfo.h));
      if (rb.x <= lt.x || rb.y <= lt.y)
        continue;

      *(quad++) = int4(lt.x, lt.y, rb.x, rb.y);
    }

    downsample_first_step.getElem()->setStates();
    const uint32_t quadCount = quad - quadBuffer;
    d3d::set_vs_constbuffer_size(quadCount + FIRST_REG);
    d3d::set_vs_const(FIRST_REG, quadBuffer, quadCount);
    d3d::draw_instanced(PRIM_TRISTRIP, 0, 2, quadCount);
  }
  // then downsample without border
  {
    TIME_D3D_PROFILE(downsample_mips);

    intermediate.getTex2D()->texmiplevel(downsampled_frame_mip, downsampled_frame_mip);

    if (intermediate2.getTex2D())
      intermediate2.getTex2D()->texaddr(TEXADDR_CLAMP);
    intermediate.getTex2D()->texaddr(TEXADDR_CLAMP);

    const int startUiMip = 0;
    for (int uiMipI = startUiMip; uiMipI < max_ui_mip; ++uiMipI)
    {
      TextureInfo tex_info;
      ui_mip.getTex2D()->getinfo(tex_info, uiMipI);
      const int srcW = uiMipI == 0 ? dinfo.w : (ui_info.w >> (uiMipI - 1)), srcH = uiMipI == 0 ? dinfo.h : (ui_info.h >> (uiMipI - 1));

      d3d::set_render_target(ui_mip.getTex2D(), uiMipI);
      ShaderGlobal::set_color4(blur_src_tex_sizeVarId, 1.0f / srcW, 1.0f / srcH, uiMipI, 0);
      ShaderGlobal::set_color4(blur_dst_tex_sizeVarId, 1.0f / tex_info.w, 1.0f / tex_info.h, 0, 0);

      if (uiMipI != startUiMip)
      {
        ui_mip.getTex2D()->texmiplevel(uiMipI - 1, uiMipI - 1);
        d3d::resource_barrier({ui_mip.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(uiMipI - 1), 1});
      }
      else
      {
        d3d::resource_barrier({intermediate.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(downsampled_frame_mip), 1});
      }
      ShaderGlobal::set_texture(blur_src_texVarId, uiMipI == startUiMip ? intermediate.getId() : ui_mip.getId());
      dag::Vector<IBBox2, framemem_allocator> mipBoxes;
      const uint32_t mask = (1 << uiMipI) - 1;

      int4 *quad = quadBuffer;
      for (auto s = begin; s != end; ++s)
      {
        const IPoint2 lt_ = s->lim[0], rb_ = s->lim[1];
        IPoint2 fLt = IPoint2(lt_.x * screenToTextureCoords.x, lt_.y * screenToTextureCoords.y);
        IPoint2 fRb = IPoint2(rb_.x * screenToTextureCoords.x, rb_.y * screenToTextureCoords.y);
        IPoint2 lt = ipoint2(floor(point2(fLt) / div)) - IPoint2(totalGaussSize + 2, totalGaussSize + 2);
        IPoint2 rb = ipoint2(ceil(point2(fRb) / div)) + IPoint2(totalGaussSize + 1, totalGaussSize + 1);

        if (rb.x <= lt.x || rb.y <= lt.y)
          continue;

        lt = IPoint2(lt.x >> (uiMipI + 1), lt.y >> (uiMipI + 1)) - IPoint2(1, 1);                   // with border for filtering
        rb = IPoint2((rb.x + mask) >> (uiMipI + 1), (rb.y + mask) >> (uiMipI + 1)) + IPoint2(1, 1); // with border for filtering

        lt = max(lt, IPoint2(0, 0));
        rb = min(rb, IPoint2(tex_info.w, tex_info.h));
        if (rb.x <= lt.x || rb.y <= lt.y)
          continue;

        *(quad++) = int4(lt.x, lt.y, rb.x, rb.y);

        if (uiMipI != startUiMip && max_ui_mip > 1) // additional blurring
          mipBoxes.push_back(IBBox2(lt, rb));
      }

      downsample_4.getElem()->setStates();
      const uint32_t quadCount = quad - quadBuffer;
      d3d::set_vs_constbuffer_size(quadCount + FIRST_REG);
      d3d::set_vs_const(FIRST_REG, quadBuffer, quadCount);
      d3d::draw_instanced(PRIM_TRISTRIP, 0, 2, quadCount);

      if (!mipBoxes.empty()) // blur even more
      {
        int intermediate2Mip = 0;
        for (int l = 0, e = intermediate2.getTex2D()->level_count(); l < e; ++l)
        {
          TextureInfo info2;
          intermediate2.getTex2D()->getinfo(info2, l);
          if (info2.w <= tex_info.w || info2.h <= tex_info.h)
          {
            intermediate2Mip = l;
            break;
          }
        }
        d3d::resource_barrier({ui_mip.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(uiMipI), 1});
        // fixme: chane this blur so it work with any intermediate texture (we just need temp textures, addressed from anywhere)
        blur_mip(uiMipI, ui_mip, intermediate2, intermediate2Mip, blur, mipBoxes.begin(), mipBoxes.end(), quadBuffer);
      }
    }
    d3d::resource_barrier({ui_mip.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(max_ui_mip - 1), 1});
    intermediate.getTex2D()->texmiplevel(-1, -1);
    ui_mip.getTex2D()->texmiplevel(-1, -1);
    d3d::set_vs_constbuffer_size(0);
  }
  shaders::overrides::reset();
  ShaderElement::invalidate_cached_state_block();
}
