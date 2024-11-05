// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_texMetaData.h>
#include <util/dag_string.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_tex3d.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_e3dColor.h>
#include <osApiWrappers/dag_localConv.h>
#include <debug/dag_debug.h>
#include <stdlib.h>
#include <ctype.h>


static const char addrSymbol[] = {'w', 'm', 'c', 'b', 'o'};
static const char aniSymbol[] = {'i', 'm', 'd', 'a'};
static const char filtSymbol[] = {'d', 's', 'b', 'n', 'p', 'l'};

static bool decodeAddr(char sym, uint8_t &addr)
{
  const char *p = (char *)memchr(addrSymbol, sym, sizeof(addrSymbol));
  if (!p)
    return false;
  addr = p - addrSymbol;
  return true;
}
static bool decodeFilter(char sym, uint8_t &filt)
{
  const char *p = (char *)memchr(filtSymbol, sym, sizeof(filtSymbol));
  if (!p)
    return false;
  filt = p - filtSymbol;
  return true;
}

static uint8_t getAddr(const char *addr, uint8_t def)
{
  if (!addr)
    return def;
  if (dd_stricmp(addr, "wrap") == 0)
    return TextureMetaData::ADDR_WRAP;
  if (dd_stricmp(addr, "mirror") == 0)
    return TextureMetaData::ADDR_MIRROR;
  if (dd_stricmp(addr, "clamp") == 0)
    return TextureMetaData::ADDR_CLAMP;
  if (dd_stricmp(addr, "border") == 0)
    return TextureMetaData::ADDR_BORDER;
  if (dd_stricmp(addr, "mirrorOnce") == 0)
    return TextureMetaData::ADDR_MIRRORONCE;

  debug("invalid addressing mode <%s>", addr);
  return 0;
}
static uint8_t getAniFunc(const char *ani, uint8_t def)
{
  if (!ani)
    return def;
  if (dd_stricmp(ani, "min") == 0)
    return TextureMetaData::AFUNC_MIN;
  if (dd_stricmp(ani, "mul") == 0)
    return TextureMetaData::AFUNC_MUL;
  if (dd_stricmp(ani, "div") == 0)
    return TextureMetaData::AFUNC_DIV;
  if (dd_stricmp(ani, "abs") == 0)
    return TextureMetaData::AFUNC_ABS;

  debug("invalid anisotropic function <%s>", ani);
  return 0;
}
static uint8_t getFilter(const char *filt, uint8_t def, bool mip)
{
  if (!filt)
    return def;

  if (dd_stricmp(filt, "smooth") == 0 && !mip)
    return TextureMetaData::FILT_SMOOTH;
  if (dd_stricmp(filt, "best") == 0 && !mip)
    return TextureMetaData::FILT_BEST;
  if (dd_stricmp(filt, "none") == 0)
    return TextureMetaData::FILT_NONE;
  if (dd_stricmp(filt, "point") == 0)
    return TextureMetaData::FILT_POINT;
  if (dd_stricmp(filt, "linear") == 0 && mip)
    return TextureMetaData::FILT_LINEAR;
  if (dd_stricmp(filt, "default") == 0)
    return TextureMetaData::FILT_DEF;

  debug("invalid filtering mode <%s> for %s", filt, mip ? "mip" : "tex");
  return 0;
}

bool TextureMetaData::isDefault() const
{
  static const TextureMetaData def;
  // This comparison is only compatible with default values, because of the string
  // The default value for the string is nullptr.
  // pragma pack should avoid any issues due to undefined bits in the struct
  return memcmp(&def, this, sizeof(def)) == 0; // -V1014
}

const char *TextureMetaData::encode(const char *fpath, String *storage) const
{
  static String buf0;
  String &buf = storage ? *storage : buf0;

  if (isDefault())
    return fpath;

  if (!isValid())
    return NULL;

  buf.printf(0, "%s?", fpath);
  if (texFilterMode != FILT_DEF)
  {
    buf.append("F");
    buf.append(filtSymbol + texFilterMode, 1);
  }
  if (mipFilterMode != FILT_DEF)
  {
    buf.append("M");
    buf.append(filtSymbol + mipFilterMode, 1);
  }
  if ((addrU == ADDR_BORDER || addrV == ADDR_BORDER || addrW == ADDR_BORDER) && borderCol)
    buf.aprintf(16, "B%08x", borderCol);
  if (lodBias)
    buf.aprintf(16, "L%d", lodBias);

  if (addrU != ADDR_WRAP || addrV != ADDR_WRAP || addrW != ADDR_WRAP)
  {
    buf.append("A");
    buf.append(addrSymbol + addrU, 1);
    if (addrU != addrV || addrU != addrW)
    {
      buf.append(addrSymbol + addrV, 1);
      if (addrV != addrW)
        buf.append(addrSymbol + addrW, 1);
    }
  }

  if (hqMip != 0 || mqMip != 1 || lqMip != 2)
  {
    buf.aprintf(4, "Q%d", hqMip);
    if (mqMip != 1 || lqMip != 2)
    {
      buf.aprintf(4, "-%d", mqMip);
      if (lqMip != 2)
        buf.aprintf(4, "-%d", lqMip);
    }
  }

  if (anisoFunc != AFUNC_MUL || anisoFactor != 1)
    buf.aprintf(8, "N%c%d", aniSymbol[anisoFunc], anisoFactor);
  if (stubTexIdx > 0)
    buf.aprintf(8, "U%02d", stubTexIdx);

  if (!(flags & FLG_OPTIMIZE))
    buf.append("T0");
  if (!(flags & FLG_PACK))
    buf.append("Z0");
  if (flags & FLG_NONPOW2)
    buf.append("P1");
  if (flags & FLG_PREMUL_A)
    buf.append("D");
  if (flags & FLG_OVERRIDE)
    buf.append("X1");
  if (flags & FLG_IES_ROT)
    buf.append("R");
  if (iesScalingFactor != 0)
    buf.aprintf(5, "V%04x", static_cast<uint16_t>(iesScalingFactor));

  if (!baseTexName.empty())
    buf.aprintf(64, "S%s", baseTexName.str());
  return buf;
}

const char *TextureMetaData::decode(const char *fstring, String *storage)
{
  static String buf0;
  const char *p = strrchr(fstring, '?');

  if (!p)
  {
    defaults();
    return fstring;
  }

  if (decodeData(p, true))
  {
    String &buf = storage ? *storage : buf0;
    int nl = p - fstring;
    buf.resize(nl + 1);
    memcpy(buf.c_str(), fstring, nl);
    buf.c_str()[nl] = '\0';
    return buf;
  }

  logerr("error while decoding string %s", fstring);
  return NULL;
}
bool TextureMetaData::decodeData(const char *fstring, bool dec_bt_name)
{
  const char *p = strrchr(fstring, '?');
  uint8_t addr = 0, filt = 0;

  defaults();
  if (!p)
    return true;

  p++;
  while (*p)
  {
    switch (*p)
    {
      case 'A':
      case 'a':
        p++;
        if (!decodeAddr(*p, addr))
          goto err;
        addrU = addr;
        p++;
        if (!decodeAddr(*p, addr))
          addrV = addrW = addrU;
        else
        {
          addrV = addr;
          p++;
          if (!decodeAddr(*p, addr))
            addrW = addrV;
          else
          {
            addrW = addr;
            p++;
          }
        }
        break;

      case 'Q':
      case 'q':
        p++;
        hqMip = atoi(p);
        while (isdigit((unsigned char)*p))
          p++;
        if (*p == '-')
        {
          p++;
          mqMip = atoi(p);
          while (isdigit((unsigned char)*p))
            p++;
          if (*p == '-')
          {
            p++;
            lqMip = atoi(p);
            while (isdigit((unsigned char)*p))
              p++;
          }
        }
        if (mqMip < hqMip)
          mqMip = hqMip;
        if (lqMip < mqMip)
          lqMip = mqMip;
        break;

      case 'N':
      case 'n':
        p++;
        if (*p == 'i')
          anisoFunc = AFUNC_MIN;
        else if (*p == 'm')
          anisoFunc = AFUNC_MUL;
        else if (*p == 'd')
          anisoFunc = AFUNC_DIV;
        else if (*p == 'a')
          anisoFunc = AFUNC_ABS;
        else
          goto err;
        p++;
        anisoFactor = atoi(p);
        while (isdigit((unsigned char)*p))
          p++;
        break;

      case 'T':
      case 't':
        p++;
        if (*p == '1')
          flags |= FLG_OPTIMIZE;
        else if (*p == '0')
          flags &= ~FLG_OPTIMIZE;
        else
          goto err;
        p++;
        break;

      case 'Z':
      case 'z':
        p++;
        if (*p == '1')
          flags |= FLG_PACK;
        else if (*p == '0')
          flags &= ~FLG_PACK;
        else
          goto err;
        p++;
        break;

      case 'P':
      case 'p':
        p++;
        if (*p == '1')
          flags |= FLG_NONPOW2;
        else if (*p == '0')
          flags &= ~FLG_NONPOW2;
        else
          goto err;
        p++;
        break;

      case 'D':
      case 'd':
        p++;
        flags |= FLG_PREMUL_A;
        break;

      case 'X':
      case 'x':
        p++;
        if (*p == '1')
          flags |= FLG_OVERRIDE;
        else if (*p == '0')
          flags &= ~FLG_OVERRIDE;
        else
          goto err;
        p++;
        break;

      case 'F':
      case 'f':
        p++;
        if (!decodeFilter(*p, filt))
          goto err;
        if (filt == TextureMetaData::FILT_LINEAR)
          goto err;
        p++;
        texFilterMode = filt;
        break;
      case 'M':
      case 'm':
        p++;
        if (!decodeFilter(*p, filt))
          goto err;
        if (filt == TextureMetaData::FILT_SMOOTH || filt == TextureMetaData::FILT_BEST)
          goto err;
        p++;
        mipFilterMode = filt;
        break;
      case 'B':
      case 'b':
        p++;
        for (int i = 0; i < 8; i++)
          if (!isxdigit((unsigned char)p[i]))
            goto err;
        borderCol = strtoul(p, NULL, 16);
        p += 8;
        break;
      case 'L':
      case 'l':
        p++;
        lodBias = strtol(p, (char **)&p, 10);
        break;

      case 'R':
      case 'r':
        p++;
        flags |= FLG_IES_ROT;
        break;

      case 'V':
      case 'v':
        p++;
        for (int i = 0; i < 4; i++)
          if (!isxdigit((unsigned char)p[i]))
            goto err;
        iesScalingFactor = static_cast<int16_t>(strtoul(p, NULL, 16));
        p += 4;
        break;

      case 'S':
      case 's':
        if (dec_bt_name)
          baseTexName = p + 1;
        p += strlen(p);
        break;

      case 'U':
      case 'u':
        p++;
        stubTexIdx = atoi(p);
        while (isdigit((unsigned char)*p))
          p++;
        break;

      case '<':
        forceLQ = 1;
        p++;
        break;

      case '>':
        forceFQ = 1;
        p++;
        break;

      default: goto err;
    }
  }

  if (isValid())
    return true;

err:
  logerr("invalid file string <%s> at %d", fstring, (int)(p - fstring));
  return false;
}

const char *TextureMetaData::decodeBaseTexName(const char *fstring)
{
  const char *p = fstring ? strrchr(fstring, '?') : NULL;
  if (!p)
    return NULL;

  p++;
  while (*p)
  {
    switch (*p)
    {
      case 'A':
      case 'a':
        p++;
        for (int i = 0; i < 3; i++)
          if (memchr(addrSymbol, *p, sizeof(addrSymbol)))
            p++;
          else if (i == 0)
            goto err;
        break;

      case 'Q':
      case 'q':
        for (int i = 0; i < 3; i++)
        {
          p++;
          while (isdigit((unsigned char)*p))
            p++;
          if (*p != '-')
            break;
        }
        break;

      case 'N':
      case 'n':
        p++;
        if (!memchr(aniSymbol, *p, sizeof(aniSymbol)))
          goto err;
        p++;
        while (isdigit((unsigned char)*p))
          p++;
        break;

      case 'T':
      case 't':
      case 'Z':
      case 'z':
      case 'P':
      case 'p':
      case 'X':
      case 'x':
        p++;
        if (*p != '1' && *p != '0')
          goto err;
        p++;
        break;

      case 'D':
      case 'd':
      case 'R':
      case 'r': p++; break;

      case 'F':
      case 'f':
      case 'M':
      case 'm':
        p++;
        if (!memchr(filtSymbol, *p, sizeof(filtSymbol)))
          goto err;
        p++;
        break;
      case 'B':
      case 'b':
        p++;
        for (int i = 0; i < 8; i++)
          if (!isxdigit((unsigned char)p[i]))
            goto err;
        p += 8;
        break;
      case 'L':
      case 'l':
        p++;
        if (*p == '-' || *p == '+')
          p++;
        while (isdigit((unsigned char)*p))
          p++;
        break;

      case 'V':
      case 'v':
        p++;
        for (int i = 0; i < 4; i++)
          if (!isxdigit((unsigned char)p[i]))
            goto err;
        p += 4;
        break;

      case 'S':
      case 's': return p + 1;

      case 'U':
      case 'u':
        p++;
        while (isdigit((unsigned char)*p))
          p++;
        break;

      default: goto err;
    }
  }
  return NULL;

err:
  logerr("invalid file string <%s> at %d", fstring, (int)(p - fstring));
  return NULL;
}

void TextureMetaData::read(const DataBlock &_blk, const char *spec_target_str)
{
#define GET_PROP(TYPE, PROP, DEF) blk.get##TYPE(PROP, &blk != &_blk ? _blk.get##TYPE(PROP, DEF) : DEF)
  const DataBlock *spec_data = _blk.getBlockByName(spec_target_str);
  const DataBlock &blk = spec_data ? *spec_data : _blk;

  defaults();
  uint8_t addr = getAddr(GET_PROP(Str, "addr", NULL), addrU);

  addrU = getAddr(GET_PROP(Str, "addrU", NULL), addr);
  addrV = getAddr(GET_PROP(Str, "addrV", NULL), addr);
  addrW = getAddr(GET_PROP(Str, "addrW", NULL), addr);

  hqMip = GET_PROP(Int, "hqMip", hqMip);
  mqMip = GET_PROP(Int, "mqMip", mqMip);
  lqMip = GET_PROP(Int, "lqMip", lqMip);
  if (mqMip < hqMip)
    mqMip = hqMip;
  if (lqMip < mqMip)
    lqMip = mqMip;

  anisoFunc = getAniFunc(GET_PROP(Str, "aniFunc", NULL), GET_PROP(Int, "anisotropy", -1) < 0 ? anisoFunc : AFUNC_ABS);
  anisoFactor = GET_PROP(Int, "anisotropy", anisoFactor);

  texFilterMode = getFilter(GET_PROP(Str, "texFilterMode", NULL), texFilterMode, false);
  mipFilterMode = getFilter(GET_PROP(Str, "mipFilterMode", NULL), mipFilterMode, true);
  borderCol = GET_PROP(E3dcolor, "borderColor", E3DCOLOR(0, 0, 0, 0));
  lodBias = int(GET_PROP(Real, "lodBias", 0.0f) * 1000.0f);
  stubTexIdx = GET_PROP(Int, "stubTexIdx", 0);

  if (GET_PROP(Bool, "nonPow2", false))
    flags |= FLG_NONPOW2;
  else
    flags &= ~FLG_NONPOW2;

  if (GET_PROP(Bool, "optimize", true))
    flags |= FLG_OPTIMIZE;
  else
    flags &= ~FLG_OPTIMIZE;

  if (GET_PROP(Bool, "pack", true))
    flags |= FLG_PACK;
  else
    flags &= ~FLG_PACK;

  if (GET_PROP(Bool, "override", false))
    flags |= FLG_OVERRIDE;
  else
    flags &= ~FLG_OVERRIDE;

  if (GET_PROP(Bool, "iesRotation", false))
    flags |= FLG_IES_ROT;
  else
    flags &= ~FLG_IES_ROT;

  setIesScale(GET_PROP(Real, "iesScale", 1.0f));

  baseTexName = GET_PROP(Str, "baseTex", NULL);

  if (!isValid())
    debug("invalid TextureMetaData read from datablock");
#undef GET_PROP
}

void TextureMetaData::write(DataBlock &blk)
{
  static const char *s_addr[] = {"wrap", "mirror", "clamp", "border", "mirrorOnce"};
  static const char *s_aniFunc[] = {"min", "mul", "div", "abs"};
  static const char *s_filt[] = {"default", "smooth", "best", "none", "point", "linear"};

  if (!isValid())
  {
    debug("cannot write invalid TextureMetaData to datablock");
    return;
  }

  blk.clearData();

  if (addrU == ADDR_WRAP && addrV == ADDR_WRAP && addrW == ADDR_WRAP)
    ; // default, do nothing
  else if (addrU == addrV && addrU == addrW)
    blk.setStr("addr", s_addr[addrU]);
  else if (addrU == addrV)
  {
    blk.setStr("addr", s_addr[addrU]);
    blk.setStr("addrW", s_addr[addrW]);
  }
  else
  {
    blk.setStr("addrU", s_addr[addrU]);
    blk.setStr("addrV", s_addr[addrV]);
    blk.setStr("addrW", s_addr[addrW]);
  }

  if (hqMip != 0)
    blk.setInt("hqMip", hqMip);
  if (mqMip != 1)
    blk.setInt("mqMip", mqMip);
  if (lqMip != 2)
    blk.setInt("lqMip", lqMip);

  if (anisoFunc != AFUNC_MUL || anisoFactor != 1)
    blk.setStr("aniFunc", s_aniFunc[anisoFunc]);
  if (anisoFactor != 1)
    blk.setInt("anisotropy", anisoFactor);

  if (texFilterMode != FILT_DEF)
    blk.setStr("texFilterMode", s_filt[texFilterMode]);
  if (mipFilterMode != FILT_DEF)
    blk.setStr("mipFilterMode", s_filt[mipFilterMode]);
  if (borderCol != 0)
    blk.setE3dcolor("borderColor", borderCol);
  if (lodBias != 0)
    blk.setReal("lodBias", lodBias / 1000.0f);
  if (stubTexIdx != 0)
    blk.setInt("stubTexIdx", stubTexIdx);

  if (flags & FLG_NONPOW2)
    blk.setBool("nonPow2", true);

  if (!(flags & FLG_OPTIMIZE))
    blk.setBool("optimize", false);

  if (!(flags & FLG_PACK))
    blk.setBool("pack", false);

  if (flags & FLG_OVERRIDE)
    blk.setBool("override", true);

  if (flags & FLG_IES_ROT)
    blk.setBool("iesRotation", true);

  if (iesScalingFactor != 0)
    blk.setReal("iesScale", getIesScale());

  if (!baseTexName.empty())
    blk.setStr("baseTex", baseTexName);
}

int TextureMetaData::d3dTexAddr(unsigned addr)
{
  int d3d_addr[] = {TEXADDR_WRAP, TEXADDR_MIRROR, TEXADDR_CLAMP, TEXADDR_BORDER, TEXADDR_MIRRORONCE};
  return d3d_addr[addr];
}
int TextureMetaData::d3dTexFilter() const
{
  int d3d_filt[] = {0, TEXFILTER_LINEAR, TEXFILTER_BEST, TEXFILTER_NONE, TEXFILTER_POINT, 0};
  return d3d_filt[texFilterMode];
}
int TextureMetaData::d3dMipFilter() const
{
  int d3d_filt[] = {0, 0, 0, TEXMIPMAP_NONE, TEXMIPMAP_POINT, TEXMIPMAP_LINEAR};
  return d3d_filt[mipFilterMode];
}

const char *TextureMetaData::decodeFileName(const char *fstring, String *storage)
{
  static String buf0;
  String &buf = storage ? *storage : buf0;
  const char *p = strrchr(fstring, '?');

  if (!p)
    return fstring;

  buf.printf(0, "%.*s", (int)(p - fstring), fstring);
  return buf;
}

#define EXPORT_PULL dll_pull_baseutil_texMetaData
#include <supp/exportPull.h>
