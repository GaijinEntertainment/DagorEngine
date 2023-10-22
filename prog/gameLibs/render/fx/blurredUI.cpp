#include <3d/dag_tex3d.h>
#include <3d/dag_drv3d.h>
#include <render/fx/blurredUI.h>
#include <render/fx/ui_blurring.h>

void BlurredUI::close()
{
  final.close();
  closeIntermediate();
  intermediate2.close();
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
  const uint32_t fmtflags = TEXCF_RTARGET | TEXCF_SRGBREAD | TEXCF_SRGBWRITE;
  BaseTexture *texP = nullptr;

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

    texP = d3d::alias_tex(tex.getTex(), nullptr, w, h, fmtflags, 1, "ui_intermediate");
    if (!texP)
      logwarn("can't use texture for blurring of %dx%d", w, h);
  }

  if (!texP)
    texP = d3d::create_tex(NULL, w, h, fmtflags, 1, "ui_intermediate");

  texP->texaddr(TEXADDR_CLAMP);
  TEXTUREID texId = register_managed_tex("ui_intermediate", texP);
  intermediate = TextureIDPair(texP, texId);
  ownsIntermediate = true;
}

void BlurredUI::ensureIntermediate2(int w, int h, int mips)
{
  const uint32_t uifmtflags = TEXCF_RTARGET | TEXCF_SRGBREAD | TEXCF_SRGBWRITE;
  intermediate2.close();
  intermediate2.set(d3d::create_tex(NULL, w, h, uifmtflags, mips, "ui_intermediate2"), "ui_intermediate2");
  intermediate2.getTex2D()->texaddr(TEXADDR_CLAMP);
}

void BlurredUI::init(int width, int height, int mips, TextureIDPair interm)
{
  close();
  const uint32_t uifmtflags = TEXCF_RTARGET | TEXCF_SRGBREAD | TEXCF_SRGBWRITE;
  final.close();
  final.set(d3d::create_tex(NULL, width / 8, height / 8, uifmtflags, mips, "ui_blurred"), "ui_blurred");
  d3d::resource_barrier({final.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  final.getTex2D()->texaddr(TEXADDR_CLAMP);
  setIntermediate(width / 4, height / 4, interm);
  if (mips > 1)
    ensureIntermediate2(width / 16, height / 16, mips - 1);
  initial_downsample.init("ui_downsample_4x4");
  initial_downsample_and_blend.init("ui_downsample_4x4_and_blend");
  subsequent_downsample.init("ui_downsample_blur");
  blur.init("ui_additional_blur");
};

void BlurredUI::updateFinalBlurred(const TextureIDPair &src, const IBBox2 *begin, const IBBox2 *end, int max_mip, bool bw)
{
  ::update_blurred_from(src, begin, end, intermediate, final, max_mip + 1, initial_downsample, subsequent_downsample, blur,
    intermediate2, bw);
}

void BlurredUI::updateFinalBlurred(const TextureIDPair &src, const TextureIDPair &bkg, const IBBox2 *begin, const IBBox2 *end,
  int max_mip, bool bw)
{
  ::update_blurred_from(src, bkg, begin, end, intermediate, final, max_mip + 1, initial_downsample_and_blend, subsequent_downsample,
    blur, intermediate2, bw);
}