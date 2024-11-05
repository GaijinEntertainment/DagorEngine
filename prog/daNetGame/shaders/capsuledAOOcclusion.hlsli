#ifndef CAPSULE_AO_OCCLUSION
#define CAPSULE_AO_OCCLUSION 1

#define MAX_AO_CAPSULES 10
#define AO_CAPSULES_PACK_BITS 8
#define MAX_AO_CAPSULES_DISTANCE 32.0f
#define AO_AROUND_UNIT 1.0f
#define UNITS_AO_GRID_SIZE 32

/*struct AOCapsuleDecoded
{
  float4 ptA_radius;
  float4 ptB_opacity;
};*/
struct AOCapsule
{
  uint2 ptARadius_ptB;
  //float3 ptA; float radius;
  //float3 ptB; float opacity;
};
struct CapsuledAOUnit
{
  float4 boxCenter, boxSize;
  AOCapsule capsules[MAX_AO_CAPSULES];
};
#endif

