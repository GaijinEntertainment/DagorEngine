// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/util/conLogWriter.h>
#include <libTools/util/fileUtils.h>
#include <assets/assetHlp.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <libTools/dtx/dtxHeader.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <3d/ddsxTex.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <debug/dag_logSys.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_title()
{
  printf("Image -> DDSx Converter v2.2\n"
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
struct AssetRec
{
  String name;
  DataBlock props;
};

int DagorWinMain(bool debugmode)
{
  ConsoleLogWriter log;
  signal(SIGINT, ctrl_break_handler);
  Tab<char *> argv(tmpmem);
  Tab<AssetRec> asset_rec;
  bool quiet = false, decode_dds_fname = false;
  String asset_blk;
  const char *proj_root = "";
  const char *asset_blk_file = NULL;
  bool no_cache = false;
  bool force_convert = false;
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
    else if (strnicmp(&dgs_argv[i][1], "splitAt:", 8) == 0)
      cp.splitAt = atoi(&dgs_argv[i][9]);
    else if (stricmp(&dgs_argv[i][1], "splitHigh") == 0)
      cp.splitHigh = true;
    else if (strnicmp(&dgs_argv[i][1], "proj_root:", 10) == 0)
      proj_root = &dgs_argv[i][11];
    else if (strnicmp(&dgs_argv[i][1], "blk:", 4) == 0)
      asset_blk.aprintf(0, "%s\n", &dgs_argv[i][5]);
    else if (strnicmp(&dgs_argv[i][1], "blk_file:", 9) == 0)
      asset_blk_file = &dgs_argv[i][10];
    else if (strnicmp(&dgs_argv[i][1], "add_base_asset:", 15) == 0)
    {
      const char *nm = dgs_argv[i] + 16;
      if (const char *p = strchr(nm, ':'))
      {
        AssetRec &r = asset_rec.push_back();
        r.name.printf(0, "%.*s", p - nm, nm);
        if (!r.props.load(p + 1))
        {
          printf("ERR: cannot load BLK %s for base_asset <%s>\n", p + 1, r.name.str());
          return 1;
        }
      }
      else
      {
        printf("ERR: bad option format <%s>\n", dgs_argv[i]);
        return 1;
      }
    }
    else if (stricmp(&dgs_argv[i][1], "no_cache") == 0)
      no_cache = true;
    else if (stricmp(&dgs_argv[i][1], "convert") == 0)
      force_convert = true;
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
    printf("usage: ddsxCvt2-dev.exe [options] <target_4cc> <src_dds> [<out_ddsx>]\n"
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
           "  -convert         force conversion of DDS files (using texconv)\n"
           "  -hq:{n}  high quality top mipmap\n"
           "  -mq:{n}  medium quality top mipmap\n"
           "  -lq:{n}  low quality top mipmap\n"
           "  -addr{u|v}:{wrap|mirror|clamp|border|mirror_once}  set default texture addressing mode\n"
           "  -splitAt:{n} set size threshold to split texture into bq/hq parts (def=0)\n"
           "  -splitHigh   builds hq part\n"
           "  -proj_root:<DIR>\n"
           "  -blk:<ASSET_PROP_STATEMENT>\n"
           "  -blk_file:<PROPS.BLK>\n"
           "  -add_base_asset:<name>:<PROPS.BLK>\n"
           "  -no_cache\n");
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

  ddsx::Buffer buf;
  if ((dd_get_fname_ext(argv[1]) && stricmp(dd_get_fname_ext(argv[1]), ".dds") != 0) || force_convert)
  {
    static const char *s_addr[] = {"wrap", "mirror", "clamp", "border", "mirrorOnce"};
    DagorAssetMgr mgr;
    DataBlock appblk;
    DataBlock props;
    appblk.addBlock("assets")->addBlock("types")->setStr("type", "tex");
    if (*proj_root)
    {
      appblk.addBlock("assets")->addBlock("export")->setStr("cache", "/develop/.cache-tex");
      appblk.setStr("appDir", proj_root);
    }
    else
    {
      appblk.addBlock("assets")->addBlock("export")->setStr("cache", ".");
      appblk.setStr("appDir", ".");
    }

    appblk.addBlock("assets")->addBlock("build")->setBool("preferZLIB", ddsx::ConvertParams::forceZlibPacking);

    if (asset_blk_file)
      props.load(asset_blk_file);
    else
      dblk::load_text(props, asset_blk, dblk::ReadFlags(), argv[1]);
    props.setStr("name", argv[1]);
    props.setBool("convert", true);
    props.setReal("gamma", props.getReal("gamma", cp.imgGamma));
    props.setInt("hqMip", props.getInt("hqMip", cp.hQMip));
    props.setInt("mqMip", props.getInt("mqMip", cp.mQMip));
    props.setInt("lqMip", props.getInt("lqMip", cp.lQMip));
    if (cp.addrU > 0)
      props.setStr("addrU", props.getStr("addrU", s_addr[cp.addrU - 1]));
    if (cp.addrV > 0)
      props.setStr("addrV", props.getStr("addrV", s_addr[cp.addrV - 1]));

    mgr.setupAllowedTypes(*appblk.getBlockByNameEx("assets")->getBlockByNameEx("types"));
    String assetName(DagorAsset::fpath2asset(argv[1]));
    if (props.getInt("splitAt", 0) > 0 && props.getBool("splitHigh", false))
      assetName.aprintf(0, "$hq");
    DagorAsset *a = mgr.makeAssetDirect(assetName, props, mgr.getTexAssetTypeId());
    for (int i = 0; i < asset_rec.size(); i++)
      mgr.makeAssetDirect(asset_rec[i].name, asset_rec[i].props, mgr.getTexAssetTypeId());

    if (!texconvcache::init(mgr, appblk, start_dir, no_cache))
    {
      printf("ERR: texconvcache::init failed\n");
      return 13;
    }
    if (!texconvcache::get_tex_asset_built_ddsx(*a, buf, four_cc, "ddsx", &log))
    {
      printf("ERR: Can't export image: %s, err=%s\n", argv[1], ddsx::get_last_error_text());
      return 13;
    }
    texconvcache::term();
  }
  else
  {
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

    if (!ddsx::convert_dds(four_cc, buf, dds.data(), data_size(dds), cp))
    {
      if (!dd_fname_equal(argv[1], out_fn))
        dd_erase(out_fn);
      if (!quiet)
        printf("ERR: cannot convert texture (%s)\n", ddsx::get_last_error_text());
      return 1;
    }
  }

  dd_mkpath(out_fn);
  file_ptr_t fp = df_open(out_fn, DF_CREATE | DF_WRITE);
  if (!fp)
  {
    if (!quiet)
      printf("ERR: cannot open output <%s>\n", (char *)out_fn);
    return 1;
  }
  InPlaceMemLoadCB crd(buf.ptr, buf.len);
  ddsx::Header hdr;
  crd.read(&hdr, sizeof(hdr));

  if (cp.canPack || !hdr.packedSz)
    df_write(fp, buf.ptr, buf.len);
  else
  {
    ddsx::Header hdr2 = hdr;
    hdr2.flags &= ~ddsx::Header::FLG_COMPR_MASK;
    hdr2.packedSz = 0;

    LFileGeneralSaveCB cwr(fp);
    cwr.write(&hdr2, sizeof(hdr2));
    if (hdr.isCompressionZSTD())
    {
      ZstdLoadCB zcrd(crd, hdr.packedSz);
      copy_stream_to_stream(zcrd, cwr, hdr.memSz);
      zcrd.ceaseReading();
    }
    else if (hdr.isCompressionOODLE())
    {
      OodleLoadCB zcrd(crd, hdr.packedSz, hdr.memSz);
      copy_stream_to_stream(zcrd, cwr, hdr.memSz);
      zcrd.ceaseReading();
    }
    else if (hdr.isCompressionZLIB())
    {
      ZlibLoadCB zlib_crd(crd, hdr.packedSz);
      copy_stream_to_stream(zlib_crd, cwr, hdr.memSz);
      zlib_crd.ceaseReading();
    }
    else if (hdr.isCompression7ZIP())
    {
      LzmaLoadCB lzma_crd(crd, hdr.packedSz);
      copy_stream_to_stream(lzma_crd, cwr, hdr.memSz);
      lzma_crd.ceaseReading();
    }
    else
      copy_stream_to_stream(crd, cwr, hdr.memSz);
  }
  df_close(fp);
  buf.free();

  return 0;
}
