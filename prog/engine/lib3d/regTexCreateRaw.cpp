// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texMgr.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_createTex.h>
#include <drv/3d/dag_driver.h>
#include <3d/ddsxTex.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_fastSeqRead.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_string.h>
#include <util/dag_texMetaData.h>
#include <util/le2be.h>
#include <util/fnameMap.h>
#include <math/dag_mathUtils.h>
#include <debug/dag_debug.h>


class RawCreateTexFactory : public ICreateTexFactory
{
public:
  virtual BaseTexture *createTex(const char *fn, int /*flg*/, int /*levels*/, const char *fn_ext, const TextureMetaData &tmd)
  {
    if (!fn_ext)
      return NULL;

    if (dd_strnicmp(fn_ext, ".raw", 4) == 0 || dd_strnicmp(fn_ext, ".r16", 4) == 0 || dd_strnicmp(fn_ext, ".r32", 4) == 0)
    {
      DataBlock params;
      dblk::load_text(params, make_span_const(fn, strlen(fn)), dblk::ReadFlag::ROBUST);

      if (!params.isValid())
      {
        logerr("cannot parse .RAW settings: '%s'", fn);
        return NULL;
      }

      const char *fileName = params.getStr("name", NULL);
      if (!fileName)
      {
        logerr("'name:t' is not found in .RAW settings: '%s'", fn);
        return NULL;
      }

      if (!dd_file_exists(fileName))
      {
        logerr(".RAW file doesn't exist '%s'", fileName);
        return NULL;
      }


      BaseTexture *t = NULL;
      BaseTexture *ref = NULL;
      if (!tmd.baseTexName.empty())
      {
        TEXTUREID id = get_managed_texture_id(tmd.baseTexName);
        if (id == BAD_TEXTUREID)
          id = add_managed_texture(tmd.baseTexName);
        ref = acquire_managed_tex(id);
        if (!ref)
        {
          release_managed_tex(id);
          return t;
        }
      }


      const char *fmtName = params.getStr("fmt", "");
      int offset = params.getInt("offset", 0); //-V666
      uint32_t format = parse_tex_format(fmtName, TEXFMT_R32F);

      const TextureFormatDesc &desc = get_tex_format_desc(format);
      G_ASSERT(desc.isBlockFormat == false);

      int pixelSize = desc.bytesPerElement;
      int bytesPerChannel = desc.r.bits / 8;
      G_ASSERT(bytesPerChannel == 1 || bytesPerChannel == 2 || bytesPerChannel == 4);
      G_ASSERT(desc.r.bits % 8 == 0);

      int width = params.getInt("w", 1);
      int height = params.getInt("h", 1);
      G_ASSERT(width >= 1 && width <= 8192);
      G_ASSERT(height >= 1 && height <= 8192);


      const char *channelOrder = params.getStr("ch_order", "RGBA");

#define FIND_CHANNEL_POS(pos, channel)                   \
  int pos = -1;                                          \
  if (const char *found = strchr(channelOrder, channel)) \
    pos = found - channelOrder;

      FIND_CHANNEL_POS(orderR, 'R');
      FIND_CHANNEL_POS(orderG, 'G');
      FIND_CHANNEL_POS(orderB, 'B');
      FIND_CHANNEL_POS(orderA, 'A');

      if (orderR < 0 || orderG < 0 || orderB < 0 || orderA < 0)
      {
        G_ASSERTF(0, "One of channels is not found in ch_order");
        return NULL;
      }

      int toR = desc.r.bits == 0 ? -1 : desc.r.offset / desc.r.bits;
      int toG = desc.g.bits == 0 ? -1 : desc.g.offset / desc.g.bits;
      int toB = desc.b.bits == 0 ? -1 : desc.b.offset / desc.b.bits;
      int toA = desc.a.bits == 0 ? -1 : desc.a.offset / desc.a.bits;

      bool usedTo[4] = {0};
      toR >= 0 ? usedTo[toR] = true : orderR = -1;
      toG >= 0 ? usedTo[toG] = true : orderG = -1;
      toB >= 0 ? usedTo[toB] = true : orderB = -1;
      toA >= 0 ? usedTo[toA] = true : orderA = -1;

      for (int i = 2; i >= 0; i--)
        if (!usedTo[i])
        {
          if (toR > i)
            toR--;
          if (toG > i)
            toG--;
          if (toB > i)
            toB--;
          if (toA > i)
            toA--;
        }

      bool usedOrder[4] = {0};
      orderR >= 0 ? usedOrder[orderR] = true : 0;
      orderG >= 0 ? usedOrder[orderG] = true : 0;
      orderB >= 0 ? usedOrder[orderB] = true : 0;
      orderA >= 0 ? usedOrder[orderA] = true : 0;

      for (int i = 2; i >= 0; i--)
        if (!usedOrder[i])
        {
          if (orderR > i)
            orderR--;
          if (orderG > i)
            orderG--;
          if (orderB > i)
            orderB--;
          if (orderA > i)
            orderA--;
        }


      file_ptr_t fp = ::df_open(fileName, DF_READ);
      if (!fp)
        return NULL;

      struct AutoClose
      {
        file_ptr_t filePtr;
        AutoClose(file_ptr_t f) : filePtr(f) {}
        ~AutoClose() { ::df_close(filePtr); }
      } autoClose(fp);

      int flen = ::df_length(fp);

      if (offset)
      {
        ::df_seek_to(fp, offset);
        flen -= offset;
      }

      if (flen <= 0)
        return NULL;

      t = d3d::create_tex(NULL, width, height, format, 1, fn);

      if (t)
      {
        apply_gen_tex_props(t, tmd);

        uint8_t *data = NULL;
        int stride = 0;

        if (((Texture *)t)->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE))
        {
          memset(data, 0, height * stride);

          for (int y = 0; y < height; y++)
          {
            int idx = stride * y;
            int lineLen = pixelSize * width;

            ::df_read(fp, data + idx, lineLen);

            if (orderR != toR || orderG != toG || orderB != toB || orderA != toA)
              switch (bytesPerChannel)
              {
                case 4:
                  for (int x = 0; x < width; x++)
                  {
                    uint32_t *p = (uint32_t *)(&data[idx + pixelSize * x]);
                    uint32_t r = orderR == -1 ? 0 : p[orderR];
                    uint32_t g = orderG == -1 ? 0 : p[orderG];
                    uint32_t b = orderB == -1 ? 0 : p[orderB];
                    uint32_t a = orderA == -1 ? 0 : p[orderA];
                    (toR >= 0 && orderR >= 0) ? p[toR] = r : 0;
                    (toG >= 0 && orderG >= 0) ? p[toG] = g : 0;
                    (toB >= 0 && orderB >= 0) ? p[toB] = b : 0;
                    (toA >= 0 && orderA >= 0) ? p[toA] = a : 0;
                  }
                  break;

                case 2:
                  for (int x = 0; x < width; x++)
                  {
                    uint16_t *p = (uint16_t *)(&data[idx + pixelSize * x]);
                    uint16_t r = orderR == -1 ? 0 : p[orderR];
                    uint16_t g = orderG == -1 ? 0 : p[orderG];
                    uint16_t b = orderB == -1 ? 0 : p[orderB];
                    uint16_t a = orderA == -1 ? 0 : p[orderA];
                    (toR >= 0 && orderR >= 0) ? p[toR] = r : 0;
                    (toG >= 0 && orderG >= 0) ? p[toG] = g : 0;
                    (toB >= 0 && orderB >= 0) ? p[toB] = b : 0;
                    (toA >= 0 && orderA >= 0) ? p[toA] = a : 0;
                  }
                  break;

                case 1:
                  for (int x = 0; x < width; x++)
                  {
                    uint8_t *p = (uint8_t *)(&data[idx + pixelSize * x]);
                    uint8_t r = orderR == -1 ? 0 : p[orderR];
                    uint8_t g = orderG == -1 ? 0 : p[orderG];
                    uint8_t b = orderB == -1 ? 0 : p[orderB];
                    uint8_t a = orderA == -1 ? 0 : p[orderA];
                    (toR >= 0 && orderR >= 0) ? p[toR] = r : 0;
                    (toG >= 0 && orderG >= 0) ? p[toG] = g : 0;
                    (toB >= 0 && orderB >= 0) ? p[toB] = b : 0;
                    (toA >= 0 && orderA >= 0) ? p[toA] = a : 0;
                  }
                  break;

                default:;
                  // already asserted just after bytesPerChannel initialization
              }


            flen -= lineLen;
            if (flen < 0)
            {
              logerr("out of file while reading raw texture '%s'", fileName);
              ((Texture *)t)->unlockimg();
              del_d3dres(t);
              return NULL;
            }
          }

          ((Texture *)t)->unlockimg();
        }
        else
        {
          G_ASSERTF(0, "cannot lock raw texture");
          del_d3dres(t);
          return NULL;
        }
      }

      return t;
    }

    return NULL;
  }
};

static RawCreateTexFactory raw_create_tex_factory;

void register_raw_tex_create_factory() { add_create_tex_factory(&raw_create_tex_factory); }
