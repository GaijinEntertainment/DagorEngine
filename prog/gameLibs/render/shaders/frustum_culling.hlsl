
bool baseTestSphereB(float3 center, float radius, float4 planes03X, float4 planes03Y, float4 planes03Z, float4 planes03W, float4 plane4, float4 plane5)
{
  float4 res03;
  res03 = center.x*planes03X + planes03W;
  res03 = center.y*planes03Y + res03;
  res03 = center.z*planes03Z + res03;
  res03 = res03 + radius.xxxx;
  bool comp = all(bool4(res03 >= 0));
  comp = ((dot(center, plane4.xyz)+radius+plane4.w) >= 0) & comp;
  comp = ((dot(center, plane5.xyz)+radius+plane5.w) >= 0) & comp;
  return comp;
}

bool baseTestBoxExtentB(float3 center, float3 extent, float4 planes03X, float4 planes03Y, float4 planes03Z, float4 planes03W, float4 plane4, float4 plane5)
{
  float4 res03;
  res03 = (center.xxxx + (extent.x * sign(planes03X))) * planes03X + planes03W;
  res03 = (center.yyyy + (extent.y * sign(planes03Y))) * planes03Y + res03;
  res03 = (center.zzzz + (extent.z * sign(planes03Z))) * planes03Z + res03;
  bool comp = all(bool4(res03 >= 0));
  comp = (dot(((extent * sign(plane4.xyz)) + center.xyz), plane4.xyz) + plane4.w >=0 ) & comp;
  comp = (dot(((extent * sign(plane5.xyz)) + center.xyz), plane5.xyz) + plane5.w >=0 ) & comp;

  return comp;
}

void get_frustum_planes(float4x4 m2, out float4 planes03X, out float4 planes03Y, out float4 planes03Z, out float4 planes03W, out float4 plane4, out float4 plane5)
{
  float4 camPlanes0 = (m2[3] - m2[0]);  // right
  float4 camPlanes1 = (m2[3] + m2[0]);  // left
  float4 camPlanes2 = (m2[3] - m2[1]);  // top
  float4 camPlanes3 = (m2[3] + m2[1]);  // bottom
  plane4 = (m2[3] - m2[2]);  // far
  plane5 = m2[2];  // near
  planes03X = float4(camPlanes0.x, camPlanes1.x, camPlanes2.x, camPlanes3.x);
  planes03Y = float4(camPlanes0.y, camPlanes1.y, camPlanes2.y, camPlanes3.y);
  planes03Z = float4(camPlanes0.z, camPlanes1.z, camPlanes2.z, camPlanes3.z);
  planes03W = float4(camPlanes0.w, camPlanes1.w, camPlanes2.w, camPlanes3.w);
}
