// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <generic/dag_tab.h>
#include <util/dag_string.h>

class FreeTypeFont
{
  FT_Face face;
  bool inited;
  int upscaleFactor;

public:
  FreeTypeFont() : inited(false), upscaleFactor(1), face(nullptr) {}
  ~FreeTypeFont() { close_ttf(); }

  bool import_ttf(const char *fn, bool symb_charmap);
  void close_ttf();
  bool resize_font(int ht, int upscale_factor = 1);

  void metrics(int *ascender, int *descender, int *height, int *linegap);
  int get_char_index(int ch);

  bool render_glyph(unsigned char *buffer, int *wd, int *ht, int *x0, int *y0, short *dx, int index, bool mono,
    bool force_auto_hinting, bool check_extents, bool do_render);

  bool has_pair_kerning(wchar_t c)
  {
    if (!FT_HAS_KERNING(face))
      return false;
    FT_UInt g0idx = FT_Get_Char_Index(face, c);
    FT_Vector pk;
    if (FT_Get_Kerning(face, g0idx, 0xFFFFFFFFU, FT_KERNING_UNSCALED, &pk) != 0)
      return false;
    return pk.x == 0;
  }
  int get_pair_kerning(int wc1, int wc2);

  String srcDesc;
};
DAG_DECLARE_RELOCATABLE(FreeTypeFont);
