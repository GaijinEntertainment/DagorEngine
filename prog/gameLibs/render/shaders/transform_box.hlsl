#ifndef TRANSFORM_BOX_HLSL
#define TRANSFORM_BOX_HLSL 1

void transform_box(out float3 ret_bmin, out float3 ret_bmax, float3 bmin, float3 bmax, float3 col0, float3 col1, float3 col2)
{
  float3 boxMulM_0_0 = bmin.x * col0;
  float3 boxMulM_0_1 = bmax.x * col0;
  float3 boxMulM_1_0 = bmin.y * col1;
  float3 boxMulM_1_1 = bmax.y * col1;
  float3 boxMulM_2_0 = bmin.z * col2;
  float3 boxMulM_2_1 = bmax.z * col2;

  // Summing y and z
  float3 boxSum_0_0 = (boxMulM_1_0 + boxMulM_2_0);
  float3 boxSum_0_1 = (boxMulM_1_0 + boxMulM_2_1);
  float3 boxSum_1_0 = (boxMulM_1_1 + boxMulM_2_0);
  float3 boxSum_1_1 = (boxMulM_1_1 + boxMulM_2_1);
#define COMBINE_BOX(a,b,c) {float3 p = (boxMulM_0_##a + boxSum_##b##_##c); ret_bmin = min(ret_bmin, p);ret_bmax = max(ret_bmax, p);}
  ret_bmin = ret_bmax = boxMulM_0_0 + boxSum_0_0;
  COMBINE_BOX(0, 0, 1);
  COMBINE_BOX(0, 1, 0);
  COMBINE_BOX(0, 1, 1);
  COMBINE_BOX(1, 0, 0);
  COMBINE_BOX(1, 0, 1);
  COMBINE_BOX(1, 1, 0);
  COMBINE_BOX(1, 1, 1);
#undef COMBINE_BOX
}

#endif