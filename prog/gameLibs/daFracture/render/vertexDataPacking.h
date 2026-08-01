// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daFracture/render/material.h>
#include <daFracture/core/destrMesh.h>


static __forceinline void channel_cvt_pack_impl(const uint32_t type, const ChannelModifier mod, Point4 val, uint8_t *data)
{
  switch (mod)
  {
    case CMOD_NONE: break;
    case CMOD_SIGNED_PACK: val = val * 2.f - Point4(1, 1, 1, 1); break;
    case CMOD_UNSIGNED_PACK: val = (val + Point4(1, 1, 1, 1)) * 0.5f; break;
    case CMOD_MUL_1K: val *= float(1 << 10); break;
    case CMOD_MUL_2K: val *= float(1 << 11); break;
    case CMOD_MUL_4K: val *= float(1 << 12); break;
    case CMOD_MUL_8K: val *= float(1 << 13); break;
    case CMOD_MUL_16K: val *= float(1 << 14); break;
    case CMOD_SIGNED_SHORT_PACK: val *= 32767.f; break;
    case CMOD_BOUNDING_PACK:
    {
      G_LOGERR_ONCE_AND_DO(0, , "channel_cvt_pack_impl: CMOD_BOUNDING_PACK is not supported");
      break;
    }
  }

  const auto clamp_s16 = [](float v) -> int16_t { return int16_t(eastl::clamp(int(v), -32768, 32767)); };
  const auto clamp_u16 = [](float v) -> uint16_t { return uint16_t(eastl::clamp(int(v), 0, 65535)); };
  const auto clamp_u8 = [](float v) -> uint8_t { return uint8_t(eastl::clamp(int(v), 0, 255)); };

  switch (type)
  {
    case SCTYPE_FLOAT1: memcpy(data, &val.x, 4); break;  // -V512
    case SCTYPE_FLOAT2: memcpy(data, &val.x, 8); break;  // -V512
    case SCTYPE_FLOAT3: memcpy(data, &val.x, 12); break; // -V512
    case SCTYPE_FLOAT4: memcpy(data, &val.x, 16); break; // -V512
    case SCTYPE_E3DCOLOR:
    {
      E3DCOLOR c;
      c.r = clamp_u8(val.x * 255.f);
      c.g = clamp_u8(val.y * 255.f);
      c.b = clamp_u8(val.z * 255.f);
      c.a = clamp_u8(val.w * 255.f);
      memcpy(data, &c.u, 4);
      break;
    }
    case SCTYPE_UBYTE4:
    {
      data[0] = clamp_u8(val.x);
      data[1] = clamp_u8(val.y);
      data[2] = clamp_u8(val.z);
      data[3] = clamp_u8(val.w);
      break;
    }
    case SCTYPE_SHORT2:
    {
      int16_t v[2] = {clamp_s16(val.x), clamp_s16(val.y)};
      memcpy(data, v, 4);
      break;
    }
    case SCTYPE_SHORT4:
    {
      int16_t v[4] = {clamp_s16(val.x), clamp_s16(val.y), clamp_s16(val.z), clamp_s16(val.w)};
      memcpy(data, v, 8);
      break;
    }
    case SCTYPE_SHORT2N:
    {
      int16_t v[2] = {clamp_s16(val.x * 32767.f), clamp_s16(val.y * 32767.f)};
      memcpy(data, v, 4);
      break;
    }
    case SCTYPE_SHORT4N:
    {
      int16_t v[4] = {clamp_s16(val.x * 32767.f), clamp_s16(val.y * 32767.f), clamp_s16(val.z * 32767.f), clamp_s16(val.w * 32767.f)};
      memcpy(data, v, 8);
      break;
    }
    case SCTYPE_USHORT2N:
    {
      uint16_t v[2] = {clamp_u16(val.x * 65535.f), clamp_u16(val.y * 65535.f)};
      memcpy(data, v, 4);
      break;
    }
    case SCTYPE_USHORT4N:
    {
      uint16_t v[4] = {clamp_u16(val.x * 65535.f), clamp_u16(val.y * 65535.f), clamp_u16(val.z * 65535.f), clamp_u16(val.w * 65535.f)};
      memcpy(data, v, 8);
      break;
    }
    case SCTYPE_UDEC3:
    {
      uint32_t x = uint32_t(eastl::clamp(int(val.x), 0, 1023));
      uint32_t y = uint32_t(eastl::clamp(int(val.y), 0, 1023));
      uint32_t z = uint32_t(eastl::clamp(int(val.z), 0, 1023));
      uint32_t u = x | (y << 10) | (z << 20);
      memcpy(data, &u, 4);
      break;
    }
    case SCTYPE_DEC3N:
    {
      auto clamp10 = [](float v) -> uint32_t {
        int32_t i = int32_t(eastl::clamp(int(v * 511.f), -511, 511));
        return uint32_t(i & 0x3FF);
      };
      uint32_t u = clamp10(val.x) | (clamp10(val.y) << 10) | (clamp10(val.z) << 20);
      memcpy(data, &u, 4);
      break;
    }
    case SCTYPE_HALF2:
    {
      uint16_t v[2] = {float_to_half(val.x), float_to_half(val.y)};
      memcpy(data, v, 4);
      break;
    }
    case SCTYPE_HALF4:
    {
      uint16_t v[4] = {float_to_half(val.x), float_to_half(val.y), float_to_half(val.z), float_to_half(val.w)};
      memcpy(data, v, 8);
      break;
    }
    case SCTYPE_UINT1:
    {
      uint32_t u = uint32_t(val.x);
      memcpy(data, &u, 4);
      break;
    }
    case SCTYPE_UINT2:
    {
      uint32_t v[2] = {uint32_t(val.x), uint32_t(val.y)};
      memcpy(data, v, 8);
      break;
    }
    case SCTYPE_UINT3:
    {
      uint32_t v[3] = {uint32_t(val.x), uint32_t(val.y), uint32_t(val.z)};
      memcpy(data, v, 12);
      break;
    }
    case SCTYPE_UINT4:
    {
      uint32_t v[4] = {uint32_t(val.x), uint32_t(val.y), uint32_t(val.z), uint32_t(val.w)};
      memcpy(data, v, 16);
      break;
    }
    default: break;
  }
}

static __forceinline Point4 channel_cvt_unpack_impl(const uint32_t type, const ChannelModifier mod, const uint8_t *data)
{
  // Step 1: decode raw bytes to float4 based on vertex type
  Point4 result;
  switch (type)
  {
    case SCTYPE_FLOAT1:
    {
      float x;
      memcpy(&x, data, 4);
      result = {x, 0, 0, 1};
      break;
    }
    case SCTYPE_FLOAT2:
    {
      float v[2];
      memcpy(v, data, 8);
      result = {v[0], v[1], 0, 1};
      break;
    }
    case SCTYPE_FLOAT3:
    {
      float v[3];
      memcpy(v, data, 12);
      result = {v[0], v[1], v[2], 1};
      break;
    }
    case SCTYPE_FLOAT4:
    {
      float v[4];
      memcpy(v, data, 16);
      result = {v[0], v[1], v[2], v[3]};
      break;
    }
    case SCTYPE_E3DCOLOR:
    {
      E3DCOLOR c;
      memcpy(&c.u, data, 4);
      result = {c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f};
      break;
    }
    case SCTYPE_UBYTE4:
    {
      result = {float(data[0]), float(data[1]), float(data[2]), float(data[3])};
      break;
    }
    case SCTYPE_SHORT2:
    {
      int16_t v[2];
      memcpy(v, data, 4);
      result = {float(v[0]), float(v[1]), 0, 1};
      break;
    }
    case SCTYPE_SHORT4:
    {
      int16_t v[4];
      memcpy(v, data, 8);
      result = {float(v[0]), float(v[1]), float(v[2]), float(v[3])};
      break;
    }
    case SCTYPE_SHORT2N:
    {
      int16_t v[2];
      memcpy(v, data, 4);
      result = {v[0] / 32767.f, v[1] / 32767.f, 0, 1};
      break;
    }
    case SCTYPE_SHORT4N:
    {
      int16_t v[4];
      memcpy(v, data, 8);
      result = {v[0] / 32767.f, v[1] / 32767.f, v[2] / 32767.f, v[3] / 32767.f};
      break;
    }
    case SCTYPE_USHORT2N:
    {
      uint16_t v[2];
      memcpy(v, data, 4);
      result = {v[0] / 65535.f, v[1] / 65535.f, 0, 1};
      break;
    }
    case SCTYPE_USHORT4N:
    {
      uint16_t v[4];
      memcpy(v, data, 8);
      result = {v[0] / 65535.f, v[1] / 65535.f, v[2] / 65535.f, v[3] / 65535.f};
      break;
    }
    case SCTYPE_UDEC3:
    {
      uint32_t u;
      memcpy(&u, data, 4);
      result = {float(u & 0x3FF), float((u >> 10) & 0x3FF), float((u >> 20) & 0x3FF), 1};
      break;
    }
    case SCTYPE_DEC3N:
    {
      uint32_t u;
      memcpy(&u, data, 4);
      auto sign_extend10 = [](uint32_t v) -> int32_t { return (v & 0x200) ? int32_t(v | 0xFFFFFC00u) : int32_t(v); };
      result = {
        sign_extend10(u & 0x3FF) / 511.f, sign_extend10((u >> 10) & 0x3FF) / 511.f, sign_extend10((u >> 20) & 0x3FF) / 511.f, 1};
      break;
    }
    case SCTYPE_HALF2:
    {
      uint16_t v[2];
      memcpy(v, data, 4);
      result = {half_to_float(v[0]), half_to_float(v[1]), 0, 1};
      break;
    }
    case SCTYPE_HALF4:
    {
      uint16_t v[4];
      memcpy(v, data, 8);
      result = {half_to_float(v[0]), half_to_float(v[1]), half_to_float(v[2]), half_to_float(v[3])};
      break;
    }
    case SCTYPE_UINT1:
    {
      uint32_t u;
      memcpy(&u, data, 4);
      result = {float(u), 0, 0, 1};
      break;
    }
    case SCTYPE_UINT2:
    {
      uint32_t v[2];
      memcpy(v, data, 8);
      result = {float(v[0]), float(v[1]), 0, 1};
      break;
    }
    case SCTYPE_UINT3:
    {
      uint32_t v[3];
      memcpy(v, data, 12);
      result = {float(v[0]), float(v[1]), float(v[2]), 1};
      break;
    }
    case SCTYPE_UINT4:
    {
      uint32_t v[4];
      memcpy(v, data, 16);
      result = {float(v[0]), float(v[1]), float(v[2]), float(v[3])};
      break;
    }
    default: result = {0, 0, 0, 0}; break;
  }

  switch (mod)
  {
    case CMOD_NONE: break;
    case CMOD_SIGNED_PACK: result = (result + Point4(1, 1, 1, 1)) * 0.5f; break;
    case CMOD_UNSIGNED_PACK: result = result * 2.f - Point4(1, 1, 1, 1); break;
    case CMOD_MUL_1K: result *= 1.f / (1 << 10); break;
    case CMOD_MUL_2K: result *= 1.f / (1 << 11); break;
    case CMOD_MUL_4K: result *= 1.f / (1 << 12); break;
    case CMOD_MUL_8K: result *= 1.f / (1 << 13); break;
    case CMOD_MUL_16K: result *= 1.f / (1 << 14); break;
    case CMOD_SIGNED_SHORT_PACK: result *= 1.f / 32767.f; break;
    case CMOD_BOUNDING_PACK:
    {
      G_LOGERR_ONCE_AND_DO(0, , "channel_cvt_unpack_impl: CMOD_BOUNDING_PACK is not supported");
      break;
    }
  }

  return result;
}


namespace frx
{

static __forceinline DestrMesh::Vertex parse_vertex(const ShaderMatChannels &desc, const uint8_t *data)
{
  DestrMesh::Vertex v;
  memset(&v, 0, sizeof(DestrMesh::Vertex));
#define UNPACK_CHANNEL(IDX) channel_cvt_unpack_impl(desc.channels[IDX].type, desc.channels[IDX].mod, data + desc.channels[IDX].offset)
  v.pos = Point3::xyz(UNPACK_CHANNEL(desc.posChIdx));
  if (desc.normChIdx >= 0)
    v.norm = Point3::xyz(UNPACK_CHANNEL(desc.normChIdx));
  if (desc.tcChIdx >= 0)
    v.tc = Point2::xy(UNPACK_CHANNEL(desc.tcChIdx));
#undef UNPACK_CHANNEL
  return v;
}

static __forceinline void pack_vertex(const ShaderMatChannels &desc, const DestrMesh::Vertex &v, uint8_t *data)
{
#define PACK_CHANNEL(IDX, VAL) \
  channel_cvt_pack_impl(desc.channels[IDX].type, desc.channels[IDX].mod, VAL, data + desc.channels[IDX].offset)
  PACK_CHANNEL(desc.posChIdx, Point4::xyz0(v.pos));
  if (desc.normChIdx >= 0)
    PACK_CHANNEL(desc.normChIdx, Point4::xyz0(v.norm));
  if (desc.tcChIdx >= 0)
    PACK_CHANNEL(desc.tcChIdx, Point4::xyz0(Point3::xy0(v.tc)));
#undef PACK_CHANNEL
}

} // namespace frx