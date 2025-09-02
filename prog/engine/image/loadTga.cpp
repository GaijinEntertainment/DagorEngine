// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_tga.h>
#include <image/dag_texPixel.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_fileIo.h>
#include <util/dag_stdint.h>
#include <debug/dag_debug.h>


static uint32_t read_app_data(IGenLoad &cb, unsigned char *app_data, uint32_t max_app_data_len)
{
  const int64_t len = cb.getTargetDataSize();
  if (len < 0)
    return ~0u;
  uint32_t current = cb.tell();
  static constexpr uint32_t tagLen = 8;
  static constexpr uint32_t truevisionLen = 18;
  static constexpr uint32_t offsetLen = 8;
  if (current + tagLen + truevisionLen + offsetLen > len)
    return ~0u;
  cb.seekto(len - (truevisionLen + offsetLen));
  uint32_t tagsOffset = ~0u, padding;
  if (cb.tryRead(&tagsOffset, sizeof(tagsOffset)) != 4 || tagsOffset >= len || cb.tryRead(&padding, sizeof(padding)) != 4) // offset to
                                                                                                                           // tags
    return ~0u;
  char buf[truevisionLen];
  if (cb.tryRead(buf, truevisionLen) != truevisionLen || memcmp(buf, "TRUEVISION-XFILE.", truevisionLen) != 0) // correct our tag
    return ~0u;

  cb.seekto(tagsOffset);
  uint16_t tagsCount = 0;
  if (cb.tryRead(&tagsCount, sizeof(tagsCount)) != 2 || tagsCount < 1) // there are tags
    return ~0u;
  uint16_t tag;
  if (cb.tryRead(&tag, sizeof(tag)) != 2 || tag != 0) // it is not our tag
    return 0;
  uint32_t tagOffset = 0;
  if (cb.tryRead(&tagOffset, sizeof(tagOffset)) != 4 || tagOffset + 4 >= len) // correct tag offset
    return ~0u;
  uint32_t real_len = 0;
  if (cb.tryRead(&real_len, sizeof(real_len)) != 4 || real_len >= len) // correct tag offset
    return ~0u;
  cb.seekto(tagOffset);
  const uint32_t dataToRead = min(max_app_data_len, real_len);
  if (app_data && cb.tryRead(app_data, dataToRead) != dataToRead)
    return ~0u;
  return real_len;
}

static inline void conv_clr(int cb, uint8_t *c, TexPixel32 &p)
{
  if (cb == 4)
  {
#if _TARGET_CPU_BE
    p.a = c[0];
    p.r = c[1];
    p.g = c[2];
    p.b = c[3];
#else
    p.b = c[0];
    p.g = c[1];
    p.r = c[2];
    p.a = c[3];
#endif
  }
  else if (cb == 3)
  {
    p.b = c[0];
    p.g = c[1];
    p.r = c[2];
    p.a = 255;
  }
  else if (cb == 2)
  {
    uint16_t s = *(uint16_t *)c;
    p.b = (s << 3) & 0xF8;
    p.g = (s >> 2) & 0xF8;
    p.r = (s >> 7) & 0xF8;
    p.a = 255;
  }
  else if (cb == 1)
  {
    p.b = p.g = p.r = p.a = *c;
  }
}

static inline uint8_t conv_clr(int cb, uint8_t *c)
{
  if (cb == 4)
  {
#if _TARGET_CPU_BE
    return (c[3] + c[1] + c[2]) / 3;
#else
    return (c[0] + c[1] + c[2]) / 3;
#endif
  }
  else if (cb == 3)
  {
    return (c[0] + c[1] + c[2]) / 3;
  }
  else if (cb == 2)
  {
    uint16_t s = *(uint16_t *)c;
    return (((s << 3) & 0xF8) + ((s >> 2) & 0xF8) + ((s >> 7) & 0xF8)) / 3;
  }
  else if (cb == 1)
  {
    return *c;
  }
  return 0;
}

static inline void conv_clr(int cb, uint8_t *c, TexPixel8a &p)
{
  if (cb == 4)
  {
    p.l = (c[0] + c[1] + c[2]) / 3;
    p.a = c[3];
  }
  else if (cb == 3)
  {
    p.l = (c[0] + c[1] + c[2]) / 3;
    p.a = 255;
  }
  else if (cb == 2)
  {
    uint16_t s = *(uint16_t *)c;
    p.l = (((s << 3) & 0xF8) + ((s >> 2) & 0xF8) + ((s >> 7) & 0xF8)) / 3;
    p.a = 255;
  }
  else if (cb == 1)
  {
    p.l = p.a = *c;
  }
}

static inline uint16_t le2be16(uint16_t v) { return (v >> 8) | ((v & 0xFF) << 8); }

#pragma pack(push, 1)
// struct TgaHeader{short id,pack;int res2,res3;short w,h;char bits,alpha;};
struct TgaHeader
{
  uint8_t idlen, cmtype, pack;
  uint16_t cmorg, cmlen;
  uint8_t cmes;
  short xo, yo, w, h;
  char bits, desc;
#if _TARGET_CPU_BE
  void swap()
  {
    cmorg = le2be16(cmorg);
    xo = le2be16(xo);
    yo = le2be16(yo);
    w = le2be16(w);
    h = le2be16(h);
  }
#else
  void swap() {}
#endif
};
#pragma pack(pop)

#define DESC_VFLIP 0x20
#define DESC_HFLIP 0x10

static bool readhdr(TgaHeader &hdr, file_ptr_t h)
{
  if (df_read(h, &hdr, sizeof(hdr)) != sizeof(hdr))
    return false;
  hdr.swap();
  if (hdr.idlen)
    if (df_seek_rel(h, hdr.idlen) == -1)
      return false;
  if (hdr.cmlen)
    if (df_seek_rel(h, hdr.cmlen * ((hdr.cmes + 7) / 8)) == -1)
      return false;
  return true;
}

static bool readhdr(TgaHeader &hdr, IGenLoad &crd)
{
  DAGOR_TRY
  {
    if (crd.tryRead(&hdr, sizeof(hdr)) != sizeof(hdr))
      return false;
    hdr.swap();
    if (hdr.idlen)
      crd.seekrel(hdr.idlen);
    if (hdr.cmlen)
      crd.seekrel(hdr.cmlen * ((hdr.cmes + 7) / 8));
  }
  DAGOR_CATCH(const IGenLoad::LoadException &) { return false; }
  return true;
}

TexImage32 *load_tga32(const char *fn, IMemAlloc *mem, bool *out_used_alpha, unsigned char *app_data, unsigned int *app_data_len)
{
  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
    return NULL;
  return load_tga32(crd, mem, out_used_alpha, app_data, app_data_len);
}

TexImage32 *load_tga32(const char *fn, IMemAlloc *mem, bool *out_used_alpha)
{
  return load_tga32(fn, mem, out_used_alpha, NULL, NULL);
}

bool read_tga32_dimensions(const char *fn, int &out_w, int &out_h, bool &out_may_have_alpha)
{
  bool ret = false;
  if (file_ptr_t h = df_open(fn, DF_READ))
  {
    TgaHeader hdr;
    if (readhdr(hdr, h))
      ret = true, out_w = hdr.w, out_h = hdr.h, out_may_have_alpha = (hdr.bits == 32);
    df_close(h);
  }
  return ret;
}

TexImage32 *load_tga32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha, unsigned char *app_data, unsigned int *app_data_len)
{
  TgaHeader hdr;
  if (!readhdr(hdr, crd))
    return NULL;

  if (hdr.pack != 2 && hdr.pack != 10 && hdr.pack != 3 && hdr.pack != 11)
    return NULL;

  if ((hdr.pack & ~8) == 2)
  {
    if (hdr.bits != 16 && hdr.bits != 24 && hdr.bits != 32)
      return NULL;
  }
  else
  {
    if (hdr.bits != 8)
      return NULL;
  }
  int cb = hdr.bits >> 3;

  TexImage32 *im = (TexImage32 *)mem->tryAlloc(sizeof(TexImage32) + hdr.w * hdr.h * 4);
  if (!im)
    return NULL;

#define CRD_READ(to, sz)               \
  if (crd.tryRead((to), (sz)) != (sz)) \
  goto read_fail

  im->w = hdr.w;
  im->h = hdr.h;
  uint8_t *buf = (uint8_t *)tmpmem->tryAlloc(cb * hdr.w);
  if (buf)
  {
    TexPixel32 *ptr = (TexPixel32 *)(im + 1);
    if (!(hdr.desc & DESC_VFLIP))
      ptr += (hdr.h - 1) * hdr.w;
    for (; hdr.h; --hdr.h)
    {
      if (!(hdr.pack & 8))
      {
        CRD_READ(buf, cb * hdr.w);
        uint8_t *b = buf;
        for (int i = 0; i < hdr.w; ++i, b += cb)
          conv_clr(cb, b, ptr[i]);
      }
      else
      {
        TexPixel32 *p = ptr;
        for (int i = 0; i < hdr.w;)
        {
          uint8_t j = 0;
          CRD_READ(&j, 1);
          if (j & 0x80)
          {
            uint8_t clr[4];
            CRD_READ(clr, cb);
            TexPixel32 c;
            conv_clr(cb, clr, c);
            for (j = (j & 0x7F) + 1, i += j; j; ++p, --j)
              p->u = c.u;
          }
          else
          {
            ++j;
            CRD_READ(buf, cb * j);
            uint8_t *b = buf;
            for (i += j; j; ++p, b += cb, --j)
              conv_clr(cb, b, *p);
          }
        }
      }
      if (hdr.desc & DESC_HFLIP)
      {
        TexPixel32 *p1 = ptr, *p2 = ptr + (hdr.w - 1);
        for (; p1 < p2; ++p1, --p2)
        {
          uint32_t a = p1->u;
          p1->u = p2->u;
          p2->u = a;
        }
      }
      if (hdr.desc & DESC_VFLIP)
        ptr += hdr.w;
      else
        ptr -= hdr.w;
    }
    memfree(buf, tmpmem);
    // DEBUG_CTX("TGA32 loaded well");
    if (app_data_len)
      *app_data_len = read_app_data(crd, app_data, *app_data_len);
    if (out_used_alpha)
    {
      if (hdr.bits == 32)
      {
        *out_used_alpha = false;
        TexPixel32 *pix = (TexPixel32 *)(im + 1);
        for (int i = 0; i < im->w * im->h; ++i, ++pix)
          if (pix->a != 255)
          {
            *out_used_alpha = true;
            break;
          }
      }
      else
        *out_used_alpha = false;
    }
    return im;
  }
  else
  {
  read_fail:
    memfree(buf, tmpmem);
    memfree(im, mem);
  }
  return NULL;
}

TexImage32 *load_tga32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha)
{
  return load_tga32(crd, mem, out_used_alpha, NULL, NULL);
}

#undef CRD_READ

// app_data_len is inout param
bool load_tga32(TexImage32 *im, char *fn, bool *out_used_alpha, unsigned char *app_data, unsigned int *app_data_len)
{
  if (!im)
    return false;

  file_ptr_t h = df_open(fn, DF_READ);

  if (!h)
    return false;

  TgaHeader hdr;
  if (!readhdr(hdr, h))
  {
    df_close(h);
    return false;
  }

  if (hdr.pack != 2 && hdr.pack != 10 && hdr.pack != 3 && hdr.pack != 11)
  {
    df_close(h);
    return false;
  }

  if ((hdr.pack & ~8) == 2)
  {
    if (hdr.bits != 16 && hdr.bits != 24 && hdr.bits != 32)
    {
      df_close(h);
      return false;
    }
  }
  else
  {
    if (hdr.bits != 8)
    {
      df_close(h);
      return false;
    }
  }
  int cb = hdr.bits >> 3;

  if ((im->w != hdr.w) || (im->h != hdr.h))
  {
    df_close(h);
    return false;
  }
  uint8_t *buf = (uint8_t *)tmpmem->tryAlloc(cb * hdr.w);
  if (!buf)
  {
    df_close(h);
    return false;
  }

  TexPixel32 *ptr = (TexPixel32 *)(im + 1);
  if (!(hdr.desc & DESC_VFLIP))
    ptr += (hdr.h - 1) * hdr.w;
  for (; hdr.h; --hdr.h)
  {
    if (!(hdr.pack & 8))
    {
      if (df_read(h, buf, cb * hdr.w) != cb * hdr.w)
        goto err;
      uint8_t *b = buf;
      for (int i = 0; i < hdr.w; ++i, b += cb)
        conv_clr(cb, b, ptr[i]);
    }
    else
    {
      TexPixel32 *p = ptr;
      for (int i = 0; i < hdr.w;)
      {
        uint8_t j = 0;
        if (df_read(h, &j, 1) != 1)
          goto err;
        if (j & 0x80)
        {
          uint8_t clr[4];
          if (df_read(h, clr, cb) != cb)
            goto err;
          TexPixel32 c;
          conv_clr(cb, clr, c);
          for (j = (j & 0x7F) + 1, i += j; j; ++p, --j)
            p->u = c.u;
        }
        else
        {
          ++j;
          if (df_read(h, buf, cb * j) != cb * j)
            goto err;
          uint8_t *b = buf;
          for (i += j; j; ++p, b += cb, --j)
            conv_clr(cb, b, *p);
        }
      }
    }
    if (hdr.desc & DESC_HFLIP)
    {
      TexPixel32 *p1 = ptr, *p2 = ptr + (hdr.w - 1);
      for (; p1 < p2; ++p1, --p2)
      {
        uint32_t a = p1->u;
        p1->u = p2->u;
        p2->u = a;
      }
    }
    if (hdr.desc & DESC_VFLIP)
      ptr += hdr.w;
    else
      ptr -= hdr.w;
  }
  memfree(buf, tmpmem);

  if (app_data_len)
  {
    LFileGeneralLoadCB cb(h);
    *app_data_len = read_app_data(cb, app_data, *app_data_len);
  }

  df_close(h);
  if (out_used_alpha)
  {
    if (hdr.bits == 32)
    {
      *out_used_alpha = false;
      TexPixel32 *pix = (TexPixel32 *)(im + 1);
      for (int i = 0; i < im->w * im->h; ++i, ++pix)
        if (pix->a != 255)
        {
          *out_used_alpha = true;
          break;
        }
    }
    else
      *out_used_alpha = false;
  }
  return true;
err:
  memfree(buf, tmpmem);
  df_close(h);
  return false;
}

bool load_tga32(TexImage32 *im, char *fn, bool *out_used_alpha) { return load_tga32(im, fn, out_used_alpha, NULL, NULL); }

TexImage8a *load_tga8a(const char *fn, IMemAlloc *mem)
{
  file_ptr_t h;

  if (!(h = df_open(fn, DF_READ)))
    return NULL;
  TgaHeader hdr;
  if (!readhdr(hdr, h))
  {
    df_close(h);
    return NULL;
  }
  if (hdr.pack != 2 && hdr.pack != 10 && hdr.pack != 3 && hdr.pack != 11)
  {
    df_close(h);
    return NULL;
  }
  if ((hdr.pack & ~8) == 2)
  {
    if (hdr.bits != 16 && hdr.bits != 24 && hdr.bits != 32)
    {
      df_close(h);
      return NULL;
    }
  }
  else
  {
    if (hdr.bits != 8)
    {
      df_close(h);
      return NULL;
    }
  }
  int cb = hdr.bits >> 3;

  TexImage8a *im = (TexImage8a *)mem->tryAlloc(sizeof(TexImage8a) + hdr.w * hdr.h * 2);
  if (!im)
  {
    df_close(h);
    return NULL;
  }
  im->w = hdr.w;
  im->h = hdr.h;
  uint8_t *buf = (uint8_t *)tmpmem->tryAlloc(cb * hdr.w);
  if (!buf)
  {
    memfree(im, mem);
    df_close(h);
    return NULL;
  }

  TexPixel8a *ptr = (TexPixel8a *)(im + 1);
  if (!(hdr.desc & DESC_VFLIP))
    ptr += (hdr.h - 1) * hdr.w;
  for (; hdr.h; --hdr.h)
  {
    if (!(hdr.pack & 8))
    {
      if (df_read(h, buf, cb * hdr.w) != cb * hdr.w)
        goto err;
      uint8_t *b = buf;
      for (int i = 0; i < hdr.w; ++i, b += cb)
        conv_clr(cb, b, ptr[i]);
    }
    else
    {
      TexPixel8a *p = ptr;
      for (int i = 0; i < hdr.w;)
      {
        uint8_t j = 0;
        if (df_read(h, &j, 1) != 1)
          goto err;
        if (j & 0x80)
        {
          uint8_t clr[4];
          if (df_read(h, clr, cb) != cb)
            goto err;
          TexPixel8a c;
          conv_clr(cb, clr, c);
          for (j = (j & 0x7F) + 1, i += j; j; ++p, --j)
            p->u = c.u;
        }
        else
        {
          ++j;
          if (df_read(h, buf, cb * j) != cb * j)
            goto err;
          uint8_t *b = buf;
          for (i += j; j; ++p, b += cb, --j)
            conv_clr(cb, b, *p);
        }
      }
    }
    if (hdr.desc & DESC_HFLIP)
    {
      TexPixel8a *p1 = ptr, *p2 = ptr + (hdr.w - 1);
      for (; p1 < p2; ++p1, --p2)
      {
        uint16_t a = p1->u;
        p1->u = p2->u;
        p2->u = a;
      }
    }
    if (hdr.desc & DESC_VFLIP)
      ptr += hdr.w;
    else
      ptr -= hdr.w;
  }
  memfree(buf, tmpmem);
  df_close(h);
  return im;
err:
  memfree(buf, tmpmem);
  memfree(im, mem);
  df_close(h);
  return NULL;
}

TexImage8 *load_tga8(const char *fn, IMemAlloc *mem)
{
  file_ptr_t h;

  if (!(h = df_open(fn, DF_READ)))
    return NULL;
  TgaHeader hdr;
  if (!readhdr(hdr, h))
  {
    df_close(h);
    return NULL;
  }
  if (hdr.pack != 2 && hdr.pack != 10 && hdr.pack != 3 && hdr.pack != 11)
  {
    df_close(h);
    return NULL;
  }
  if ((hdr.pack & ~8) == 2)
  {
    if (hdr.bits != 16 && hdr.bits != 24 && hdr.bits != 32)
    {
      df_close(h);
      return NULL;
    }
  }
  else
  {
    if (hdr.bits != 8)
    {
      df_close(h);
      return NULL;
    }
  }
  int cb = hdr.bits >> 3;

  TexImage8 *im = (TexImage8 *)mem->tryAlloc(sizeof(TexImage8) + hdr.w * hdr.h * 1);
  if (!im)
  {
    df_close(h);
    return NULL;
  }
  im->w = hdr.w;
  im->h = hdr.h;
  uint8_t *buf = (uint8_t *)tmpmem->tryAlloc(cb * hdr.w);
  if (!buf)
  {
    memfree(im, mem);
    df_close(h);
    return NULL;
  }

  uint8_t *ptr = (uint8_t *)(im + 1);
  if (!(hdr.desc & DESC_VFLIP))
    ptr += (hdr.h - 1) * hdr.w;
  for (; hdr.h; --hdr.h)
  {
    if (!(hdr.pack & 8))
    {
      if (df_read(h, buf, cb * hdr.w) != cb * hdr.w)
        goto err;
      uint8_t *b = buf;
      for (int i = 0; i < hdr.w; ++i, b += cb)
        ptr[i] = conv_clr(cb, b);
    }
    else
    {
      uint8_t *p = ptr;
      for (int i = 0; i < hdr.w;)
      {
        uint8_t j = 0;
        if (df_read(h, &j, 1) != 1)
          goto err;
        if (j & 0x80)
        {
          uint8_t clr[4];
          if (df_read(h, clr, cb) != cb)
            goto err;
          uint8_t c = conv_clr(cb, clr);
          for (j = (j & 0x7F) + 1, i += j; j; ++p, --j)
            *p = c;
        }
        else
        {
          ++j;
          if (df_read(h, buf, cb * j) != cb * j)
            goto err;
          uint8_t *b = buf;
          for (i += j; j; ++p, b += cb, --j)
            *p = conv_clr(cb, b);
        }
      }
    }
    if (hdr.desc & DESC_HFLIP)
    {
      uint8_t *p1 = ptr, *p2 = ptr + (hdr.w - 1);
      for (; p1 < p2; ++p1, --p2)
      {
        uint8_t a = *p1;
        *p1 = *p2;
        *p2 = a;
      }
    }
    if (hdr.desc & DESC_VFLIP)
      ptr += hdr.w;
    else
      ptr -= hdr.w;
  }
  memfree(buf, tmpmem);
  df_close(h);
  return im;
err:
  memfree(buf, tmpmem);
  memfree(im, mem);
  df_close(h);
  return NULL;
}
