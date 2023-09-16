#define __UNLIMITED_BASE_PATH 1
#include <startup/dag_mainCon.inc.cpp>

#include <image/dag_texPixel.h>
#include <image/dag_tga.h>
#include <math/dag_color.h>
#include <util/dag_bitArray.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/dagUuid.h>

typedef struct
{
} rgbe_header_info;

int RGBE_WriteHeader(file_ptr_t fp, int width, int height, rgbe_header_info *info);
int RGBE_WritePixels_RLE(file_ptr_t fp, float *data, int scanline_width, int num_scanlines);

void showUsage()
{
  printf("\nUsage: ltout2img.exe <file.LToutput.dat> [<out.tga>|<out.pfm>|<out.hdr>]\n");
  printf("  when output name is ommitted, it is constructed as file.tga\n"
         "  output filename extension determines format:\n"
         "   .tga - Targa, 32-bit RGBA\n"
         "   .pfm - Portable float map, 3x32-bit float RGB\n"
         "   .hdr - Radiant 32-bit RGBE\n");
}

bool convert_ltoutput(const char *lto_fname, const char *img_fname)
{
  String img_fn;
  if (!img_fname)
  {
    img_fn = lto_fname;
    ::remove_trailing_string(img_fn, ".ltoutput.dat");
    img_fn += ".tga";
    img_fname = img_fn;
  }

  FullFileLoadCB crd(lto_fname);
  Color3 *cpix;
  Bitarray bmDone;
  Color3 c, cmax;
  int w, h, pass, label, vltnum;
  bool success = false;

  if (!crd.fileHandle)
  {
    printf("ERROR: cannot read %s", lto_fname);
    return false;
  }

  label = crd.readInt();
  if (label != _MAKE4C('LTo3'))
  {
    printf("ERROR: wrong ltinput format %c%c%c%c\n", _DUMP4C(label));
    return false;
  }
  crd.seekrel(sizeof(DagUUID));
  pass = crd.readInt();
  w = crd.readInt();
  h = crd.readInt();
  vltnum = crd.readInt();

  printf("LTo3 is made after pass %d, size %dx%d\n", pass, w, h);
  if (!w)
    w = 1024;
  int eff_h = h + (vltnum + w - 1) / w;
  cpix = new Color3[w * eff_h];
  memset(cpix + w * h + vltnum, 0, (w * eff_h - w * h - vltnum) * sizeof(*cpix));

  cmax = Color3(0, 0, 0);
  crd.read(cpix, w * h * sizeof(Color3));

  crd.read(cpix + w * h, vltnum * sizeof(Color3));
  if (crd.beginTaggedBlock() == _MAKE4C('Lbit'))
  {
    Bitarray bmInternal;
    bmDone.resize(w * h);
    bmInternal.resize(w * h);

    crd.read((void *)bmDone.getPtr(), data_size(bmDone));
    crd.read((void *)bmInternal.getPtr(), data_size(bmInternal));
    crd.endBlock();
  }

  for (int i = 0; i < w * h + vltnum; i++)
  {
    inplace_max(cmax.r, cpix[i].r);
    inplace_max(cmax.g, cpix[i].g);
    inplace_max(cmax.b, cpix[i].b);
  }
  printf("max color: %.4f, %.4f, %.4f  - ", cmax.r, cmax.g, cmax.b);

  if (::trail_stricmp(img_fname, ".tga"))
  {
    printf("Saving to Targa 32-bit RGB\n");

    TexPixel32 *pix;
    pix = new TexPixel32[w * eff_h];
    memset(pix, 0, w * eff_h * sizeof(TexPixel32));
    for (int i = 0; i < w * h + vltnum; i++)
    {
      c = cpix[i];
      c.clamp01();
      pix[i].r = c.r * 255;
      pix[i].g = c.g * 255;
      pix[i].b = c.b * 255;
      pix[i].a = i < w * h ? (bmDone[i] ? 255 : 0) : 255;
    }
    success = save_tga32(img_fname, pix, w, eff_h, w * 4);
    delete pix;
  }
  else if (::trail_stricmp(img_fname, ".pfm"))
  {
    printf("Saving to Portable Float Map 3x32-bit float RGB\n");
    FullFileSaveCB cwr(img_fname);
    if (cwr.fileHandle)
    {
      String hdr(128, "PF\n%d %d\n-1.0\n", w, eff_h);
      cwr.write((char *)hdr, hdr.length());
      cwr.write(cpix, (w * eff_h) * sizeof(Color3));
      success = true;
    }
  }
  else if (::trail_stricmp(img_fname, ".hdr"))
  {
    printf("Saving to Radiant 32-bit RGBE\n");
    file_ptr_t fp = df_open(img_fname, DF_CREATE | DF_WRITE);
    if (fp)
    {
      if (RGBE_WriteHeader(fp, w, eff_h, NULL) == 0)
        if (RGBE_WritePixels_RLE(fp, (float *)cpix, w, eff_h) == 0)
          success = true;
      df_close(fp);
    }
  }

  delete cpix;

  if (!success)
  {
    printf("ERROR: cannot write %s\n", img_fname);
    return false;
  }
  return true;
}

int DagorWinMain(bool debugmode)
{
  printf("LToutput->image Conversion Tool v1.1\n"
         "Copyright (C) Gaijin Games KFT, 2023\n");

  // get options
  if (__argc < 2)
  {
    showUsage();
    return 0;
  }

  if (stricmp(__argv[1], "-h") == 0 || stricmp(__argv[1], "-H") == 0 || stricmp(__argv[1], "/?") == 0)
  {
    showUsage();
    return 0;
  }

  convert_ltoutput(__argv[1], __argc > 2 ? __argv[2] : NULL);
  return 0;
}
