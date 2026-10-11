#ifndef POINT_DISTANCE_HLSL_INCLUDED
#define POINT_DISTANCE_HLSL_INCLUDED 1

  float length_sq(float3 a) {return dot(a,a); }
  float distance_sq(float3 a, float3 b) { return length_sq(a - b); }

  float sqDistancePointBox(float3 bmin, float3 bmax, float3 p)
  {
    float3 distSq = max(max(bmin - p.xyz, p.xyz - bmax), 0);
    return dot(distSq, distSq);
  }

  float distancePointBoxSigned(float3 bmin, float3 bmax, float3 p)
  {
    float3 q = max(bmin - p.xyz, p.xyz - bmax);
    return min(max3(q.x, q.y, q.z), 0) + length(max(q, 0));
  }

  float sqDistanceBoxBox(float3 centerA, float3 extentA, float3 centerB, float3 extentB)
  {
    float3 axisDistances = max(abs(centerB - centerA) - (extentA + extentB), 0);
    return dot(axisDistances, axisDistances);
  }

  float sqDistanceBoxBoxScaled(float3 centerA, float3 extentA, float3 centerB, float3 extentB, float3 axisScale)
  {
    float3 axisDistances = max(abs(centerB - centerA) - (extentA + extentB), 0)*axisScale;
    return dot(axisDistances, axisDistances);
  }

  float pointTriangleDistanceSq(float3 p, float3 A, float3 B, float3 C)
  {
    float3 BA = B - A; float3 PA = p - A;
    float3 CB = C - B; float3 PB = p - B;
    float3 AC = A - C; float3 PC = p - C;

    float3 N = cross( BA, AC );
#if 1
    FLATTEN
    if (dot(cross(BA, N), PA) < 0 ||
        dot(cross(CB, N), PB) < 0 ||
        dot(cross(AC, N), PC) < 0)
      return min3(
          length_sq(BA*saturate(dot(BA, PA)/length_sq(BA)) - PA),
          length_sq(CB*saturate(dot(CB, PB)/length_sq(CB)) - PB),
          length_sq(AC*saturate(dot(AC, PC)/length_sq(AC)) - PC));
#else
    FLATTEN
    if (dot(cross(BA, N), PA) < 0)
      return distance_sq(BA*saturate(dot(BA, PA)/length_sq(BA)), PA);
    FLATTEN
    if (dot(cross(CB, N), PB) < 0)
      return distance_sq(CB*saturate(dot(CB, PB)/length_sq(CB)), PB);
    FLATTEN
    if (dot(cross(AC, N), PC) < 0)
      return distance_sq(AC*saturate(dot(AC, PC)/length_sq(AC)), PC);
      //return AC*saturate(dot(AC, PC)/length_sq(AC)) + C;
#endif
    //float3 Q = p - dot(PA, N) * N/dot(N, N);
    //return distance_sq(p, Q); // Point P is directly above/below the triangle
    return abs(dot(N, PA)*dot(N,PB)/dot(N, N));
  }

  struct TrianglePlanes
  {
    float3 A,B,C, BA, CB, AC, edgePlane1, edgePlane2, edgePlane3, N;
    float triPlane, twiceArea;
    float invLenSqBA, invLenSqAC, invLenSqCB;
  };
  TrianglePlanes get_triangle_planes(float3 A, float3 B, float3 C)
  {
    TrianglePlanes r;
    r.A = A;
    r.B = B;
    r.C = C;
    r.BA = (B - A);
    r.CB = (C - B);
    r.AC = (A - C);
    r.N = cross(r.BA, r.AC);
    r.edgePlane1 = cross(r.BA, r.N);
    r.edgePlane2 = cross(r.CB, r.N);
    r.edgePlane3 = cross(r.AC, r.N);
    r.twiceArea = length(r.N);
    r.N /= r.twiceArea;
    r.triPlane = dot(r.N, r.A);
    r.invLenSqBA = rcp(length_sq(r.BA));
    r.invLenSqAC = rcp(length_sq(r.AC));
    r.invLenSqCB = rcp(length_sq(r.CB));
    return r;
  }
  float pointTriangleSignedDistancePlanes(float3 p, TrianglePlanes tri)
  {
    float3 PA = p - tri.A;
    float3 PB = p - tri.B;
    float3 PC = p - tri.C;

    FLATTEN
    if (dot(tri.edgePlane1, PA) < 0 ||
        dot(tri.edgePlane2, PB) < 0 ||
        dot(tri.edgePlane3, PC) < 0)
      return sqrt(min3(
          length_sq(tri.BA*saturate(dot(tri.BA, PA)*tri.invLenSqBA) - PA),
          length_sq(tri.CB*saturate(dot(tri.CB, PB)*tri.invLenSqCB) - PB),
          length_sq(tri.AC*saturate(dot(tri.AC, PC)*tri.invLenSqAC) - PC)));
    //float3 Q = p - dot(PA, N) * N/dot(N, N);
    //return distance_sq(p, Q); // Point P is directly above/below the triangle
    float distSq = abs(dot(tri.N, PA)*dot(tri.N,PB));
    float dist = sqrt(distSq);
    return dot(tri.N, p) < tri.triPlane ? -dist : dist;
  }

  float pointTriangleSignedDistance(float3 p, float3 A, float3 B, float3 C)
  {
    float3 BA = B - A; float3 PA = p - A;
    float3 CB = C - B; float3 PB = p - B;
    float3 AC = A - C; float3 PC = p - C;

    float3 N = cross( BA, AC );
    FLATTEN
    if (dot(cross(BA, N), PA) < 0 ||
        dot(cross(CB, N), PB) < 0 ||
        dot(cross(AC, N), PC) < 0)
    {
      return sqrt(min3(
          length_sq(BA*saturate(dot(BA, PA)/length_sq(BA)) - PA),
          length_sq(CB*saturate(dot(CB, PB)/length_sq(CB)) - PB),
          length_sq(AC*saturate(dot(AC, PC)/length_sq(AC)) - PC)));
    }
    //float3 Q = p - dot(PA, N) * N/dot(N, N);
    //return distance_sq(p, Q); // Point P is directly above/below the triangle
    float distSq = abs(dot(N, PA)*dot(N,PB)/dot(N, N));
    float dist = sqrt(distSq);
    return dot(N, p) < dot(N, A) ? -dist : dist;
  }


  float3 closestPointOnTriangle(float3 p, float3 A, float3 B, float3 C)
  {
    float3 BA = B - A; float3 PA = p - A;
    float3 CB = C - B; float3 PB = p - B;
    float3 AC = A - C; float3 PC = p - C;

    float3 N = cross( BA, AC );
    #if 0
    FLATTEN
    if (dot(cross(BA, N), PA) < 0
        || dot(cross(CB, N), PB) < 0
        || dot(cross(AC, N), PC) < 0)
    {
      float3 BAP = BA*saturate(dot(BA, PA)/length_sq(BA)) + A;
      float3 CBP = CB*saturate(dot(CB, PB)/length_sq(CB)) + B;
      float3 ACP = AC*saturate(dot(AC, PC)/length_sq(AC)) + C;
      float l1 = distance_sq(BAP, p), l2 = distance_sq(CBP, p), l3 = distance_sq(ACP, p);
      return (l1 < l2 && l1 < l3) ? BAP : (l2 < l3 ? CBP : ACP);

    }
    #else
    FLATTEN
    if (dot(cross(BA, N), PA) < 0)
      return BA*saturate(dot(BA, PA)/length_sq(BA)) + A;
    FLATTEN
    if (dot(cross(CB, N), PB) < 0)
      return CB*saturate(dot(CB, PB)/length_sq(CB)) + B;
    FLATTEN
    if (dot(cross(AC, N), PC) < 0)
      return AC*saturate(dot(AC, PC)/length_sq(AC)) + C;
    #endif
    //point is within edge planes, just return projection on plane
    return p - (dot(PA, N) * N)/length_sq(N);
  }

  // Ericson's edge test (0 <= num <= len, outside the edge); len > 0 keeps a zero-length edge of a
  // degenerate triangle from claiming the point that the other two edges own
  bool closestPointOnEdgeRegion(float num, float len, float area) { return num >= 0 && num <= len && area <= 0 && len > 0; }

  // Closest point to p on triangle ABC by Ericson's Voronoi regions (RTCD 5.1.5), slivers included.
  // constraint: dag_swBLAS_leaf.h closestPointOnTriVec picks the same regions, but answers a sliver by its nearest edge
  float3 closestPointOnTriangleBarycentric(float3 p, float3 A, float3 B, float3 C)
  {
    float3 AB = B - A, AC = C - A, BC = C - B;
    float3 PA = p - A, PB = p - B;
    float3 N = cross(AB, AC);
    // per edge (AB and AC from A, BC from B): numerator, squared length, and the area on the far
    // side of the edge as a triple product, whose error scales with |N| and stays exact for slivers
    float3 num = float3(dot(AB, PA), dot(AC, PA), dot(BC, PB));
    float3 len = float3(dot(AB, AB), dot(AC, AC), dot(BC, BC));
    float3 area = float3(dot(cross(N, AB), PA), dot(cross(AC, N), PA), dot(cross(N, BC), PB));
    float3 t = num / max(len, 1e-37);
    bool3 lo = num <= 0, hi = num >= len;

    // lowest priority first, so each later select overrides, in Ericson's test order
    float2 vw = float2(area.y, area.x) * rcp(max(dot(N, N), 1e-37)); // weights of B and C
    vw = closestPointOnEdgeRegion(num.z, len.z, area.z) ? float2(1 - t.z, t.z) : vw;
    vw = closestPointOnEdgeRegion(num.y, len.y, area.y) ? float2(0, t.y) : vw;
    vw = hi.y && hi.z ? float2(0, 1) : vw;
    vw = closestPointOnEdgeRegion(num.x, len.x, area.x) ? float2(t.x, 0) : vw;
    vw = hi.x && lo.z ? float2(1, 0) : vw;
    vw = lo.x && lo.y ? float2(0, 0) : vw;
    return A + AB * vw.x + AC * vw.y;
  }

#endif
