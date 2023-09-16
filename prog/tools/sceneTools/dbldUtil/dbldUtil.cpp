#include <libTools/util/binDumpReader.h>
#include <libTools/util/makeBindump.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <scene/dag_loadLevelVer.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_fastIntList.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_globDef.h>
#include <util/dag_strUtil.h>
#include <math/dag_bounds3.h>
#include <gameRes/grpData.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>
#include <hash/sha1.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_header()
{
  printf("DBLD import/export/dep utility v1.4\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}

static bool exportDbldFields(BinDumpReader &crd, unsigned exp_tag, const char *exp_fname)
{
  int tag;

  if (crd.readFourCC() != _MAKE4C('DBLD'))
    return false;
  if (crd.readFourCC() != DBLD_Version)
    return false;
  crd.readInt32e(); // total tex number

  FullFileSaveCB exp_cwr(exp_fname);
  if (!exp_cwr.fileHandle)
  {
    printf("ERROR: cannot open output file for export <%s>\n", exp_fname);
    return true;
  }

  for (;;)
  {
    tag = crd.beginTaggedBlock();

    if (tag == _MAKE4C('END'))
    {
      // valid end of binary dump
      crd.endBlock();
      return true;
    }
    else if (tag == exp_tag)
    {
      int sz = crd.getBlockRest() + 8;
      crd.endBlock();
      crd.seekto(crd.tell() - sz);
      copy_stream_to_stream(crd.getRawReader(), exp_cwr, sz);
      continue;
    }

    crd.endBlock();
  }
  return true;
}

static bool importDbldFields(BinDumpReader &crd, unsigned imp_tag, const char *imp_fname, mkbindump::BinDumpSaveCB &cwr)
{
  int tag;
  bool replaced = false;

  if (imp_fname && !dd_file_exist(imp_fname))
  {
    printf("ERROR: cannot read input file for import <%s>\n", imp_fname);
    return false;
  }

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
      if (!replaced && imp_fname)
        copy_file_to_stream(imp_fname, cwr.getRawWriter());

      // valid end of binary dump
      cwr.beginTaggedBlock(_MAKE4C('END'));
      cwr.endBlock();
      crd.endBlock();
      return true;
    }
    else if (tag == imp_tag)
    {
      if (!replaced)
      {
        replaced = true;
        if (imp_fname)
          copy_file_to_stream(imp_fname, cwr.getRawWriter());
      }
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

static bool dbldStripGfxFields(BinDumpReader &crd, mkbindump::BinDumpSaveCB &cwr)
{
  int tag;
  bool stripped = false;

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
    else if (tag == _MAKE4C('TEX') || tag == _MAKE4C('SCN'))
    {
      stripped = true;
      crd.endBlock();
      continue;
    }
    else if (tag == _MAKE4C('lmap'))
    {
      cwr.beginTaggedBlock(tag);
      unsigned v = crd.readFourCC();
      if (v != _MAKE4C('lndm'))
      {
        printf("ERROR: Invalid file format %c%c%c%c\n", _DUMP4C(v));
        return false;
      }
      cwr.writeFourCC(v);

      v = crd.readInt32e();
      if (v != 4)
      {
        printf("ERROR: Unsupported file version %d at %d\n", v, crd.tell());
        return false;
      }
      cwr.writeInt32e(v);

      struct LandHdr
      {
        float gridCellSize, landCellSize;
        int mapSizeX, mapSizeY, origin_x, origin_y;
      } lnd_hdr;
      crd.read32ex(&lnd_hdr, sizeof(lnd_hdr));
      cwr.write32ex(&lnd_hdr, sizeof(lnd_hdr));

      int useTile = crd.readInt32e();
      cwr.writeInt32e(0);

      int baseDataOffset = crd.tell();
      int meshMapOfs = baseDataOffset + crd.readInt32e();
      int detailDataOfs = baseDataOffset + crd.readInt32e();
      int tileDataOfs = baseDataOffset + crd.readInt32e();
      int rayTracerOfs = baseDataOffset + crd.readInt32e();

      int cell_bounds_data_sz = lnd_hdr.mapSizeX * lnd_hdr.mapSizeY * (sizeof(BBox3) + sizeof(float)), ofs = 16;
      if (meshMapOfs < baseDataOffset + 16 || detailDataOfs <= meshMapOfs)
        cell_bounds_data_sz = 0;

      cwr.writeInt32e(ofs);
      ofs += cell_bounds_data_sz;
      cwr.writeInt32e(ofs);
      cwr.writeInt32e(ofs);
      ofs += 4;
      cwr.writeInt32e(rayTracerOfs > baseDataOffset ? ofs : -1);

      crd.seekto(detailDataOfs - cell_bounds_data_sz);
      copy_stream_to_stream(crd.getRawReader(), cwr.getRawWriter(), cell_bounds_data_sz);

      if (rayTracerOfs > baseDataOffset)
      {
        crd.seekto(rayTracerOfs - 4);
        copy_stream_to_stream(crd.getRawReader(), cwr.getRawWriter(), crd.getBlockRest());
      }
      cwr.endBlock();
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

static void copy_stream_to_stream_h(IGenLoad &crd, IGenSave &cwr, int size, String &out_hash)
{
  static const int BUF_SZ = 32 << 10;
  int len;
  char buf[BUF_SZ];

  sha1_context sha1c;
  sha1_starts(&sha1c);
  while (size > 0)
  {
    len = size > BUF_SZ ? BUF_SZ : size;
    size -= len;
    crd.read(buf, len);
    cwr.write(buf, len);
    sha1_update(&sha1c, (unsigned char *)buf, len);
  }
  unsigned char digest[20];
  sha1_finish(&sha1c, digest);
  String tmpStr;
  data_to_str_hex(tmpStr, digest, sizeof(digest));
  tmpStr.toUpper();
  out_hash.printf(0, " SHA1=%s", tmpStr);
}

static bool dumpDbldFields(BinDumpReader &crd, const char *outDir = NULL)
{
  int tag;

  if (crd.readFourCC() != _MAKE4C('DBLD'))
    return false;
  if (crd.readFourCC() != DBLD_Version)
    return false;
  printf("tex count: %d\n", crd.readInt32e()); // total tex number

  String hash;
  for (;;)
  {
    tag = crd.beginTaggedBlock();

    if (tag == _MAKE4C('END'))
    {
      // valid end of binary dump
      crd.endBlock();
      printf("valid end at %08X\n", crd.tell());
      return true;
    }
    else if (outDir)
    {
      String fn(0, "%s/%c%c%c%c.field.bin", outDir, _DUMP4C(tag));
      dd_mkpath(fn);
      FullFileSaveCB exp_cwr(fn);
      int pos = crd.tell();
      int sz = crd.getBlockRest() + 8;
      crd.getRawReader().seekrel(-8);
      if (exp_cwr.fileHandle)
        copy_stream_to_stream_h(crd.getRawReader(), exp_cwr, sz, hash);
      else
        clear_and_shrink(hash);
      crd.seekto(pos);
    }
    printf("%c%c%c%c at %08X..%08X  size=%-9d%s\n", _DUMP4C(tag), crd.tell(), crd.tell() + crd.getBlockRest() - 1, crd.getBlockRest(),
      hash.str());

    crd.endBlock();
  }
  return true;
}

extern bool dumpDbldDeps(IGenLoad &crd, const DataBlock &env);

static void stderr_report_fatal_error(const char *, const char *msg, const char *call_stack)
{
  fprintf(stderr, "Fatal error: %s\n%s", msg, call_stack);
  quit_game(1, false);
}


int DagorWinMain(bool debugmode)
{
  dgs_report_fatal_error = stderr_report_fatal_error;
  if (__argc < 4)
  {
    print_header();
    printf("usage: dbldUtil-dev.exe <platform_4CC> <input.level.bin> -exp:<field_4CC> <export.bin>\n"
           "       dbldUtil-dev.exe <platform_4CC> <inout.level.bin> -imp:<field_4CC> <import.bin>\n"
           "       dbldUtil-dev.exe <platform_4CC> <inout.level.bin> -dump\n"
           "       dbldUtil-dev.exe <platform_4CC> <inout.level.bin> -dumpData:<out_dir>\n"
           "       dbldUtil-dev.exe <platform_4CC> <inout.level.bin> -stripGfx[:<out_stripped.bin]>\n"
           "       dbldUtil-dev.exe <platform_4CC> <inout.level.bin> -deps:<env_config.blk>\n\n"
           "env_config.blk format:\n"
           "  root:t=\"...\"\n"
           "  vromfs { file:t=\"...\"; mnt:t=\"...\"; }...\n"
           "  shaders:t=\"...\"\n"
           "  resList:t=\"...\"\n"
           "  settings:t=\"...\"\n"
           "  gameParams:t=\"...\"\n"
           "  riDmg:t=\"...\"\n"
           "  fields { TAG1:b=no; ... }\n\n");
    return -1;
  }
  signal(SIGINT, ctrl_break_handler);

  unsigned targetCode = 0;
  const char *targetStr = __argv[1];
  while (*targetStr)
  {
    targetCode = (targetCode >> 8) | (*targetStr << 24);
    targetStr++;
  }
  bool read_be = dagor_target_code_be(targetCode);

  FullFileLoadCB base_crd(__argv[2]);
  if (!base_crd.fileHandle)
  {
    printf("ERROR: cannot open <%s>\n", __argv[2]);
    return 13;
  }

  BinDumpReader crd(&base_crd, targetCode, read_be);
  if (strnicmp(__argv[3], "-exp:", 5) == 0 && __argc > 4)
  {
    unsigned tag = 0;
    targetStr = __argv[3] + 5;
    while (*targetStr)
    {
      tag = (tag >> 8) | (*targetStr << 24);
      targetStr++;
    }
    if (!exportDbldFields(crd, tag, __argv[4]))
    {
      printf("ERROR: invalid DBLD <%s>\n", __argv[2]);
      unlink(__argv[4]);
      return 13;
    }
  }
  else if (strnicmp(__argv[3], "-imp:", 5) == 0 && __argc > 4)
  {
    unsigned tag = 0;
    targetStr = __argv[3] + 5;
    while (*targetStr)
    {
      tag = (tag >> 8) | (*targetStr << 24);
      targetStr++;
    }

    mkbindump::BinDumpSaveCB cwr(df_length(base_crd.fileHandle), targetCode, read_be);
    if (!importDbldFields(crd, tag, stricmp(__argv[4], "null") == 0 ? NULL : __argv[4], cwr))
      return 13;
    base_crd.close();
    FullFileSaveCB fcwr(__argv[2]);
    cwr.copyDataTo(fcwr);
  }
  else if (stricmp(__argv[3], "-dump") == 0)
  {
    if (!dumpDbldFields(crd))
      return 13;
  }
  else if (strnicmp(__argv[3], "-dumpData:", 10) == 0 && __argv[3][10])
  {
    if (!dumpDbldFields(crd, __argv[3] + 10))
      return 13;
  }
  else if (stricmp(__argv[3], "-stripGfx") == 0 || strnicmp(__argv[3], "-stripGfx:", 10) == 0)
  {
    mkbindump::BinDumpSaveCB cwr(df_length(base_crd.fileHandle), targetCode, read_be);
    if (!dbldStripGfxFields(crd, cwr))
      return 13;
    base_crd.close();
    FullFileSaveCB fcwr(strlen(__argv[3]) > 10 ? __argv[3] + 10 : __argv[2]);
    cwr.copyDataTo(fcwr);
  }
  else if (strnicmp(__argv[3], "-deps:", 6) == 0)
  {
    DataBlock env;
    if (read_be)
    {
      printf("ERROR: -deps is not implemented for BE streams\n");
      return 13;
    }
    if (!env.load(__argv[3] + 6))
    {
      printf("ERROR: invalid env BLK <%s>\n", __argv[3] + 6);
      return 13;
    }
    if (!dumpDbldDeps(crd.getRawReader(), env))
      return 13;
  }
  else
  {
    printf("ERROR: unknown op <%s> or opcount=%d\n", __argv[3], __argc - 1);
    return 13;
  }

  return 0;
}
