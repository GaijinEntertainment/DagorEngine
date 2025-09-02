// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <util/dag_globDef.h>

namespace ddstexture
{
struct Header;

enum Quality
{
  quality_High = 0,
  quality_Medium = 1,
  quality_Low = 2,
  quality_UltraLow = 3,
  quality_User = -1
};

inline static int get_char_id(char value, const char *vector)
{
  const char *p = strchr(vector, value);
  if (!p)
    return 0;
  return p - vector;
}


inline static bool check_signature(int label) { return label == _MAKE4C('DDS '); };
inline static bool check_signature(void *ptr) { return check_signature(*(unsigned int *)ptr); };


//==========================================================================
//  DDS header
//==========================================================================
#pragma pack(push)
#pragma pack(1)
struct Header
{
public:
  Header(); // default footer
  bool getFromName(const char *filename);
  bool getFromFile(const char *filename);
  String makeName(const char *filename);
  static Tab<Header> &presets(const char *blk_name = NULL);

  int mip_level_from_dtx_quality(Quality quality) const;

  bool operator==(const Header &h) const
  {
    return addr_mode_u == h.addr_mode_u && addr_mode_v == h.addr_mode_v && HQ_mip == h.HQ_mip && MQ_mip == h.MQ_mip &&
           LQ_mip == h.LQ_mip && contentType == h.contentType;
  }

  //--------------------------------------------------------------------------
  unsigned int version;     // format version
  unsigned int addr_mode_u; // texture addressing mode
  unsigned int addr_mode_v;

  unsigned short HQ_mip;
  unsigned short contentType;

  unsigned int reserved1;
  unsigned short MQ_mip;
  unsigned short reserved2;

  unsigned int reserved3;
  unsigned short LQ_mip;
  unsigned short reserved4;

  static const int CURRENT_VERSION = 4;

  enum ContentType
  {
    CONTENT_TYPE_RGBA = 0,
    CONTENT_TYPE_RGBM = 1
  };

  //--------------------------------------------------------------------------------
private:
  int fromHex(char ch);
};
#pragma pack(pop)


bool getInfo(unsigned char *data, size_t size, int *addr_u, int *addr_v, int *dds_start_offset);
} // namespace ddstexture
