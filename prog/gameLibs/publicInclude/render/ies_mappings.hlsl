float3 inv_octahedral_mapping(float2 tc, float zoom, bool rotate)
{
  tc = (tc * 2 - 1)/zoom;
  if (rotate)
    tc = float2(tc.x - tc.y, tc.x + tc.y) / 2;
  float3 dir = float3(tc.xy, 1.0 - (abs(tc.x) + abs(tc.y)));
  if (dir.z < 0)
    dir.xy = float2(-(abs(dir.y) - 1) * sign(dir.x), -(abs(dir.x) - 1) * sign(dir.y));
  return normalize(dir);
}

half2 octahedral_mapping(half3 co, float zoom, bool rotate)
{
  co /= dot(half3(1, 1, 1), abs(co));
  co.xy = co.y < 0.0
    ? (1.0 - abs(co.zx)) * (co.xz < 0 ? float2(-1, -1) : float2(1, 1))
    : co.xz;
  if (rotate)
  {
    float tempX = co.x;
    co.x = (co.x + co.y);
    co.y = (co.y - tempX);
  }
  co.x *= zoom;
  co.y *= zoom;
  return co.xy * 0.5 + 0.5;
}
