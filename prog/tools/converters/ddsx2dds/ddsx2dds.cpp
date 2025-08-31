// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/ddsxTex.h>
#include <3d/ddsFormat.h>
#include <3d/ddsxTexMipOrder.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void __cdecl ctrl_break_handler(int) { quit_game(0); }


static bool ddsx_to_dds(IGenLoad &crd, IGenSave &cwr)
{
  struct BitMaskFormat
  {
    uint32_t bitCount;
    uint32_t alphaMask;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint32_t format;
    uint32_t flags;
  };
  static BitMaskFormat bitMaskFormat[] = {
    {24, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_R8G8B8, DDPF_RGB},
    {32, 0xff000000, 0xff0000, 0xff00, 0xff, D3DFMT_A8R8G8B8, DDPF_RGB | DDPF_ALPHAPIXELS},
    {32, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_X8R8G8B8, DDPF_RGB},
    {16, 0x00000000, 0x00f800, 0x07e0, 0x1f, D3DFMT_R5G6B5, DDPF_RGB},
    {16, 0x00008000, 0x007c00, 0x03e0, 0x1f, D3DFMT_A1R5G5B5, DDPF_RGB | DDPF_ALPHAPIXELS},
    {16, 0x00000000, 0x007c00, 0x03e0, 0x1f, D3DFMT_X1R5G5B5, DDPF_RGB},
    {16, 0x0000f000, 0x000f00, 0x00f0, 0x0f, D3DFMT_A4R4G4B4, DDPF_RGB | DDPF_ALPHAPIXELS},
    {8, 0x00000000, 0x0000e0, 0x001c, 0x03, D3DFMT_R3G3B2, DDPF_RGB},
    {8, 0x000000ff, 0x000000, 0x0000, 0x00, D3DFMT_A8, DDPF_ALPHA | DDPF_ALPHAPIXELS},
    {8, 0x00000000, 0x0000FF, 0x0000, 0x00, D3DFMT_L8, DDPF_LUMINANCE},
    {16, 0x0000ff00, 0x0000e0, 0x001c, 0x03, D3DFMT_A8R3G3B2, DDPF_RGB | DDPF_ALPHAPIXELS},
    {16, 0x00000000, 0x000f00, 0x00f0, 0x0f, D3DFMT_X4R4G4B4, DDPF_RGB},
    {32, 0xff000000, 0xff0000, 0xff00, 0xff, D3DFMT_A8B8G8R8, DDPF_RGB | DDPF_ALPHAPIXELS},
    {32, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_X8B8G8R8, DDPF_RGB},
    {16, 0x00000000, 0x0000ff, 0xff00, 0x00, D3DFMT_V8U8, DDPF_BUMPDUDV},
    {16, 0x0000ff00, 0x0000ff, 0x0000, 0x00, D3DFMT_A8L8, DDPF_LUMINANCE | DDPF_ALPHAPIXELS},
    {16, 0x00000000, 0xFFFF, 0x00000000, 0, D3DFMT_L16, DDPF_LUMINANCE},
  };

  ddsx::Header hdr;
  crd.read(&hdr, sizeof(hdr));
  if (!hdr.checkLabel())
  {
    printf("invalid DDSx format\n");
    return false;
  }

  Tab<char> texData;
  texData.resize(hdr.memSz);
  if (hdr.isCompressionZSTD())
  {
    ZstdLoadCB zcrd(crd, hdr.packedSz);
    zcrd.read(texData.data(), hdr.memSz);
  }
  else if (hdr.isCompressionOODLE())
  {
    OodleLoadCB zcrd(crd, hdr.packedSz, hdr.memSz);
    zcrd.read(texData.data(), hdr.memSz);
  }
  else if (hdr.isCompressionZLIB())
  {
    ZlibLoadCB zlib_crd(crd, hdr.packedSz);
    zlib_crd.read(texData.data(), hdr.memSz);
  }
  else if (hdr.isCompression7ZIP())
  {
    LzmaLoadCB lzma_crd(crd, hdr.packedSz);
    lzma_crd.read(texData.data(), hdr.memSz);
  }
  else
    crd.read(texData.data(), hdr.memSz);

  ddsx_forward_mips_inplace(hdr, texData.data(), data_size(texData));

  // dds header preparing
  DDSURFACEDESC2 targetHeader;
  memset(&targetHeader, 0, sizeof(DDSURFACEDESC2));
  targetHeader.dwSize = sizeof(DDSURFACEDESC2);
  targetHeader.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
  targetHeader.dwHeight = hdr.h;
  targetHeader.dwWidth = hdr.w;
  targetHeader.dwDepth = hdr.depth;
  targetHeader.dwMipMapCount = hdr.levels;
  targetHeader.ddsCaps.dwCaps = 0;
  targetHeader.ddsCaps.dwCaps2 = 0;

  if (hdr.flags & ddsx::Header::FLG_CUBTEX)
    targetHeader.ddsCaps.dwCaps2 |= DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES;
  else if (hdr.flags & ddsx::Header::FLG_VOLTEX)
  {
    targetHeader.dwFlags |= DDSD_DEPTH;
    targetHeader.ddsCaps.dwCaps2 |= DDSCAPS2_VOLUME;
  }
  else
    targetHeader.ddsCaps.dwCaps |= DDSCAPS_TEXTURE;

  if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER))
    targetHeader.dwMipMapCount = 1;

  DDPIXELFORMAT &pf = targetHeader.ddpfPixelFormat;
  pf.dwSize = sizeof(DDPIXELFORMAT);

  // pixel format search
  int i;
  for (i = 0; i < sizeof(bitMaskFormat) / sizeof(BitMaskFormat); i++)
    if (hdr.d3dFormat == bitMaskFormat[i].format)
      break;

  if (i < sizeof(bitMaskFormat) / sizeof(BitMaskFormat))
  {
    pf.dwFlags = bitMaskFormat[i].flags;
    pf.dwRGBBitCount = bitMaskFormat[i].bitCount;
    pf.dwRBitMask = bitMaskFormat[i].redMask;
    pf.dwGBitMask = bitMaskFormat[i].greenMask;
    pf.dwBBitMask = bitMaskFormat[i].blueMask;
    pf.dwRGBAlphaBitMask = bitMaskFormat[i].alphaMask;
  }
  else
  {
    pf.dwFlags = DDPF_FOURCC;
    pf.dwFourCC = hdr.d3dFormat;
  }


  // dds output
  uint32_t FourCC = MAKEFOURCC('D', 'D', 'S', ' ');
  cwr.write(&FourCC, sizeof(FourCC));
  cwr.write(&targetHeader, sizeof(targetHeader));
  cwr.write(texData.data(), hdr.memSz);
  return true;
}
static bool dump_ddsx(IGenLoad &crd)
{
  static const char *addr_str[16] = {
    "0", "wrap", "mirror", "clamp", "border", "mirrorOnce", "*", "*", "*", "*", "*", "*", "*", "*", "*", "*"};
  ddsx::Header hdr;
  crd.read(&hdr, sizeof(hdr));
  if (!hdr.checkLabel())
  {
    printf("invalid DDSx format\n");
    return false;
  }

  ddsx::Header hdr_e;
  memset(&hdr_e, 0, sizeof(hdr_e));
  hdr_e.label = hdr.label;
  if (memcmp(&hdr, &hdr_e, sizeof(hdr)) == 0)
  {
    printf("--- %s dump: empty DDSx\n", crd.getTargetName());
    return true;
  }

  printf("--- %s dump:\n"
         "D3D fmt 0x%08X (%c%c%c%c)  bpp=%d code=%d\n"
         "mem sz  %d,  packed sz %d,  compr=%d%%\n",
    crd.getTargetName(), hdr.d3dFormat, _DUMP4C(hdr.d3dFormat),
    hdr.bitsPerPixel ? hdr.bitsPerPixel : (hdr.getSurfaceSz(0) * 8 / hdr.w / hdr.h), hdr.dxtShift, hdr.memSz, hdr.packedSz,
    hdr.memSz ? hdr.packedSz * 100 / hdr.memSz : 0);
  if (hdr.flags & hdr.FLG_CUBTEX)
    printf("texCube %dx%d\n", hdr.w, hdr.h);
  else if (hdr.flags & hdr.FLG_VOLTEX)
    printf("tex3D   %dx%dx%d\n", hdr.w, hdr.h, hdr.depth);
  else
    printf("tex2D   %dx%d\n", hdr.w, hdr.h);
  printf("mips    %d (hqPartMips=%d)\n", hdr.levels, hdr.hqPartLevels);
  printf("quality mqMip=%d  lqMip=%d  uqMip=%d\n", hdr.mQmip, hdr.lQmip, hdr.uQmip);
  printf("addr    %s, %s\n", addr_str[hdr.flags & 0xF], addr_str[(hdr.flags >> 4) & 0xF]);
  printf("flags   ");
#define PRN_FLG(X)             \
  if (hdr.flags & hdr.FLG_##X) \
  printf(#X "  ")
  PRN_FLG(CONTIGUOUS_MIP);
  PRN_FLG(NONPACKED);
  PRN_FLG(HASBORDER);
  PRN_FLG(CUBTEX);
  PRN_FLG(VOLTEX);
  PRN_FLG(ARRTEX);
  PRN_FLG(GENMIP_BOX);
  PRN_FLG(GENMIP_KAIZER);
  PRN_FLG(GAMMA_EQ_1);
  PRN_FLG(HOLD_SYSMEM_COPY);
  PRN_FLG(NEED_PAIRED_BASETEX);
  PRN_FLG(REV_MIP_ORDER);
  PRN_FLG(HQ_PART);
  PRN_FLG(MOBILE_TEXFMT);
#undef PRN_FLG
  printf("\n");
#define PRN_COMPR(X)                        \
  if (hdr.compressionType() == hdr.FLG_##X) \
  printf("compr   " #X "\n")
  PRN_COMPR(7ZIP);
  PRN_COMPR(ZLIB);
  PRN_COMPR(ZSTD);
  PRN_COMPR(OODLE);
#undef PRN_COMPR
  printf("\nmip data chain%s:\n",
    (hdr.flags & hdr.FLG_CUBTEX) && !(hdr.flags & hdr.FLG_REV_MIP_ORDER) ? " (6 faces consisted of full mip chain each)" : "");

  for (int i = 0; i < hdr.levels; i++)
  {
    int level = (hdr.flags & hdr.FLG_REV_MIP_ORDER) ? hdr.levels - 1 - i : i;
    int d = max(hdr.depth >> level, 1);
    printf("  mip %2d: ", level);
    if (hdr.flags & hdr.FLG_VOLTEX)
      printf("%4dx%-4dx%-3d   %4d (pitch) * %4d (lines) * %3d (slices) = %8d\n", max(hdr.w >> level, 1), max(hdr.h >> level, 1), d,
        hdr.getSurfacePitch(level), hdr.getSurfaceScanlines(level), d, hdr.getSurfaceSz(level) * d);
    else if (hdr.flags & hdr.FLG_CUBTEX)
      printf("%4dx%-4d   %4d (pitch) * %4d (lines) * 6 (faces) = %8d\n", max(hdr.w >> level, 1), max(hdr.h >> level, 1),
        hdr.getSurfacePitch(level), hdr.getSurfaceScanlines(level), hdr.getSurfaceSz(level) * 6);
    else
      printf("%4dx%-4d   %4d (pitch) * %4d (lines) = %8d\n", max(hdr.w >> level, 1), max(hdr.h >> level, 1),
        hdr.getSurfacePitch(level), hdr.getSurfaceScanlines(level), hdr.getSurfaceSz(level));
    if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER))
      break;
  }
  printf("\n");

  return true;
}

#include <libTools/dtx/astcenc.h>
ASTCENC_DECLARE_STATIC_DATA();
static bool decode_ddsx_astc(IGenLoad &crd, const char *base_nm)
{
  ddsx::Header hdr;
  crd.read(&hdr, sizeof(hdr));
  if (!hdr.checkLabel())
  {
    printf("invalid DDSx format\n");
    return false;
  }

  ddsx::Header hdr_e;
  memset(&hdr_e, 0, sizeof(hdr_e));
  hdr_e.label = hdr.label;
  if (memcmp(&hdr, &hdr_e, sizeof(hdr)) == 0)
  {
    printf("--- %s dump: empty DDSx\n", crd.getTargetName());
    return true;
  }

  printf("--- %s dump:\n"
         "D3D fmt 0x%08X (%c%c%c%c)  bpp=%d code=%d\n"
         "mem sz  %d,  packed sz %d,  compr=%d%%\n",
    crd.getTargetName(), hdr.d3dFormat, _DUMP4C(hdr.d3dFormat),
    hdr.bitsPerPixel ? hdr.bitsPerPixel : (hdr.getSurfaceSz(0) * 8 / hdr.w / hdr.h), hdr.dxtShift, hdr.memSz, hdr.packedSz,
    hdr.memSz ? hdr.packedSz * 100 / hdr.memSz : 0);
  if (hdr.flags & hdr.FLG_CUBTEX)
    printf("texCube %dx%d\n", hdr.w, hdr.h);
  else if (hdr.flags & hdr.FLG_VOLTEX)
    printf("tex3D   %dx%dx%d\n", hdr.w, hdr.h, hdr.depth);
  else
    printf("tex2D   %dx%d\n", hdr.w, hdr.h);
  printf("mips    %d (hqPartMips=%d)\n", hdr.levels, hdr.hqPartLevels);
  printf("quality mqMip=%d  lqMip=%d  uqMip=%d\n", hdr.mQmip, hdr.lQmip, hdr.uQmip);
  printf("flags   ");
#define PRN_FLG(X)             \
  if (hdr.flags & hdr.FLG_##X) \
  printf(#X "  ")
  PRN_FLG(CONTIGUOUS_MIP);
  PRN_FLG(NONPACKED);
  PRN_FLG(HASBORDER);
  PRN_FLG(CUBTEX);
  PRN_FLG(VOLTEX);
  PRN_FLG(ARRTEX);
  PRN_FLG(GENMIP_BOX);
  PRN_FLG(GENMIP_KAIZER);
  PRN_FLG(GAMMA_EQ_1);
  PRN_FLG(HOLD_SYSMEM_COPY);
  PRN_FLG(NEED_PAIRED_BASETEX);
  PRN_FLG(REV_MIP_ORDER);
  PRN_FLG(HQ_PART);
  PRN_FLG(MOBILE_TEXFMT);
#undef PRN_FLG
  printf("\n");
#define PRN_COMPR(X)                        \
  if (hdr.compressionType() == hdr.FLG_##X) \
  printf("compr   " #X "\n")
  PRN_COMPR(7ZIP);
  PRN_COMPR(ZLIB);
  PRN_COMPR(ZSTD);
  PRN_COMPR(OODLE);
#undef PRN_COMPR
  if (hdr.d3dFormat != _MAKE4C('AST4') && hdr.d3dFormat != _MAKE4C('AST8') && hdr.d3dFormat != _MAKE4C('ASTC'))
  {
    printf("ERR: DDSx is not ASTC format, decoding skipped\n");
    return false;
  }
  ASTCEncoderHelperContext::setupAstcEncExePathname();
  ASTCEncoderHelperContext astc_context;
  if (!astc_context.prepareTmpFilenames("decode"))
  {
    printf("ERR: can't arrange tmp files for astcenc\n");
    return false;
  }

  alignas(16) union
  {
    char zstd[sizeof(ZstdLoadCB)];
    char oodle[sizeof(OodleLoadCB)];
    char zlib[sizeof(BufferedZlibLoadCB)];
    char lzma[sizeof(BufferedLzmaLoadCB)];
  } zcrd_stor;
  IGenLoad *zcrd = &crd;
  if (hdr.isCompressionZSTD())
    zcrd = new (zcrd_stor.zstd, _NEW_INPLACE) ZstdLoadCB(crd, hdr.packedSz);
  else if (hdr.isCompressionOODLE())
    zcrd = new (zcrd_stor.oodle, _NEW_INPLACE) OodleLoadCB(crd, hdr.packedSz, hdr.memSz);
  else if (hdr.isCompressionZLIB())
    zcrd = new (zcrd_stor.zlib, _NEW_INPLACE) ZlibLoadCB(crd, hdr.packedSz);
  else if (hdr.isCompression7ZIP())
    zcrd = new (zcrd_stor.lzma, _NEW_INPLACE) LzmaLoadCB(crd, hdr.packedSz);

  printf("\nmip data chain%s:\n",
    (hdr.flags & hdr.FLG_CUBTEX) && !(hdr.flags & hdr.FLG_REV_MIP_ORDER) ? " (6 faces consisted of full mip chain each)" : "");

  unsigned blk_w = 0, blk_h = 0, blk_d = 0;
  switch (hdr.d3dFormat)
  {
    case _MAKE4C('AST4'): blk_w = 4, blk_h = 4, blk_d = 1; break;
    case _MAKE4C('AST8'): blk_w = 8, blk_h = 8, blk_d = 1; break;
    case _MAKE4C('ASTC'): blk_w = 12, blk_h = 12, blk_d = 1; break;
    default: break;
  }
  Tab<char> pdata;
  for (int i = 0; i < hdr.levels; i++)
  {
    int level = (hdr.flags & hdr.FLG_REV_MIP_ORDER) ? hdr.levels - 1 - i : i;
    int mipw = max(hdr.w >> level, 1), miph = max(hdr.h >> level, 1), mipd = max(hdr.depth >> level, 1);
    unsigned slices = (hdr.flags & hdr.FLG_CUBTEX) ? 6 : ((hdr.flags & hdr.FLG_VOLTEX) ? mipd : 1);
    printf("  mip %2d: ", level);
    if (hdr.flags & hdr.FLG_VOLTEX)
      printf("%4dx%-4dx%-3d   %4d (pitch) * %4d (lines) * %3d (slices) = %8d%s", mipw, miph, mipd, hdr.getSurfacePitch(level),
        hdr.getSurfaceScanlines(level), mipd, hdr.getSurfaceSz(level) * mipd, slices > 1 ? "\n" : "");
    else if (hdr.flags & hdr.FLG_CUBTEX)
      printf("%4dx%-4d   %4d (pitch) * %4d (lines) * 6 (faces) = %8d%s", mipw, miph, hdr.getSurfacePitch(level),
        hdr.getSurfaceScanlines(level), hdr.getSurfaceSz(level) * 6, slices > 1 ? "\n" : "");
    else
      printf("%4dx%-4d   %4d (pitch) * %4d (lines) = %8d%s", mipw, miph, hdr.getSurfacePitch(level), hdr.getSurfaceScanlines(level),
        hdr.getSurfaceSz(level), slices > 1 ? "\n" : "");

    for (unsigned s = 0; s < slices; s++)
    {
      String fn(0, "%s_mip%d", base_nm, level);
      if (slices > 1)
        fn.aprintf(0, "_s%d", s);
      fn += ".tga";
      pdata.resize(hdr.getSurfaceSz(level));
      zcrd->read(pdata.data(), pdata.size());
      if (astc_context.decodeOneSurface(fn, pdata, mipw, miph, mipd, blk_w, blk_h, blk_d))
        printf("%s%s\n", slices > 1 ? "         " : "  -> ", fn.c_str());
    }
  }
  printf("\n");
  if (zcrd != &crd)
  {
    zcrd->ceaseReading();
    zcrd->~IGenLoad();
  }

  return true;
}
static bool check_ddsx(IGenLoad &crd, int compr_flg)
{
  ddsx::Header hdr;
  crd.read(&hdr, sizeof(hdr));
  if (!hdr.checkLabel())
    return false;

  ddsx::Header hdr_e;
  memset(&hdr_e, 0, sizeof(hdr_e));
  hdr_e.label = hdr.label;
  if (memcmp(&hdr, &hdr_e, sizeof(hdr)) == 0)
    return false;
  if (hdr.compressionType() == compr_flg)
  {
    printf("%s\n", crd.getTargetName());
    return true;
  }
  return false;
}

static bool skip_exp_cache_header(IGenLoad &crd)
{
  if (crd.readInt() != _MAKE4C('fC1'))
    return false;

  const int cacheFileVersion = crd.readInt();
  if (cacheFileVersion != 2 && cacheFileVersion != 3)
    return false;

  // read target file data hash
  crd.seekrel(16 + 4 + 4);

  // read file data hashes
  int num = crd.readInt();
  for (int i = num; i > 0; i--)
    crd.skipString();
  crd.seekrel(num * (16 + 4 + 4));

  // read asset exported data references
  num = crd.readInt();
  for (int i = num; i > 0; i--)
    crd.skipString();
  crd.seekrel(num * (4 + 4));

  // read versions of exporters
  crd.beginBlock();
  crd.endBlock();

  if (cacheFileVersion >= 3)
    for (int i = crd.readInt(); i > 0; --i)
      crd.skipString();

  if (crd.readInt() != _MAKE4C('.end'))
    return false;
  crd.beginBlock();
  return true;
}

static bool skip_ta_bin_header(IGenLoad &crd)
{
  if (crd.readInt() != _MAKE4C('TA.'))
    return false;
  crd.seekrel(crd.readInt());
  return true;
}

static bool trail_stricmp(const char *str, const char *trail_str)
{
  int s_len = strlen(str);
  int t_len = strlen(trail_str);
  if (s_len < t_len)
    return false;
  return stricmp(str + (s_len - t_len), trail_str) == 0;
}

static void print_header()
{
  printf("DDSx -> DDS back converter v1.3 (PC only)\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}

int DagorWinMain(bool debugmode)
{
  signal(SIGINT, ctrl_break_handler);
  if (dgs_argc < 2)
  {
    print_header();
    printf("usage: ddsx2dds.exe <src_ddsx> [<out_dds> | -dump | -check:<ZSTD|ZLIB|7ZIP|OODLE>]\n");
    return 1;
  }

  String out_fn;
  if (dgs_argc > 2)
    out_fn = dgs_argv[2];

  int compr_flag = 1; // not set
  if (strncmp(out_fn, "-check:", 7) == 0)
  {
    if (strcmp(&out_fn[7], "ZSTD") == 0)
      compr_flag = ddsx::Header::FLG_ZSTD;
    else if (strcmp(&out_fn[7], "7ZIP") == 0)
      compr_flag = ddsx::Header::FLG_7ZIP;
    else if (strcmp(&out_fn[7], "ZLIB") == 0)
      compr_flag = ddsx::Header::FLG_ZLIB;
    else if (strcmp(&out_fn[7], "OODLE") == 0)
      compr_flag = ddsx::Header::FLG_OODLE;
    else if (strcmp(&out_fn[7], "NONE") == 0)
      compr_flag = 0;
    else
    {
      print_header();
      printf("ERR: bad option <%s>\n", out_fn.str());
      return -1;
    }
  }

  FullFileLoadCB crd(dgs_argv[1]);
  if (!crd.fileHandle)
  {
    if (compr_flag != 1)
      return -1;
    print_header();
    printf("ERR: cannot read <%s>\n", dgs_argv[1]);
    return -1;
  }
  if (trail_stricmp(dgs_argv[1], ".c4.bin") || trail_stricmp(dgs_argv[1], ".ct.bin"))
  {
    // skip fC1 header
    if (!skip_exp_cache_header(crd))
    {
      if (compr_flag != 1)
        return -1;
      print_header();
      printf("ERR: cannot read <%s> C4 header\n", dgs_argv[1]);
      return -1;
    }


    if (out_fn.empty())
    {
      out_fn = dgs_argv[1];
      erase_items(out_fn, strlen(out_fn) - 7, 7);
      const char *ext = dd_get_fname_ext(out_fn);
      if (ext)
        erase_items(out_fn, ext - (char *)out_fn, strlen(ext));
      out_fn += ".dds";
    }
  }

  if (trail_stricmp(dgs_argv[1], ".ta.bin"))
  {
    // skip TA. header
    if (!skip_ta_bin_header(crd))
    {
      if (compr_flag != 1)
        return -1;
      print_header();
      printf("ERR: cannot read <%s> C4 header\n", dgs_argv[1]);
      return -1;
    }

    if (out_fn.empty())
    {
      out_fn = dgs_argv[1];
      erase_items(out_fn, strlen(out_fn) - 7, 7);
      const char *ext = dd_get_fname_ext(out_fn);
      if (ext)
        erase_items(out_fn, ext - (char *)out_fn, strlen(ext));
      out_fn += ".dds";
    }
  }

  if (compr_flag != 1)
    return check_ddsx(crd, compr_flag) ? 0 : -1;

  if (out_fn == "-dump")
    return dump_ddsx(crd) ? 0 : -1;
  if (out_fn == "-decodeASTC")
    return decode_ddsx_astc(crd, dgs_argv[1]) ? 0 : -1;
  if (out_fn.empty())
  {
    out_fn = dgs_argv[1];
    const char *ext = dd_get_fname_ext(out_fn);
    if (ext)
      erase_items(out_fn, ext - (char *)out_fn, strlen(ext));
    out_fn += ".dds";
  }

  FullFileSaveCB cwr(out_fn);
  if (!cwr.fileHandle)
  {
    print_header();
    printf("ERR: cannot write <%s>\n", out_fn.str());
    return -1;
  }

  return ddsx_to_dds(crd, cwr) ? 0 : -1;
}

#define __UNLIMITED_BASE_PATH 1
#include <startup/dag_mainCon.inc.cpp>
