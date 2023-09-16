#include <supp/dag_math.h>
#include <string.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_span.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <image/dag_texPixel.h>
#include <image/dag_tga.h>
#include <util/dag_stdint.h>

namespace loadmask
{

bool loadMaskFromTga(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h)
{
  TexImage8 *img = ::load_tga8(filename, tmpmem);

  if (!img)
    return false;

  w = img->w;
  h = img->h;
  clear_and_resize(hmap, w * h);
  uint8_t *ptr = (uint8_t *)(img + 1);
  for (int i = 0; i < hmap.size(); ++i, ++ptr)
    hmap[i] = *ptr / float((1 << 8) - 1);

  memfree(img, tmpmem);
  return true;
}

bool loadMaskFromRaw32(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h)
{
  file_ptr_t handle = df_open(filename, DF_READ);

  if (!handle)
    return false;

  int length = df_length(handle);

  w = int(floor(sqrt((double)(length / 4)) + 0.0001));
  h = w;

  clear_and_resize(hmap, w * h);

  for (int y = h - 1, cnt = 0; y >= 0; --y)
  {
    int index = y * w;
    if (df_read(handle, &hmap[index], w * 4) != w * 4)
      return false;
  }
  return true;
}


bool loadMaskFromRaw16(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h)
{
  file_ptr_t handle = df_open(filename, DF_READ);

  if (!handle)
    return false;

  int length = df_length(handle);

  w = int(floor(sqrt((double)(length / 2)) + 0.0001));
  h = w;

  clear_and_resize(hmap, w * h);

  SmallTab<uint16_t, TmpmemAlloc> buf;
  clear_and_resize(buf, w);
  for (int y = h - 1, cnt = 0; y >= 0; --y)
  {
    if (df_read(handle, buf.data(), data_size(buf)) != data_size(buf))
      return false;
    int index = y * w;
    for (int x = 0; x < w; ++x, index++)
      hmap[index] = buf[x] / float((1 << 16) - 1);
  }
  return true;
}

bool loadMaskFromFile(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h)
{
  const char *ext = dd_get_fname_ext(filename);
  if (!ext)
    return false;
  if (stricmp(ext, ".r32") == 0)
    return loadMaskFromRaw32(filename, hmap, w, h);
  else if (stricmp(ext, ".r16") == 0)
    return loadMaskFromRaw16(filename, hmap, w, h);
  else
    return loadMaskFromTga(filename, hmap, w, h);
}

} // namespace loadmask