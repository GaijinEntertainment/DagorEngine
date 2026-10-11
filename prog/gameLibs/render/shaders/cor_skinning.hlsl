#ifndef COR_SKINNING_HLSL
#define COR_SKINNING_HLSL 1

// constraint: the includer defines bone_t { float4 r0, r1, r2; }
// constraint: a bone is r0 = rotation quaternion, r1.xyz = position, r2.xyz = scale (dynrend calc_bone_tm)

#include "quaternion.hlsl"

float3x3 quat_to_float3x3(float4 q)
{
  float x = q.x, y = q.y, z = q.z, w = q.w;
  return float3x3(1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w),
                  2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w),
                  2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y));
}

// The row matrix of a bone, for the code that blends skinning matrices.
bone_t bone_matrix(bone_t b)
{
  float3x3 rot = quat_to_float3x3(b.r0);
  bone_t tm;
  tm.r0 = float4(rot[0] * b.r2.xyz, b.r1.x);
  tm.r1 = float4(rot[1] * b.r2.xyz, b.r1.y);
  tm.r2 = float4(rot[2] * b.r2.xyz, b.r1.z);
  return tm;
}

// constraint: must match encode_cor_offset in shSkinMeshData.cpp
float3 cor_offset_decode(float4 packed)
{
  float4 b = round(BGRA_SWIZZLE(packed) * 255);
  return (b.xyz - 256 * step(128, b.xyz)) * exp2(b.w * 0.125 - 25);
}

// cor_offset is the center of rotation relative to pos; zero gives the linear blend skinning position.
// A zero scale bone gives no rotation, and a vertex with a weight on it stays on linear blend skinning, so that it
// collapses with the bone.
// Antipodal signs are fixed against b0, which the skin builder writes as the heaviest bone.
void cor_skin(bone_t b0, bone_t b1, bone_t b2, bone_t b3, float4 bw, float3 pos, float3 cor_offset,
  out float3 skinned_pos, out float3x3 skinned_rot)
{
  float4 qw = bw * float4(any(b0.r2.xyz), any(b1.r2.xyz), any(b2.r2.xyz), any(b3.r2.xyz));
  if (any(qw != bw))
    cor_offset = 0;
  float4 q = b0.r0 * qw.x + b1.r0 * (dot(b1.r0, b0.r0) < 0 ? -qw.y : qw.y) +
             b2.r0 * (dot(b2.r0, b0.r0) < 0 ? -qw.z : qw.z) + b3.r0 * (dot(b3.r0, b0.r0) < 0 ? -qw.w : qw.w);
  skinned_rot = quat_to_float3x3(dot(q, q) > 0 ? normalize(q) : float4(0, 0, 0, 1));
  float3 center = pos + cor_offset;
  // constraint: a zero weight slot can index rows outside the skin bones, so the weight goes in before the rotation
  skinned_pos = b0.r1.xyz * bw.x + quat_rotate(b0.r0, b0.r2.xyz * bw.x * center) +
                b1.r1.xyz * bw.y + quat_rotate(b1.r0, b1.r2.xyz * bw.y * center) +
                b2.r1.xyz * bw.z + quat_rotate(b2.r0, b2.r2.xyz * bw.z * center) +
                b3.r1.xyz * bw.w + quat_rotate(b3.r0, b3.r2.xyz * bw.w * center) - mul(skinned_rot, cor_offset);
}

#endif
