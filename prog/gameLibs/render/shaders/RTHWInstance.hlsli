static const int RT_INSTANCE_FLAG_NONE = 0x00;
static const int RT_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE = 0x01;
static const int RT_INSTANCE_FLAG_TRIANGLE_CULL_FLIP_WINDING = 0x02;
static const int RT_INSTANCE_FLAG_FORCE_OPAQUE = 0x04;
static const int RT_INSTANCE_FLAG_FORCE_NO_OPAQUE = 0x08;

struct RTHWInstance
{
  float4 transform0;
  float4 transform1;
  float4 transform2;
  uint instanceId_mask; // 24_8
  uint instanceOffset_flags; //24_8
  uint2 blas;
};
