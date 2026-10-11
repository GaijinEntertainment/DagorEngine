// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "rtssScheduler.h"
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <EASTL/sort.h>
#include <EASTL/utility.h>
#include <math.h>

struct RtssRasterGroup
{
  int level, bit, first, count; // a run of order with one (level, bit)
  IBBox2 pages;
  int batch;            // the scratch fill that draws it
  IPoint2 origin, size; // its region in the scratch: the page box and its border
  bool transposed;      // the region is turned by 90 deg: light x runs down its rows
};
struct RtssRasterPlan
{
  dag::Vector<uint32_t> order; // plan job indices by (level, bit), then by cull group
  dag::Vector<RtssRasterGroup> cullGroups;
  IPoint2 size = IPoint2(0, 0);   // the scratch target in texels
  int batches = 0;                // the scratch fills of this frame
  dag::Vector<uint32_t> jobGroup; // by plan job index, set for the jobs of order only
};

inline int rtss_raster_job_key(const RtssJob &job) { return int(job.levelSlot >> 16) * 2 + int(job.flags & RTSS_JOB_BIT); }
inline IPoint2 rtss_job_page(const RtssJob &job) { return IPoint2(job.page.x, job.page.y); }

inline uint32_t rtss_raster_levels(const RtssScheduler &sched)
{
  const RtssSchedulerSettings &s = sched.getSettings();
  uint32_t mask = 0;
  for (int k = 0; k < s.levels; ++k)
    mask |= s.recipes[k].raster != RtssRaster::None ? 1u << k : 0u;
  return mask;
}

// the jobs of raster_levels in raster order, each with the scratch texel of its slot window in rasterTile, x | y << 16,
// and RTSS_TILE_TRANSPOSED on a turned region; a cull group is a box of pages of one level and bit that rtss_box_takes grows
// in plan order, as the scheduler prices its call, and is drawn once into its region: the regions fill shelves of the scratch
inline void rtss_raster_plan(dag::Span<RtssJob> jobs, uint32_t raster_levels, const RtssScheduler &sched, RtssRasterPlan &out)
{
  const RtssSchedulerSettings &s = sched.getSettings();
  out.size = rtss_raster_scratch_size(s);
  const int border = 2 * s.borderTexels, pageTexels = s.pageTexels();
  out.order.clear();
  for (int i = 0; i < (int)jobs.size(); ++i)
    if (raster_levels & (1u << (jobs[i].levelSlot >> 16)))
      out.order.push_back(i);
  const int n = (int)out.order.size();
  // constraint: a sort that allocates can stall the frame on a heap that streaming threads hold
  eastl::insertion_sort(out.order.begin(), out.order.end(),
    [&](uint32_t a, uint32_t b) { return rtss_raster_job_key(jobs[a]) < rtss_raster_job_key(jobs[b]); });
  out.cullGroups.clear();
  out.jobGroup.resize(jobs.size());
  for (int first = 0, end = 0; first < n; first = end)
  {
    const int key = rtss_raster_job_key(jobs[out.order[first]]), g0 = (int)out.cullGroups.size();
    for (end = first; end < n && rtss_raster_job_key(jobs[out.order[end]]) == key; ++end)
    {
      const IPoint2 p = rtss_job_page(jobs[out.order[end]]);
      int g = g0;
      while (g < (int)out.cullGroups.size() && !rtss_box_takes(s, out.cullGroups[g].pages, out.cullGroups[g].count, p))
        ++g;
      if (g == (int)out.cullGroups.size())
        out.cullGroups.push_back({key / 2, key & 1, 0, 0});
      out.cullGroups[g].pages += p;
      ++out.cullGroups[g].count;
      out.jobGroup[out.order[end]] = g;
    }
    eastl::insertion_sort(out.order.begin() + first, out.order.begin() + end,
      [&](uint32_t a, uint32_t b) { return out.jobGroup[a] < out.jobGroup[b]; });
    for (int g = g0, f = first; g < (int)out.cullGroups.size(); f += out.cullGroups[g++].count)
      out.cullGroups[g].first = f;
  }
  out.batches = 0;
  for (int i = 0, x = 0, y = 0, shelf = 0; i < (int)out.cullGroups.size(); ++i)
  {
    RtssRasterGroup &g = out.cullGroups[i];
    g.size = (g.pages.width() + IPoint2(1, 1)) * pageTexels + IPoint2(border, border);
    g.transposed = g.size.y > out.size.y;
    if (g.transposed)
      g.size = IPoint2(g.size.y, g.size.x);
    if (x + g.size.x > out.size.x)
      x = 0, y += shelf, shelf = 0;
    if (!out.batches || y + g.size.y > out.size.y)
      x = y = shelf = 0, ++out.batches;
    g.batch = out.batches - 1;
    g.origin = IPoint2(x, y);
    x += g.size.x;
    shelf = eastl::max(shelf, g.size.y);
    // constraint: the viewport row grows down, so the page of the largest y is at the top, or of the smallest x when turned
    for (int r = g.first; r < g.first + g.count; ++r)
    {
      RtssJob &job = jobs[out.order[r]];
      const IPoint2 tile = g.origin + (g.transposed ? IPoint2(job.page.y - g.pages[0].y, job.page.x - g.pages[0].x)
                                                    : IPoint2(job.page.x - g.pages[0].x, g.pages[1].y - job.page.y)) *
                                        pageTexels;
      job.rasterTile = uint32_t(tile.x) | uint32_t(tile.y) << 16 | (g.transposed ? RTSS_TILE_TRANSPOSED : 0);
    }
  }
}

// the light space box of a whole-slot job, relative to the window origin and to z0
struct RtssRasterRect
{
  double x0, y0, x1, y1, zStart, zEnd;
};
inline RtssRasterRect rtss_raster_rect(const RtssScheduler &s, const RtssJob &job)
{
  const int level = job.levelSlot >> 16, S = s.getSettings().slotTexels;
  const IPoint2 o = s.windowOrigin(level);
  const double texel = s.texel(level), pw = s.pageWorld(level), b = s.getSettings().borderTexels * texel;
  const double x0 = (job.page.x - o.x) * pw - b, y0 = (job.page.y - o.y) * pw - b;
  return {x0, y0, x0 + S * texel, y0 + S * texel, job.zStart, job.zEnd};
}

inline RtssRasterRect rtss_raster_group_rect(const RtssScheduler &s, dag::ConstSpan<RtssJob> jobs, const RtssRasterPlan &plan,
  const RtssRasterGroup &group)
{
  RtssRasterRect u = rtss_raster_rect(s, jobs[plan.order[group.first]]);
  for (int i = group.first + 1; i < group.first + group.count; ++i)
  {
    const RtssRasterRect r = rtss_raster_rect(s, jobs[plan.order[i]]);
    u = {eastl::min(u.x0, r.x0), eastl::min(u.y0, r.y0), eastl::max(u.x1, r.x1), eastl::max(u.y1, r.y1),
      eastl::min(u.zStart, r.zStart), eastl::max(u.zEnd, r.zEnd)};
  }
  return u;
}

// view: the orthonormal rows ex, ey, d, so a callback can invert it as a light view;
// proj: the rectangle to [-1, 1] with its y divided by sy, zStart..zEnd to 0..1, turned by 90 deg for a transposed region
inline void rtss_raster_matrices(const RtssScheduler &s, int level, int bit, const RtssRasterRect &r, bool transposed, TMatrix &view,
  TMatrix4 &proj)
{
  const RtssLightXform &x = s.xform(level, bit);
  const DPoint2 o = DPoint2(s.windowOrigin(level).x, s.windowOrigin(level).y) * s.pageWorld(level);
  for (int c = 0; c < 3; ++c)
    view.setcol(c, Point3(x.ex[c], x.ey[c], x.d[c]));
  view.setcol(3, Point3(float(x.c.x - o.x), float((x.c.y - o.y) / x.sy), float(x.c.z - x.z0)));
  // render() draws no empty depth range, but its matrix stays finite for a cull ahead
  const double zEnd = r.zEnd > r.zStart ? r.zEnd : r.zStart + 1;
  proj = matrix_ortho_off_center_lh(float(r.x0), float(r.x1), float(r.y0 / x.sy), float(r.y1 / x.sy), float(r.zStart), float(zEnd));
  // constraint: a turn, not a mirror, so the triangles keep their winding
  for (int i = 0; transposed && i < 4; ++i)
    proj.m[i][0] = eastl::exchange(proj.m[i][1], -proj.m[i][0]);
}

// a raster job that is not ready is skipped with RTSS_JOB_SAME_PAGE, else built as provisional; both lists sorted
inline void rtss_raster_skips(const RtssRasterPlan &plan, dag::ConstSpan<uint8_t> job_class, dag::ConstSpan<uint8_t> ready,
  dag::Vector<uint32_t> &skipped, dag::Vector<uint32_t> &provisional)
{
  skipped.clear();
  provisional.clear();
  for (uint32_t i : plan.order)
    if (!ready[i])
      (job_class[i] & RTSS_JOB_SAME_PAGE ? skipped : provisional).push_back(i);
  eastl::sort(skipped.begin(), skipped.end());
  eastl::sort(provisional.begin(), provisional.end());
}
