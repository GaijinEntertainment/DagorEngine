
bool baseTestSphereB(float3 center, float radius, float4 planes[6])
{
  float4 res03;
  res03 = center.x*planes[0] + planes[3];
  res03 = center.y*planes[1] + res03;
  res03 = center.z*planes[2] + res03;
  res03 = res03 + radius.xxxx;
  bool comp = all(bool4(res03 >= 0));
  comp = ((dot(center, planes[4].xyz)+radius+planes[4].w) >= 0) & comp;
  comp = ((dot(center, planes[5].xyz)+radius+planes[5].w) >= 0) & comp;
  return comp;
}

bool baseTestBoxExtentB(float3 center, float3 extent, float4 planes[6])
{
  float4 res03;
  res03 = (center.xxxx + (extent.x * sign(planes[0]))) * planes[0] + planes[3];
  res03 = (center.yyyy + (extent.y * sign(planes[1]))) * planes[1] + res03;
  res03 = (center.zzzz + (extent.z * sign(planes[2]))) * planes[2] + res03;
  bool comp = all(bool4(res03 >= 0));
  comp = (dot(((extent * sign(planes[4].xyz)) + center.xyz), planes[4].xyz) + planes[4].w >=0 ) & comp;
  comp = (dot(((extent * sign(planes[5].xyz)) + center.xyz), planes[5].xyz) + planes[5].w >=0 ) & comp;

  return comp;
}

bool baseTestBoxExtentIntersects(float3 center, float3 extent, float4 planes[6])
{
  float4 res03;
  res03 = (center.xxxx - extent.x * sign(planes[0])) * planes[0] + planes[3];
  res03 = (center.yyyy - extent.y * sign(planes[1])) * planes[1] + res03;
  res03 = (center.zzzz - extent.z * sign(planes[2])) * planes[2] + res03;

  bool outside = all(bool4(res03 >= 0));
  outside = (dot(center.xyz - extent * sign(planes[4].xyz), planes[4].xyz) + planes[4].w >=0 ) & outside;
  outside = (dot(center.xyz - extent * sign(planes[5].xyz), planes[5].xyz) + planes[5].w >=0 ) & outside;
  return !outside;
}

uint baseTestBoxExtent(float3 center, float3 extent, float4 planes[6])
{

  float4 res03;
  res03 = (center.xxxx + extent.x * sign(planes[0])) * planes[0] + planes[3];
  res03 = (center.yyyy + extent.y * sign(planes[1])) * planes[1] + res03;
  res03 = (center.zzzz + extent.z * sign(planes[2])) * planes[2] + res03;
  bool inside = all(bool4(res03 >= 0));
  inside = (dot(center.xyz + extent * sign(planes[4].xyz), planes[4].xyz) + planes[4].w >=0 ) & inside;
  inside = (dot(center.xyz + extent * sign(planes[5].xyz), planes[5].xyz) + planes[5].w >=0 ) & inside;

  bool intersects = baseTestBoxExtentIntersects(center, extent, planes);
  return inside ? 1+uint(intersects) : 0;
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
