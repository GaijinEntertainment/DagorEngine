#include <startup/dag_startupTex.h>
#include <image/dag_loadImage.h>
#include <image/dag_texPixel.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_genIo.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define malloc  memalloc_default
#define free    memfree_default
#define realloc memrealloc_default
#define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include <nanosvg/nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#define NSVG_RED_INDEX   2
#define NSVG_GREEN_INDEX 1
#define NSVG_BLUE_INDEX  0
#include <nanosvg/nanosvgrast.h>
#include "decodeFnameSuffix.h"

class SvgLoadImageFactory : public ILoadImageFactory
{
public:
  virtual TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    int w, h;
    bool keep_ar, premul;
    if (!decodeSvgSuffix(fn_ext, w, h, keep_ar, premul))
      return NULL;

    fn_ext = dd_get_fname_ext(fn);
    file_ptr_t filePtr = df_open(dd_strnicmp(fn_ext, ".svg:", 5) == 0 ? String(0, "%.*s", fn_ext + 4 - fn, fn) : fn, DF_READ);
    if (!filePtr)
    {
      logerr("no such file %s", fn);
      return NULL;
    }
    Tab<char> svg(tmpmem);
    int file_sz = df_length(filePtr);
    svg.resize(file_sz + 1);
    df_read(filePtr, svg.data(), file_sz);
    svg[file_sz] = '\0';
    df_close(filePtr);
    if (out_used_alpha)
      *out_used_alpha = true;
    return rasterize_svg(make_span(svg), mem, w, h, keep_ar, premul, fn);
  }
  virtual TexImage32 *loadImage(IGenLoad &crd, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    int w, h;
    bool keep_ar, premul;
    if (!decodeSvgSuffix(fn_ext, w, h, keep_ar, premul))
      return NULL;
    Tab<char> svg(tmpmem);
    for (;;)
    {
      static constexpr int BUF_SZ = 4096;
      char buf[BUF_SZ];
      int rd = crd.tryRead(buf, BUF_SZ);
      if (rd)
        append_items(svg, rd, buf);
      if (rd < BUF_SZ)
        break;
    }
    svg.push_back('\0');
    if (out_used_alpha)
      *out_used_alpha = true;
    return rasterize_svg(make_span(svg), mem, w, h, keep_ar, premul, crd.getTargetName());
  }
  virtual bool supportLoadImage2() { return false; }
  virtual void *loadImage2(const char *, IAllocImg &, const char *) { return NULL; }

  static bool decodeSvgSuffix(const char *fn_ext, int &out_w, int &out_h, bool &out_keep_ar, bool &out_premul)
  {
    out_keep_ar = true;
    out_premul = false;
    if (!fn_ext)
      return false;
    if (dd_stricmp(fn_ext, ".svg") == 0)
    {
      out_w = out_h = 0;
      return true;
    }
    if (dd_strnicmp(fn_ext, ".svg:", 5) == 0)
      return decode_fname_suffix(fn_ext + 5, out_w, out_h, out_keep_ar, out_premul, fn_ext);
    return false;
  }

  TexImage32 *rasterize_svg(dag::Span<char> svg, IMemAlloc *mem, int pic_w, int pic_h, bool keep_ar, bool premul, const char *fn)
  {
    struct NsvgPtr
    {
      NSVGimage *image = NULL;
      NSVGrasterizer *rast = NULL;
      ~NsvgPtr()
      {
        if (image)
          nsvgDelete(image);
        if (rast)
          nsvgDeleteRasterizer(rast);
      }
    } nsvg;

    nsvg.image = nsvgParse(svg.data(), "px", 96.0f);
    if (nsvg.image == NULL)
    {
      logerr("Could not open SVG image for <%s>.", fn);
      return NULL;
    }
    nsvg.rast = nsvgCreateRasterizer();
    if (nsvg.rast == NULL)
    {
      logerr("Could not init rasterizer");
      return NULL;
    }

    float w = nsvg.image->width, h = nsvg.image->height;
    float src_aspect = w / h;
    if (pic_w > 0 && pic_h > 0)
    {
      w = pic_w;
      h = pic_h;
      if (keep_ar) // special case dimensions are set, but we request keep aspect ratio
      {
        float w_from_h = h * src_aspect;
        float h_from_w = w / src_aspect;
        if (w_from_h < w)
          w = w_from_h;
        else if (h_from_w < h)
          h = h_from_w;
      }
    }
    else if (pic_w > 0)
      pic_set_dim(w, h, pic_w, 1.0f / src_aspect, keep_ar);
    else if (pic_h > 0)
      pic_set_dim(h, w, pic_h, src_aspect, keep_ar);

    if (pic_w < 0)
      pic_adjust_dim(w, h, -pic_w, 1.0f / src_aspect, keep_ar);
    if (pic_h < 0)
      pic_adjust_dim(h, w, -pic_h, src_aspect, keep_ar);
    int wi = (int)max(1.f, floorf(w + 0.5f)), hi = (int)max(1.0f, floorf(h + 0.5f));

    TexImage32 *img = TexImage32::create(wi, hi, mem);
    if (!img)
    {
      logerr("failed to create image %dx%d for %s", wi, hi, fn);
      return NULL;
    }
    nsvgRasterize(nsvg.rast, nsvg.image, 0, 0, w / nsvg.image->width, h / nsvg.image->height, (uint8_t *)img->getPixels(), wi, hi,
      wi * 4);
    if (premul)
      for (TexPixel32 *p = img->getPixels(), *pe = p + img->w * img->h; p < pe; p++)
      {
        p->r = unsigned(p->r) * p->a / 255;
        p->g = unsigned(p->g) * p->a / 255;
        p->b = unsigned(p->b) * p->a / 255;
        // p->a  is left intact
      }
    return img;
  }
};

static SvgLoadImageFactory svg_load_image_factory;

void register_svg_tex_load_factory() { add_load_image_factory(&svg_load_image_factory); }
