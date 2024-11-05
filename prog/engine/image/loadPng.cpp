// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_png.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_fileIo.h>
#include <image/libpng-1.4.22/png.h>
#include <debug/dag_log.h>

static void __cdecl crd_read(png_struct *png_ptr, png_byte *buf, png_size_t sz)
{
  if (png_ptr)
    if (((IGenLoad *)png_get_io_ptr(png_ptr))->tryRead(buf, (int)sz) != sz)
      png_error(png_ptr, "Read Error");
}

TexImage32 *load_png32(const char *fn, IMemAlloc *mem, bool *out_used_alpha, eastl::string *comments)
{
  FullFileLoadCB crd(fn);
  if (crd.fileHandle)
    return load_png32(crd, mem, out_used_alpha, comments);
  return NULL;
}
TexImage32 *load_png32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha, eastl::string *comments)
{
  png_structp png_ptr;
  png_infop info_ptr;

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (png_ptr == NULL)
    return NULL;

  /* Allocate/initialize the memory for image information.  REQUIRED. */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL)
  {
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return NULL;
  }

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    /* Free all of the memory associated with the png_ptr and info_ptr */
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    /* If we get here, we had a problem reading the file */
    return NULL;
  }

  /* One of the following I/O initialization methods is REQUIRED */
  /* Set up the input control if you are using standard C streams */
  // png_init_io(png_ptr, fp);
  png_set_write_fn(png_ptr, NULL, NULL, NULL);
  png_set_read_fn(png_ptr, &crd, &crd_read);

  /* If we have already read some of the signature */
  // png_set_sig_bytes(png_ptr, sig_read);

  /*
   * If you have enough memory to read in the entire image at once,
   * and you need to specify only transforms that can be controlled
   * with one of the PNG_TRANSFORM_* bits (this presently excludes
   * quantizing, filling, setting background, and doing gamma
   * adjustment), then you can read the entire image (including
   * pixels) into the info structure with this call:
   */
  png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_GRAY_TO_RGB | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR, NULL);

  int img_type = -1;
  if (png_get_bit_depth(png_ptr, info_ptr) == 8 && png_get_channels(png_ptr, info_ptr) == 4)
    img_type = 4;
  else if (png_get_bit_depth(png_ptr, info_ptr) == 8 && png_get_channels(png_ptr, info_ptr) == 3)
    img_type = 3;
  if (img_type < 0)
  {
    logerr("unsupported bit_depth=%d channels=%d in %s", png_get_bit_depth(png_ptr, info_ptr), png_get_channels(png_ptr, info_ptr),
      crd.getTargetName());
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return NULL;
  }

  /* At this point you have read the entire image */
  png_bytep *row_pointers = png_get_rows(png_ptr, info_ptr);

  // copy image
  TexImage32 *im = TexImage32::create(png_get_image_width(png_ptr, info_ptr), png_get_image_height(png_ptr, info_ptr), mem);
  switch (img_type)
  {
    case 4:
      for (int i = 0; i < im->h; i++)
        memcpy(im->getPixels() + im->w * i, row_pointers[i], im->w * sizeof(TexPixel32));
      break;

    case 3:
      for (int i = 0; i < im->h; i++)
      {
        TexPixel32 *p = im->getPixels() + im->w * i, *pe = p + im->w;
        png_byte *s = row_pointers[i];
        for (; p < pe; p++, s += 3)
          p->r = s[2], p->g = s[1], p->b = s[0], p->a = 255;
      }
      break;
  }

  if (out_used_alpha)
    *out_used_alpha = img_type == 4 ? true : false;

  if (comments)
  {
    png_textp text_ptr;
    int num_text = 0;
    if (png_get_text(png_ptr, info_ptr, &text_ptr, &num_text))
      for (int i = 0; i < num_text; ++i)
        comments->append(text_ptr[i].text);
  }

  /* Clean up after the read, and free any memory allocated - REQUIRED */
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  /* That's it */
  return im;
}
