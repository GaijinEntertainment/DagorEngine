#include <landMesh/lmeshManager.h>
#include <math/dag_bounds2.h>
#include "../collisionGlobals.h"

namespace rendinst::gen
{
float custom_max_trace_distance = 100;
bool custom_trace_ray(const Point3 &src, const Point3 &dir, real &dist, Point3 *out_norm)
{
  G_UNUSED(src);
  G_UNUSED(dir);
  G_UNUSED(dist);
  G_UNUSED(out_norm);
  return false;
}
bool custom_trace_ray_earth(const Point3 &src, const Point3 &dir, real &dist)
{
  LandMeshManager *lmeshMgr = dacoll::get_lmesh();
  return lmeshMgr ? lmeshMgr->traceray(src, dir, dist, NULL) : false;
}
bool custom_get_height(Point3 &pos, Point3 *out_norm)
{
  LandMeshManager *lmeshMgr = dacoll::get_lmesh();
  return lmeshMgr ? lmeshMgr->getHeight(Point2::xz(pos), pos.y, out_norm) : false;
}
vec3f custom_update_pregen_pos_y(vec4f pos, int16_t *dest_packed_y, float csz_y, float oy)
{
  G_UNUSED(pos);
  G_UNUSED(dest_packed_y);
  G_UNUSED(csz_y);
  G_UNUSED(oy);
  return pos;
}

void custom_get_land_min_max(BBox2 bbox_xz, float &out_min, float &out_max)
{
  const float LAND_HEIGHT_DELTA = 100.0f;
  out_min = MAX_REAL;
  out_max = MIN_REAL;
  LandMeshManager *lmeshMgr = dacoll::get_lmesh();
  if (lmeshMgr)
  {
    const float eps = 1e-3;
    for (int z = floorf(bbox_xz.lim[0].y / lmeshMgr->getLandCellSize() + eps);
         z <= floorf(bbox_xz.lim[1].y / lmeshMgr->getLandCellSize() - eps); z++)
    {
      for (int x = floorf(bbox_xz.lim[0].x / lmeshMgr->getLandCellSize() + eps);
           x <= floorf(bbox_xz.lim[1].x / lmeshMgr->getLandCellSize() - eps); x++)
      {
        BBox3 bbox = lmeshMgr->getBBox(x, z);
        out_min = min(out_min, bbox.lim[0].y);
        out_max = max(out_max, bbox.lim[1].y);
      }
    }
  }

  if (out_min > out_max)
  {
    const float land_height_fallback = 8192.f;
    out_min = 0.f;
    out_max = land_height_fallback;
  }
  else
  {
    out_min -= LAND_HEIGHT_DELTA / 5.0f;
    out_max += LAND_HEIGHT_DELTA;
  }
}
} // namespace rendinst::gen
