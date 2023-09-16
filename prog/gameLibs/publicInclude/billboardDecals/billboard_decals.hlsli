#ifndef BILLBOARD_DECAL_INSTANCE
#define BILLBOARD_DECAL_INSTANCE 1
struct BillboardDecalInstance
{
  uint2 normal_id;
  uint2 du_matrixId;
  float4 pos_size;
};
struct BillboardToUpdate
{
  float4 pos_size;
  float3 normal; uint texture_id;
  float3 up; uint matrix_id;
  // padding is unused, but it's needed to have the correct alignment
  uint id; float pad0; float pad1; float pad2;
};
#endif
