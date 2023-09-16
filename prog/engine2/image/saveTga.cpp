#include <image/dag_tga.h>
#include <image/dag_texPixel.h>
#include <osApiWrappers/dag_files.h>
#include <debug/dag_debug.h>

#pragma pack(push, 1)
// struct TgaHeader{short id,pack;int res2,res3;short w,h;char bits,alpha;};
struct TgaHeader
{
  uint8_t idlen, cmtype, pack;
  uint16_t cmorg, cmlen;
  uint8_t cmes;
  short xo, yo, w, h;
  char bits, desc;
  TgaHeader() : idlen(0), cmtype(0), pack(0), cmorg(0), cmlen(0), cmes(0), xo(0), yo(0), w(0), h(0), bits(0), desc(0) {}
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


static void save_tga_app_data(file_ptr_t h, unsigned char *app_data, unsigned int app_data_len)
{
  if (!app_data || !app_data_len)
    return;
  // write footer: targa2
  int current = df_tell(h);
  if (current < 0)
    return;
  df_write(h, "\1\0", 2); // number of tags, 1
  df_write(h, "\0\0", 2); // tag
  const uint32_t tagOffset = current + 2 /*tags count*/ + 2 /*tag*/ + 4 /*data offset*/ + 4 /*data size*/;
  df_write(h, &tagOffset, 4);    // tag data offset
  df_write(h, &app_data_len, 4); // tag data len
  df_write(h, app_data, app_data_len);
  df_write(h, &current, 4);
  df_write(h, "\0\0\0\0", 4);
  df_write(h, "TRUEVISION-XFILE", 16);
  df_write(h, ".\0", 2);
  // end of  footer
}


int save_tga8a(const char *fn, TexPixel8a *ptr, int wd, int ht, int stride, unsigned char *app_data, unsigned int app_data_len)
{
  if (!ptr)
    return 0;
  file_ptr_t h = df_open(fn, DF_WRITE | DF_CREATE);
  if (!h)
    return 0;
  df_write(h, "\0\0\2\0\0\0\0\0\0\0\0\0", 12);
  df_write(h, &wd, 2);
  df_write(h, &ht, 2);
  df_write(h, "\x20\x08", 2);
  char *p = (char *)ptr + stride * (ht - 1);
  for (int i = 0; i < ht; ++i, p -= stride)
  {
    for (int j = 0; j < wd; ++j)
    {
      TexPixel32 pix;
      pix.b = pix.g = pix.r = ((TexPixel8a *)p)[j].l;
      pix.a = ((TexPixel8a *)p)[j].a;
      df_write(h, &pix, 4);
    }
  }
  save_tga_app_data(h, app_data, app_data_len);
  df_close(h);
  return 1;
}

int save_tga8(const char *fn, unsigned char *ptr, int wd, int ht, int stride, unsigned char *app_data, unsigned int app_data_len)
{
  if (!ptr)
    return 0;
  file_ptr_t h = df_open(fn, DF_WRITE | DF_CREATE);
  if (!h)
    return 0;
  TgaHeader hdr;
  hdr.w = wd;
  hdr.h = ht;
  hdr.pack = 3;
  hdr.desc = hdr.bits = 8;
  df_write(h, &hdr, sizeof(hdr));
  unsigned char *p = (unsigned char *)ptr + stride * (ht - 1);
  for (int i = 0; i < ht; ++i, p -= stride)
    df_write(h, p, wd);
  save_tga_app_data(h, app_data, app_data_len);
  df_close(h);
  return 1;
}

int save_tga32(const char *fn, TexPixel32 *ptr, int wd, int ht, int stride, unsigned char *app_data, unsigned int app_data_len)
{
  if (!ptr)
  {
    debug_ctx("save_tga32, ptr=NULL");
    return 0;
  }
  file_ptr_t h = df_open(fn, DF_WRITE | DF_CREATE);
  if (!h)
  {
    debug_ctx("save_tga32, cannot open file '%s'", fn);
    return 0;
  }
  df_write(h, "\0\0\2\0\0\0\0\0\0\0\0\0", 12);
  df_write(h, &wd, 2);
  df_write(h, &ht, 2);
  df_write(h, "\x20\x08", 2);
  char *p = (char *)ptr + stride * (ht - 1);
  for (int i = 0; i < ht; ++i, p -= stride)
    df_write(h, p, wd * 4);
  save_tga_app_data(h, app_data, app_data_len);
  df_close(h);
  return 1;
}

int save_tga32(const char *fn, TexImage32 *im, unsigned char *app_data, unsigned int app_data_len)
{
  if (!im)
    return 0;
  return save_tga32(fn, (TexPixel32 *)(im + 1), im->w, im->h, im->w * 4, app_data, app_data_len);
}

int save_tga24(const char *fn, char *ptr, int wd, int ht, int stride, unsigned char *app_data, unsigned int app_data_len)
{
  if (!ptr)
    return 0;
  file_ptr_t h = df_open(fn, DF_WRITE | DF_CREATE);
  if (!h)
    return 0;
  df_write(h, "\0\0\2\0\0\0\0\0\0\0\0\0", 12);
  df_write(h, &wd, 2);
  df_write(h, &ht, 2);
  df_write(h, "\x18\x00", 2);
  char *p = (char *)ptr + stride * (ht - 1);
  for (int i = 0; i < ht; ++i, p -= stride)
    df_write(h, p, wd * 3);
  save_tga_app_data(h, app_data, app_data_len);
  df_close(h);
  return 1;
}

int save_tga24(const char *fn, TexImage *im, unsigned char *app_data, unsigned int app_data_len)
{
  if (!im)
    return 0;
  return save_tga24(fn, (char *)(im + 1), im->w, im->h, im->w * 3, app_data, app_data_len);
}
