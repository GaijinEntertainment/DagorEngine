half3 inv_octahedral_mapping(half2 tc, half zoom, bool rotate)
{
  tc = (tc * 2 - 1)/zoom;
  if (rotate)
    tc = half2(tc.x - tc.y, tc.x + tc.y) / 2;
  half3 dir = half3(tc.xy, 1.0h - (abs(tc.x) + abs(tc.y)));
  if (dir.z < 0)
    dir.xy = (1.0h - abs(dir.yx)) * half2(sign(dir.xy));
  return normalize(dir);
}

half2 octahedral_mapping(half3 co, half zoom, bool rotate)
{
  co /= dot(half3(1, 1, 1), abs(co));
  co.xy = co.y < 0.0
    ? (1.0h - abs(co.zx)) * select(co.xz < 0, half2(-1, -1), half2(1, 1))
    : co.xz;
  if (rotate)
  {
    half tempX = co.x;
    co.x = (co.x + co.y);
    co.y = (co.y - tempX);
  }
  co.x *= zoom;
  co.y *= zoom;
  return co.xy * 0.5h + 0.5h;
}
