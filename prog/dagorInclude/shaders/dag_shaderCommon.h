//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/************************************************************************
  common shader types & constants
************************************************************************/

#include <3d/dag_texMgr.h>
#include <3d/dag_drv3dConsts.h>
#include <generic/dag_span.h>
#include <shaders/dag_shaderVarType.h>

class ShaderMatVdata;
class ShaderMaterial;
class ShaderElement;
class BaseTexture;
typedef BaseTexture Texture;

// if this parameter passed to ShaderElement->render
// as 3rd parameter (startIndex), use d3d::draw instead d3d::drawind
#define RELEM_NO_INDEX_BUFFER -1

enum ShaderFlags
{
  SC_STAGE_IDX_MASK = 0x0007,
  SC_NEW_STAGE_FMT = 0x1000,
};

static constexpr char ARRAY_SIZE_GETTER_NAME[] = "get_array_size(%s)";

enum ChannelModifier
{
  CMOD_NONE,              // nothing
  CMOD_SIGNED_PACK,       // rescale data [0..+1] -> [-1..+1]
  CMOD_UNSIGNED_PACK,     // rescale data [-1..+1] -> [0..+1]
  CMOD_MUL_1K,            // multiply by  1024
  CMOD_MUL_2K,            // multiply by  2048
  CMOD_MUL_4K,            // multiply by  4096
  CMOD_MUL_8K,            // multiply by  8192
  CMOD_MUL_16K,           // multiply by 16384
  CMOD_BOUNDING_PACK,     // rescale data [minbounding..maxbounding] -> [0..+1] or [-1 .. 1] (according to target format)
  CMOD_SIGNED_SHORT_PACK, // multiply by 32767(!)
};

struct ShaderChannelId
{
  uint16_t u = 0, streamId = 0, ui = 0, resv = 0; // usage and usage index specifying source data
  int t = 0;                                      // data type
  int vbu = 0, vbui = 0;                          // VB usage and usage index for this channel

  ChannelModifier mod = CMOD_NONE; // modifier

  // val=(val+offset)*mul
  static void setPosBounding(const Point3 &offset, const Point3 &mul);
  static void getPosBounding(Point3 &offset, Point3 &mul);
  static void setBoundingUsed();
  static bool getBoundingUsed();
  static void resetBoundingUsed();
};


struct CompiledShaderChannelId
{
  int t; // data type
  int vbu;
  int16_t vbui; // VB usage and usage index for this channel
  uint16_t streamId;
  CompiledShaderChannelId() : t(-1) {} //-V730, intentionally uninitialized
  CompiledShaderChannelId(int t_, int vbu_, int16_t vbui_, uint16_t sid = 0) : t(t_), vbu(vbu_), vbui(vbui_), streamId(sid) {}

  inline void convertFrom(const ShaderChannelId &id)
  {
    t = id.t;
    vbu = id.vbu;
    vbui = id.vbui;
    streamId = id.streamId;
  }
};

enum
{
  SCUSAGE_POS,
  SCUSAGE_NORM,
  SCUSAGE_VCOL,
  SCUSAGE_TC,
  SCUSAGE_LIGHTMAP,
  SCUSAGE_EXTRA,
};

#define SCTYPE_FLOAT1   VSDT_FLOAT1
#define SCTYPE_FLOAT2   VSDT_FLOAT2
#define SCTYPE_FLOAT3   VSDT_FLOAT3
#define SCTYPE_FLOAT4   VSDT_FLOAT4
#define SCTYPE_E3DCOLOR VSDT_E3DCOLOR
#define SCTYPE_UBYTE4   VSDT_UBYTE4
#define SCTYPE_SHORT2   VSDT_SHORT2
#define SCTYPE_SHORT4   VSDT_SHORT4
#define SCTYPE_HALF2    VSDT_HALF2
#define SCTYPE_HALF4    VSDT_HALF4
#define SCTYPE_SHORT2N  VSDT_SHORT2N
#define SCTYPE_SHORT4N  VSDT_SHORT4N
#define SCTYPE_USHORT2N VSDT_USHORT2N
#define SCTYPE_USHORT4N VSDT_USHORT4N
#define SCTYPE_UDEC3    VSDT_UDEC3
#define SCTYPE_DEC3N    VSDT_DEC3N
#define SCTYPE_UINT1    VSDT_UINT1
#define SCTYPE_UINT2    VSDT_UINT2
#define SCTYPE_UINT3    VSDT_UINT3
#define SCTYPE_UINT4    VSDT_UINT4


inline bool channel_size(unsigned t, unsigned &channelSize)
{
  switch (t)
  {
    case VSDT_UINT3:
    case VSDT_FLOAT3: channelSize = 12; break;
    case VSDT_UINT4:
    case VSDT_FLOAT4: channelSize = 16; break;

    case VSDT_UINT1:
    case VSDT_FLOAT1:
    case VSDT_UDEC3:
    case VSDT_DEC3N:
    case VSDT_E3DCOLOR:
    case VSDT_UBYTE4:
    case VSDT_HALF2:
    case VSDT_SHORT2N:
    case VSDT_USHORT2N:
    case VSDT_SHORT2: channelSize = 4; break;
    case VSDT_UINT2:
    case VSDT_FLOAT2:
    case VSDT_HALF4:
    case VSDT_SHORT4N:
    case VSDT_USHORT4N:
    case VSDT_SHORT4: channelSize = 8; break;
    default: return false;
  }
  return true;
}

inline bool channel_signed(unsigned t)
{
  switch (t)
  {
    case VSDT_FLOAT1:
    case VSDT_FLOAT2:
    case VSDT_FLOAT3:
    case VSDT_FLOAT4:
    case VSDT_HALF2:
    case VSDT_SHORT2N:
    case VSDT_SHORT2:
    case VSDT_HALF4:
    case VSDT_SHORT4N:
    case VSDT_SHORT4:
    case VSDT_DEC3N: return true;
    case VSDT_USHORT2N:
    case VSDT_USHORT4N:
    case VSDT_UDEC3:
    case VSDT_E3DCOLOR:
    case VSDT_UBYTE4:
    case VSDT_UINT1:
    case VSDT_UINT2:
    case VSDT_UINT3:
    case VSDT_UINT4:
    default: return false;
  }
}


class ShaderChannelsEnumCB
{
public:
  virtual void enum_shader_channel(int usage, int usage_index, int type, int vb_usage, int vb_usage_index, ChannelModifier mod,
    int stream = 0) = 0;
};

enum
{
  SHFLG_LIGHTMAP = 0 << 0,   // shader material has lightmap
  SHFLG_VLTMAP = 0 << 1,     // shader material has "vertex lightmap"
  SHFLG_2SIDED = 1 << 2,     // shader material is two-sided
  SHFLG_REAL2SIDED = 1 << 3, // two-sided lighting if material is two-sided
};

struct Color4;
typedef dag::Span<Color4> *VprArrayPtr;
