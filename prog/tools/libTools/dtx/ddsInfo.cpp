// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dtx/dtxHeader.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>
#include <stdlib.h>

namespace ddstexture
{
#pragma pack(push)
#pragma pack(1)
struct PixelFormat
{
  uint32_t dwSize;
  uint32_t dwFlags;
  uint32_t dwFourCC;
  uint32_t dwRGBBitCount;
  uint32_t dwRBitMask;
  uint32_t dwGBitMask;
  uint32_t dwBBitMask;
  uint32_t dwRGBAlphaBitMask;
};

struct SurfaceDesc2
{
  uint32_t dwSize;
  uint32_t dwFlags;
  uint32_t dwHeight;
  uint32_t dwWidth;
  uint32_t dwPitchOrLinearSize;
  uint32_t dwDepth;
  uint32_t dwMipMapCount;
  uint32_t dwReserved1[11];
  PixelFormat pixelFormat;
  uint8_t caps2[16];
  uint32_t dwReserved2;
};
#pragma pack(pop)

enum TexFormat
{
  TEX_RGBA,
  TEX_DXT1,
  TEX_DXT3,
  TEX_DXT5,
  TEX_RGBE,
  TEX_R32G32B32B32F
};


//======================================================================================================================
// new-style dds loading
//======================================================================================================================
Header::Header() :
  version(CURRENT_VERSION),
  addr_mode_u(TEXADDR_WRAP),
  addr_mode_v(TEXADDR_WRAP)

  ,
  HQ_mip(0),
  contentType(CONTENT_TYPE_RGBA)

  ,
  reserved1(0),
  MQ_mip(1),
  reserved2(0)

  ,
  reserved3(0),
  LQ_mip(2),
  reserved4(0)
{}

//======================================================================================================================
Tab<Header> &Header::presets(const char *blk_name)
{
  static Tab<Header> presets(midmem_ptr());
  static bool needLoad = true;

  if (needLoad || blk_name)
  {
    needLoad = false;
    DataBlock prefs(blk_name ? blk_name : "config/gameparams.blk");
    const DataBlock *bPresets = prefs.getBlockByNameEx("texturesPresets");
    presets.resize(bPresets->paramCount());
    for (int i = 0; i < bPresets->paramCount(); i++)
      presets[i].getFromName(String(0, "preset@%s.dds", bPresets->getStr(i)));
  }

  return presets;
}

//======================================================================================================================
bool Header::getFromName(const char *name)
{
  char loc[260];
  const char *short_name;
  dd_get_fname_location(loc, name);
  short_name = dd_get_fname(name);

  const char *info = strchr(short_name, '@');
  if (!info)
    return false;

  char tex_addr[] = "wmcbWMCB";
  int tex_ids[] = {TEXADDR_WRAP, TEXADDR_MIRROR, TEXADDR_CLAMP, TEXADDR_BORDER,

    TEXADDR_WRAP, TEXADDR_MIRROR, TEXADDR_CLAMP, TEXADDR_BORDER,

    0};

  info++;

  if (*info == 'F' || *info == 'f')
  {
    info++;
    if (!strnicmp(info, "RGBM", strlen("RGBM")))
      contentType = CONTENT_TYPE_RGBM;
    info += strlen("RGBM");
    if (*info == '_')
      info++;
  }

  // check preset
  if (*info == 'P' || *info == 'p')
  {
    int id = atoi(info + 1);
    if (id < 0 || id >= presets().size())
      return false;
    operator=(presets()[id]);
    return true;
  }

  addr_mode_v = addr_mode_u = TEXADDR_WRAP;
  if (*info == 'A' || *info == 'a')
  {
    info++;
    if (*info != '_' && *info != '.')
      addr_mode_v = addr_mode_u = tex_ids[get_char_id(*(info++), tex_addr)];
    if (*info != '_' && *info != '.')
      addr_mode_v = tex_ids[get_char_id(*(info++), tex_addr)];
  }

  if (*info == '_')
    info++;

  HQ_mip = 0;
  MQ_mip = 1;
  LQ_mip = 2;

  if (*info == 'Q' || *info == 'q')
  {
    info++;
    if (*info != '_' && *info != '.')
      HQ_mip = fromHex(*(info++));
    if (*info != '_' && *info != '.')
      MQ_mip = fromHex(*(info++));
    if (*info != '_' && *info != '.')
      LQ_mip = fromHex(*(info++));
  }
  return true;
}


//======================================================================================================================
bool Header::getFromFile(const char *filename)
{
  if (stricmp(dd_get_fname_ext(filename), ".dds") == 0)
    return getFromName(filename);

  // read from dtx (old style)
  bool success = false;
  file_ptr_t file = df_open(filename, DF_READ);

  if (file)
  {
    if (!df_seek_end(file, -4 - (int)sizeof(*this)))
    {
      if (df_read(file, this, sizeof(*this)) == sizeof(*this))
        success = true;
      if ((addr_mode_u & 7) == 0)
        addr_mode_u = TEXADDR_WRAP;
      if ((addr_mode_v & 7) == 0)
        addr_mode_v = addr_mode_u;
    }
    df_close(file);
  }

  return success;
}


//======================================================================================================================
int Header::fromHex(char ch)
{
  if (ch >= '0' && ch <= '9')
    return ch - '0';

  if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;

  if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;

  return 0;
}


//======================================================================================================================
int Header::mip_level_from_dtx_quality(Quality quality) const
{
  int mip = 0;

  if (quality == quality_User)
    quality = Quality(::dgs_tex_quality);

  switch (quality)
  {
    case quality_High: mip = HQ_mip; break;
    case quality_Medium: mip = MQ_mip; break;
    case quality_Low: mip = LQ_mip; break;
    case quality_UltraLow: mip = 100; break; //< always use lowest available mip
    default: G_ASSERT(0);
  }

  if ((quality != quality_UltraLow) && (mip > 3))
  {
    // debug("invalid mip value (%i), forcing to 3",mip);
    mip = 3; // workaround against wrong/corrupted DTX footers
  }

  return mip;
}

//======================================================================================================================
bool getInfo(unsigned char *p, size_t /*datalen*/, int *addr_u, int *addr_v, int *dds_start_offset)
{
  Header info;
  bool success = true;
  if (check_signature(p))
  {
    if (addr_u)
      *addr_u = info.addr_mode_u;
    if (addr_v)
      *addr_v = info.addr_mode_v;
    if (dds_start_offset)
      *dds_start_offset = 0;
    return true;
  }
  else
  {
    int footer_size = *(int *)p;
    info = *(Header *)(p + 4);
    if (info.addr_mode_u < TEXADDR_WRAP || info.addr_mode_u > TEXADDR_BORDER || info.version > Header::CURRENT_VERSION)
    {
      success = false;
    }
    *dds_start_offset = footer_size + 4;
  }


  if ((info.addr_mode_u & 7) == 0)
    info.addr_mode_u = TEXADDR_WRAP;

  if ((info.addr_mode_v & 7) == 0)
    info.addr_mode_v = info.addr_mode_u;

  *addr_u = info.addr_mode_u;
  *addr_v = info.addr_mode_v;
  return success;
}
} // namespace ddstexture
