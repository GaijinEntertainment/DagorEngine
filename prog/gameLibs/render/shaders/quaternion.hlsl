#ifndef QUATERNION_HLSL
#define QUATERNION_HLSL

// rotate vector v by unit quaternion q (matches vecmath v_quat_mul_vec3)
float3 quat_rotate(float4 q, float3 v)
{
  float3 t = 2 * cross(q.xyz, v);
  return v + q.w * t + cross(q.xyz, t);
}

#endif
