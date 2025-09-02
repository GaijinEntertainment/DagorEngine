#ifndef MOA_MATH
#define MOA_MATH

#ifdef UNPACKED_MOA_SHAPES
float dist_circle(float2 xy, UnpackedCircle circle)
{
  float2 dr = circle.center - xy;
  return length(dr) - circle.radius;
}

float dist_ring(float2 xy, UnpackedRing ring)
{
  float2 dr = ring.center - xy;
  float l = length(dr);

  float dist1 = l - ring.radiusFar;
  float dist2 = ring.radiusNear - l;

  return max(dist2, dist1);
}

float2 get_line_closest_point(float2 line_begin_to_point, float2 edge)
{
  float t = saturate(dot(line_begin_to_point,edge) / dot(edge,edge));
  return edge * t;
}

float dist_line(float2 xy, UnpackedLine ln)
{
  float2 beginToP = xy-ln.begin;
  float2 edge = ln.end-ln.begin;
  float2 closestPoint = get_line_closest_point(beginToP, edge);
  float dist = length( beginToP - closestPoint) - ln.halfWidth;
  return dist;
}

float dist_triangle(float2 xy, UnpackedTriangle tri)
{
  float2 e0 = tri.b - tri.a;
  float2 e1 = tri.c - tri.b;
  float2 e2 = tri.a - tri.c;

  float2 ap = xy - tri.a;
  float2 bp = xy - tri.b;
  float2 cp = xy - tri.c;

  float2 pToE0ClosestPoint = ap - get_line_closest_point(ap, e0);
  float2 pToE1ClosestPoint = bp - get_line_closest_point(bp, e1);
  float2 pToE2ClosestPoint = cp - get_line_closest_point(cp, e2);

  float2 e0N = float2(e0.y, -e0.x);
  float2 e1N = float2(e1.y, -e1.x);
  float2 e2N = float2(e2.y, -e2.x);

  float s = sign(dot(e0, e2N));

  float2 d0 = float2(dot(pToE0ClosestPoint,pToE0ClosestPoint), s * dot(ap, e0N));
  float2 d1 = float2(dot(pToE1ClosestPoint,pToE1ClosestPoint), s * dot(bp, e1N));
  float2 d2 = float2(dot(pToE2ClosestPoint,pToE2ClosestPoint), s * dot(cp, e2N));

  float2 d = min(min(d0,d1),d2);

  return -sqrt(d.x) * sign(d.y);
}

float dist_arc(float2 xy, UnpackedArc arc)
{
  float2 p = xy - arc.center;
  p = float2(
    dot(p, arc.rotation.xy),
    dot(p, arc.rotation.zw)
  );
  p.x = abs(p.x);

  bool isWithinArc = arc.edgePlaneN.x * p.x <= -arc.edgePlaneN.y * p.y;
  float ringDist = abs(length(p) - arc.radius) - arc.halfWidth;
  float edgeDist = length(p - arc.edgePoint) - arc.halfWidth;

  return isWithinArc ? ringDist : edgeDist;
}
#endif

float3 intersect_plane(float3 ray_pos, float3 ray_dir, float3 plane_center, float3 plane_normal)
{
  float t = dot(plane_normal, plane_center - ray_pos)
          / dot(plane_normal, ray_dir);
  return ray_pos + ray_dir * t;
}

float2 rad_to_moa(float2 v)
{
  return v * 180.0 / PI * 60.0;
}

float2 calc_dir_moa(float3 dir, float3 up, float3 right)
{
  float rightAngle = PI/2 - acos(dot(dir, right));
  float upAngle = PI/2 - acos(dot(dir, up));

  return rad_to_moa(float2(rightAngle, upAngle));
}

float2 calc_parallax_moa(float3 e2p, float3 eye_to_la_origin, float3 eye_to_pl_origin,
                         float3 lens_fwd_dir, float3 lens_right_dir, float3 lens_up_dir)
{
  float3 rayBegin = e2p;
  float3 rayDir = normalize(e2p);
  float3 e2ParallaxIntersection = intersect_plane(rayBegin, rayDir, eye_to_pl_origin, lens_fwd_dir);

  float3 laOrigin2ParallaxIntersection = e2ParallaxIntersection - eye_to_la_origin;
  return calc_dir_moa(normalize(laOrigin2ParallaxIntersection), lens_up_dir, lens_right_dir);
}

#endif