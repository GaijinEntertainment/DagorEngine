#ifndef BVH_MESH_META_INCLDUED
#define BVH_MESH_META_INCLDUED 1

#include "bvh_texcoord_formats.hlsli"

#define BVH_EMISSION_MODE_ALBEDO 0
#define BVH_EMISSION_MODE_LEGACY 1

#define BVH_EMISSION_MASK_NONE 0
#define BVH_EMISSION_MASK_DIFFUSE_ALPHA 1
#define BVH_EMISSION_MASK_EXTRA_TEXTURE 2

#define BVH_EMISSION_FADE_MAX_DISTANCE 4096.0f

struct BVHMeta
{
  float4 materialData1;
  float4 materialData2;
  uint4 layerData;
  uint initialized: 1;
  uint materialType: 31;
  uint alphaTextureIndex : 16;
  uint secondaryMaskTextureIndex : 16;
  uint ahsVertexBufferIndex : 20;
  uint forceNonMetal: 1;
  uint hasColorMod: 1;
  uint emissionMaskSource : 2;
  uint colorOffset: 8;
  uint indexCount : 16;
  uint texcoordOffset: 8;
  uint normalOffset: 8;
  uint indexBit: 1;
  uint texcoordFormatIndex : 3;
  uint emissionMode : 1;
  uint emissionInvertMask : 1;
  uint emissionSrgbMask : 1;
  uint emissionOnlyAtNight : 1;
  uint emissionPart : 4;
  uint emissionAlbedoMult : 4;
  uint emissionFadeStart : 8;
  uint emissionFadeEnd : 8;
  uint indexBufferIndex : 20;
  uint vertexStride: 8;
  uint vertexBufferIndexHigh: 4;
  uint vertexBufferIndexLow: 16;
  uint albedoTextureIndex : 16;
  uint normalTextureIndex : 16;
  uint extraTextureIndex: 16;
  uint startIndex;
  uint startVertex;
  float texcoordScale;
  uint atlasTileSize;
  uint atlasFirstLastTile;
  uint vertexOffset;
  uint texcoordAdd;
  uint emissionColor;
};

#define BVH_BINDLESS_BUFFER_MAX (0xFFFFF)
#define BVH_BINDLESS_BUFFER_LOW_MASK (0x0FFFF)
#define BVH_BINDLESS_BUFFER_HIGH_MASK (0xF0000)

#endif