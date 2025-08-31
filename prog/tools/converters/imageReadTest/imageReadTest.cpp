// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <signal.h>
#include <startup/dag_startupTex.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_perfTimer.h>
#include <ioSys/dag_ioUtils.h>
#include <image/dag_texPixel.h>
#include <image/dag_loadImage.h>
#include <image/dag_tga.h>
#include <image/dag_dds.h>
#include <debug/dag_logSys.h>
#include <osApiWrappers/dag_direct.h>
#include <stdio.h>

int DagorWinMain(bool debugmode)
{
  debug_enable_timestamps(false);
  ::register_jpeg_tex_load_factory();
  ::register_tga_tex_load_factory();
  ::register_psd_tex_load_factory();
  ::register_png_tex_load_factory();
  ::register_avif_tex_load_factory();
  ::register_tiff_tex_load_factory();
  ::register_svg_tex_load_factory();

  close_debug_files();

  if (dgs_argc < 2 || dgs_argc > 3)
  {
    printf("imageReadTest tool\n"
           "Copyright (C) Gaijin Games KFT, 2025\n\n");
    printf("usage: imageReadTest-dev.exe source_image_filename [dest_tga_filename]\n");
    return -1;
  }

  bool dds_input = false;
  if (const char *fn_ext = dd_get_fname_ext(dgs_argv[1]))
    dds_input = strcmp(fn_ext, ".dds") == 0;

  if (dgs_argc == 2 && !dds_input)
  {
    int w = 0, h = 0;
    bool may_have_alpha = false;
    auto reft = profile_ref_ticks();
    if (read_image_dimensions(dgs_argv[1], w, h, may_have_alpha))
      printf("%4dx%-4d%s (read info for %.2f msec from %s)\n", //
        w, h, may_have_alpha ? " A" : "", profile_time_usec(reft) / 1e3f, dgs_argv[1]);
    else
    {
      printf("ERROR: cannot read dimensions from %s\n", dgs_argv[1]);
      return 1;
    }
  }
  else if (dgs_argc == 3 && !dds_input)
  {
    auto reft = profile_ref_ticks();
    bool may_have_alpha = false;
    if (TexImage32 *im = load_image(dgs_argv[1], tmpmem, &may_have_alpha))
    {
      printf("%s loaded for %.2f msec:  %4dx%-4d%s\n", //
        dgs_argv[1], profile_time_usec(reft) / 1e3f, im->w, im->h, may_have_alpha ? " A" : "");
      if (strcmp(dgs_argv[2], "nul") != 0)
      {
        if (save_tga32(dgs_argv[2], im))
          printf("-> written loaded image as TGA to %s\n", dgs_argv[2]);
        else
          printf("ERROR: cannot write TGA image to %s\n", dgs_argv[2]);
      }
      memfree(im, tmpmem);
    }
    else
    {
      printf("ERROR: cannot load image from %s\n", dgs_argv[1]);
      return 1;
    }
  }
  else if (dgs_argc == 2 && dds_input)
  {
    ImageInfoDDS dds_info;
    auto reft = profile_ref_ticks();
    if (read_dds_info(dgs_argv[1], dds_info))
      printf("%4dx%-4dx%d,L%d %s fmt=0x%x (read info for %.2f msec from %s)\n", //
        dds_info.width, dds_info.height, dds_info.depth, dds_info.nlevels,
        dds_info.cube_map ? "CUBE" : (dds_info.volume_tex ? "VOL" : "2D"), dds_info.d3dFormat, profile_time_usec(reft) / 1e3f,
        dgs_argv[1]);
    else
    {
      printf("ERROR: cannot read dimensions from %s\n", dgs_argv[1]);
      return 1;
    }
  }
  else
    printf("ERROR: cannot load image from %s\n", dgs_argv[1]);
  return 0;
}

#define __UNLIMITED_BASE_PATH 1
#define __DEBUG_FILEPATH      "*"
#include <startup/dag_mainCon.inc.cpp>

size_t dagormem_max_crt_pool_sz = 512 << 20;
