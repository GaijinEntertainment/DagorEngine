// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <image/dag_texPixel.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <generic/dag_tabUtils.h>
#include <image/dag_tga.h>
#include <stdio.h>
#include <stdlib.h>


static void error(const char *er)
{
  printf("%s\n", er);
  exit(0);
}

struct Coords
{
  int left;
  int top;
  int width;
  int height;
};

struct ImageDesc
{


  ImageDesc() : buffer(tmpmem), stride(0) {}

  ~ImageDesc() {}

  inline void init(const TexImage32 *img)
  {
    buffer = make_span_const((char *)img, img->w * img->h * sizeof(TexPixel32) + sizeof(TexImage32));
    stride = img->w * sizeof(TexPixel32);
  }

  inline void init(int w, int h)
  {
    G_ASSERT(w > 0);
    G_ASSERT(h > 0);
    buffer.resize(w * h * sizeof(TexPixel32) + sizeof(TexImage32));
    stride = w * sizeof(TexPixel32);
    image().w = w;
    image().h = h;
  }

  inline TexImage32 &image()
  {
    G_ASSERT(buffer.size() > 0);
    return *((TexImage32 *)&buffer[0]);
  };
  inline const TexImage32 &image() const
  {
    G_ASSERT(buffer.size() > 0);
    return *((TexImage32 *)&buffer[0]);
  };

  inline const TexPixel32 *line(int y) const
  {
    G_ASSERT(buffer.size() > 0);
    return (TexPixel32 *)&buffer[stride * y + sizeof(TexImage32)];
  };

  inline TexPixel32 *line(int y)
  {
    G_ASSERT(buffer.size() > 0);
    return (TexPixel32 *)&buffer[stride * y + sizeof(TexImage32)];
  };

  inline int getStride() const { return stride; }

private:
  Tab<char> buffer;
  int stride;
};

static Tab<String> tgaFiles(tmpmem_ptr());

static bool outputTex(const char *tga_tex_name, const ImageDesc &srcImage, const Coords &crd)
{
  if (tabutils::find(tgaFiles, String(tga_tex_name)))
  {
    DAG_FATAL("texture '%s' duplicated!", tga_tex_name);
  }
  else
  {
    tgaFiles.push_back(String(tga_tex_name));
  }

  ImageDesc destImage;
  int imageWidth = crd.width;
  if (crd.left + crd.width > srcImage.image().w)
    imageWidth = srcImage.image().w - crd.left;

  destImage.init(imageWidth, crd.height);
  debug("src=%d %d; dst=%d %d", srcImage.image().w, srcImage.image().h, destImage.image().w, destImage.image().h);

  for (int y = 0; y < crd.height; y++)
  {
    memcpy(destImage.line(y), &(srcImage.line(y + crd.top)[crd.left]), destImage.getStride());
  }

  TexImage32 *im = &destImage.image();

  debug("src=%d %d; dst=%d %d", srcImage.image().w, srcImage.image().h, im->w, im->h);

  if (!save_tga32((char *)tga_tex_name, &destImage.image()))
  {
    error(String(128, "cannot save output file '%s'!", tga_tex_name));
    return false;
  }

  return true;
}

int unpackTex(const char *coordblk, const char *dest_dir, bool no_stripes)
{
  printf("unpacking texture file\n\'%s\' into directory\n\'%s\'\n", coordblk, dest_dir);

  DataBlock blk;
  if (!blk.load(coordblk))
    error("cannot open source file!");
  String destDir(dest_dir);
  if (!destDir.length() || destDir[destDir.length() - 1] != '\\' || destDir[destDir.length() - 1] != '/')
  {
    destDir = destDir + "/";
  }

  // enum all textures
  for (int i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock &texBlock = *blk.getBlock(i);
    if (stricmp(texBlock.getBlockName(), "tex") != 0)
      continue;

    TexImage32 *img = load_tga32((char *)texBlock.getStr("name", NULL), tmpmem);
    if (!img)
    {
      error(String(512, "cannot open source image file '%s'", texBlock.getStr("name", NULL)));
    }

    ImageDesc srcImage;
    srcImage.init(img);
    memfree(img, tmpmem);

    // enum all rectangles & split it into many textures
    for (int j = 0; j < texBlock.blockCount(); j++)
    {
      const DataBlock &picBlock = *texBlock.getBlock(j);
      if (stricmp(picBlock.getBlockName(), "pic") == 0)
      {
        const String outputTexName(512, "%s%s.tga", (char *)destDir, picBlock.getStr("name", NULL));

        Coords crd;
        crd.left = picBlock.getReal("left", 0);
        crd.top = picBlock.getReal("top", 0);
        crd.width = picBlock.getReal("wd", 0);
        crd.height = picBlock.getReal("ht", 0);
        debug("loaded pic %s: %d %d %d %d", picBlock.getStr("name", NULL), crd.left, crd.top, crd.width, crd.height);

        if (!outputTex(outputTexName, srcImage, crd))
          return 0;
      }
      else if (stricmp(picBlock.getBlockName(), "stripe") == 0)
      {
        Coords crd;
        crd.left = picBlock.getReal("left", 0);
        crd.top = picBlock.getReal("top", 0);
        crd.width = picBlock.getReal("wd", 0);
        crd.height = picBlock.getReal("ht", 0);
        debug("loaded stripe %s: %d %d %d %d", picBlock.getStr("prefix", NULL), crd.left, crd.top, crd.width, crd.height);

        int count = picBlock.getInt("count", 0);
        int base = picBlock.getInt("base", 0);
        const char *name = picBlock.getStr("prefix", NULL);

        if (no_stripes)
        {
          for (int s = 0; s < count; s++)
          {
            const String outputTexName(512, "%s%s%d.tga", (char *)destDir, name, s + base);
            Coords sCrd;
            sCrd.left = crd.left + crd.width * s;
            sCrd.top = crd.top;
            sCrd.width = crd.width;
            sCrd.height = crd.height;

            if (!outputTex(outputTexName, srcImage, sCrd))
              return 0;
          }
        }
        else
        {
          crd.width *= count;
          const String outputTexName(512, "%s%s@@%d@@%d.tga", (char *)destDir, name, count, base);
          if (!outputTex(outputTexName, srcImage, crd))
            return 0;
        }
      }
    }
  }


  return 0;
}
