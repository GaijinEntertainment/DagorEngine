#ifndef PROJECTIVE_DECAL_INSTANCE
#define PROJECTIVE_DECAL_INSTANCE 1
struct ProjectiveDecalInstance
{
  uint2 normal_du_id_matrixId; //31 bits normal, 17 bit tangent(du), 7 bit id, 9 matrix id
  uint2 params;
  float4 pos_size;
};
struct ProjectiveToUpdate
{
  float4 pos_size;
  float3 normal; uint decal_id;
  float3 up; uint tid_mid_flags; //7 bits texture id, 9 bit matrix id, 16 bits flags
  float4 params;
};
#endif