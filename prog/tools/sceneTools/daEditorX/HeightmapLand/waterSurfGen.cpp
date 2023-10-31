#include "hmlPlugin.h"
#include "hmlSplineObject.h"
#include <de3_interface.h>
#include <de3_genObjUtil.h>
#include <de3_rasterizePoly.h>
#include <dllPluginCore/core.h>
#include <libTools/staticGeom/matFlags.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/util/svgWrite.h>

#include <generic/dag_sort.h>
#include <math/dag_mesh.h>
#include <math/integer/dag_IPoint4.h>
#include <util/dag_bfind.h>
#include <util/dag_fastIntList.h>
#include <util/dag_hierBitArray.h>
#include <perfMon/dag_cpuFreq.h>

#include <math/DelaunayTriangulator/source/DelaunayTriangulation.h>
#include <math/DelaunayTriangulator/source/TIN.h>
#include <math/DelaunayTriangulator/source/Util.h>
#include <math/clipper-4.x/clipper.hpp>
#include <math/misc/polyLineReduction.h>
#include <algorithm>

extern bool allow_debug_bitmap_dump;

class Point3Deref : public Point3
{
public:
  Point3 getPt() const { return *this; }
};

#if 1
static void draw_poly(objgenerator::HugeBitmask &m, dag::ConstSpan<Point2> pts, float ofs_x, float ofs_y, float scale)
{
  for (int i = 0; i < pts.size(); i++)
  {
    Point2 p0 = pts[i];
    Point2 d = pts[(i + 1) % pts.size()] - p0;
    float len = length(d);
    if (len < 1e-36)
      continue;
    for (float t = 0.0, dt = 1.f / (len * scale * 4); t <= 1; t += dt)
      m.set((p0.x + d.x * t - ofs_x) * scale, (p0.y + d.y * t - ofs_y) * scale);
  }
}
static void write_tif(objgenerator::HugeBitmask &m, const char *fname)
{
  IBitMaskImageMgr::BitmapMask img;
  int w = m.getW(), h = m.getH();
  HmapLandPlugin::bitMaskImgMgr->createBitMask(img, w, h, 1);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      img.setMaskPixel1(x, y, m.get(x, y) ? 128 : 0);

  HmapLandPlugin::bitMaskImgMgr->saveImage(img, ".", fname);
  HmapLandPlugin::bitMaskImgMgr->destroyImage(img);
}
static void draw_solution(objgenerator::HugeBitmask &m, const ClipperLib::ExPolygons &sol, float ox, float oz, float scl)
{
  Tab<Point2> pts_2(tmpmem);
  Tab<Point3> pts_3(tmpmem);
  Tab<Point3Deref *> pts(tmpmem);

  // debug("sol=%d", sol.size());
  for (int i = 0; i < sol.size(); i++)
  {
    // debug_(" %d: outer sz=%d ", i, sol[i].outer.size());
    pts_2.resize(sol[i].outer.size());
    for (int j = 0; j < sol[i].outer.size(); j++)
    {
      pts_2[j].set(sol[i].outer[j].X * 0.001f, sol[i].outer[j].Y * 0.001f);
      // debug_(" %.3f,%.3f ", sol[i].outer[j].X*0.001f, sol[i].outer[j].Y*0.001f);
    }
    // debug("");
    draw_poly(m, pts_2, ox, oz, scl);

    for (int k = 0; k < sol[i].holes.size(); k++)
    {
      // debug_("  holes[%d]: sz=%d ", k, sol[i].holes[k].size());
      pts_3.resize(sol[i].holes[k].size());
      pts.resize(pts_3.size());
      for (int j = 0; j < sol[i].holes[k].size(); j++)
      {
        pts_3[j].set(sol[i].holes[k][j].X * 0.001f, 0, sol[i].holes[k][j].Y * 0.001f);
        pts[j] = static_cast<Point3Deref *>(&pts_3[j]);
        // debug_(" %.3f,%.3f ", sol[i].holes[k][j].X*0.001f, sol[i].holes[k][j].Y*0.001f);
      }
      // debug("");
      rasterize_poly_2_nz(m, pts, ox, oz, scl);
    }
  }
}

static void draw_solution(const char *svg_fn, const ClipperLib::ExPolygons &sol, float ox, float oz, float scl, int w = 512,
  int h = 512, float over_scl = 1.0)
{
  FILE *fp = fp = svg_open(svg_fn, w * over_scl, h * over_scl);
  ox *= over_scl;
  oz *= over_scl;
  scl /= over_scl;

  // debug("sol=%d", sol.size());
  for (int i = 0; i < sol.size(); i++)
  {
    svg_begin_group(fp, "stroke-width=\"0.005\" stroke=\"black\" opacity=\"0.8\" fill=\"red\"");
    svg_start_poly(fp, Point2(sol[i].outer[0].X * 0.001 - ox, sol[i].outer[0].Y * 0.001 - oz) * scl);
    for (int k = 1; k < sol[i].outer.size(); k++)
      svg_add_poly_point(fp, Point2(sol[i].outer[k].X * 0.001 - ox, sol[i].outer[k].Y * 0.001 - oz) * scl);
    svg_end_poly(fp);
    svg_end_group(fp);

    // debug_(" %d: outer(%d) sz=%d ", i, ClipperLib::Orientation(sol[i].outer), sol[i].outer.size());
    // for (int j = 0; j < sol[i].outer.size(); j++)
    //   debug_(" %.3f,%.3f ", sol[i].outer[j].X*0.001f, sol[i].outer[j].Y*0.001f);
    // debug("");

    for (int k = 0; k < sol[i].holes.size(); k++)
    {
      svg_begin_group(fp, "stroke-width=\"0.005\" stroke=\"blue\" opacity=\"0.5\" fill=\"green\"");
      svg_start_poly(fp, Point2(sol[i].holes[k][0].X * 0.001 - ox, sol[i].holes[k][0].Y * 0.001 - oz) * scl);
      for (int j = 1; j < sol[i].holes[k].size(); j++)
        svg_add_poly_point(fp, Point2(sol[i].holes[k][j].X * 0.001 - ox, sol[i].holes[k][j].Y * 0.001 - oz) * scl);
      svg_end_poly(fp);
      svg_end_group(fp);
      // debug_("  holes[%d]: (%d) sz=%d ",
      //   k, ClipperLib::Orientation(sol[i].holes[k]), sol[i].holes[k].size());
      // for (int j = 0; j < sol[i].holes[k].size(); j++)
      //   debug_(" %.3f,%.3f ", sol[i].holes[k][j].X*0.001f, sol[i].holes[k][j].Y*0.001f);
      // debug("");
    }
  }
  svg_close(fp);
}
#endif

static void optimize_polygon(ClipperLib::Polygon &poly, float tolerance = 0.01)
{
  Tab<double> pt(tmpmem);
  Tab<int> ptmark(tmpmem);
  int n = (int)poly.size(), original_n = n;
  pt.resize(n * 2);
  ptmark.resize(n);
  mem_set_0(ptmark);

  for (int i = 0; i < n; i++)
  {
    pt[i] = poly[i].X;
    pt[i + n] = poly[i].Y;
  }
  ReducePoints(pt.data(), pt.data() + n, n, ptmark.data(), tolerance * 1000);
  if (poly[0].X == poly[n - 1].X && poly[0].Y == poly[n - 1].Y)
    n--;
  poly.resize(1);
  for (int i = 1; i < n; i++)
    if (ptmark[i])
      poly.push_back(ClipperLib::IntPoint(pt[i], pt[i + n]));
  if (original_n > poly.size())
    debug("optimized poly %d->%d", original_n, poly.size());
}

// rasterize polygon on pre-created bitmap, non-zero filling rule
static void rasterize_poly_2_nz(objgenerator::HugeBitmask &bm, ClipperLib::ExPolygons &ep, float ofs_x, float ofs_z, float scale)
{
  TabSortedInline<IPoint2, Ipoint2CmpLambda> pos(tmpmem);
  SmallTab<Point4, TmpmemAlloc> seg;
  SmallTab<char, TmpmemAlloc> seg_dy;
  Point2 p0, p1;
  int w = bm.getW(), h = bm.getH();
  float cell = 1.0f / scale;

  // compute bounds
  int seg_cnt = 0;
  for (int k = 0; k < ep.size(); k++)
  {
    seg_cnt += (int)ep[k].outer.size();
    for (int j = 0; j < ep[k].holes.size(); j++)
      seg_cnt += (int)ep[k].holes[j].size();
  }

  clear_and_resize(seg, seg_cnt);
  clear_and_resize(seg_dy, seg_cnt);
  seg_cnt = 0;
  for (int k = 0; k < ep.size(); k++)
  {
    ClipperLib::Polygon &p = ep[k].outer;
    p1.set(p[0].X * 0.001, p[0].Y * 0.001);
    for (int i = p.size() - 1; i >= 0; i--, p1 = p0, seg_cnt++)
    {
      Point4 &s = seg[seg_cnt];
      p0.set(p[i].X * 0.001, p[i].Y * 0.001);
      int cx = (p0.x - ofs_x) / cell, cy = (p0.y - ofs_z) / cell;
      if (cx >= 0 && cy >= 0 && cx < bm.getW() && cy < bm.getH())
        bm.set(cx, cy);

      if (fabsf(p1.y - p0.y) < 1e-12)
      {
        float fc = (p0.y - ofs_z) / cell;
        if (fc - floor(fc) >= 0.5)
          p0.y -= cell * 0.001;
        else
          p0.y += cell * 0.001;
      }
      s.x = p0.x;
      s.y = p0.y;
      s[2] = p1.y;
      s[3] = (p1.y - p0.y);
      seg_dy[seg_cnt] = s[3] > 0 ? 2 : 0;
      s[3] = (p1.x - p0.x) / s[3];
    }
    for (int i = 0, b = seg_cnt - p.size(); i < p.size(); i++)
      seg_dy[b + i] = (seg_dy[b + i] & 0xF) | ((seg_dy[b + (i + p.size() - 1) % p.size()] & 0xF) << 4);

    for (int j = 0; j < ep[k].holes.size(); j++)
    {
      ClipperLib::Polygon &p = ep[k].holes[j];
      p1.set(p[0].X * 0.001, p[0].Y * 0.001);
      for (int i = p.size() - 1; i >= 0; i--, p1 = p0, seg_cnt++)
      {
        Point4 &s = seg[seg_cnt];
        p0.set(p[i].X * 0.001, p[i].Y * 0.001);
        int cx = (p0.x - ofs_x) / cell, cy = (p0.y - ofs_z) / cell;
        if (cx >= 0 && cy >= 0 && cx < bm.getW() && cy < bm.getH())
          bm.set(cx, cy);

        if (fabsf(p1.y - p0.y) < 1e-12)
        {
          float fc = (p0.y - ofs_z) / cell;
          if (fc - floor(fc) >= 0.5)
            p0.y -= cell * 0.001;
          else
            p0.y += cell * 0.001;
        }
        s.x = p0.x;
        s.y = p0.y;
        s[2] = p1.y;
        s[3] = (p1.y - p0.y);
        seg_dy[seg_cnt] = s[3] > 0 ? 2 : 0;
        s[3] = (p1.x - p0.x) / s[3];
      }
      for (int i = 0, b = seg_cnt - (int)p.size(); i < p.size(); i++)
        seg_dy[b + i] = (seg_dy[b + i] & 0xF) | ((seg_dy[b + (i + p.size() - 1) % p.size()] & 0xF) << 4);
    }
  }

  pos.reserve(seg.size());
  for (int y = h - 1; y >= 0; y--)
  {
    float fy = y * cell + ofs_z, fx;

    // gather sorted intersections
    pos.clear();
    for (int j = 0; j < seg.size(); j++)
    {
      Point4 p = seg[j];
      if ((p.y - fy) * (p[2] - fy) > 0)
        continue;

      int c_dy = (seg_dy[j] & 0xF) - 1;
      int n_dy = ((seg_dy[j] >> 4) & 0xF) - 1;
      if (c_dy == n_dy && fabsf(p[2] - fy) < 1e-6)
        continue;

      if (float_nonzero(p.y - fy))
        fx = p.x + (fy - p.y) * p[3] - ofs_x;
      else
        fx = p.x - ofs_x;

      pos.insert(IPoint2(fx * scale * 1000, c_dy));
    }

    // rasterize bitmap using intersection points (nonZero filling rule)
    int v0 = 0, x0 = -2000000000;

    for (int j = 0; j < pos.size(); j++)
    {
      int x1 = pos[j].x / 1000, v1 = v0 + pos[j].y;
      if (!v0 && v1)
        x0 = x1;
      else if (v0 && !v1 && x1 >= 0)
      {
        if (x0 < 0)
          x0 = 0;
        if (x1 > w - 1)
          x1 = w - 1;
        for (; x0 <= x1; x0++)
          bm.set(x0, y);
        if (x1 == w - 1)
          break;
      }
      v0 = v1;
      if (x1 >= w - 1)
        break;
    }
    if (v0)
    {
      if (x0 < 0)
        x0 = 0;
      for (; x0 < w; x0++)
        bm.set(x0, y);
    }
  }
}

static inline int cmp_ip4_x(const IPoint4 *a, const IPoint4 *b) { return a->x - b->x; }
static inline int cmp_ptr_ip4_z(IPoint4 *const *a, IPoint4 *const *b)
{
  if (a[0]->z == b[0]->z)
    return a[0] - b[0];
  return a[0]->z - b[0]->z;
}

struct IPoint4CmpX : public IPoint4
{
  static bool cmpGt(const IPoint4CmpX *e, int key) { return e->x > key; }
  static bool cmpEq(const IPoint4CmpX *e, int key) { return e->x == key; }
};

struct IPoint4PtrCmpZ
{
  IPoint4 *p;
  static bool cmpGt(const IPoint4PtrCmpZ *e, int key) { return e->p->z > key; }
  static bool cmpEq(const IPoint4PtrCmpZ *e, int key) { return e->p->z == key; }
};

static void add_border_segments(Tab<Point3> &water_border_polys, const ClipperLib::Polygon &poly, const BBox3 &hmap_bb,
  float waterSurfaceLevel)
{
  int st_id = -1, num;
  bool whole = true;

  for (int k = 0; k < poly.size(); k++)
  {
    Point3 bp(poly[k].X * 0.001f, waterSurfaceLevel, poly[k].Y * 0.001f);

    if (hmap_bb & bp)
    {
      if (st_id < 0)
      {
        water_border_polys.push_back().set(1.1e12f, 0, 0);
        st_id = water_border_polys.size();
      }
      water_border_polys.push_back(bp);
    }
    else if (st_id >= 0)
    {
      whole = false;
      num = water_border_polys.size() - st_id;
      if (num > 1)
      {
        water_border_polys[st_id - 1].y = num;
        water_border_polys[st_id - 1].z = 1;
      }
      else
      {
        if (num > 0)
          water_border_polys.pop_back();
        water_border_polys.pop_back();
      }
      st_id = -1;
    }
    else
      whole = false;
  }
  if (!whole && st_id >= 0)
  {
    Point3 bp(poly[0].X * 0.001f, waterSurfaceLevel, poly[0].Y * 0.001f);
    if (hmap_bb & bp)
      water_border_polys.push_back(bp);
  }

  if (st_id < 0)
    return;

  num = water_border_polys.size() - st_id;
  if (num > (whole ? 0 : 1))
  {
    water_border_polys[st_id - 1].y = num;
    water_border_polys[st_id - 1].z = whole ? 0 : 1;
  }
  else
  {
    if (num > 0)
      water_border_polys.pop_back();
    water_border_polys.pop_back();
  }
}
static int cmp_area(const uint64_t *a, const uint64_t *b) { return *a < *b ? -1 : (*a > *b ? 1 : 0); }

void HmapLandPlugin::rebuildWaterSurface(Tab<Point3> *p_loft_pt, Tab<Point3> *p_water_border, Tab<Point2> *p_hmap_sweep)
{
  if (hasWorldOcean)
    heightmapChanged(true);
  applyHmModifiers();
  if (detDivisor > 1)
  {
    for (int y = detRectC[0].y - detDivisor / 2; y < detRectC[1].y + detDivisor / 2; y += detDivisor)
      for (int x = detRectC[0].x - detDivisor / 2; x < detRectC[1].x + detDivisor / 2; x += detDivisor)
      {
        double sum = 0;
        int sum_n = 0;
        for (int k = 0; k <= detDivisor; k++)
          for (int m = 0; m <= detDivisor; m++)
            if (insideDetRectC(x + m, y + k))
              sum += heightMapDet.getFinalData(x + m, y + k), sum_n++;
        if (sum_n)
        {
          heightMap.setInitialData(x / detDivisor + 1, y / detDivisor + 1, sum / sum_n);
          heightMap.setFinalData(x / detDivisor + 1, y / detDivisor + 1, sum / sum_n);
        }
      }
  }
  else if (detDivisor == 1)
    for (int y = detRectC[0].y; y < detRectC[1].y; y++)
      for (int x = detRectC[0].x; x < detRectC[1].x; x++)
      {
        heightMap.setInitialData(x, y, heightMapDet.getFinalData(x, y));
        heightMap.setFinalData(x, y, heightMapDet.getFinalData(x, y));
      }

  Tab<Point3> l_loft_pt(tmpmem);
  Tab<Point3> l_water_border(tmpmem);
  Tab<Point2> l_hmap_sweep(tmpmem);

  Tab<Point3> &loft_pt_cloud = p_loft_pt ? *p_loft_pt : l_loft_pt;
  Tab<Point3> &water_border_polys = p_water_border ? *p_water_border : l_water_border;
  Tab<Point2> &hmap_sweep_polys = p_hmap_sweep ? *p_hmap_sweep : l_hmap_sweep;

  waterMaskScale = gridCellSize / 2;
  if (waterMaskScale < 4)
    waterMaskScale = 4;
  else if (waterMaskScale > 16)
    waterMaskScale = 16;


  objEd.gatherLoftLandPts(loft_pt_cloud, water_border_polys, hmap_sweep_polys);
  /*
  for (int i = 0; i < water_border_polys.size(); i++)
    debug("w[%3d] " FMT_P3 "", i, P3D(water_border_polys[i]));
  for (int i = 0; i < hmap_sweep_polys.size(); i++)
    debug("h[%3d] " FMT_P2 "", i, P2D(hmap_sweep_polys[i]));
  for (int i = 0; i < loft_pt_cloud.size(); i++)
    debug("p[%3d] " FMT_P3 "", i, P3D(loft_pt_cloud[i]));
  */

  BBox3 hmap_bb;
  hmap_bb.lim[0].set(heightMapOffset.x, waterSurfaceLevel - 4000, heightMapOffset.y);
  hmap_bb.lim[1] = hmap_bb.lim[0] + Point3(getHeightmapSizeX() * gridCellSize, 16000, getHeightmapSizeY() * gridCellSize);
  ClipperLib::ExPolygons waterPoly;
  ClipperLib::ExPolygons waterPoly_non_ocean;
  Ptr<MaterialData> sm = NULL;
  IAssetService *assetSrv = DAGORED2->queryEditorInterface<IAssetService>();

  objEd.getClearedWaterGeom();
  if (!hasWaterSurface)
  {
    if (waterMask.getW())
      heightmapChanged(true);
    waterMask.reset();
    if (p_water_border)
      clear_and_shrink(*p_water_border);
    goto process_loft_pt;
  }

  waterMask.resize(0, 0);
  if (hasWorldOcean)
  {
    int time0l = dagTools->getTimeMsec();
    ClipperLib::Clipper clpr;
    ClipperLib::Polygon ocean_poly;

    if (worldOceanExpand > 0)
    {
      ocean_poly.resize(8);
      ocean_poly[0].X = heightMapOffset.x * 1000;
      ocean_poly[0].Y = (heightMapOffset.y - worldOceanExpand) * 1000;
      ocean_poly[1].X = (heightMapOffset.x - worldOceanExpand) * 1000;
      ocean_poly[1].Y = heightMapOffset.y * 1000;

      ocean_poly[4].X = (heightMapOffset.x + getHeightmapSizeX() * gridCellSize) * 1000;
      ocean_poly[4].Y = (heightMapOffset.y + getHeightmapSizeY() * gridCellSize + worldOceanExpand) * 1000;
      ocean_poly[5].X = (heightMapOffset.x + getHeightmapSizeX() * gridCellSize + worldOceanExpand) * 1000;
      ocean_poly[5].Y = (heightMapOffset.y + getHeightmapSizeY() * gridCellSize) * 1000;

      ocean_poly[2].X = ocean_poly[1].X;
      ocean_poly[2].Y = ocean_poly[5].Y;
      ocean_poly[3].X = ocean_poly[0].X;
      ocean_poly[3].Y = ocean_poly[4].Y;
      ocean_poly[6].X = ocean_poly[5].X;
      ocean_poly[6].Y = ocean_poly[1].Y;
      ocean_poly[7].X = ocean_poly[4].X;
      ocean_poly[7].Y = ocean_poly[0].Y;
    }
    else
    {
      ocean_poly.resize(4);
      ocean_poly[0].X = heightMapOffset.x * 1000;
      ocean_poly[0].Y = heightMapOffset.y * 1000;
      ocean_poly[2].X = (heightMapOffset.x + getHeightmapSizeX() * gridCellSize) * 1000;
      ocean_poly[2].Y = (heightMapOffset.y + getHeightmapSizeY() * gridCellSize) * 1000;
      ocean_poly[1].X = ocean_poly[0].X;
      ocean_poly[1].Y = ocean_poly[2].Y;
      ocean_poly[3].X = ocean_poly[2].X;
      ocean_poly[3].Y = ocean_poly[0].Y;
    }
    if (!ClipperLib::Orientation(ocean_poly))
      reverse(ocean_poly.begin(), ocean_poly.end());

    clpr.AddPolygon(ocean_poly, ClipperLib::ptSubject);

    Tab<Point2> pt(tmpmem);
    Tab<IPoint4> edges(tmpmem);
    Tab<IPoint4 *> edges_rev(tmpmem);
    int edge_w = getHeightmapSizeX() + 2;

    int edge_idx_remap[13] = {0, 8, edge_w * 8, edge_w * 8 + 8, 2, 3, 3 + 8, 2 + edge_w * 8, 1, 4, 5, 6, 7};
    int pair_tbl[13][6] = {
      /* 0*/ {5, 1, 2, 8, 10, 11},
      /* 1*/ {4, 3, 8, 9, 12},
      /* 2*/ {4, 3, 8, 9, 12},
      /* 3*/ {3, 8, 10, 11},

      /* 4*/ {3, 8, 9, 10},
      /* 5*/ {3, 8, 9, 11},
      /* 6*/ {3, 8, 10, 12},
      /* 7*/ {3, 8, 11, 12},

      /* 8*/ {0},
      /* 9*/ {2, 10, 11},
      /*10*/ {1, 12},
      /*11*/ {1, 12},
      /*12*/ {0},
    };

    waterMask.resize(getHeightmapSizeX() * waterMaskScale, getHeightmapSizeY() * waterMaskScale);

    float minx = heightMapOffset[0], maxx = minx + (getHeightmapSizeX() - 1) * gridCellSize;
    float miny = heightMapOffset[1], maxy = miny + (getHeightmapSizeY() - 1) * gridCellSize;

    for (int z = -1, ze = getHeightmapSizeY(); z < ze; z++)
      for (int x = -1, xe = getHeightmapSizeX(); x < xe; x++)
      {
        float h0 = -0.1f, hx = -0.1f, hz = -0.1f, hxz = -0.1f;

        if (x >= 0 && z >= 0)
        {
          h0 = heightMap.getFinalData(x, z) - waterSurfaceLevel;
          // if (h0 <= 0)
          //   waterMask.set(x*waterMaskScale, z*waterMaskScale);
          if (x + 1 < xe)
            hx = heightMap.getFinalData(x + 1, z) - waterSurfaceLevel;
          if (z + 1 < ze)
            hz = heightMap.getFinalData(x, z + 1) - waterSurfaceLevel;
          if (x + 1 < xe && z + 1 < ze)
            hxz = heightMap.getFinalData(x + 1, z + 1) - waterSurfaceLevel;
        }
        else if (x >= 0)
        {
          hz = heightMap.getFinalData(x, z + 1) - waterSurfaceLevel;
          if (x + 1 < xe)
            hxz = heightMap.getFinalData(x + 1, z + 1) - waterSurfaceLevel;
        }
        else if (z >= 0)
        {
          hx = heightMap.getFinalData(x + 1, z) - waterSurfaceLevel;
          if (z + 1 < ze)
            hxz = heightMap.getFinalData(x + 1, z + 1) - waterSurfaceLevel;
        }
        else
          hxz = heightMap.getFinalData(x + 1, z + 1) - waterSurfaceLevel;

        if (h0 <= 0 && hx <= 0 && hz <= 0 && hxz <= 0)
          continue;
        if (h0 > 0 && hx > 0 && hz > 0 && hxz > 0)
          continue;

        float hm = (h0 + hx + hz + hxz) * 0.25f;
        unsigned edge_cross_m = 0;
        int pt_ofs[13];
        float cx0 = x * gridCellSize + heightMapOffset[0];
        float cz0 = z * gridCellSize + heightMapOffset[1];

#define CHECK_PT(y0, x0, z0, idx)                                         \
  if (fabs(y0) < 1e-6)                                                    \
  {                                                                       \
    edge_cross_m |= 1 << idx;                                             \
    pt_ofs[idx] = pt.size();                                              \
    pt.push_back().set(cx0 + x0 * gridCellSize, cz0 + z0 * gridCellSize); \
  }

#define CHECK_SEG(y0, y1, x0, z0, x1, z1, idx, vi0, vi1)                                                            \
  if (!(edge_cross_m & ((1 << vi0) | (1 << vi1))) && y0 * y1 < 0)                                                   \
  {                                                                                                                 \
    edge_cross_m |= 1 << idx;                                                                                       \
    float t = (0.0f - y0) / (y1 - y0);                                                                              \
    pt_ofs[idx] = pt.size();                                                                                        \
    pt.push_back().set(cx0 + (x1 * t + x0 * (1 - t)) * gridCellSize, cz0 + (z1 * t + z0 * (1 - t)) * gridCellSize); \
  }

        CHECK_PT(h0, 0, 0, 0);
        CHECK_PT(hx, 1, 0, 1);
        CHECK_PT(hz, 0, 1, 2);
        CHECK_PT(hxz, 1, 1, 3);

        CHECK_SEG(h0, hx, 0, 0, 1, 0, 4, 0, 1);
        CHECK_SEG(h0, hz, 0, 0, 0, 1, 5, 0, 2);
        CHECK_SEG(hx, hxz, 1, 0, 1, 1, 6, 1, 3);
        CHECK_SEG(hz, hxz, 0, 1, 1, 1, 7, 2, 3);

        CHECK_PT(hm, 0.5, 0.5, 8);
        CHECK_SEG(hm, h0, 0.5, 0.5, 0, 0, 9, 8, 0);
        CHECK_SEG(hm, hx, 0.5, 0.5, 1, 0, 10, 8, 1);
        CHECK_SEG(hm, hz, 0.5, 0.5, 0, 1, 11, 8, 2);
        CHECK_SEG(hm, hxz, 0.5, 0.5, 1, 1, 12, 8, 3);

        if (!edge_cross_m)
          continue;

        int edge_base = ((z + 1) * edge_w + (x + 1)) * 8;
        // debug("%d,%d: %p, mask=%p", x, z, edge_base, edge_cross_m);
        for (int i = 0; i < 13; i++)
          if (edge_cross_m & (1 << i))
          {
            Point2 &pc = pt[pt_ofs[i]];
            if (pc.x <= minx)
              pc.x -= worldOceanExpand * 2 + 1;
            else if (pc.x >= maxx)
              pc.x += worldOceanExpand * 2 + 1;
            if (pc.y <= miny)
              pc.y -= worldOceanExpand * 2 + 1;
            else if (pc.y >= maxy)
              pc.y += worldOceanExpand * 2 + 1;

            for (int *j = &pair_tbl[i][1], *je = j + pair_tbl[i][0]; j < je; j++)
              if (edge_cross_m & (1 << *j))
              {
                // debug("  %p-%p (%d-%d)",
                //   edge_base+edge_idx_remap[i], edge_base+edge_idx_remap[*j], i, *j);
                edges.push_back().set(edge_base + edge_idx_remap[i], pt_ofs[i], edge_base + edge_idx_remap[*j], pt_ofs[*j]);
              }
          }
      }

    sort(edges, &cmp_ip4_x);
    edges_rev.resize(edges.size());
    for (int i = 0; i < edges_rev.size(); i++)
      edges_rev[i] = &edges[i];
    sort(edges_rev, &cmp_ptr_ip4_z);

    HierBitArray<ConstSizeBitArray<15>> edge_mark;
    int edges_left = edges.size(), edges_count = edges_left, poly_count = 0;
    edge_mark.resize(edges_left);

    /*
    for (int i = 0; i < edges.size(); i ++)
      debug("%d: %p(%d) - %p(%d)", i, edges[i].x, edges[i].y, edges[i].z, edges[i].w);
    for (int i = 0; i < edges_rev.size(); i ++)
      debug("%d: %d(%d) - %d(%d)", edges_rev[i]-edges.data(),
        edges_rev[i]->x, edges_rev[i]->y, edges_rev[i]->z, edges_rev[i]->w);
    */

    int start_edge_idx = 0;
    ClipperLib::Polygon island_poly;
    ClipperLib::Polygons islands;
    Tab<int> eid(tmpmem);
    Tab<uint64_t> island_area_idx(tmpmem);
    int sum_t = 0;

    while (edges_left)
    {
      int start_id = -1;
      int first_id = -1;
      int next_id = -1;
      bool closed = false;

      eid.resize(0);
      island_poly.resize(0);

      for (int i = start_edge_idx, ie = edge_mark.getSz(); i < ie; i++)
        if (!edge_mark.get(i))
        {
          edges_left--;
          edge_mark.set(i);
          start_edge_idx = i + 1;
          island_poly.push_back(ClipperLib::IntPoint(pt[edges[i].y].x * 1000, pt[edges[i].y].y * 1000));
          island_poly.push_back(ClipperLib::IntPoint(pt[edges[i].w].x * 1000, pt[edges[i].w].y * 1000));
          start_id = first_id = edges[i].x;
          next_id = edges[i].z;
          eid.clear();
          break;
        }

      // debug("start: %p,%p", first_id, next_id);
      while (edges_left)
      {
        bool back = true;
        int idx = bfind_rep_idx(make_span_const(static_cast<IPoint4CmpX *>(edges.data()), edges.size()), next_id);
        if (idx > 0)
        {
          for (; idx < edges.size(); idx++)
            if (edges[idx].x != next_id)
            {
              idx = edges.size();
              break;
            }
            else if (!edge_mark.get(idx))
              break;
        }
        if (idx < 0 || idx == edges.size())
        {
          idx = bfind_rep_idx(make_span_const(reinterpret_cast<IPoint4PtrCmpZ *>(edges_rev.data()), edges_rev.size()), next_id);
          if (idx >= 0)
          {
            for (; idx < edges.size(); idx++)
              if (edges_rev[idx]->z != next_id)
              {
                idx = edges.size();
                break;
              }
              else if (!edge_mark.get(edges_rev[idx] - edges.data()))
                break;
          }

          if (idx < 0 || idx == edges.size())
          {
            if (!eid.size())
              break;
            debug("  not found %p (sz=%d, edges_left=%d) reverting to %p", next_id, island_poly.size(), edges_left, eid.back());
            next_id = eid.back();
            eid.pop_back();
            island_poly.pop_back();
            continue;
          }
          idx = edges_rev[idx] - edges.data();
          back = false;
        }

        eid.push_back(next_id);
        edges_left--;
        edge_mark.set(idx);
        if (back)
        {
          island_poly.push_back(ClipperLib::IntPoint(pt[edges[idx].w].x * 1000, pt[edges[idx].w].y * 1000));
          next_id = edges[idx].z;
        }
        else
        {
          island_poly.push_back(ClipperLib::IntPoint(pt[edges[idx].y].x * 1000, pt[edges[idx].y].y * 1000));
          next_id = edges[idx].x;
        }
        // debug("  add %d: next=%p", back, next_id);
        if (next_id == start_id)
        {
          // debug("loop closed: %d", island_poly.size());
          closed = true;
          break;
        }
      }

      if (!closed)
      {
        // debug("drop unclosed %d segments", island_poly.size());
        continue;
      }

      // for (int i = 0; i < island_poly.size(); i ++)
      //   debug("%d: %d  %d,%d", i, (i>0 && i-1<eid.size()) ? eid[i-1] : -1,
      //     int(island_poly[i].X), int(island_poly[i].Y));

      // check and remove small loops
      FastIntList ueid;
      for (int i = eid.size() - 1; i >= 0; i--)
        if (!ueid.addInt(eid[i]))
          for (int j = i + 1; j < eid.size(); j++)
            if (eid[j] == eid[i])
            {
              if (j > i + 16)
              {
                debug("skip removing large loop %d..%d", i + 1, j + 1 - 1);
                break;
              }
              debug("remove loop %d..%d (%d,%d - %d,%d)", i + 1, j + 1 - 1, int(island_poly[i + 1].X), int(island_poly[i + 1].Y),
                int(island_poly[j + 1].X), int(island_poly[j + 1].Y));
              erase_items(eid, i, j - i);
              island_poly.erase(island_poly.begin() + i + 1, island_poly.begin() + j + 1);
              break;
            }

      optimize_polygon(island_poly, worldOceanShorelineTolerance);
      if (!ClipperLib::Orientation(island_poly))
        reverse(island_poly.begin(), island_poly.end());

      islands.push_back(island_poly);
      island_area_idx.push_back((uint64_t(fabs(ClipperLib::Area(island_poly)) * 1e-6) << 16) + island_area_idx.size());
    }

    sort(island_area_idx, &cmp_area);
    for (int i = 0; i < island_area_idx.size(); i++)
    {
      if (clpr.AddPolygon(islands[island_area_idx[i] & 0xFFFF], ClipperLib::ptClip))
      {
        poly_count++;
        clpr.Execute(ClipperLib::ctXor, waterPoly, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
        clpr.Clear();
        for (int k = 0; k < waterPoly.size(); k++)
        {
          clpr.AddPolygon(waterPoly[k].outer, ClipperLib::ptSubject);
          clpr.AddPolygons(waterPoly[k].holes, ClipperLib::ptSubject);
        }
      }
    }

    // compress ocean poly for 1mm inward to avoid precision bugs during intersection
    int opcx = (heightMapOffset.x + getHeightmapSizeX() * gridCellSize / 2) * 1000;
    int opcy = (heightMapOffset.y + getHeightmapSizeY() * gridCellSize / 2) * 1000;
    for (int k = 0; k < ocean_poly.size(); k++)
    {
      ocean_poly[k].X += ocean_poly[k].X < opcx ? 1 : -1;
      ocean_poly[k].Y += ocean_poly[k].Y < opcy ? 1 : -1;
    }
    clpr.AddPolygon(ocean_poly, ClipperLib::ptClip);
    clpr.Execute(ClipperLib::ctIntersection, waterPoly, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
    clpr.Clear();

    debug("optimize final solution");
    int final_pts = 0, final_polys = 0;
    for (int k = 0; k < waterPoly.size(); k++)
    {
      final_polys += 1 + (int)waterPoly[k].holes.size();
      optimize_polygon(waterPoly[k].outer, 0.05);
      final_pts += (int)waterPoly[k].outer.size();
      for (int l = 0; l < waterPoly[k].holes.size(); l++)
      {
        optimize_polygon(waterPoly[k].holes[l], 0.05);
        final_pts += (int)waterPoly[k].holes[l].size();
      }
    }

    DAEDITOR3.conNote("contoured islands (%d edges, %d points, %d polys ->"
                      " %d points, %d polys) for %.1f sec",
      edges_count, pt.size(), poly_count, final_pts, final_polys, (dagTools->getTimeMsec() - time0l) / 1000.f);

    if (allow_debug_bitmap_dump)
      draw_solution(DAGORED2->getPluginFilePath(this, "../../../islandMask.svg"), waterPoly, heightMapOffset.x, heightMapOffset.y,
        1.0f / gridCellSize, getHeightmapSizeX(), getHeightmapSizeY(), 1.2);
  }

  bool has_water_exclude = false;
  if (water_border_polys.size())
  {
    ClipperLib::Clipper clpr, clpr_sep;
    ClipperLib::Polygon poly;
    for (int k = 0; k < waterPoly.size(); k++)
    {
      clpr.AddPolygon(waterPoly[k].outer, ClipperLib::ptSubject);
      clpr.AddPolygons(waterPoly[k].holes, ClipperLib::ptSubject);
    }

    for (int i = 0; i < water_border_polys.size(); i++)
      if (water_border_polys[i].x > 1e12f)
      {
        if (water_border_polys[i].z > 0)
        {
          i += water_border_polys[i].y;
          has_water_exclude = true;
          continue;
        }

        ClipperLib::PolyType type = ClipperLib::ptSubject;
        poly.resize(water_border_polys[i].y);
        i++;
        for (int j = 0; j < poly.size(); j++, i++)
        {
          poly[j].X = ClipperLib::long64(water_border_polys[i].x * 1000);
          poly[j].Y = ClipperLib::long64(water_border_polys[i].z * 1000);
        }
        if (!ClipperLib::Orientation(poly))
        {
          debug_cp();
          reverse(poly.begin(), poly.end());
        }
        clpr.AddPolygon(poly, type);
        if (hasWorldOcean)
          clpr_sep.AddPolygon(poly, type);
        i--;
      }

    waterPoly.resize(0);
    clpr.Execute(ClipperLib::ctUnion, waterPoly, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
    if (hasWorldOcean)
      clpr_sep.Execute(ClipperLib::ctUnion, waterPoly_non_ocean, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
  }

  if (has_water_exclude && water_border_polys.size())
  {
    ClipperLib::Clipper clpr, clpr_sep;
    ClipperLib::Polygon poly;
    for (int k = 0; k < waterPoly.size(); k++)
    {
      clpr.AddPolygon(waterPoly[k].outer, ClipperLib::ptSubject);
      clpr.AddPolygons(waterPoly[k].holes, ClipperLib::ptSubject);
    }

    for (int i = 0; i < water_border_polys.size(); i++)
      if (water_border_polys[i].x > 1e12f)
      {
        if (water_border_polys[i].z <= 0)
        {
          i += water_border_polys[i].y;
          continue;
        }

        ClipperLib::PolyType type = ClipperLib::ptClip;
        poly.resize(water_border_polys[i].y);
        i++;
        for (int j = 0; j < poly.size(); j++, i++)
        {
          poly[j].X = ClipperLib::long64(water_border_polys[i].x * 1000);
          poly[j].Y = ClipperLib::long64(water_border_polys[i].z * 1000);
        }
        if (!ClipperLib::Orientation(poly))
          reverse(poly.begin(), poly.end());
        clpr.AddPolygon(poly, type);
        if (hasWorldOcean)
          clpr_sep.AddPolygon(poly, type);
        i--;
      }

    waterPoly.resize(0);
    clpr.Execute(ClipperLib::ctDifference, waterPoly, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

    if (hasWorldOcean)
      clpr_sep.Execute(ClipperLib::ctDifference, waterPoly_non_ocean, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
  }

  if (waterPoly.size() > 0)
  {
    if (allow_debug_bitmap_dump)
      draw_solution(DAGORED2->getPluginFilePath(this, "../../../waterMask.svg"), waterPoly, heightMapOffset.x, heightMapOffset.y,
        1.0f / gridCellSize, getHeightmapSizeX(), getHeightmapSizeY(), 1.2);

    waterMask.resize(getHeightmapSizeX() * waterMaskScale, getHeightmapSizeY() * waterMaskScale);
    rasterize_poly_2_nz(waterMask, waterPoly, heightMapOffset.x, heightMapOffset.y, waterMaskScale / gridCellSize);
    if (allow_debug_bitmap_dump)
      write_tif(waterMask, "../../waterMask.tif");
  }

  if (assetSrv)
    sm = assetSrv->getMaterialData(waterMatAsset);

  if (sm && waterPoly.size() > 0)
  {
    // build water surface mesh
    debug_flush(false);
    Mesh *pm_water = new Mesh;
    Mesh &m_water = *pm_water;

    BBox2 bb;
    for (const auto &wpoly : waterPoly)
      for (const auto &p : wpoly.outer)
        bb += Point2(p.X * 0.001f, p.Y * 0.001f);
    bb[0] -= Point2(1, 1);
    bb[1] += Point2(1, 1);

    ctl::PointList boundary;
    boundary.push_back({bb[0].x, bb[0].y});
    boundary.push_back({bb[1].x, bb[0].y});
    boundary.push_back({bb[1].x, bb[1].y});
    boundary.push_back({bb[0].x, bb[1].y});

    ctl::DelaunayTriangulation dt{boundary, 16 << 10};

    dag::Vector<dag::Vector<ctl::PointList>> all_wpolys;
    all_wpolys.reserve(waterPoly.size());
    for (const auto &wpoly : waterPoly)
    {
      auto &wpoly1 = all_wpolys.push_back();
      wpoly1.reserve(wpoly.holes.size() + 1);
      auto &outer_pts = wpoly1.push_back();
      outer_pts.reserve(wpoly.outer.size());
      for (const auto &p : wpoly.outer)
        outer_pts.push_back({p.X * 0.001f, p.Y * 0.001f, waterSurfaceLevel});
      outer_pts.push_back({outer_pts[0]});
      dt.InsertConstrainedLineString(outer_pts);

      for (const auto &hole : wpoly.holes)
      {
        auto &hole_pts = wpoly1.push_back();
        hole_pts.reserve(hole.size());
        for (const auto &p : hole)
          hole_pts.push_back({p.X * 0.001f, p.Y * 0.001f, waterSurfaceLevel});
        hole_pts.push_back({hole_pts[0]});
        dt.InsertConstrainedLineString(hole_pts);
      }
    }

    debug_flush(true);

    ctl::TIN tin(&dt);

    Tab<int> vert_remap;
    vert_remap.resize(tin.verts.size());
    mem_set_ff(vert_remap);

    m_water.vert.reserve(tin.verts.size());
    m_water.face.reserve(tin.triangles.size() / 3);
    debug_cp();
    for (int i = 0; i < tin.triangles.size(); i += 3)
    {
      ctl::Point centroid(
        (tin.verts[tin.triangles[i + 0]].x + tin.verts[tin.triangles[i + 1]].x + tin.verts[tin.triangles[i + 2]].x) / 3,
        (tin.verts[tin.triangles[i + 0]].y + tin.verts[tin.triangles[i + 1]].y + tin.verts[tin.triangles[i + 2]].y) / 3,
        (tin.verts[tin.triangles[i + 0]].z + tin.verts[tin.triangles[i + 1]].z + tin.verts[tin.triangles[i + 2]].z) / 3);

      bool keepTriangle = false;
      for (const auto &wpoly1 : all_wpolys)
      {
        for (const auto &p : wpoly1)
          if (ctl::PointInPolygon(centroid, p))
          {
            if (&p == wpoly1.begin()) // in outer
              keepTriangle = true;
            else
            {
              keepTriangle = false; // in hole
              break;
            }
          }
          else if (&p == wpoly1.begin()) // not in outer
            break;
        if (keepTriangle)
          break;
      }
      if (keepTriangle)
      {
        Face &f = m_water.face.push_back();
        f.v[0] = tin.triangles[i + 0];
        f.v[2] = tin.triangles[i + 1];
        f.v[1] = tin.triangles[i + 2];
        for (unsigned &vi : f.v)
          if (vert_remap[vi] >= 0)
            vi = vert_remap[vi];
          else
          {
            m_water.vert.push_back().set_xzy(tin.verts[vi]);
            vi = vert_remap[vi] = m_water.vert.size() - 1;
          }
        f.mat = 0;
        f.smgr = 1;
      }
    }
    m_water.vert.shrink_to_fit();
    m_water.face.shrink_to_fit();
    debug("water mesh: %d verts, %d faces (before holes removal: %d verts, %d faces)", m_water.vert.size(), m_water.face.size(),
      tin.verts.size(), tin.triangles.size() / 3);
    clear_and_shrink(all_wpolys);
    clear_and_shrink(vert_remap);

    m_water.calc_ngr();
    m_water.calc_vertnorms();

    GeomObject &waterGeom = objEd.getClearedWaterGeom();

    MaterialDataList *mdl = new MaterialDataList;
    mdl->addSubMat(sm);

    StaticGeometryContainer *g = dagGeom->newStaticGeometryContainer();
    dagGeom->objCreator3dAddNode("global_water_surface", pm_water, mdl, *g);

    StaticGeometryContainer *geom = waterGeom.getGeometryContainer();
    for (int i = 0; i < g->nodes.size(); ++i)
    {
      StaticGeometryNode *node = g->nodes[i];

      dagGeom->staticGeometryNodeCalcBoundBox(*node);
      dagGeom->staticGeometryNodeCalcBoundSphere(*node);

      node->flags |= StaticGeometryNode::FLG_RENDERABLE | StaticGeometryNode::FLG_COLLIDABLE;
      geom->addNode(dagGeom->newStaticGeometryNode(*node, tmpmem));
    }
    dagGeom->deleteStaticGeometryContainer(g);

    waterGeom.setTm(TMatrix::IDENT);
    SplineObject::PolyGeom::recalcLighting(&waterGeom);

    waterGeom.notChangeVertexColors(true);
    dagGeom->geomObjectRecompile(waterGeom);
    waterGeom.notChangeVertexColors(false);
  }
  else if (!sm && waterPoly.size() > 0 && assetSrv && !waterMatAsset.empty())
    DAEDITOR3.conError("bad water surface mat: %s", waterMatAsset);

  water_border_polys.clear();
  for (int i = 0; i < waterPoly.size(); i++)
  {
    ClipperLib::ExPolygon &p = waterPoly[i];
    // optimize_polygon(p.outer);
    add_border_segments(water_border_polys, p.outer, hmap_bb, waterSurfaceLevel);

    for (int j = 0; j < p.holes.size(); j++)
    {
      // optimize_polygon(p.holes[j]);
      add_border_segments(water_border_polys, p.holes[j], hmap_bb, waterSurfaceLevel);
    }
  }

  heightmapChanged(true);

process_loft_pt:
  for (int i = 0; i < loft_pt_cloud.size(); i++)
    if (loft_pt_cloud[i].x > 1e12f)
    {
      i++;

      int st_id = -1, num;
      for (int j = 0, je = loft_pt_cloud[i - 1].y; j < je; j++, i++)
      {
        int x = (loft_pt_cloud[i].x - heightMapOffset.x) * waterMaskScale / gridCellSize;
        int z = (loft_pt_cloud[i].z - heightMapOffset.y) * waterMaskScale / gridCellSize;
        bool expel = false;

        if (x >= 0 && z >= 0 && x < waterMask.getW() && z < waterMask.getH())
          if (waterMask.get(x, z))
            if (loft_pt_cloud[i].y >= waterSurfaceLevel - 0.01)
              expel = true;

        if (!expel && (hmap_bb & loft_pt_cloud[i]))
        {
          if (st_id < 0)
          {
            water_border_polys.push_back().set(1.1e12f, 0, 1);
            st_id = water_border_polys.size();
          }
          water_border_polys.push_back(loft_pt_cloud[i]);
        }
        else if (st_id >= 0)
        {
          num = water_border_polys.size() - st_id;
          if (num > 1)
            water_border_polys[st_id - 1].y = num;
          else
          {
            if (num > 0)
              water_border_polys.pop_back();
            water_border_polys.pop_back();
          }
          st_id = -1;
        }
      }

      if (st_id >= 0)
      {
        num = water_border_polys.size() - st_id;
        if (num > 1)
          water_border_polys[st_id - 1].y = num;
        else
        {
          if (num > 0)
            water_border_polys.pop_back();
          water_border_polys.pop_back();
        }
      }

      i--;
    }
  loft_pt_cloud.clear();

  if (allow_debug_bitmap_dump)
  {
    float ox = heightMapOffset.x;
    float oz = heightMapOffset.y;
    float scl = 1.0f / gridCellSize;
    int w = getHeightmapSizeX();
    int h = getHeightmapSizeY();
    float over_scl = 1.2;

    FILE *fp = fp = svg_open(DAGORED2->getPluginFilePath(this, "../../../border.svg"), w * over_scl, h * over_scl);
    ox *= over_scl;
    oz *= over_scl;
    scl /= over_scl;

    svg_begin_group(fp, "stroke-width=\"2\" stroke=\"black\" style=\"fill:none\"");
    for (int i = 0; i < water_border_polys.size(); i++)
    {
      if (water_border_polys[i].x > 1e12f)
      {
        bool closed = water_border_polys[i].z < 0.5f;
        i++;
        Point2 p0 = Point2(water_border_polys[i].x - ox, water_border_polys[i].z - oz) * scl;
        i++;
        svg_start_poly(fp, p0);
        for (int ie = i + water_border_polys[i - 2].y - 1; i < ie; i++)
          svg_add_poly_point(fp, Point2(water_border_polys[i].x - ox, water_border_polys[i].z - oz) * scl);
        i--;
        if (closed)
          svg_add_poly_point(fp, p0);
        svg_end_poly(fp);
      }
    }
    svg_end_group(fp);
    svg_close(fp);
  }
}
bool HmapLandPlugin::isPointUnderWaterSurf(float fx, float fz)
{
  int x = (fx - heightMapOffset.x) * waterMaskScale / gridCellSize;
  int z = (fz - heightMapOffset.y) * waterMaskScale / gridCellSize;
  if (x >= 0 && z >= 0 && x < waterMask.getW() && z < waterMask.getH())
    if (waterMask.get(x, z))
      return true;
  return false;
}
