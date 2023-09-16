#ifndef DAFX_DISPATCH_DESC_HLSL
#define DAFX_DISPATCH_DESC_HLSL

//
// cpu->gpu dispatch params for master shader (dafx_emission_shader/dafx_simulation_shader)
//
struct DispatchDesc
{
  uint headOffset;
  uint startAndCount; // packed 16b + 16b
  uint rndSeedAndCullingId; // packed 8b + 24b
  uint aliveStartAndCount;  // packed 16b + 16b
};
#define DAFX_BUCKET_GROUP_SIZE 4096

#endif