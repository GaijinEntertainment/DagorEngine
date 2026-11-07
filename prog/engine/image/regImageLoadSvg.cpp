// Copyright (C) Gaijin Games KFT.  All rights reserved.

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
#include <zlib.h>

// route nanosvg allocations through the engine allocator
#define malloc  memalloc_default   //-V1059
#define free    memfree_default    //-V1059
#define realloc memrealloc_default //-V1059
#define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include <nanosvg/nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#define NSVG_RED_INDEX   2
#define NSVG_GREEN_INDEX 1
#define NSVG_BLUE_INDEX  0
#include <nanosvg/nanosvgrast.h>
#include "decodeFnameSuffix.h"

static bool check_svg_size(const char *fn, uint32_t size)
{
  if (size > (1 << 20))
  {
    logerr("SVG '%.128s' is too big: %u bytes, more than 1 MB, not loaded", fn, size);
    return false;
  }
  if (size > (100 << 10))
    logerr("SVG '%.128s' is too big: %u bytes, keep it under 100 KB", fn, size);
  return true;
}

static bool is_gzip(const char *data, int size) { return size >= 18 && uint8_t(data[0]) == 0x1F && uint8_t(data[1]) == 0x8B; }

static bool gunzip(dag::ConstSpan<char> src, Tab<char> &dst, const char *fn)
{
  uint32_t unpacked_sz; // gzip trailer ends with the unpacked size
  memcpy(&unpacked_sz, src.data() + src.size() - sizeof(unpacked_sz), sizeof(unpacked_sz));
  z_stream zs = {};
  if (!check_svg_size(fn, unpacked_sz) || inflateInit2(&zs, MAX_WBITS + 16) != Z_OK)
    return false;
  dst.reserve(unpacked_sz + 1);
  dst.resize(unpacked_sz);
  zs.next_in = (Bytef *)src.data();
  zs.avail_in = src.size();
  zs.next_out = (Bytef *)dst.data();
  zs.avail_out = unpacked_sz;
  int res = inflate(&zs, Z_FINISH);
  inflateEnd(&zs);
  return res == Z_STREAM_END && zs.total_out == unpacked_sz;
}

static bool finish_svg_text(Tab<char> &svg, const char *fn)
{
  if (is_gzip(svg.data(), svg.size()))
  {
    Tab<char> unpacked(tmpmem);
    if (!gunzip(svg, unpacked, fn))
    {
      logerr("cannot unpack gzipped SVG <%s>", fn);
      return false;
    }
    svg.swap(unpacked);
  }
  svg.push_back('\0');
  return true;
}

static bool read_svg_file(const char *fn, int df_flags, Tab<char> &svg)
{
  const char *fn_ext = dd_get_fname_ext(fn);
  const char *suffix = fn_ext ? strchr(fn_ext, ':') : nullptr;
  file_ptr_t filePtr = df_open(suffix ? String::mk_sub_str(fn, suffix).c_str() : fn, df_flags);
  if (!filePtr)
  {
    if (!(df_flags & DF_IGNORE_MISSING))
      logerr("no such file %s", fn);
    return false;
  }
  int file_sz = df_length(filePtr);
  if (!check_svg_size(fn, file_sz))
  {
    df_close(filePtr);
    return false;
  }
  svg.resize(file_sz + 1);
  svg.resize(max(df_read(filePtr, svg.data(), file_sz), 0));
  df_close(filePtr);
  return finish_svg_text(svg, fn);
}

class SvgLoadImageFactory : public ILoadImageFactory
{
public:
  bool readImageDimensions(const char *fn, const char *fn_ext, int &out_w, int &out_h, bool &out_may_have_alpha) override
  {
    bool keep_ar, premul;
    if (!decodeSvgSuffix(fn_ext, out_w, out_h, keep_ar, premul))
      return false;

    out_may_have_alpha = true;
    if (out_w && out_h && !keep_ar)
      return true;

    Tab<char> svg(tmpmem);
    if (!read_svg_file(fn, DF_READ | DF_IGNORE_MISSING, svg))
      return false;

    float w = 0.f, h = 0.f;
    if (NSVGimage *image = nsvgParse(svg.data(), "px", 96.0f))
    {
      w = image->width;
      h = image->height;
      nsvgDelete(image);
    }
    if (w > 0 && h > 0)
    {
      int pic_w = out_w, pic_h = out_h;

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
      out_w = max(1, (int)floorf(w + 0.5f));
      out_h = max(1, (int)floorf(h + 0.5f));
    }
    return true;
  }
  TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha) override
  {
    int w, h;
    bool keep_ar, premul;
    if (!decodeSvgSuffix(fn_ext, w, h, keep_ar, premul))
      return NULL;

    Tab<char> svg(tmpmem);
    if (!read_svg_file(fn, DF_READ, svg))
      return NULL;
    if (out_used_alpha)
      *out_used_alpha = true;
    return rasterize_svg(make_span(svg), mem, w, h, keep_ar, premul, fn);
  }
  TexImage32 *loadImage(IGenLoad &crd, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha) override
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
      if (svg.size() > (4 << 20))
      {
        logerr("SVG stream '%.128s' is too big: more than 4 MB, not loaded", crd.getTargetName());
        return NULL;
      }
      if (rd < BUF_SZ)
        break;
    }
    if (!finish_svg_text(svg, crd.getTargetName()))
      return NULL;
    if (out_used_alpha)
      *out_used_alpha = true;
    return rasterize_svg(make_span(svg), mem, w, h, keep_ar, premul, crd.getTargetName());
  }
  bool supportLoadImage2() override { return false; }
  void *loadImage2(const char *, IAllocImg &, const char *) override { return nullptr; }

  static bool decodeSvgSuffix(const char *fn_ext, int &out_w, int &out_h, bool &out_keep_ar, bool &out_premul)
  {
    out_keep_ar = true;
    out_premul = false;
    if (!fn_ext || dd_strnicmp(fn_ext, ".svg", 4) != 0)
      return false;
    const char *suffix = fn_ext + 4 + (tolower(fn_ext[4]) == 'z');
    if (!*suffix)
    {
      out_w = out_h = 0;
      return true;
    }
    if (*suffix == ':')
      return decode_fname_suffix(suffix + 1, out_w, out_h, out_keep_ar, out_premul, fn_ext);
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

    TexImage32 *img = TexImage32::tryCreate(wi, hi, mem);
    if (DAGOR_UNLIKELY(!img))
    {
      logerr("failed to alloc image %dx%d for %s", wi, hi, fn);
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
