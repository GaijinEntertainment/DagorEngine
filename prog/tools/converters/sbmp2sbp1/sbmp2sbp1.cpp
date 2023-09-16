#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpReader.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <scene/dag_loadLevelVer.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_zlibIo.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <math/dag_color.h>
#include <math/dag_Point3.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_title()
{
  printf("Binary level (DBLD 2112) SBMP -> SBP1 Converter v1.0\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}

static const char *export_prefix = NULL, *import_prefix = NULL;
static bool skip_convert = false;

static bool copy_level_bin_scn(BinDumpReader &crd, mkbindump::BinDumpSaveCB &cwr)
{
  struct SceneHdr // format 'scn2'
  {
    int version, rev;
    int texNum, matNum, vdataNum, mvhdrSz;
    int ltmapNum, ltmapFormat;
    float lightmapScale, directionalLightmapScale;
    int objNum, fadeObjNum, ltobjNum;
    int objDataSize;
  };
  SceneHdr sHdr;
  int hdr_wr_pos = cwr.tell();

  crd.read32ex(&sHdr, sizeof(sHdr));
  cwr.write32ex(&sHdr, sizeof(sHdr));
  if (crd.readBE())
  {
    sHdr.version = crd.le2be32(sHdr.version);
    sHdr.ltmapFormat = crd.le2be32(sHdr.ltmapFormat);
  }

  if (sHdr.version != _MAKE4C('scn2'))
  {
    printf("ERROR: label scn2 not found\n");
    return false;
  }
  if (sHdr.rev < 0x20071906)
  {
    printf("ERROR: old scene revision %08X, must be >=2007/19/06\n", sHdr.rev);
    return false;
  }

  if (sHdr.ltmapFormat != _MAKE4C('DUAL') && sHdr.ltmapFormat != _MAKE4C('DDS') && sHdr.ltmapFormat != _MAKE4C('TGA') &&
      sHdr.ltmapFormat != _MAKE4C('HDR') && sHdr.ltmapFormat != _MAKE4C('SBMP'))
  {
    printf("ERROR: unknown ltmap format %c%c%c%c", _DUMP4C(sHdr.ltmapFormat));
    return false;
  }

  if (sHdr.ltmapFormat != _MAKE4C('SBMP'))
  {
    printf("WARNING: ltmap format is %c%c%c%c, not SBMP, no conversion needed\n", _DUMP4C(sHdr.ltmapFormat));
    return false;
  }

  if (sHdr.ltmapFormat == _MAKE4C('SBMP'))
  {
    copy_stream_to_stream(crd.getRawReader(), cwr.getRawWriter(), sizeof(Color3));
    copy_stream_to_stream(crd.getRawReader(), cwr.getRawWriter(), sizeof(Point3));
  }
  crd.beginBlock();
  cwr.beginBlock();
  copy_stream_to_stream(crd.getRawReader(), cwr.getRawWriter(), sHdr.texNum * 4);

  // read lightmaps catalog (offsets to data)
  int lm_base = crd.tell();
  Tab<int> lm_ofs(tmpmem);
  lm_ofs.resize(sHdr.ltmapNum * (sHdr.ltmapFormat == _MAKE4C('DUAL') ? 2 : 1));
  crd.read32ex(lm_ofs.data(), data_size(lm_ofs));

  int wr_pos = cwr.tell();
  cwr.write32ex(lm_ofs.data(), data_size(lm_ofs));
  copy_stream_to_stream(crd.getRawReader(), cwr.getRawWriter(), lm_ofs[0] + lm_base - crd.tell());

  Tab<char> lm_data(tmpmem);
  for (int i = 0; i < lm_ofs.size(); i++)
  {
    crd.seekto(lm_ofs[i] + lm_base);
    cwr.writeInt32eAt(cwr.tell() - wr_pos, wr_pos + i * 4);
    if (sHdr.ltmapFormat == _MAKE4C('SBMP'))
    {
      struct ZpackedHdr
      {
        int dataSz, packedSz;
      };
      ZpackedHdr zhdr;

      for (unsigned int lmi = 0; lmi < 2; lmi++)
      {
        crd.read32ex(&zhdr, sizeof(zhdr));

        if (!import_prefix)
        {
          int ofs = crd.tell();
          lm_data.resize(zhdr.dataSz);
          ZlibLoadCB zlibCb(crd.getRawReader(), zhdr.packedSz);
          zlibCb.readTabData(lm_data);
        }
        else
        {
          String lmfn(32, "%s.lm%d.%d.dds", import_prefix, i, lmi);
          FullFileLoadCB lmcrd(lmfn);
          if (!lmcrd.fileHandle)
          {
            printf("ERROR: cannot import %s\n", (char *)lmfn);
            return false;
          }
          lm_data.resize(df_length(lmcrd.fileHandle));
          lmcrd.readTabData(lm_data);
          if (data_size(lm_data) != zhdr.dataSz)
            printf("WARNING: original LM size=%d, imported=%d\n", zhdr.dataSz, data_size(lm_data));
          crd.seekrel(zhdr.packedSz);
        }

        if (export_prefix)
        {
          FullFileSaveCB lmcwr(String(32, "%s.lm%d.%d.dds", export_prefix, i, lmi));
          lmcwr.writeTabData(lm_data);
        }

        /*
        // JUST TO TEST THAT CONVERSION IS BINARY EQUAL
        {
          G_ASSERT(!import_prefix);
          crd.seekrel(-zhdr.packedSz);
          cwr.write32ex(&zhdr, sizeof(zhdr));
          copy_stream_to_stream(crd.getRawReader(), cwr.getRawWriter(), zhdr.packedSz);
          continue;
        }
        */
        if (skip_convert)
          continue;

        ddsx::Buffer buf;
        ddsx::ConvertParams cp;
        cp.allowNonPow2 = false;
        cp.addrU = ddsx::ConvertParams::ADDR_CLAMP;
        cp.addrV = ddsx::ConvertParams::ADDR_CLAMP;
        cp.packSzThres = 0;
        cp.canPack = true;
        cp.mQMip = 1;
        cp.lQMip = 2;

        if (!ddsx::convert_dds(crd.getTarget(), buf, lm_data.data(), data_size(lm_data), cp))
        {
          printf("ERROR: cannot convert LM texture (%s)\n", ddsx::get_last_error_text());
          return false;
        }

        cwr.beginBlock();
        cwr.writeRaw(buf.ptr, buf.len);
        cwr.endBlock();

        if (buf.len)
          printf("DDSx lm[%d]: compression ratio %.2f -> %.2f [packed %d -> %d]\n", i, zhdr.dataSz / (float)zhdr.packedSz,
            data_size(lm_data) / (float)buf.len, zhdr.packedSz, buf.len);
        buf.free();
      }
    }
  }

  crd.endBlock();
  cwr.endBlock();

  if (sHdr.ltmapFormat == _MAKE4C('SBMP'))
  {
    if (crd.readBE())
    {
      sHdr.version = crd.le2be32(sHdr.version);
      sHdr.ltmapFormat = crd.le2be32(_MAKE4C('SB1'));
    }
    wr_pos = cwr.tell();
    cwr.seekto(hdr_wr_pos);
    cwr.write32ex(&sHdr, sizeof(sHdr));
    cwr.seekto(wr_pos);
  }
  return true;
}

static bool copy_level_bin(BinDumpReader &crd, mkbindump::BinDumpSaveCB &cwr)
{
  int tag;

  if (crd.readFourCC() != _MAKE4C('DBLD'))
  {
    printf("ERROR: label DBLD not found\n");
    return false;
  }
  if (crd.readFourCC() != DBLD_Version)
  {
    printf("ERROR: invalid vesion\n");
    return false;
  }

  cwr.writeFourCC(_MAKE4C('DBLD'));
  cwr.writeFourCC(DBLD_Version);
  cwr.writeInt32e(crd.readInt32e()); // total tex number

  for (;;)
  {
    tag = crd.beginTaggedBlock();
    if (tag == _MAKE4C('END'))
    {
      // valid end of binary dump
      cwr.beginTaggedBlock(_MAKE4C('END'));
      cwr.endBlock();
      crd.endBlock();
      return true;
    }
    else if (tag == _MAKE4C('SCN'))
    {
      cwr.beginTaggedBlock(tag);
      if (!copy_level_bin_scn(crd, cwr))
        return false;

      copy_stream_to_stream(crd.getRawReader(), cwr.getRawWriter(), crd.getBlockRest());
      cwr.endBlock();
      crd.endBlock();
      continue;
    }


    cwr.beginTaggedBlock(tag);
    copy_stream_to_stream(crd.getRawReader(), cwr.getRawWriter(), crd.getBlockRest());
    cwr.endBlock();

    crd.endBlock();
  }
  printf("ERROR: unexpected EOF");
  return false;
}

int DagorWinMain(bool debugmode)
{
  signal(SIGINT, ctrl_break_handler);
  Tab<const char *> args(tmpmem);
  bool export_lm = false, import_lm = false, write_bin = true;

  for (int i = 1; i < __argc; i++)
  {
    if (__argv[i][0] != '-')
      args.push_back(__argv[i]);
    else if (stricmp(&__argv[i][1], "export") == 0)
      export_lm = true;
    else if (stricmp(&__argv[i][1], "import") == 0)
      import_lm = true;
    else if (stricmp(&__argv[i][1], "nowrite") == 0)
    {
      write_bin = false;
      skip_convert = true;
    }
  }

  if (args.size() < 2)
  {
    print_title();
    printf("usage: sbmp2sbp1.exe [options] <target_4cc> <level.bin> [<out_level.bin>]\n"
           "options are:\n"
           "  -export   exports lightmap dds to files <level.bin.lm#.dds>\n"
           "  -import   imports lightmap dds to files <level.bin.lm#.dds>\n"
           "  -nowrite  don't write resulting binary (useful for integrity checks or with -export)\n");
    return -1;
  }

  FullFileLoadCB base_crd(args[1]);
  if (!base_crd.fileHandle)
  {
    printf("can't read source: %s\n", args[1]);
    return -1;
  }

  char start_dir[260];
  if (_fullpath(start_dir, __argv[0], 260))
  {
    char *p = strrchr(start_dir, '\\');
    if (p)
      *p = '\0';
  }

  int pc = ddsx::load_plugins(start_dir);
  printf("Loaded %d DDSx plugin(s)\n", pc);

  unsigned targetCode = 0;
  const char *targetStr = args[0];
  while (*targetStr)
  {
    targetCode = (targetCode >> 8) | (*targetStr << 24);
    targetStr++;
  }
  bool write_be = dagor_target_code_be(targetCode);
  printf("Convert for target: %c%c%c%c (%s endian)\n", _DUMP4C(targetCode), write_be ? "big" : "little");

  BinDumpReader crd(&base_crd, targetCode, write_be);
  mkbindump::BinDumpSaveCB cwr(8 << 20, targetCode, write_be);
  export_prefix = export_lm ? args[1] : NULL;
  import_prefix = import_lm ? args[1] : NULL;
  if (!copy_level_bin(crd, cwr))
  {
    printf("ERROR: corrupted file %s\n", args[1]);
    return -2;
  }
  base_crd.close();

  if (write_bin)
  {
    const char *out_fn = args.size() >= 3 ? args[2] : args[1];
    FullFileSaveCB file_cwr(out_fn);
    if (!file_cwr.fileHandle)
    {
      printf("can't write dest: %s\n", out_fn);
      return -1;
    }
    cwr.copyDataTo(file_cwr);
    printf("Successfuly converted %s to %s", args[1], out_fn);
  }
  else
    printf("Successfuly parsed %s", args[1]);
  return 0;
}
