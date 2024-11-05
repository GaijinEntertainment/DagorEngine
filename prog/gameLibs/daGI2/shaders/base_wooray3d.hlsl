#ifndef WOORAY3D_INCLUDED
#define WOORAY3D_INCLUDED 1
  #include <ray_box_intersect.hlsl>
  struct WooRay3d
  {
    float3 p, wdir;
    int3 pt, stepCell;
    float3 tMax, tDelta;
  };

  WooRay3d initWoo(float3 p, float3 dir, float leafSize)
  {
    WooRay3d ret;
    ret.p = p;
    ret.wdir = dir;
    ret.pt = int3(floor(p / leafSize));
    ret.stepCell = dir >= 0.0f ? 1 : -1;
    int3 nextPt = ret.pt + max(0, ret.stepCell);
    float3 absDir = abs(dir);
    ret.tDelta = absDir > 1e-9f ? leafSize / absDir : 0;

    // this calculations should be made in doubles for precision
    ret.tMax = absDir > 1e-9 ? (nextPt * leafSize - p) / dir : 1e15;
    return ret;
  }

  void nextCell(inout WooRay3d ray, inout float t)
  {
    if (ray.tMax.x < ray.tMax.y) {
      if (ray.tMax.x < ray.tMax.z) {
        ray.pt.x += ray.stepCell.x;
        t = ray.tMax.x;
        ray.tMax.x += ray.tDelta.x;
      } else {
        ray.pt.z += ray.stepCell.z;
        t = ray.tMax.z;
        ray.tMax.z += ray.tDelta.z;
      }
    } else {
      if (ray.tMax.y < ray.tMax.z) {
        ray.pt.y += ray.stepCell.y;
        t = ray.tMax.y;
        ray.tMax.y += ray.tDelta.y;
      } else {
        ray.pt.z += ray.stepCell.z;
        t = ray.tMax.z;
        ray.tMax.z += ray.tDelta.z;
      }
    }
  }

  float3 ray_box_intersect_normal(float3 wpos, float3 wdir, float3 bmin, float3 bmax)
  {
    float3 cb = (wdir >= 0.0f) ? bmin : bmax;

    float3 rzr = 1.0 / wdir;
    bool3 nonzero = (abs(wdir) > 1e-6);
    float3 startOfs = nonzero ? max(0, (cb - wpos) * rzr) : 0;
    float maxStart = max3(startOfs.x, startOfs.y, startOfs.z);
    return -(maxStart == startOfs.x ? float3(sign(wdir.x),0,0) : maxStart == startOfs.y ? float3(0, sign(wdir.y),0) : float3(0, 0, sign(wdir.z)));
  }

#endif
