// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dtx/ddsxPlugin.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <libTools/dtx/dtxHeader.h>
#include <libTools/util/fileUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_title()
{
  printf("DDS -> DDSx Converter v1.1\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}
static unsigned parseAddrMode(const char *addr_mode)
{
  if (stricmp(addr_mode, "wrap") == 0)
    return ddsx::ConvertParams::ADDR_WRAP;
  if (stricmp(addr_mode, "clamp") == 0)
    return ddsx::ConvertParams::ADDR_CLAMP;
  if (stricmp(addr_mode, "mirror") == 0)
    return ddsx::ConvertParams::ADDR_MIRROR;
  if (stricmp(addr_mode, "mirror_once") == 0)
    return ddsx::ConvertParams::ADDR_MIRRORONCE;
  if (stricmp(addr_mode, "border") == 0)
    return ddsx::ConvertParams::ADDR_BORDER;

  printf("unknown addr mode: %s\n", addr_mode);
  exit(1);
  return 0;
}

int DagorWinMain(bool debugmode)
{
  signal(SIGINT, ctrl_break_handler);
  Tab<char *> argv(tmpmem);
  bool quiet = false, decode_dds_fname = false;
  ddsx::ConvertParams cp;

  for (int i = 1; i < dgs_argc; i++)
  {
    if (dgs_argv[i][0] != '-')
      argv.push_back(dgs_argv[i]);
    else if (stricmp(&dgs_argv[i][1], "q") == 0)
      quiet = true;
    else if (stricmp(&dgs_argv[i][1], "opt") == 0)
      cp.optimize = true;
    else if (stricmp(&dgs_argv[i][1], "opt-") == 0)
      cp.optimize = false;
    else if (stricmp(&dgs_argv[i][1], "pow2") == 0)
      cp.allowNonPow2 = false;
    else if (stricmp(&dgs_argv[i][1], "pow2-") == 0)
      cp.allowNonPow2 = true;
    else if (stricmp(&dgs_argv[i][1], "pack") == 0 || stricmp(&dgs_argv[i][1], "zlib") == 0)
      cp.canPack = true;
    else if (stricmp(&dgs_argv[i][1], "pack-") == 0 || stricmp(&dgs_argv[i][1], "zlib-") == 0)
      cp.canPack = false;
    else if (stricmp(&dgs_argv[i][1], "no7z") == 0)
      ddsx::ConvertParams::forceZlibPacking = true;
    else if (stricmp(&dgs_argv[i][1], "pack:7zip") == 0)
      ddsx::ConvertParams::preferZstdPacking = ddsx::ConvertParams::forceZlibPacking = ddsx::ConvertParams::allowOodlePacking = false;
    else if (stricmp(&dgs_argv[i][1], "pack:zlib") == 0)
      ddsx::ConvertParams::forceZlibPacking = true;
    else if (stricmp(&dgs_argv[i][1], "pack:zstd") == 0)
      ddsx::ConvertParams::preferZstdPacking = true;
    else if (stricmp(&dgs_argv[i][1], "pack:oodle") == 0)
      ddsx::ConvertParams::preferZstdPacking = ddsx::ConvertParams::allowOodlePacking = true;
    else if (strnicmp(&dgs_argv[i][1], "packThres:", 10) == 0)
      cp.packSzThres = atoi(&dgs_argv[i][10]);
    else if (strnicmp(&dgs_argv[i][1], "addru:", 6) == 0)
      cp.addrU = parseAddrMode(&dgs_argv[i][7]);
    else if (strnicmp(&dgs_argv[i][1], "addrv:", 6) == 0)
      cp.addrV = parseAddrMode(&dgs_argv[i][7]);
    else if (strnicmp(&dgs_argv[i][1], "hq:", 3) == 0)
      cp.hQMip = atoi(&dgs_argv[i][4]);
    else if (strnicmp(&dgs_argv[i][1], "mq:", 3) == 0)
      cp.mQMip = atoi(&dgs_argv[i][4]);
    else if (strnicmp(&dgs_argv[i][1], "lq:", 3) == 0)
      cp.lQMip = atoi(&dgs_argv[i][4]);
    else if (stricmp(&dgs_argv[i][1], "ddsSuffix") == 0)
      decode_dds_fname = true;
    else if (stricmp(&dgs_argv[i][1], "gamma1") == 0)
      cp.imgGamma = 1.0;
    else if (stricmp(&dgs_argv[i][1], "mipRev") == 0)
      cp.mipOrdRev = true;
    else if (stricmp(&dgs_argv[i][1], "mipRev-") == 0)
      cp.mipOrdRev = false;
    else
    {
      print_title();
      printf("ERR: unknown option <%s>\n", dgs_argv[i]);
      return 1;
    }
  }
  if (argv.size() < 2)
  {
    print_title();
    printf("usage: ddsxCvt.exe [options] <target_4cc> <src_dds> [<out_ddsx>]\n"
           "options:\n"
           "  -q       run in quiet mode\n"
           "  -opt[-]  enable/disable optimal texture format\n"
           "  -pow2[-] force-pow-2/allow non-pow2\n"
           "  -pack[-] enable/disable texture data packing (new alias for -zlib[-])\n"
           "  -pack:zlib   use zlib for texture data packing (new alias for -no7z)\n"
           "  -pack:zstd   use zstd for texture data packing\n"
           "  -pack:7zip   use 7zip for texture data packing\n"
           "  -pack:oodle  use OODLE for texture data packing\n"
           "  -packThres:{sz}  size threshold for using texture data packing\n"
           "  -gamma1          image is not gamma correct\n"
           "  -ddsSuffix       use settings decoded from DDS fname suffix\n"
           "  -mipRev[-]       use reversed mips order (OPTIMAL for partial reading)/use legacy mip order\n"
           "  -hq:{n}  high quality top mipmap\n"
           "  -mq:{n}  medium quality top mipmap\n"
           "  -lq:{n}  low quality top mipmap\n"
           "  -addr{u|v}:{wrap|mirror|clamp|border|mirror_once}"
           "  set default texture addressing mode\n");
    return -1;
  }
  if (!quiet)
    print_title();

  if (decode_dds_fname)
  {
    ddstexture::Header dds_header;
    dds_header.getFromFile(argv[1]);

    cp.addrU = dds_header.addr_mode_u;
    cp.addrV = dds_header.addr_mode_v;
    cp.hQMip = dds_header.HQ_mip;
    cp.mQMip = dds_header.MQ_mip;
    cp.lQMip = dds_header.LQ_mip;
  }

  file_ptr_t fp = df_open(argv[1], DF_READ);
  SmallTab<char, TmpmemAlloc> dds;
  if (!fp)
  {
    if (!quiet)
      printf("ERR: cannot open <%s>\n", argv[1]);
    return 1;
  }
  clear_and_resize(dds, df_length(fp));
  df_read(fp, dds.data(), data_size(dds));
  df_close(fp);

  char start_dir[260];
  dag_get_appmodule_dir(start_dir, sizeof(start_dir));

  int pc = ddsx::load_plugins(String(260, "%s/plugins/ddsx", start_dir));
  if (!quiet)
    printf("Loaded %d plugin(s)\n", pc);

  unsigned four_cc = 0;
  for (int i = 0; i < 4; i++)
    if (argv[0][i] == '\0')
      break;
    else
      four_cc = (four_cc >> 8) | (argv[0][i] << 24);

  if (!quiet)
    printf("Convert for target: %c%c%c%c\n", _DUMP4C(four_cc));

  String out_fn;
  if (argv.size() > 2)
    out_fn = argv[2];
  else
  {
    out_fn = argv[1];
    const char *ext = dd_get_fname_ext(out_fn);
    if (ext)
      erase_items(out_fn, ext - (char *)out_fn, strlen(ext));
    if (decode_dds_fname)
    {
      ext = strrchr(out_fn, '@');
      if (ext)
        erase_items(out_fn, ext - (char *)out_fn, strlen(ext));
    }
    out_fn += ".ddsx";
  }

  dd_mkpath(out_fn);
  ddsx::Buffer buf;
  if (!ddsx::convert_dds(four_cc, buf, dds.data(), data_size(dds), cp))
  {
    if (!dd_fname_equal(argv[1], out_fn))
      dd_erase(out_fn);
    if (!quiet)
      printf("ERR: cannot convert texture (%s)\n", ddsx::get_last_error_text());
    return 1;
  }

  fp = df_open(out_fn, DF_CREATE | DF_WRITE);
  if (!fp)
  {
    if (!quiet)
      printf("ERR: cannot open output <%s>\n", (char *)out_fn);
    return 1;
  }
  df_write(fp, buf.ptr, buf.len);
  df_close(fp);
  buf.free();

  return 0;
}
