// Copyright (C) Gaijin Games KFT.  All rights reserved.

// truetypefont.cpp : Defines the entry point for the DLL application.
//

#include "freeTypeFont.h"
#include "alphaGen.cpp"


bool FreeTypeFont::import_ttf(const char *fn, bool symb_charmap)
{
  close_ttf();

  int error = 0;

  FT_Library library;

  error = FT_Init_FreeType(&library);
  if (error)
    return false;
  String real_fn;
  if (strncmp(fn, "<system>", 8) == 0)
  {
#if _TARGET_PC_WIN
    real_fn.printf(0, "%s/Fonts/%s", getenv("SystemRoot"), fn + 8);
    fn = real_fn;
#endif
  }
  error = FT_New_Face(library, fn, 0, &face);
  if (error)
    return false;

  if (symb_charmap)
  {
    error = FT_Select_Charmap(face, FT_ENCODING_MS_SYMBOL);
    if (error)
      return false;
  }

  error = FT_Set_Char_Size(face, /* handle to face object           */
    0,                           /* char_width in 1/64th of points  */
    32 * 64,                     /* char_height in 1/64th of points */
    300,                         /* horizontal device resolution    */
    300);                        /* vertical device resolution      */
  inited = true;
  upscaleFactor = 1;
  return error == 0;
}

void FreeTypeFont::close_ttf()
{
  if (!inited)
    return;
  FT_Done_Face(face);
  inited = false;
  upscaleFactor = 1;
}

bool FreeTypeFont::resize_font(int ht, int upscale_factor)
{
  int error = 0;
  upscaleFactor = upscale_factor;
  error = FT_Set_Pixel_Sizes(face, /* handle to face object */
    0,                             /* pixel_width           */
    ht * upscaleFactor);           /* pixel_height          */

  return error == 0;
}


bool FreeTypeFont::render_glyph(unsigned char *buffer, int *wd, int *ht, int *x0, int *y0, short *dx, int index, bool monochrome,
  bool force_auto_hinting, bool check_extents, bool do_render)
{
  *wd = *ht = 0;
  if (!inited)
    return 0;

  int error = 0;
  FT_UInt glyph_index = index;

  FT_UInt hintingType = force_auto_hinting ? FT_LOAD_FORCE_AUTOHINT : 0;
  /* load glyph image into the slot (erase previous one) */
  // error = FT_Load_Glyph( face, glyph_index, FT_LOAD_RENDER/*FT_LOAD_DEFAULT*/ );
  error = FT_Load_Glyph(face, glyph_index, hintingType | (monochrome ? FT_LOAD_TARGET_MONO : FT_LOAD_TARGET_NORMAL)); //-V616

  if (error)
    return false;

  /* convert to an anti-aliased bitmap */
  FT_GlyphSlot slot = face->glyph; /* a small shortcut */
  if (do_render)
  {
    error = FT_Render_Glyph(slot, monochrome ? FT_RENDER_MODE_MONO : FT_RENDER_MODE_NORMAL);
    G_ASSERTF(slot->bitmap.width == slot->metrics.width >> 6, "%d and %d", slot->bitmap.width, slot->metrics.width >> 6);
    G_ASSERTF(slot->bitmap.rows == slot->metrics.height >> 6, "%d and %d", slot->bitmap.rows, slot->metrics.height >> 6);
    G_ASSERTF(slot->bitmap_left == slot->metrics.horiBearingX >> 6, "%d and %d", slot->bitmap_left, slot->metrics.horiBearingX >> 6);
    G_ASSERTF(slot->bitmap_top == slot->metrics.horiBearingY >> 6, "%d and %d", slot->bitmap_top, slot->metrics.horiBearingY >> 6);
  }
  else
  {
    slot->bitmap.width = slot->metrics.width >> 6;
    slot->bitmap.rows = slot->metrics.height >> 6;
    slot->bitmap_left = slot->metrics.horiBearingX >> 6;
    slot->bitmap_top = slot->metrics.horiBearingY >> 6;
  }

  if (error)
    return false;

  int add = upscaleFactor / 2;
  int g_x0 = slot->bitmap_left, g_dx = slot->advance.x >> 6;

  if (check_extents)
  {
    if (g_x0 < 0)
    {
      g_dx += -g_x0;
      g_x0 = 0;
    }

    if (g_dx < g_x0 + slot->bitmap.width)
      g_dx = g_x0 + slot->bitmap.width;
  }

  *wd = (slot->bitmap.width + add) / upscaleFactor;
  *ht = (slot->bitmap.rows + add) / upscaleFactor;
  *x0 = (g_x0 + add) / upscaleFactor;
  *y0 = (slot->bitmap_top + add) / upscaleFactor;

  if (*wd > 256 || *ht > 256)
    return false;

  if (g_dx < 0 || g_dx > 511 * upscaleFactor)
  {
    printf("glyph[%d] dx=%d (%X) lt=%d,%d  sz=%dx%d\n", index, int(slot->advance.x >> 6), int(slot->advance.x), slot->bitmap_left,
      slot->bitmap_top, slot->bitmap.width, slot->bitmap.rows);
    g_dx = clamp(g_dx, 0, 511 * upscaleFactor);
  }

  *dx = (g_dx + add) / upscaleFactor;

  if (!do_render)
    return true;

  if (upscaleFactor > 1)
  {
    generate_texture_alpha(slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows, slot->bitmap.pitch, buffer, *wd, *ht, *wd,
      upscaleFactor, 2, false, false);
    return true;
  }

  unsigned char *dest = buffer, *src = slot->bitmap.buffer;
  for (int i = slot->bitmap.rows; i > 0; i--, src += slot->bitmap.pitch, dest += slot->bitmap.width)
    memcpy(dest, src, slot->bitmap.width);

  if (monochrome)
  {
    dest = buffer;
    for (int i = slot->bitmap.rows * slot->bitmap.width; i > 0; i--, dest++)
      if (*dest < 128)
        *dest = 0;
      else
        *dest = 255;
  }

  return true;
}

int FreeTypeFont::get_char_index(int ch) { return inited ? FT_Get_Char_Index(face, ch) : 0; }

void FreeTypeFont::metrics(int *ascender, int *descender, int *height, int *linegap)
{
  FT_Size_Metrics *metrics = &face->size->metrics;
  int add = upscaleFactor / 2;
  *ascender = (((metrics->ascender + (1 << (6 - 1))) >> 6) + add) / upscaleFactor;
  *descender = (((-metrics->descender + (1 << (6 - 1))) >> 6) + add) / upscaleFactor;
  *height = (metrics->y_ppem + add) / upscaleFactor;
  *linegap = (((metrics->height - metrics->ascender + metrics->descender + (1 << (6 - 1))) >> 6) + add) / upscaleFactor;
}

int FreeTypeFont::get_pair_kerning(int wc1, int wc2)
{
  FT_UInt g0idx = FT_Get_Char_Index(face, wc1);
  FT_UInt g1idx = FT_Get_Char_Index(face, wc2);
  FT_Vector pk;
  if (FT_Get_Kerning(face, g0idx, g1idx, FT_KERNING_DEFAULT, &pk) != 0)
    return 0;
  return pk.x >> 6;
}
