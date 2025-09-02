#ifndef BVH_MESH_META_INCLDUED
#define BVH_MESH_META_INCLDUED 1

struct BVHMeta
{
  float4 materialData1;
  float4 materialData2;
  uint4 layerData;
  uint initialized: 1;
  uint materialType: 31;
  uint alphaTextureIndex : 16;
  uint alphaSamplerIndex : 16;
  uint ahsVertexBufferIndex : 20;
  uint padding: 4;
  uint colorOffset: 8;
  uint indexCount : 16;
  uint texcoordOffset: 8;
  uint normalOffset: 8;
  uint indexBit: 1;
  uint texcoordFormat : 31; // TODO this could be remapped to be much smaller
  uint indexBufferIndex : 20;
  uint vertexStride: 8;
  uint vertexBufferIndexHigh: 4;
  uint vertexBufferIndexLow: 16;
  uint albedoTextureIndex : 16;
  uint albedoAndNormalSamplerIndex: 16;
  uint normalTextureIndex : 16;
  uint extraTextureIndex: 16;
  uint extraSamplerIndex: 16;
  uint startIndex;
  uint startVertex;
  float texcoordScale;
  uint atlasTileSize;
  uint atlasFirstLastTile;
  uint vertexOffset;
  uint texcoordAdd;
};

#define BVH_BINDLESS_BUFFER_MAX (0xFFFFF)
#define BVH_BINDLESS_BUFFER_LOW_MASK (0x0FFFF)
#define BVH_BINDLESS_BUFFER_HIGH_MASK (0xF0000)

enum InstanceContributionToHitGroupIndexData
{
  ICTHGI_NO_PER_INSTANCE_DATA = 0,
  ICTHGI_LOAD_PER_INSTANCE_DATA_WITH_INSTANCE_INDEX = 1,
  ICTHGI_USE_AS_777_ISTANCE_COLOR = 2,
  ICTHGI_LOAD_PER_INSTANCE_DATA_WITH_ICTHGI_INDEX = 3,
  ICTHGI_REPLACE_ALBEDO_TEXTURE = 4,
  ICTHGI_USE_AS_HASH_VALUE = 5,
  ICTHGI_MASK = 7
};

#endif