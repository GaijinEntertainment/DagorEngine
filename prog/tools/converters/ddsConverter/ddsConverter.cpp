// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <signal.h>
#include <startup/dag_startupTex.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_smallTab.h>
#include <scene/dag_loadLevelVer.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/dtx/dtx.h>
#include <libTools/util/strUtil.h>
#include <image/dag_texPixel.h>
#include <image/dag_loadImage.h>
#include <debug/dag_logSys.h>
#include <stdio.h>


void __cdecl ctrl_break_handler(int) { quit_game(0); }


//=======================================================================================
//  options converter
//=======================================================================================
class CmdLineOptions : protected ddstexture::Converter
{
public:
  //=========================================================================================
  CmdLineOptions(int argc, char **argv) : overwrite(true)
  {
    pow2 = 1;
    //----------------------------------------------------------------------------------------------
    srcName = argv[1];
    String loc;
    ::split_path(srcName, loc, dstName);
    srcName.toLower();
    if (dstName.find('@') && dstName.suffix(".dds"))
    {
      dstName = "";
      return;
    }
    dstName = ::get_file_name_wo_ext(dstName);
    ::make_ident_like_name(dstName);

    //---------------------------------------------------------------------------------------------
    //  check alpha
    if (sourceWithAlpha())
    {
      printf("Source image with alpha. Default format fmtDXT5\n");
      format = fmtDXT5;
    }

    //----------------------------------------------------------------------------------------------
    for (int i = 2; i < argc; i++)
    {
      if (argv[i][0] != '-')
      {
        printf("invalid param [%s]\n", argv[i]);
        dstName = "";
        return;
      }
      if (strcmp(argv[i], "-quiet") == 0)
        continue;

      String param(argv[i] + 1);
      const char *separator = param.find(':');
      if (!separator || !processParam(param, String(separator + 1)))
      {
        printf("invalid param [%s]\n", argv[i]);
        dstName = "";
        return;
      }
    }
    if (suffix != "")
      dstName += "@" + suffix + ".dds";
    else
      dstName += ".dds";
  }

  //=========================================================================================
  bool processParam(const String &param, const String &value)
  {
    if (param.prefix("suffix"))
      suffix = value;
    else if (param.prefix("type"))
    {
      if (value == "type2D")
        type = type2D;
      else if (value == "typeCube")
        type = typeCube;
      else if (value == "typeVolume")
        type = typeVolume;
      else if (value == "typeDefault")
        type = typeDefault;
      else
        return false;
    }
    else if (param.prefix("format"))
    {
      if (value == "fmtCurrent")
        format = fmtCurrent;
      else if (value == "fmtARGB")
        format = fmtARGB;
      else if (value == "fmtRGB")
        format = fmtRGB;
      else if (value == "fmtDXT1")
        format = fmtDXT1;
      else if (value == "fmtDXT1a")
        format = fmtDXT1a;
      else if (value == "fmtDXT3")
        format = fmtDXT3;
      else if (value == "fmtDXT5")
        format = fmtDXT5;
      else if (value == "fmtASTC4")
        format = fmtASTC4;
      else if (value == "fmtASTC8")
        format = fmtASTC8;
      else if (value == "fmt4444")
        format = fmtRGBA4444;
      else if (value == "fmt5551")
        format = fmtRGBA5551;
      else if (value == "fmt565")
        format = fmtRGB565;
      else if (value == "fmtR8")
        format = fmtL8;
      else if (value == "fmtA8R8")
        format = fmtA8L8;
      else
        return false;
    }
    else if (param.prefix("mipmapType"))
    {
      if (value == "mipmapNone")
        mipmapType = mipmapNone;
      else if (value == "mipmapUseExisting")
        mipmapType = mipmapUseExisting;
      else if (value == "mipmapGenerate")
        mipmapType = mipmapGenerate;
      else
        return false;
    }
    else if (param.prefix("mipmapFilter"))
    {
      if (value == "filterPoint")
        mipmapFilter = filterPoint;
      else if (value == "filterBox")
        mipmapFilter = filterBox;
      else if (value == "filterTriangle")
        mipmapFilter = filterTriangle;
      else if (value == "filterQuadratic")
        mipmapFilter = filterQuadratic;
      else if (value == "filterCubic")
        mipmapFilter = filterCubic;
      else if (value == "filterCatrom")
        mipmapFilter = filterCatrom;
      else if (value == "filterMitchell")
        mipmapFilter = filterMitchell;
      else if (value == "filterGaussian")
        mipmapFilter = filterGaussian;
      else if (value == "filterSinc")
        mipmapFilter = filterSinc;
      else if (value == "filterBessel")
        mipmapFilter = filterBessel;
      else if (value == "filterHanning")
        mipmapFilter = filterHanning;
      else if (value == "filterHamming")
        mipmapFilter = filterHamming;
      else if (value == "filterBlackman")
        mipmapFilter = filterBlackman;
      else if (value == "filterKaiser")
        mipmapFilter = filterKaiser;
      else
        return false;
    }
    else if (param.prefix("mipmapFilterWidth"))
      mipmapFilterWidth = atof(value);
    else if (param.prefix("mipmapFilterBlur"))
      mipmapFilterBlur = atof(value);
    else if (param.prefix("mipmapFilterGamma"))
      mipmapFilterGamma = atof(value);
    else if (param.prefix("overwrite"))
      overwrite = (value == "yes");
    else if (param.prefix("depth"))
      volTexDepth = atoi(value);
    else if (param.prefix("pow2"))
    {
      if (value == "none")
        pow2 = 2;
      else if (value == "smallest")
        pow2 = -1;
      else if (value == "nearest")
        pow2 = 0;
      else if (value == "biggest")
        pow2 = 1;
      else
        return false;
    }
    else
      return false;

    return true;
  }


  //=========================================================================================
  int convert()
  {
    if (dstName == "" || dstName == srcName)
      return -1;

    if (!overwrite && ::dd_file_exist(dstName))
      return -1;

    if (!volTexDepth && type == typeVolume)
    {
      printf("depth (required for typeVolume) is not specifed or invalid: %d\n", volTexDepth);
      return -1;
    }

    printf("Convert to file [%s]\n", (char *)dstName);

    if (srcName.suffix(".dds") || srcName.suffix(".dtx"))
    {
      FILE *src = ::fopen(srcName, "rb");
      if (!src)
        return -1;

      FILE *dst = ::fopen(dstName, "wb");
      if (!dst)
        return -1;

      const int buffer_size = 0x10000;
      char buffer[buffer_size];
      while (int read = fread(buffer, 1, buffer_size, src))
        if (read < 0 || fwrite(buffer, 1, read, dst) == 0)
        {
          printf("Can't copy from [%s] to [%s]\n", (char *)srcName, (char *)dstName);
          fclose(src);
          fclose(dst);
          return -1;
        }

      fclose(src);
      fclose(dst);

      return 0;
    }

    if (format == fmtASTC4 || format == fmtASTC8 || format == fmtRGBA4444 || format == fmtRGBA5551 || format == fmtRGB565 ||
        format == fmtL8 || format == fmtA8L8)
    {
      bool alpha;
      TexImage32 *img = load_image(srcName, tmpmem, &alpha);
      if (!img)
      {
        printf("Can't read image from [%s]\n", srcName.str());
        return false;
      }
      FullFileSaveCB cwr(dstName);
      if (!cwr.fileHandle)
      {
        printf("Can't open output file [%s]\n", dstName.str());
        return false;
      }
      if (!Converter::convertImage(cwr, img->getPixels(), img->w, img->h, img->w * 4, false, pow2))
      {
        printf("Can't convert image to fmt=%d\n", format);
        return false;
      }
      return true;
    }

    return Converter::convert(srcName, dstName, overwrite, false, pow2);
  }

  //=========================================================================================
  bool sourceWithAlpha()
  {
    if (!srcName.suffix(".tga"))
      return false;

    FullFileLoadCB crd(srcName);
    if (!crd.fileHandle)
      return false;

#pragma pack(push, 1)
    struct TgaHeader
    {
      uint8_t idlen, cmtype, pack;
      uint16_t cmorg, cmlen;
      uint8_t cmes;
      short xo, yo, w, h;
      char bits, desc;
    } hdr;
#pragma pack(pop)

    try
    {
      crd.read(&hdr, sizeof(hdr));
      ;
      if (hdr.idlen)
        crd.seekrel(hdr.idlen);
      if (hdr.cmlen)
        crd.seekrel(hdr.cmlen * ((hdr.cmes + 7) / 8));
    }
    catch (IGenLoad::LoadException &e)
    {
      return false;
    }

    return hdr.bits == 32;
  }

  static const char *getLastError() { return ddstexture::Converter::getLastError(); }

private:
  String srcName, dstName, suffix;
  bool overwrite;
  int pow2;
};


//=========================================================================================
int DagorWinMain(bool debugmode)
{
  debug_enable_timestamps(false);
  ::register_jpeg_tex_load_factory();
  ::register_tga_tex_load_factory();
  ::register_psd_tex_load_factory();
  ::register_png_tex_load_factory();
  ::register_avif_tex_load_factory();

  printf("DDS converter\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
  close_debug_files();

  if (dgs_argc < 2)
  {
    printf("usage: ddsConverter-dev.exe source_filename <commands>\n"
           "where command can be:\n"
           "\t-suffix:A[TEXADDR_U {w|m|c|b}, TEXADDR_V{w|m|c|b}]_Q[HQ_mip{0-f}, MQ_mip{0-f}, LQ_mip{0-f}]\n"
           "\t\t(default Aww_Q012)\n\n"
           "\t-type:{ type2D | typeCube | typeVolume } (default type2D)\n"
           "\t-format:{ fmtCurrent | fmtARGB | fmtRGB | fmtDXT1 | fmtDXT1a | fmtDXT3 | fmtDXT5 | }\n"
           "\t        { fmtASTC4 | fmtASTC8 | fmt4444 | fmt5551 | fmt565 | fmtR8 | fmtA8R8 }\n"
           "\t\t(default fmtDXT1. If source tga-file with alpha, default - fmtDXT5)\n\n"
           "\t-depth:value   (default 0) abs(depth) is texture depth for typeVolume\n"
           "\t\t\tsign specifies stripe direction in source image ('+'=vertical, '-'=horizontal\n\n"
           "\t-mipmapType:{ mipmapNone | mipmapUseExisting | mipmapGenerate } (default mipmapUseExisting)\n"
           "\t-mipmapFilter:{ filterPoint | filterBox | filterTriangle | filterQuadratic | filterCubic |\n"
           "\t\t\tfilterCatrom | filterMitchell | filterGaussian | filterSinc | filterBessel |\n"
           "\t\t\tfilterHanning | filterHamming | filterBlackman | filterKaiser } (default filterKaiser)\n"
           "\t-mipmapFilterWidth:value (default 0.0)\n"
           "\t-mipmapFilterBlur:value  (default 1.0)\n"
           "\t-mipmapFilterGamma:value (default 2.2)\n\n"
           "\t-pow2:{none|smallest|nearest|biggest} (default biggest)\n"
           "\t-overwrite:{yes|no} (default yes)\n");
    return -1;
  }

  CmdLineOptions opt(dgs_argc, dgs_argv);
  int result = opt.convert();
  if (result > 0)
    printf("done\n");
  else
    printf("failed\n");

  return result;
}
