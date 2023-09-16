#pragma once

#include <util/dag_globDef.h>
#include <util/dag_stdint.h>
#include <memory/dag_mem.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>

struct TexPixel32;

class BinFormat
{
public:
  DAG_DECLARE_NEW(midmem)

  struct Glyph
  {
    real u0, v0, u1, v1;
    short x0, y0, x1, y1, dx;
    int8_t tex;
    int8_t dynGrp;
    uint16_t pkOfs;
    int16_t noPk;
  };
  struct UnicodeTable
  {
    int first, last;
    uint16_t idx[256];
  };

  struct FontData
  {
    String name;
    int fontht, linegap, ascent, descent, btbd;
    Tab<Glyph> glyph;
    Tab<uint32_t> glyphPk;
    UnicodeTable *ut[256];
    int maxUtg;
    uint16_t at[256];

    FontData() : name(), fontht(0), linegap(0), ascent(0), descent(0), btbd(0), glyph(midmem_ptr()), maxUtg(0)
    {
      memset(ut, 0, sizeof(ut));
      memset(at, 0, sizeof(at));
    }
  };

  Tab<TexPixel32 *> ims;
  Tab<int> texws;
  Tab<int> texhs;

  Tab<FontData> fonts;

public:
  BinFormat();
  ~BinFormat();

  void clear();
  void prepareUnicodeTable(int font_index, uint16_t *cmap, int count);
  void shrinkUnicodeTable(int font_index);
  bool saveBinary(const char *fn, int targetCode, bool rgba32, bool valve_type, bool half_pixel_expand, const FastNameMap &dynTTFUsed);
  int getGlyphsCount();
};
