//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
// NOTE: this file is only provided for compatibility of some tools
//       that rely on constants that are defined here, those tools
//       should be updated to use compiled_meta_data.h instead and
//       this file should be removed then (for example hlsl2msl
//       tool needs this file).
// contains the layout of compiled shaders as spir-v
namespace spirv
{
const uint32_t VERTEX_SHADER_TEXTURE_UNITS = 4;
const uint32_t FRAGMENT_SHADER_TEXTURE_UNITS = 16;
const uint32_t FRAGMENT_SHADER_UAV_UNITS = 8;

const uint32_t PER_TEXTURE_UNIT_TYPE_BITS = 2;

const uint32_t SHADER_UNIFORM_BUFFER_DESCRIPTOR_SET = 0;
const uint32_t VERTEX_SHADER_TEXUTRE_DESCRIPTOR_SET = 2;
const uint32_t FRAGMENT_SHADER_TEXTURE_DESCRIPTOR_SET = 1;
const uint32_t FRAGMENT_SHADER_UAV_SET = 3;

enum TextureType
{
  TT_2D = 0,
  TT_3D = 1,
  TT_CUBE = 2,
  TT_ARRAY = 3,

  TT_COUNT = 4,
};

typedef uint16_t TextureUnitBits_t;
typedef uint32_t TextureUnitType_t;
typedef uint8_t UAVUnitBits_t;
typedef uint8_t Hash_t[20];

struct SpirVHeader
{
  Hash_t hash;
  TextureUnitBits_t texUnitMask;
  TextureUnitBits_t texUnitDepthMask;
  TextureUnitType_t texUnitTypes;
  UAVUnitBits_t uavUnitMask;
  uint8_t padding[sizeof(uint32_t) - sizeof(UAVUnitBits_t)];
};

// sole purpose of this is to trigger the static asserts here, this header is used
// in tools and engine/drv
inline void do_trigger_static_assert_if_not_matching()
{
  G_STATIC_ASSERT(FRAGMENT_SHADER_TEXTURE_UNITS <= sizeof(TextureUnitBits_t) * 8);
  G_STATIC_ASSERT(VERTEX_SHADER_TEXTURE_UNITS <= sizeof(TextureUnitBits_t) * 8);
  G_STATIC_ASSERT(FRAGMENT_SHADER_TEXTURE_UNITS <= sizeof(TextureUnitType_t) * 8 / PER_TEXTURE_UNIT_TYPE_BITS);
  G_STATIC_ASSERT(VERTEX_SHADER_TEXTURE_UNITS <= sizeof(TextureUnitType_t) * 8 / PER_TEXTURE_UNIT_TYPE_BITS);
  G_STATIC_ASSERT(FRAGMENT_SHADER_UAV_UNITS <= sizeof(UAVUnitBits_t) * 8);
}
} // namespace spirv
