#include "fontFileFormat.h"
#include <3d/ddsxTex.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_lzmaIo.h>
#include <image/dag_texPixel.h>
#include <libTools/util/makeBindump.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>
#include <generic/dag_smallTab.h>
#include <startup/dag_globalSettings.h>
#include <stdio.h>
#include <stdlib.h>
#include "mutexAA.h"
#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX
#include <unistd.h>
#endif

static void loadDdsxPluginsOnce();


BinFormat::BinFormat() : fonts(midmem_ptr()), ims(midmem_ptr()), texws(midmem_ptr()), texhs(midmem_ptr()) {}


BinFormat::~BinFormat() { clear(); }


void BinFormat::clear()
{
  for (int i = 0; i < ims.size(); i++)
    memfree(ims[i], tmpmem);
  clear_and_shrink(ims);
  clear_and_shrink(texws);
  clear_and_shrink(texhs);
  for (int i = 0; i < fonts.size(); i++)
  {
    clear_and_shrink(fonts[i].glyph);
    fonts[i].maxUtg = 0;
    memset(fonts[i].ut, 0, sizeof(fonts[i].ut));
    memset(fonts[i].at, 0, sizeof(fonts[i].at));
    for (int j = 0; j < 256; j++)
      if (fonts[i].ut[j])
        delete fonts[i].ut[j];
  }
  clear_and_shrink(fonts);
}

void BinFormat::prepareUnicodeTable(int font_index, uint16_t *cmap, int count)
{
  for (int i = 0; i < count; i++)
  {
    if (!cmap[i])
      continue;

    int gidx = cmap[i] >> 8;

    if (!fonts[font_index].ut[gidx])
    {
      fonts[font_index].ut[gidx] = new UnicodeTable;
      memset(fonts[font_index].ut[gidx]->idx, 0xFF, sizeof(fonts[font_index].ut[gidx]->idx));
      fonts[font_index].ut[gidx]->first = 0;
      fonts[font_index].ut[gidx]->last = 0;
      if (gidx > fonts[font_index].maxUtg)
      {
        fonts[font_index].maxUtg = gidx;
      }
    }
  }
}
void BinFormat::shrinkUnicodeTable(int font_index)
{
  for (int gi = 0; gi < 256; gi++)
    if (fonts[font_index].ut[gi])
    {
      int si = 0;
      uint16_t *idx = fonts[font_index].ut[gi]->idx;

      while (si < 256 && idx[si] == 0xFFFF)
        si++;

      if (si == 256)
      {
        if (!::dgs_execute_quiet)
          printf("remove expected group %02Xxx\n", gi);
        delete fonts[font_index].ut[gi];
        fonts[font_index].ut[gi] = NULL;
        continue;
      }
      fonts[font_index].ut[gi]->first = si;

      si = 255;
      while (si > 0 && idx[si] == 0xFFFF)
        si--;
      fonts[font_index].ut[gi]->last = si;

      idx = fonts[font_index].ut[gi]->idx;
      for (si = 0; si < 256; si++, idx++)
        if (*idx == 0xFFFF)
          *idx = 0;
    }
}

static Tab<uint8_t> tmp_data(tmpmem);

bool BinFormat::saveBinary(const char *fn, int targetCode, bool rgba32, bool valve_type, bool half_pixel_expand,
  const FastNameMap &dynTTFUsed)
{
  using namespace mkbindump;
  if (dynTTFUsed.nameCount() > 125)
  {
    printf("ERR: dynTTFUsed.nameCount()=%d for %s, no more than 125 is allowed\n", dynTTFUsed.nameCount(), fn);
    unlink(fn);
    return false;
  }

  FullFileSaveCB fcwr(fn);
  if (!fcwr.fileHandle)
    return false;

  if (!::dgs_execute_quiet)
    printf("Saving font atlas: %s (%d fonts)\n", fn, fonts.size());

  bool bigendian = dagor_target_code_be(targetCode);
  BinDumpSaveCB cwr(8 << 20, targetCode, bigendian);

  PatchTabRef gref, utref, texref;
  PatchTabRef gpkidx_ref, gpkdata_ref;
  cwr.writeFourCC(_MAKE4C('DFz\7'));
  cwr.writeInt32e(fonts.size());
  cwr.writeInt32e(dynTTFUsed.nameCount());

  for (int font_index = 0; font_index < fonts.size(); font_index++)
  {
    BinDumpSaveCB cwr1(2 << 20, targetCode, bigendian);

#define cwr cwr1
    cwr.beginBlock();
    cwr.setOrigin();
    G_ASSERT(fonts[font_index].fontht >= 0 && fonts[font_index].fontht < 256);
    G_ASSERT(fonts[font_index].ascent >= 0 && fonts[font_index].ascent < 256);
    G_ASSERT(fonts[font_index].descent >= 0 && fonts[font_index].descent < 256);
    G_ASSERT(fonts[font_index].btbd >= 0 && fonts[font_index].btbd < 256);
    int ybaseline = 255, xbaseline = 255;
    for (int i = 0; i < fonts[font_index].glyph.size(); i++)
      if (fonts[font_index].glyph[i].tex >= 0 || fonts[font_index].glyph[i].dynGrp >= 0)
      {
        if (ybaseline > fonts[font_index].glyph[i].y0)
          ybaseline = fonts[font_index].glyph[i].y0;
        if (xbaseline > fonts[font_index].glyph[i].x0)
          xbaseline = fonts[font_index].glyph[i].x0;
      }
    if (half_pixel_expand)
      xbaseline--;
    bool has_pk = fonts[font_index].glyphPk.size() > 0;

    gref.reserveTab(cwr);                                              // PatchableTab<const GlyphData> glyph;
    utref.reserveTab(cwr);                                             // PatchableTab<const UnicodeTable> uTable;
    texref.reserveTab(cwr);                                            // PatchableTab<Tex> tex;
    cwr.writeInt32e(fonts[font_index].fontht);                         // int32_t fontHt
    cwr.writeInt32e(fonts[font_index].ascent);                         // int32_t ascent
    cwr.writeInt32e(fonts[font_index].descent);                        // int32_t descent
    cwr.writeInt32e(fonts[font_index].btbd);                           // int32_t lineSpacing
    cwr.writeInt32e(xbaseline);                                        // int32_t xBase
    cwr.writeInt32e(ybaseline);                                        // int32_t yBase
    cwr.write16ex(fonts[font_index].at, sizeof(fonts[font_index].at)); // uint16_t asciiGlyphIdx[256];
    int name_pos = cwr.tell();
    cwr.writePtr64e(0);                     // PatchablePtr<const char> name;
    cwr.writePtr64e(0);                     // PATCHABLE_DATA64(void*, selfData);
    cwr.writeInt32e(valve_type ? 0 : 0xFF); // uint32_t classicFont;
    cwr.writeInt32e(has_pk ? 1 : 0);        // uint32_t _resv;
    if (!::dgs_execute_quiet)
      printf("  font[%d] fontHt=%-3d ascent=%-3d descent=%-3d lineSpacing=%-3d <%s>\n", font_index, fonts[font_index].fontht,
        fonts[font_index].ascent, fonts[font_index].descent, fonts[font_index].btbd, fonts[font_index].name.str());

    if (has_pk)
    {
      gpkidx_ref.reserveTab(cwr);  // PatchableTab<const uint16_t> glyphPairKernIdx;
      gpkdata_ref.reserveTab(cwr); // PatchableTab<const uint32_t> pairKernData;
    }
    cwr.writePtr32eAt(cwr.tell(), name_pos);
    cwr.writeRaw((char *)fonts[font_index].name, fonts[font_index].name.length() + 1);
    cwr.align16();

    gref.startData(cwr, fonts[font_index].glyph.size());
    for (int i = 0; i < fonts[font_index].glyph.size(); i++)
    {
      Glyph &g = fonts[font_index].glyph[i];
      if (g.tex < 0 && g.dynGrp < 0)
      {
        cwr.writeZeroes(4 * sizeof(float) + 4 * sizeof(uint8_t));
        cwr.writeInt16e(g.dx);
        cwr.writeInt16e(-1);
        continue;
      }
      if (g.dynGrp >= 0 && g.tex < 0)
        g.u0 = g.v0 = g.u1 = g.v1 = 0;

      g.x0 -= xbaseline;
      g.x1 -= xbaseline;
      g.y0 -= ybaseline;
      g.y1 -= ybaseline;
      if (half_pixel_expand)
      {
        g.x1++;
        g.y1++;
      }

      G_ASSERT(g.x0 >= 0 && g.x0 < 256);
      G_ASSERT(g.y0 >= 0 && g.y0 < 256);
      G_ASSERT(g.x1 >= 0 && g.x1 < 256);
      G_ASSERT(g.y1 >= 0 && g.y1 < 256);
      G_ASSERTF(g.dx >= 0, "g[%d][%d].dx=%d (%d/%d)", font_index, i, g.dx, g.tex, g.dynGrp);

      if (half_pixel_expand)
      {
        g.u0 -= 0.5 / texws[g.tex];
        g.v0 -= 0.5 / texhs[g.tex];
        g.u1 += 0.5 / texws[g.tex];
        g.v1 += 0.5 / texhs[g.tex];
      }
      if (g.x0 == g.x1 && g.y0 == g.y1) // non-printable glyph
        g.dynGrp = -1;

      cwr.writeReal(g.u0); // float u0, v0, u1, v1;
      cwr.writeReal(g.v0);
      cwr.writeReal(g.u1);
      cwr.writeReal(g.v1);
      cwr.writeInt8e(g.x0); // uint8_t x0, y0, x1, y1;
      cwr.writeInt8e(g.y0);
      cwr.writeInt8e(g.x1);
      cwr.writeInt8e(g.y1);
      cwr.writeInt16e(g.dx); // uint16_t dx, texIdx;
      cwr.writeInt8e(g.tex);
      cwr.writeInt8e(g.dynGrp + 1 + (g.noPk ? 0 : 0x80));
      // debug("gly[%d] %d, %d,  %d,%d,%d,%d  %.3f,%.3f,%.3f,%.3f", i, g.tex, g.dynGrp, g.x0, g.y0, g.x1, g.y1, g.u0, g.v0, g.u1,
      // g.v1);
    }
    gref.finishTab(cwr);

    if (has_pk)
    {
      cwr.align4();
      gpkidx_ref.startData(cwr, fonts[font_index].glyph.size() + 1);
      for (int i = 0; i < fonts[font_index].glyph.size(); i++)
        cwr.writeInt16e(fonts[font_index].glyph[i].pkOfs);
      cwr.writeInt16e(fonts[font_index].glyphPk.size());
      gpkidx_ref.finishTab(cwr);

      cwr.align4();
      gpkdata_ref.startData(cwr, fonts[font_index].glyphPk.size());
      cwr.write32ex(fonts[font_index].glyphPk.data(), data_size(fonts[font_index].glyphPk));
      gpkdata_ref.finishTab(cwr);
    }

    SharedStorage<uint16_t> gmap;
    SmallTab<Ref, TmpmemAlloc> utgref;
    clear_and_resize(utgref, fonts[font_index].maxUtg + 1);
    for (int i = 0; i <= fonts[font_index].maxUtg; i++)
      if (fonts[font_index].ut[i])
      {
        gmap.getRef(utgref[i], &fonts[font_index].ut[i]->idx[fonts[font_index].ut[i]->first],
          fonts[font_index].ut[i]->last - fonts[font_index].ut[i]->first + 1);
      }

    cwr.writeStorage16e(gmap);
    cwr.align8();
    utref.startData(cwr, fonts[font_index].maxUtg + 1);
    for (int i = 0; i <= fonts[font_index].maxUtg; i++)
      if (!fonts[font_index].ut[i])
      {
        cwr.writePtr64e(-1);
        cwr.writeInt32e(0);
        cwr.writeInt32e(0);
      }
      else
      {
        // uint16_t *glyphIdx;
        // uint16_t startIdx, count;
        cwr.writePtr64e(gmap.indexToOffset(utgref[i].start));
        cwr.writeInt32e(fonts[font_index].ut[i]->first);
        cwr.writeInt32e(utgref[i].count);
      }
    utref.finishTab(cwr);

    cwr.align8();
    texref.reserveData(cwr, ims.size(), 8 + 4 + 4);
    texref.finishTab(cwr);

    cwr.popOrigin();
    cwr.endBlock();
#undef cwr

    cwr.beginBlock();
    MemoryLoadCB mcrd(cwr1.getMem(), false);
    lzma_compress_data(cwr.getRawWriter(), 9, mcrd, cwr1.getSize());
    cwr.endBlock();
  }


  int mem_texsz = 0, file_texsz = 0;
  cwr.beginBlock();
  for (int i = 0; i < ims.size(); ++i)
  {
    cwr.beginBlock();

    if (rgba32)
      tmp_data.resize(texws[i] * texhs[i] * 4 + 0x80);
    else
      tmp_data.resize(texws[i] * texhs[i] + 0x80);

    TexPixel32 *im = ims[i];
    uint32_t *dds_hdr = (uint32_t *)tmp_data.data();
    uint8_t *pix = &tmp_data[0x80];
    int pix_num = texws[i] * texhs[i];

    memset(dds_hdr, 0, 0x80);
    dds_hdr[0] = _MAKE4C('DDS ');
    dds_hdr[1] = 0x7C;       // sizeof(DDSURFACEDESC2)
    dds_hdr[2] = 0x00081007; // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE
    dds_hdr[3] = texhs[i];
    dds_hdr[4] = texws[i];
    dds_hdr[5] = pix_num * (rgba32 ? 4 : 1);
    dds_hdr[19] = 0x20;                             // sizeof(DDPIXELFORMAT)
    dds_hdr[20] = rgba32 ? 0x00000041 : 0x00020000; // DDPF_LUMINANCE
    dds_hdr[22] = rgba32 ? 0x00000020 : 8;
    if (rgba32)
    {
      dds_hdr[23] = 0x00FF0000;
      dds_hdr[24] = 0x0000FF00;
      dds_hdr[25] = 0x000000FF;
      dds_hdr[26] = 0xFF000000;
    }
    else
      dds_hdr[23] = 0xFF;
    dds_hdr[27] = 0x00001000; // DDSD_PIXELFORMAT

    if (rgba32)
      memcpy(pix, im, pix_num * 4);
    else
      for (; pix_num > 0; pix_num--, im++, pix++)
        *pix = im->a;

    if (dagor_target_code_valid(targetCode))
    {
      loadDdsxPluginsOnce();

      ddsx::Buffer buf;
      ddsx::ConvertParams cp;
      cp.allowNonPow2 = true;
      cp.packSzThres = 16 << 10;
      cp.addrU = ddsx::ConvertParams::ADDR_CLAMP;
      cp.addrV = ddsx::ConvertParams::ADDR_CLAMP;
      cp.imgGamma = 1.0;

      if (ddsx::convert_dds(targetCode, buf, tmp_data.data(), data_size(tmp_data), cp))
      {
        file_texsz += buf.len;
        unsigned msz = ((ddsx::Header *)buf.ptr)->memSz;
        mem_texsz += mkbindump::le2be32_cond(msz, cwr.WRITE_BE);
        cwr.writeRaw(buf.ptr, buf.len);
        buf.free();
      }
      else
      {
        printf("[FATAL ERROR] cannot convert font to DDSx\n");
        fcwr.close();
        dd_erase(fn);
        return false;
      }
    }

    cwr.endBlock();
  }
  cwr.endBlock();

  if (dynTTFUsed.nameCount())
  {
    cwr.beginBlock();

    G_ASSERT(dynTTFUsed.nameCount() < 0xFFFF);
    for (int i = 0; i < dynTTFUsed.nameCount(); i++)
    {
      const char *nm = dynTTFUsed.getName(i);
      const char *sz_p = strchr(nm, '?');
      const char *v_p = sz_p ? strchr(sz_p + 1, '?') : nullptr;
      const char *h_p = v_p ? strchr(v_p + 1, '?') : nullptr;
      G_ASSERT(sz_p && v_p);
      cwr.writeDwString(String(0, "%.*s", sz_p - nm, nm));
      cwr.writeInt16e(atoi(sz_p + 1));
      cwr.writeInt8e(atoi(h_p + 1));
      cwr.writeInt8e(atoi(v_p + 1));
    }
    cwr.align16();
    cwr.endBlock();
  }

  DAGOR_TRY
  {
    MutexAutoAcquire mutexAa(fn);
    cwr.copyDataTo(fcwr);
  }
  DAGOR_CATCH(IGenSave::SaveException &)
  {
    printf("can't write dump to output: %s", fn);
    fcwr.close();
    dd_erase(fn);
    return false;
  }
  printf("fileSize: %dK, texSize: %dK (%dK packed in file)\n", cwr.getSize() >> 10, mem_texsz >> 10, file_texsz >> 10);
  return true;
}

int BinFormat::getGlyphsCount()
{
  int result = 0;
  for (int i = 0; i < fonts.size(); i++)
    result += fonts[i].glyph.size();
  return result;
}

#include <stdlib.h>
static void loadDdsxPluginsOnce()
{
  static bool loaded = false;
  if (loaded)
    return;

  char start_dir[260];
  dag_get_appmodule_dir(start_dir, sizeof(start_dir));

#if _TARGET_PC_LINUX
  ddsx::load_plugins(String(260, "%s/../bin-linux64/plugins/ddsx", start_dir));
#elif _TARGET_PC_MACOSX
  ddsx::load_plugins(String(260, "%s/../bin-macosx/plugins/ddsx", start_dir));
#elif _TARGET_64BIT
  ddsx::load_plugins(String(260, "%s/../bin64/plugins/ddsx", start_dir));
#else
  ddsx::load_plugins(String(260, "%s/../bin/plugins/ddsx", start_dir));
#endif
  loaded = true;
}
