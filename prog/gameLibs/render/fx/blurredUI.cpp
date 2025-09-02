// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_renderTarget.h>
#include <render/fx/blurredUI.h>
#include <render/fx/ui_blurring.h>

void BlurredUI::close()
{
  finalSdr.close();
  final.close();
  closeIntermediate();
  intermediate2.close();
  sdrBackbufferTex.close();
}

void BlurredUI::closeIntermediate()
{
  if (ownsIntermediate)
    intermediate.releaseAndEvictTexId();

  intermediate = TextureIDPair(NULL, BAD_TEXTUREID);
  ownsIntermediate = false;
}

void BlurredUI::setIntermediate(int w, int h, TextureIDPair tex)
{
  const uint32_t fmtflags = TEXCF_RTARGET | TEXFMT_R11G11B10F | TEXCF_CLEAR_ON_CREATE;
  if (tex.getTex2D() != nullptr)
  {
    int levels = tex.getTex2D()->level_count(), l = 0;
    for (; l < levels; ++l)
    {
      TextureInfo info;
      tex.getTex2D()->getinfo(info, l);
      if ((info.w == w) && (info.h == h))
      {
        intermediate = tex;
        ownsIntermediate = false;
        return;
      }
    }
    logwarn("can't use texture for blurring of %dx%d", w, h);
  }

  auto texP = d3d::create_tex(NULL, w, h, fmtflags, 1, "ui_intermediate");

  TEXTUREID texId = register_managed_tex("ui_intermediate", texP);
  intermediate = TextureIDPair(texP, texId);
  ownsIntermediate = true;
}

void BlurredUI::ensureIntermediate2(int w, int h, int mips)
{
  const uint32_t uifmtflags = TEXCF_RTARGET | TEXFMT_R11G11B10F | TEXCF_CLEAR_ON_CREATE;
  intermediate2.close();
  intermediate2.set(d3d::create_tex(NULL, w, h, uifmtflags, mips, "ui_intermediate2"), "ui_intermediate2");
}

void BlurredUI::initSdrTex(int width, int height, int mips)
{
  const uint32_t uifmtflags = TEXCF_RTARGET | TEXFMT_R11G11B10F | TEXCF_CLEAR_ON_CREATE;
  finalSdr.close();
  finalSdr.set(d3d::create_tex(NULL, width / 8, height / 8, uifmtflags, mips, "ui_blurred_sdr"), "ui_blurred_sdr");
  finalSdrSampler = d3d::request_sampler({});
}

void BlurredUI::init(int width, int height, int mips, TextureIDPair interm)
{
  close();
  const uint32_t uifmtflags = TEXCF_RTARGET | TEXFMT_R11G11B10F | TEXCF_CLEAR_ON_CREATE;
  final.set(d3d::create_tex(NULL, width / 8, height / 8, uifmtflags, mips, "ui_blurred"), "ui_blurred");
  d3d::resource_barrier({final.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    finalSampler = d3d::request_sampler(smpInfo);
  }
  setIntermediate(width / 4, height / 4, interm);
  if (mips > 1)
    ensureIntermediate2(width / 16, height / 16, mips - 1);
  initialDownsample.init("ui_downsample_4x4");
  initialDownsampleAndBlend.init("ui_downsample_4x4_and_blend");
  subsequentDownsample.init("ui_downsample_blur");
  blur.init("ui_additional_blur");
};

void BlurredUI::updateFinalBlurred(const TextureIDPair &src, const IBBox2 *begin, const IBBox2 *end, int max_mip, bool bw)
{
  ::update_blurred_from(src, begin, end, intermediate, final, max_mip + 1, initialDownsample, subsequentDownsample, blur,
    intermediate2, bw);
  if (finalSdr.getTex())
  {
    updateSdrBackBufferTex();
    ::update_blurred_from({sdrBackbufferTex.getTex2D(), sdrBackbufferTex.getTexId()}, begin, end, intermediate, finalSdr, max_mip + 1,
      initialDownsample, subsequentDownsample, blur, intermediate2, bw);
  }
}

void BlurredUI::updateFinalBlurred(const TextureIDPair &src, const TextureIDPair &bkg, const IBBox2 *begin, const IBBox2 *end,
  int max_mip, bool bw)
{
  ::update_blurred_from(src, bkg, begin, end, intermediate, final, max_mip + 1, initialDownsampleAndBlend, subsequentDownsample, blur,
    intermediate2, bw);
  if (finalSdr.getTex())
  {
    updateSdrBackBufferTex();
    ::update_blurred_from({sdrBackbufferTex.getTex2D(), sdrBackbufferTex.getTexId()}, bkg, begin, end, intermediate, final,
      max_mip + 1, initialDownsampleAndBlend, subsequentDownsample, blur, intermediate2, bw);
  }
}

void BlurredUI::updateSdrBackBufferTex()
{
  if (sdrBackbufferTex && sdrBackbufferTex.getBaseTex() == d3d::get_secondary_backbuffer_tex())
    return;
  sdrBackbufferTex.close();
  sdrBackbufferTex = dag::get_secondary_backbuffer();
}
