// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_png.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_fileIo.h>
#include <image/libpng-1.4.22/png.h>
#include <EASTL/unique_ptr.h>

static void PNGAPI user_write_stream(png_structp png_ptr, png_bytep data, png_size_t length)
{
  IGenSave *cwr = (IGenSave *)png_get_io_ptr(png_ptr);
  if (cwr->tryWrite(data, (int)length) != (int)length)
  {
    debug("savePng: stream write error!");
    png_error(png_ptr, "Stream write error");
  }
}

static void PNGAPI user_log_fn(png_structp png_ptr, const char *message) { debug("savePng: %s", message); }

bool save_png32(const char *fn, const TexPixel32 *ptr, int wd, int ht, int stride, unsigned char *text_data,
  unsigned int text_data_len, bool save_alpha)
{
  FullFileSaveCB cwr(fn);
  if (!cwr.fileHandle)
    return false;
  return save_png32(ptr, wd, ht, stride, cwr, save_alpha, text_data, text_data_len);
}

bool save_png32(const TexPixel32 *ptr, int wd, int ht, int stride, IGenSave &cwr, bool save_alpha, unsigned char *text_data,
  unsigned int text_data_len)
{
  const uint32_t buf_size = (save_alpha ? 4 : 3) * wd;
  auto buf = eastl::make_unique<png_byte[]>(buf_size);

  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr)
    return false;

  png_infop png_info = png_create_info_struct(png_ptr);
  if (!png_info)
  {
    png_destroy_write_struct(&png_ptr, nullptr);
    return false;
  }

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_write_struct(&png_ptr, &png_info);
    return false;
  }

  const int format = save_alpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB;

  png_set_error_fn(png_ptr, nullptr, user_log_fn, nullptr);
  png_set_write_fn(png_ptr, &cwr, user_write_stream, nullptr);
  png_set_IHDR(png_ptr, png_info, (uint32_t)wd, (uint32_t)ht, 8, format, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT);

  png_text meta_info{};
  const char *meta_key = "Comment";

  if (text_data_len > 0)
  {
    meta_info.compression = PNG_TEXT_COMPRESSION_NONE;
    meta_info.key = (png_charp)meta_key;
    meta_info.text = (png_charp)text_data;
    meta_info.text_length = text_data_len;
    png_set_text(png_ptr, png_info, &meta_info, 1);
  }

  png_write_info(png_ptr, png_info);
  const png_byte *pix_ptr = (const png_byte *)ptr;

  for (int y = 0; y < ht; y++)
  {
    int pix = 0;
    auto row_ptr = (const TexPixel32 *)pix_ptr;
    for (int x = 0; x < wd; x++)
    {
      buf[pix++] = row_ptr[x].r;
      buf[pix++] = row_ptr[x].g;
      buf[pix++] = row_ptr[x].b;
      if (save_alpha)
        buf[pix++] = row_ptr[x].a;
    }
    png_write_row(png_ptr, buf.get());
    pix_ptr += stride;
  }

  png_write_end(png_ptr, png_info);
  png_destroy_write_struct(&png_ptr, &png_info);
  return true;
}