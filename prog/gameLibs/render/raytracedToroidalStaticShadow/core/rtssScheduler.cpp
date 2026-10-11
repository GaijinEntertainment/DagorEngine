// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rtssScheduler.h"
#include <debug/dag_assert.h>
#include <debug/dag_log.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/tuple.h>
#include <math/dag_adjpow2.h>
#include <float.h>

static constexpr double RAD_PER_DEG = 3.14159265358979323846 / 180;
static constexpr double NO_FRONT = DBL_MAX;
static constexpr double NEVER = DBL_MAX;
static constexpr float MIN_TEXEL0 = 0.001f;

static float angle_between(const Point3 &a, const Point3 &b) { return atan2f(length(a % b), a * b); }

static int ifloor(double v) { return int(floor(v)); }

static int valid_setting(int v, int lo, int hi, bool pow2, const char *name)
{
  const int c = eastl::clamp(v, lo, hi), r = pow2 ? get_bigger_pow2(c) : c;
  if (r != v)
    logerr("rtss: %s %d is out of %d..%d%s, using %d", name, v, lo, hi, pow2 ? " or not a power of two" : "", r);
  return r;
}

static float valid_setting(float v, float lo, float hi, const char *name)
{
  if (v >= lo && v <= hi)
    return v;
  const float r = v > hi ? hi : lo;
  logerr("rtss: %s %g is out of %g..%g, using %g", name, v, lo, hi, r);
  return r;
}

static void ls_bounds(const RtssLightXform &x, const BBox3 &box, DPoint3 &lo, DPoint3 &hi)
{
  lo = DPoint3(DBL_MAX, DBL_MAX, DBL_MAX);
  hi = -lo;
  for (int c = 0; c < 8; ++c)
  {
    const DPoint3 ls = x.toLs(dpoint3(box.point(c)));
    lo = DPoint3(eastl::min(lo.x, ls.x), eastl::min(lo.y, ls.y), eastl::min(lo.z, ls.z));
    hi = DPoint3(eastl::max(hi.x, ls.x), eastl::max(hi.y, ls.y), eastl::max(hi.z, ls.z));
  }
}

RtssScheduler::RtssScheduler(const RtssSchedulerSettings &s) : set(s)
{
  set.levels = valid_setting(set.levels, 1, RTSS_MAX_LEVELS, false, "levels");
  set.windowPages = valid_setting(set.windowPages, 4, 32, true, "windowPages");
  set.slotTexels = valid_setting(set.slotTexels, 32, 128, true, "slotTexels");
  set.borderTexels = valid_setting(set.borderTexels, 1, 4, false, "borderTexels");
  set.texel0 = valid_setting(set.texel0, MIN_TEXEL0, 1000.f, "texel0");
  set.maxAnisotropy = valid_setting(set.maxAnisotropy, 1.f, 8.f, "maxAnisotropy");
  for (int k = 0; k < set.levels; ++k)
    set.recipes[k].pageCost = valid_setting(set.recipes[k].pageCost, 0.1f, 100.f, "pageCost");
  const int W = set.windowPages;
  lvl.resize(set.levels);
  for (int k = 0; k < set.levels; ++k)
  {
    Level &l = lvl[k];
    l.texel = set.texel0 * double(1 << k);
    l.pageWorld = set.pageTexels() * l.texel;
    l.extent = double(W / 2 - 1) * l.pageWorld;
    l.slots.resize(W * W);
    l.front = NO_FRONT;
  }
  levelTab.resize(set.levels);
  slotTab.resize(set.levels * W * W);
}

void RtssScheduler::setWorldBox(const BBox3 &casters_box)
{
  if (hasWorldBox && casters_box == worldBox)
    return;
  worldBox = casters_box;
  hasWorldBox = true;
  invalidate();
}

void RtssScheduler::setSun(const Point3 &dir_to_sun, const Point3 &angular_velocity)
{
  sunDir = normalize(dir_to_sun);
  sunAngVel = angular_velocity;
}

bool RtssScheduler::setRecipes(const RtssSchedulerRecipes &recipes)
{
  bool changed = false;
  for (int k = 0; k < set.levels; ++k)
  {
    const RtssLevelRecipe &r = recipes.recipes[k];
    const bool invalidate = r.raster != set.recipes[k].raster;
    changed |= invalidate;
    if (invalidate)
      invalidateLevel(lvl[k]);
    set.recipes[k] = {r.raster, valid_setting(r.pageCost, 0.1f, 100.f, "pageCost")};
  }
  if (changed)
  {
    planned.clear();
    idle = false;
  }
  return changed;
}

void RtssScheduler::invalidateLevel(Level &l)
{
  for (Slot &s : l.slots)
    if (s.state != EMPTY)
      addDirty(s, wholeSlot());
}

void RtssScheduler::invalidate()
{
  for (Level &l : lvl)
    invalidateLevel(l);
  planned.clear();
  idle = false;
}

void RtssScheduler::addDirty(Slot &s, const Rect &r)
{
  const Rect &d = s.dirty;
  s.dirty =
    s.state != DIRTY ? r : Rect{eastl::min(d.x0, r.x0), eastl::min(d.y0, r.y0), eastl::max(d.x1, r.x1), eastl::max(d.y1, r.y1)};
  s.state = DIRTY;
}

void RtssScheduler::invalidateBoxes(const BBox3 *world_boxes, uint32_t cnt, uint32_t level_mask)
{
  const int S = set.slotTexels, B = set.borderTexels, P = set.pageTexels();
  for (int k = 0; k < set.levels; ++k)
  {
    Level &l = lvl[k];
    if (!(level_mask & (1u << k)) || !l.live)
      continue;
    for (uint32_t i = 0; i < cnt; ++i)
    {
      DPoint3 lo[2], hi[2];
      for (int b = 0; b < 2; ++b)
        if (l.live & (1 << b))
          ls_bounds(l.xform[b], world_boxes[i], lo[b], hi[b]);
      auto small = [&](int b) { return !(l.live & (1 << b)) || (hi[b].x - lo[b].x < 2 * l.texel && hi[b].y - lo[b].y < 2 * l.texel); };
      if (small(0) && small(1))
        continue;
      for (Slot &s : l.slots)
      {
        if (s.state == EMPTY)
          continue;
        const double ox = s.page.x * double(P) - B, oy = s.page.y * double(P) - B;
        auto texel = [&](double v, double o) { return int(eastl::clamp(v - o, 0.0, double(S))); };
        const int x0 = texel(floor(lo[s.bit].x / l.texel), ox), x1 = texel(ceil(hi[s.bit].x / l.texel), ox);
        const int y0 = texel(floor(lo[s.bit].y / l.texel), oy), y1 = texel(ceil(hi[s.bit].y / l.texel), oy);
        if (x0 < x1 && y0 < y1)
          addDirty(s, {uint8_t(x0), uint8_t(y0), uint8_t(x1), uint8_t(y1)});
      }
    }
  }
  planned.clear();
  idle = false;
}

void RtssScheduler::reset()
{
  for (Level &l : lvl)
  {
    l.live = 0;
    l.swept = 0;
    l.fast = false;
    for (Slot &s : l.slots)
      s.state = EMPTY;
  }
  eastl::fill(slotTab.begin(), slotTab.end(), RtssSlot{});
  slotTabDirty = true;
  updateBalance = 0;
  planned.clear();
  idle = false;
}

// constraint: the slot ends the tie, so sort() keeps the order in which the queue is filled
bool RtssScheduler::innerLevelOrder(const QueueItem &a, const QueueItem &b)
{
  return eastl::tie(a.inner, a.level, a.order, a.slot) < eastl::tie(b.inner, b.level, b.order, b.slot);
}

IPoint2 RtssScheduler::windowPage(const Level &l, int slot) const
{
  const int W = set.windowPages;
  return IPoint2(l.origin.x + ((slot % W - l.origin.x) & (W - 1)), l.origin.y + ((slot / W - l.origin.y) & (W - 1)));
}

bool RtssScheduler::incomingPage(const Level &l, const IPoint2 &p, IPoint2 &in) const
{
  const int W = set.windowPages;
  bool outgoing = false;
  in = p;
  for (int a = 0; a < 2; ++a)
    if (const int dir = l.pretrace[a].dir; dir && p[a] == (dir > 0 ? l.origin[a] : l.origin[a] + W - 1))
    {
      in[a] += W * dir;
      outgoing = true;
    }
  return outgoing;
}

bool RtssScheduler::edgeClaims(const Level &l, int slot, const IPoint2 &except) const
{
  const int W = set.windowPages;
  const IPoint2 wp = windowPage(l, slot);
  for (int dy = -1; l.sweeping() && dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx)
    {
      const IPoint2 q = wp + IPoint2(dx * W, dy * W);
      if ((dx || dy) && q != except && (l.cube[l.cur ^ 1] & q))
        return true;
    }
  return false;
}

bool RtssScheduler::depthOk(const Level &l, const Slot &s) const { return s.zStart <= l.reqZ[s.bit][0] && s.zEnd >= l.reqZ[s.bit][1]; }

bool RtssScheduler::isInner(int level, const IPoint2 &p) const
{
  if (level == 0)
    return false;
  const Level &l = lvl[level], &f = lvl[level - 1];
  const double h = (1 - set.seamBand) * f.extent - f.pageWorld, hy = h * l.xform[l.cur].sy / f.xform[f.cur].sy;
  const DPoint3 &c = l.camLs[l.cur];
  return p.x * l.pageWorld >= c.x - h && (p.x + 1) * l.pageWorld <= c.x + h && p.y * l.pageWorld >= c.y - hy &&
         (p.y + 1) * l.pageWorld <= c.y + hy;
}

double RtssScheduler::hOverE(const Level &l, double duration) const
{
  const double d = l.lastSweepSeconds > l.lastPlannedSeconds ? eastl::max(duration, l.lastSweepSeconds) : duration;
  return 1 + eastl::max(0.5, 1.25 * length(dpoint3(cameraVel)) * d / l.extent);
}

void RtssScheduler::updateSweep(int level)
{
  Level &l = lvl[level];
  if (!l.live)
  {
    l.xform[0] = rtss_first_transform(sunDir, camera, rtss_aniso_scale(sunDir, set.maxAnisotropy));
    l.live = 1;
    l.cur = 0;
    l.origin = rtss_page(l.xform[0], camera, l.pageWorld) - IPoint2(set.windowPages / 2, set.windowPages / 2);
  }
  const double rate = length(dpoint3(sunAngVel));
  double t0 = now;
  if (
    l.sweeping() && eastl::none_of(l.slots.begin(), l.slots.end(), [&](const Slot &s) { return s.state != EMPTY && s.bit == l.cur; }))
  {
    const double plannedEnd = l.t0 + l.duration;
    if (rate > 1e-9 && !l.fast && now - plannedEnd < 0.1 * l.duration)
      t0 = eastl::min(now, plannedEnd);
    l.lastSweepSeconds = now - l.t0;
    l.lastPlannedSeconds = l.duration;
    l.cur ^= 1;
    l.live = 1 << l.cur;
    l.fast = false;
    l.front = NO_FRONT;
  }

  double tol = eastl::min(set.toleranceDeg0 * double(1 << level), double(set.toleranceDegMax)) * RAD_PER_DEG, duration, h, fastLimit;
  for (int clamped = 0;; tol = 0.5 * fastLimit, clamped = 1)
  {
    duration =
      rate > 1e-9 ? eastl::clamp(tol / rate, double(set.minSweepSeconds), double(set.maxSweepSeconds)) : set.staticSweepSeconds;
    h = hOverE(l, duration);
    fastLimit = eastl::min(set.fastSweepDeg * RAD_PER_DEG, 0.8 * rtss_window_limit_rad(set.windowPages, float(h), l.xform[l.cur].sy));
    if (clamped || tol <= 0.5 * fastLimit)
      break;
  }
  l.tolRad = float(tol);
  l.fastLimitRad = float(fastLimit);

  if (l.sweeping())
  {
    const DPoint3 rel = l.xform[l.cur].toLs(camera) - l.xform[l.cur].toLs(l.pivot);
    const double theta = angle_between(l.xform[0].d, l.xform[1].d), e = l.extent;
    const bool order = rel.z - e >= eastl::max(0.0, rel.x * l.u.x + rel.y * l.u.y + sqrt(2.0) * e) * tan(theta / 2);
    const double sy = eastl::max(l.xform[0].sy, l.xform[1].sy);
    const bool window = (rel.z + e) * sin(theta) * sy + e * (cos(theta) - 1) <= l.pageWorld;
    if (!order || !window || (!belowHorizon && angle_between(-l.xform[l.cur ^ 1].d, sunDir) > fastLimit))
      l.fast = true;
    return;
  }
  const double step = angle_between(-l.xform[l.cur].d, sunDir);
  if (!belowHorizon && step >= tol * 0.5)
    startSweep(l, sunDir, t0, duration, h, step > fastLimit);
}

void RtssScheduler::startSweep(Level &l, const Point3 &dir_end, double t0, double duration, double h_over_e, bool fast)
{
  const int tgt = l.cur ^ 1;
  const RtssLightXform &old = l.xform[l.cur];
  l.pivot = camera - dpoint3(old.d) * (h_over_e * l.extent);
  const float sy = rtss_aniso_scale(dir_end, set.maxAnisotropy);
  fast |= sy != old.sy;
  l.xform[tgt] = rtss_next_transform(old, dir_end, l.pivot, camera, sy);
  if (!rtss_shift_dir(old, l.xform[tgt], l.u))
    l.u = Point2(1, 0);
  l.live |= 1 << tgt;
  l.fast = fast;
  l.t0 = t0;
  l.duration = duration;
  l.front = NO_FRONT;
  l.swept = 0;
  ++l.sweeps;
}

void RtssScheduler::updateWindow(int level)
{
  Level &l = lvl[level];
  const int W = set.windowPages, cur = l.cur, tgt = cur ^ 1;
  const double pw = l.pageWorld, e = l.extent;
  const RtssLightXform &x = l.xform[cur];
  const DPoint3 dt = l.sweeping() ? dpoint3(l.xform[tgt].d) : DPoint3(0, 0, 0);
  const double sweepZ = e * (fabs(dt * dpoint3(x.ex)) + fabs(dt * dpoint3(x.ey)) / x.sy + fabs(dt * dpoint3(x.d)));
  if (!l.sweeping())
    l.cube[tgt] = IBBox2();
  for (int b = 0; b < 2; ++b)
    if (l.live & (1 << b))
    {
      const DPoint3 c = l.camLs[b] = l.xform[b].toLs(camera);
      l.cube[b] =
        IBBox2(IPoint2(ifloor((c.x - e) / pw), ifloor((c.y - e) / pw)), IPoint2(ifloor((c.x + e) / pw), ifloor((c.y + e) / pw)));
      DPoint3 boxLo(0, 0, -DBL_MAX), boxHi(0, 0, DBL_MAX);
      if (hasWorldBox)
        ls_bounds(l.xform[b], worldBox, boxLo, boxHi);
      const double rz = b == tgt ? eastl::max(e, sweepZ) : e, hz = eastl::max(double(set.maxCasterReach), 2 * rz);
      const double lo = eastl::max(c.z - rz, boxLo.z), hi = eastl::min(c.z + rz, boxHi.z);
      l.boxZ[b][0] = eastl::max(c.z - hz, boxLo.z);
      l.boxZ[b][1] = eastl::max(l.boxZ[b][0], eastl::min(c.z + hz, boxHi.z)); // constraint: the L16 depth step needs a short range
      l.reqZ[b][0] = lo <= hi ? lo : DBL_MAX;
      l.reqZ[b][1] = lo <= hi ? hi : -DBL_MAX;
    }
  for (int i = 0; l.sweeping() && i < 8; ++i)
  {
    const DPoint3 ls = l.xform[tgt].toLs(
      camera + dpoint3(x.ex) * ((i & 1) ? e : -e) + dpoint3(x.ey) * (((i & 2) ? e : -e) / x.sy) + dpoint3(x.d) * ((i & 4) ? e : -e));
    l.cube[tgt] += IPoint2(ifloor(ls.x / pw), ifloor(ls.y / pw));
  }

  const IPoint2 c(ifloor(l.camLs[cur].x / pw), ifloor(l.camLs[cur].y / pw));
  l.pretraceBlockedNow = false;
  const IPoint2 oldOrigin = l.origin;
  for (int a = 0; a < 2; ++a)
  {
    const int move = c[a] - eastl::clamp(c[a], l.origin[a] + W / 2 - 1, l.origin[a] + W / 2);
    if (move)
    {
      l.origin[a] += move;
      l.pretraceBlockedNow |= l.pretrace[a].blocked;
      l.pretrace[a] = Pretrace();
    }
  }
  // constraint: rtss_pack_page keeps 16 bits of a page, so a page more than W out of the window could alias
  for (int i = 0; l.origin != oldOrigin && i < W * W; ++i)
  {
    Slot &s = l.slots[i];
    const IPoint2 d = s.page - windowPage(l, i);
    if (s.state != EMPTY && eastl::max(abs(d.x), abs(d.y)) > W)
    {
      s.state = EMPTY;
      slotTab[level * W * W + i] = {};
      slotTabDirty = true;
    }
  }
  for (int a = 0; a < 2; ++a)
    updatePretrace(level, a);

  l.edges.clear();
  l.edgeNext = 0;
  l.head = 0;
  l.headBlocked = false;
  l.frontAfter = l.front;
  l.sweepList.clear();
  if (!l.sweeping())
    return;
  for (int i = 0; i < W * W; ++i)
    if (l.slots[i].state != EMPTY && l.slots[i].bit == cur)
      l.sweepList.push_back({i, key(l, l.slots[i].page)});
  eastl::sort(l.sweepList.begin(), l.sweepList.end(),
    [](const SweepEntry &a, const SweepEntry &b) { return a.key > b.key || (a.key == b.key && a.slot < b.slot); });
  for (int y = l.cube[tgt].lim[0].y; y <= l.cube[tgt].lim[1].y; ++y)
    for (int x = l.cube[tgt].lim[0].x; x <= l.cube[tgt].lim[1].x; ++x)
    {
      const IPoint2 p(x, y);
      const Slot &s = l.slots[slotIndex(p)];
      if (windowPage(l, slotIndex(p)) != p && !(s.state != EMPTY && s.page == p && s.bit == tgt && depthOk(l, s)))
        l.edges.push_back({p, key(l, p)});
    }
  eastl::sort(l.edges.begin(), l.edges.end(), [](const EdgePage &a, const EdgePage &b) {
    return eastl::tie(b.key, a.page.y, a.page.x) < eastl::tie(a.key, b.page.y, b.page.x);
  });
}

void RtssScheduler::updatePretrace(int level, int axis)
{
  Level &l = lvl[level];
  const int W = set.windowPages, cur = l.cur, tgt = cur ^ 1;
  const double pw = l.pageWorld, center = double(l.origin[axis] + W / 2) * pw;
  const double sweepEnd = l.sweeping() ? eastl::max(now, l.t0 + l.duration) : now;
  int dir = 0;
  double tMove = NEVER;
  for (int d = -1; d <= 1; d += 2)
  {
    auto cross = [&](int b, double from) {
      const double v = dpoint3(axis ? l.xform[b].ey * l.xform[b].sy : l.xform[b].ex) * dpoint3(cameraVel);
      const double dist = d * (center + d * pw - l.camLs[b][axis] - v * (from - now));
      return dist <= 0 ? from : d * v > 1e-6 ? from + dist / (d * v) : NEVER;
    };
    const int outer = l.sweeping() && d * l.camLs[tgt][axis] > d * l.camLs[cur][axis] ? tgt : cur;
    if ((l.camLs[outer][axis] >= center) != (d > 0))
      continue;
    double t = cross(cur, now);
    if (l.sweeping() && t > sweepEnd)
      t = cross(tgt, sweepEnd);
    if (t < tMove)
      tMove = t, dir = d;
  }
  Pretrace &pt = l.pretrace[axis];
  if (dir != pt.dir)
  {
    pt = Pretrace();
    pt.dir = dir;
    pt.tStart = now;
  }
  pt.tMove = tMove;
}

// a later sweep head of the frame can still move a new page to the target bit
uint8_t RtssScheduler::jobBit(const Level &l, const Slot &s, const IPoint2 &page, JobKind kind) const
{
  if (kind == SAME_PAGE || kind == DIRTY_RECT)
    return s.bit;
  return kind == SWEEP_PAGE || key(l, page) > headKey(l) ? l.cur ^ 1 : l.cur;
}

int RtssScheduler::joinBox(int level, int bit, const IPoint2 &p) const
{
  for (int b = 0; b < (int)boxes.size(); ++b)
    if (boxes[b].level == level && boxes[b].bit == bit && rtss_box_takes(set, boxes[b].pages, boxes[b].count, p))
      return b;
  return boxes.size();
}

bool RtssScheduler::addJob(int level, int slot, const IPoint2 &page, JobKind kind, uint8_t cls, double &budget, bool swept)
{
  Slot &s = lvl[level].slots[slot];
  const bool raster = set.recipes[level].raster != RtssRaster::None;
  const Rect rect = kind == DIRTY_RECT && !raster ? s.dirty : wholeSlot();
  const int bit = jobBit(lvl[level], s, page, kind), box = raster ? joinBox(level, bit, page) : -1;
  const bool pays = box >= 0 && (box == (int)boxes.size() || !boxes[box].paid) && callCost(level) > 0;
  const double cost = jobCost(level, rect.area()) + (pays ? callCost(level) : 0.0);
  if (s.jobFrame == frame || cost > budget + 1e-9)
    return false;
  budget -= cost;
  s.jobFrame = frame;
  if (box == (int)boxes.size())
    boxes.push_back({IBBox2(), level, bit, 0, false});
  if (box >= 0)
  {
    boxes[box].pages += page;
    ++boxes[box].count;
    boxes[box].paid = true;
  }
  planned.push_back({level, slot, page, rect, kind, cls, 0, swept, 0, 0, box, pays});
  return true;
}

int RtssScheduler::edgesNeeded(int level, double above_key) const
{
  const Level &l = lvl[level];
  int n = 0;
  for (int i = l.edgeNext; i < (int)l.edges.size() && l.edges[i].key > above_key; ++i)
    n += edgeFree(l, l.slots[slotIndex(l.edges[i].page)], l.edges[i].page);
  return n;
}

bool RtssScheduler::requireEdges(int level, double above_key)
{
  const int n = edgesNeeded(level, above_key);
  const double need = n * pageCost(level) + (n ? callCost(level) : 0.0);
  double &left = requiredLeft(level);
  if (need > left + 1e-9 && need <= (level == set.levels - 1 ? set.validityBurstPagesPerFrame : set.updatePagesPerFrame) + 1e-9)
    return false;
  Level &l = lvl[level];
  for (; l.edgeNext < (int)l.edges.size() && l.edges[l.edgeNext].key > above_key; ++l.edgeNext)
  {
    const IPoint2 &p = l.edges[l.edgeNext].page;
    const int s = slotIndex(p);
    if (!edgeFree(l, l.slots[s], p))
      continue;
    if (!addJob(level, s, p, NEW_PAGE, RTSS_REQUIRED, left))
      break;
    jobStats.pagesEdge += 1;
  }
  return l.edgeNext == (int)l.edges.size() || l.edges[l.edgeNext].key <= above_key; // need counts one call only
}

int RtssScheduler::sweepHead(const Level &l) const { return l.sweeping() && l.head < (int)l.sweepList.size() ? l.head : -1; }

double RtssScheduler::headKey(const Level &l) const
{
  return !l.sweeping() ? DBL_MAX : l.head < (int)l.sweepList.size() ? l.sweepList[l.head].key : -DBL_MAX;
}

bool RtssScheduler::takeSweepHead(int level, double &budget, uint8_t cls)
{
  Level &l = lvl[level];
  const int h = sweepHead(l);
  if (h < 0 || l.headBlocked)
    return false;
  const SweepEntry &e = l.sweepList[h];
  const Slot &s = l.slots[e.slot];
  int replaced = -1;
  for (int i = int(planned.size()) - 1; s.jobFrame == frame && i >= 0 && replaced < 0; --i)
    if (planned[i].level == level && planned[i].slot == e.slot)
      replaced = i;
  if (replaced >= 0 && planned[replaced].kind == NEW_PAGE)
  {
    ++l.head;
    return true;
  }
  const IPoint2 wp = windowPage(l, e.slot);
  IPoint2 want;
  const bool outgoing = incomingPage(l, wp, want);
  if (s.jobFrame != frame && s.page != want && !inUse(l, s) && !edgeClaims(l, e.slot, want))
  {
    const uint8_t newCls = cls == RTSS_REQUIRED ? cls : outgoing ? RTSS_PRETRACE : RTSS_FILL;
    l.headBlocked = !addJob(level, e.slot, want, NEW_PAGE, newCls, budget, true);
    l.head += !l.headBlocked;
    return !l.headBlocked;
  }
  const bool depthJob = replaced >= 0 && planned[replaced].kind == SAME_PAGE;
  double &pay = depthJob ? requiredLeft(level) : budget;
  const double refund =
    replaced >= 0 ? jobCost(level, planned[replaced].rect.area()) + (planned[replaced].paysCall ? callCost(level) : 0.0) : 0.0;
  if (!requireEdges(level, e.key) || (replaced >= 0 && pay + refund < pageCost(level) + callCost(level)))
  {
    l.headBlocked = true;
    return false;
  }
  if (replaced >= 0) // -V779 a replaced SAME_PAGE job reaches here
  {
    pay += refund;
    if (const PlannedJob &r = planned[replaced]; r.box >= 0)
    {
      --boxes[r.box].count;
      boxes[r.box].paid &= !r.paysCall;
    }
    planned.erase(planned.begin() + replaced);
    l.slots[e.slot].jobFrame = ~0u;
  }
  if (!addJob(level, e.slot, s.page, SWEEP_PAGE, depthJob ? uint8_t(RTSS_REQUIRED) : cls, pay, true))
  {
    l.headBlocked = true;
    return false;
  }
  l.frontAfter = e.key;
  ++l.head;
  const int next = sweepHead(l);
  if (next >= 0 && l.slots[l.sweepList[next].slot].jobFrame == frame)
    takeSweepHead(level, budget, cls);
  return true;
}

double RtssScheduler::accrueUpdate()
{
  const double batch = double(set.batchFrames) * set.updatePagesPerFrame;
  updateBalance = eastl::clamp(updateBalance + set.updatePagesPerFrame, -batch, batch); // a budget cut forgives the deeper debt
  return batch;
}

void RtssScheduler::planRequired()
{
  burstLeft = set.validityBurstPagesPerFrame;
  const double batch = accrueUpdate();
  updateLeft = updateBalance > 1e-9 ? batch : 0.0; // a frame in debt builds nothing until the balance is positive again
  updateBalance -= updateLeft;
  required.clear();
  for (int k = 0; k < set.levels; ++k)
  {
    const Level &l = lvl[k];
    for (int s = 0; s < (int)l.slots.size(); ++s)
    {
      const IPoint2 p = windowPage(l, s);
      const Slot &sl = l.slots[s];
      const bool holds = sl.state != EMPTY && sl.page == p;
      if (inUse(l, p) && !(holds && depthOk(l, sl)) && (holds || !inUse(l, sl)))
        required.push_back({camDist2(l, p), k, s, p, RTSS_REQUIRED, isInner(k, p)});
    }
  }
  const int last = set.levels - 1;
  eastl::sort(required.begin(), required.end(), [last](const QueueItem &a, const QueueItem &b) {
    if ((a.level == last) != (b.level == last))
      return a.level == last;
    // constraint: the coarsest level has no fallback, so its pages go nearest the camera first, inner or not
    return a.level == last ? eastl::tie(a.order, a.slot) < eastl::tie(b.order, b.slot) : innerLevelOrder(a, b);
  });
  auto require = [&](const QueueItem &q) {
    Level &l = lvl[q.level];
    const bool samePage = l.slots[q.slot].state != EMPTY && l.slots[q.slot].page == q.page;
    const int h = sweepHead(l);
    if (samePage && h >= 0 && l.sweepList[h].slot == q.slot && takeSweepHead(q.level, requiredLeft(q.level), RTSS_REQUIRED))
      return;
    addJob(q.level, q.slot, q.page, samePage ? SAME_PAGE : NEW_PAGE, RTSS_REQUIRED, requiredLeft(q.level));
  };

  // constraint: no fallback, so a missing coarsest page takes the update budget too, past the burst and first
  burstLeft += updateLeft;
  size_t i = 0;
  for (; i < required.size() && required[i].level == last; ++i)
    require(required[i]);
  requireEdges(last, lvl[last].front);
  updateLeft = eastl::min(updateLeft, burstLeft);
  burstLeft -= updateLeft;
  while (lvl[last].sweeping() && lvl[last].fast && takeSweepHead(last, burstLeft, RTSS_REQUIRED)) {}
  for (int k = 0; k < last; ++k)
  {
    for (; i < required.size() && required[i].level == k && !required[i].inner; ++i)
      require(required[i]);
    requireEdges(k, lvl[k].front);
  }
  for (int k = 0; k < last; ++k)
    while (lvl[k].sweeping() && lvl[k].fast && takeSweepHead(k, updateLeft, RTSS_REQUIRED)) {}
  for (; i < required.size(); ++i)
    require(required[i]);
}

// the queue pages from index from on that join a box of the level of the last box at page cost, ahead of their turn
void RtssScheduler::fillBox(size_t from, double horizon, double &budget)
{
  const int level = boxes.back().level;
  const Level &l = lvl[level];
  for (size_t i = from; i < queue.size() && budget >= pageCost(level) - 1e-9; ++i)
  {
    const QueueItem &q = queue[i];
    const JobKind kind = q.cls == RTSS_DIRTY ? DIRTY_RECT : NEW_PAGE;
    if (q.level == level && !(q.cls == RTSS_PRETRACE && q.order > horizon) && !(q.cls == RTSS_DIRTY && l.sweeping()) &&
        joinBox(level, jobBit(l, l.slots[q.slot], q.page, kind), q.page) < (int)boxes.size())
      addJob(level, q.slot, q.page, kind, q.cls, budget);
  }
}

void RtssScheduler::planQueue()
{
  const int W = set.windowPages;
  double &queueLeft = updateLeft, minCost = DBL_MAX;
  for (int k = 0; k < set.levels; ++k)
    minCost = eastl::min(minCost, pageCost(k));
  queue.clear();
  for (int k = 0; k < set.levels; ++k)
  {
    Level &l = lvl[k];
    int rank[2][32];
    for (int a = 0; a < 2; ++a)
    {
      const int jc = eastl::clamp(ifloor(l.camLs[l.cur][1 - a] / l.pageWorld) - l.origin[1 - a], 0, W - 1);
      for (int d = 0, n = 0; n < W; ++d)
      {
        if (jc + d < W)
          rank[a][jc + d] = n++;
        if (d && jc - d >= 0)
          rank[a][jc - d] = n++;
      }
    }
    for (int s = 0; s < W * W; ++s)
    {
      const Slot &sl = l.slots[s];
      const IPoint2 p = windowPage(l, s);
      IPoint2 in;
      const bool outgoing = incomingPage(l, p, in);
      if (sl.state == DIRTY && sl.jobFrame != frame && (sl.page == p || inUse(l, sl)))
        queue.push_back({now, k, s, sl.page, RTSS_DIRTY, isInner(k, sl.page)});
      if (!outgoing && sl.jobFrame != frame && !(sl.state != EMPTY && (sl.page == p || inUse(l, sl))) && !edgeClaims(l, s, p))
        queue.push_back({camDist2(l, p), k, s, p, RTSS_FILL, isInner(k, p)});
      if (!outgoing || sl.jobFrame == frame || (sl.state != EMPTY && sl.page == in))
        continue;
      const bool blocked = inUse(l, sl) || edgeClaims(l, s, in);
      double deadline = -DBL_MAX;
      for (int a = 0; a < 2; ++a)
      {
        Pretrace &pt = l.pretrace[a];
        if (in[a] == p[a])
          continue;
        pt.blocked |= blocked;
        deadline = eastl::max(deadline, pt.tStart + (rank[a][p[1 - a] - l.origin[1 - a]] + 1) * (pt.tMove - frameDt - pt.tStart) / W);
      }
      l.pretraceBlockedNow |= blocked;
      if (!blocked)
        queue.push_back({deadline, k, s, in, RTSS_PRETRACE, false});
    }
  }
  eastl::sort(queue.begin(), queue.end(), [](const QueueItem &a, const QueueItem &b) {
    const bool fa = a.cls == RTSS_FILL, fb = b.cls == RTSS_FILL;
    return fa != fb ? fb
           : fa     ? innerLevelOrder(a, b)
                    : eastl::tie(a.inner, a.order, a.level, a.slot, a.cls) < eastl::tie(b.inner, b.order, b.level, b.slot, b.cls);
  });
  auto headDeadline = [&](const Level &l) {
    const int h = sweepHead(l);
    return l.fast || l.headBlocked || h < 0 ? NEVER : l.t0 + (l.swept + h + 1) * l.duration / (l.swept + l.sweepList.size());
  };
  bool due = !planned.empty();
  for (const QueueItem &q : queue)
    due |= q.cls == RTSS_DIRTY || (q.cls == RTSS_PRETRACE && q.order <= now);
  for (const Level &l : lvl)
    due |= headDeadline(l) <= now;
  // constraint: a dispatch waits for its slowest rays
  const double horizon = due ? NEVER : now, sweepHorizon = now + (set.batchFrames - 1) * frameDt;
  for (size_t i = 0;; ++i)
  {
    const double order = i < queue.size() && queue[i].cls != RTSS_FILL ? eastl::min(queue[i].order, horizon) : horizon;
    for (int best = 0; best >= 0;)
    {
      best = -1;
      for (int k = 0; k < set.levels; ++k)
        if (const double d = headDeadline(lvl[k]); d <= eastl::min(order, sweepHorizon) && (best < 0 || d < headDeadline(lvl[best])))
          best = k;
      if (best >= 0)
        takeSweepHead(best, queueLeft, RTSS_SWEEP);
    }
    if (i == queue.size() || (queue[i].cls == RTSS_FILL && queueLeft < minCost))
      break;
    const QueueItem &q = queue[i];
    if (q.cls == RTSS_PRETRACE && q.order > horizon)
      continue;
    const Level &l = lvl[q.level];
    const int h = l.fast ? -1 : sweepHead(l);
    if (q.cls == RTSS_DIRTY && h >= 0 && l.sweepList[h].slot == q.slot && l.slots[q.slot].jobFrame != frame &&
        takeSweepHead(q.level, queueLeft, RTSS_SWEEP))
      continue;
    const size_t opened = boxes.size();
    addJob(q.level, q.slot, q.page, q.cls == RTSS_DIRTY ? DIRTY_RECT : NEW_PAGE, q.cls, queueLeft);
    if (boxes.size() > opened && callCost(q.level) > 0)
      fillBox(i + 1, horizon, queueLeft);
  }
}

void RtssScheduler::buildJobs()
{
  const int W = set.windowPages, S = set.slotTexels;
  for (int i = 0; i < (int)planned.size(); ++i)
  {
    PlannedJob &j = planned[i];
    const Level &l = lvl[j.level];
    const Slot &s = l.slots[j.slot];
    j.bit = jobBit(l, s, j.page, j.kind);
    const int area = j.rect.area();
    jobStats.pages[j.cls] += area / double(S * S);
    jobStats.cost[j.cls] += jobCost(j.level, area) + (j.paysCall ? callCost(j.level) : 0.0);
    jobStats.rays += set.recipes[j.level].raster == RtssRaster::All ? 0 : area;
    j.zStart = area == S * S ? l.boxZ[j.bit][0] : s.zStart;
    j.zEnd = area == S * S ? l.boxZ[j.bit][1] : s.zEnd;
    RtssJob &job = framePlan.jobs.push_back();
    job.levelSlot = (j.level << 16) | ((j.slot / W) << 8) | (j.slot % W);
    job.flags = j.bit;
    job.page = int2(j.page.x, j.page.y);
    job.rect = j.rect.x0 | (j.rect.y0 << 8) | (j.rect.x1 << 16) | (uint32_t(j.rect.y1) << 24);
    job.zStart = float(j.zStart - l.xform[j.bit].z0);
    job.zEnd = float(j.zEnd - l.xform[j.bit].z0);
    job.rasterTile = ~0u;
    framePlan.jobClass.push_back(j.cls | (j.paysCall ? RTSS_JOB_CALL : 0) | (isInner(j.level, j.page) ? RTSS_JOB_INNER : 0) |
                                 (j.swept ? RTSS_JOB_SWEPT : 0) |
                                 (s.state != EMPTY && s.page == j.page && s.bit == j.bit ? RTSS_JOB_SAME_PAGE : 0));
    for (int g = 0; g < (area + RTSS_BUILD_GROUP - 1) / RTSS_BUILD_GROUP; ++g)
      framePlan.groups.push_back((uint32_t(i) << 16) | g);
  }
}

void RtssScheduler::buildLevelTable()
{
  for (int k = 0; k < set.levels; ++k)
  {
    const Level &l = lvl[k];
    RtssLevel &t = levelTab[k];
    for (int b = 0; b < 2; ++b)
    {
      Point4 rows[3];
      rtss_pack_xform(l.liveXform(b), dpoint3(rtssOrigin), l.origin, l.pageWorld, rows);
      for (int r = 0; r < 3; ++r)
        t.xform[b * 3 + r] = rows[r];
    }
    t.pageOrigin = int2(l.origin.x, l.origin.y);
    t.flags = l.newestBit() | (l.cur ? RTSS_LEVEL_CURRENT_BIT : 0) | (l.sweeping() ? RTSS_LEVEL_SWEEPING : 0);
    t.invPage = float(1 / l.pageWorld);
  }
}

RtssFramePlan &RtssScheduler::plan(const DPoint3 &camera_pos, const Point3 &camera_velocity, double time_sec)
{
  frameDt = frame ? eastl::max(time_sec - now, 0.0) : 0.0;
  now = time_sec;
  ++frame;
  if (idle && camera_pos == lastCameraPos && camera_velocity == lastCameraVel && sunDir == lastSunDir && sunAngVel == lastSunAngVel)
  {
    accrueUpdate();
    return framePlan;
  }
  lastCameraPos = camera_pos;
  lastCameraVel = camera_velocity;
  lastSunDir = sunDir;
  lastSunAngVel = sunAngVel;
  camera = camera_pos;
  rtssOrigin = point3(camera_pos);
  cameraVel = length(camera_velocity) <= set.maxCameraSpeed ? camera_velocity : Point3(0, 0, 0);
  if (hasWorldBox && (camera.y < worldBox[0].y || camera.y > worldBox[1].y))
  {
    // constraint: no caster or receiver lies outside the height of the world box, so the window keeps to its edge
    camera.y = eastl::clamp(camera.y, double(worldBox[0].y), double(worldBox[1].y));
    cameraVel.y = 0;
  }
  planned.clear();
  boxes.clear();
  jobStats = {};
  framePlan.jobs.clear();
  framePlan.jobClass.clear();
  framePlan.groups.clear();

  belowHorizon = sunDir.y < set.minSunY;
  for (int k = 0; k < set.levels; ++k)
    updateSweep(k);
  for (int k = 0; k < set.levels; ++k)
    updateWindow(k);
  planRequired();
  planQueue();
  updateBalance += updateLeft;
  for (int k = 0; k < set.levels; ++k)
    lvl[k].requiredPending = lvl[k].sweeping() ? edgesNeeded(k, lvl[k].frontAfter) : 0;
  for (const QueueItem &q : required) // the queue phase can build a REQUIRED page
    lvl[q.level].requiredPending += lvl[q.level].slots[q.slot].jobFrame != frame;
  idle = planned.empty() && queue.empty();
  for (const Level &l : lvl)
    idle &= l.live && !l.sweeping() && !l.requiredPending;
  buildJobs();
  buildLevelTable();
  return framePlan;
}

void RtssScheduler::commit(dag::ConstSpan<uint32_t> skipped_jobs, dag::ConstSpan<uint32_t> provisional_jobs)
{
  const int W = set.windowPages;
  uint32_t sweepSkipped = 0;
  auto has = [](dag::ConstSpan<uint32_t> list, uint32_t i) { return eastl::binary_search(list.begin(), list.end(), i); };
  for (uint32_t i = 0; i < planned.size(); ++i)
  {
    const PlannedJob &j = planned[i];
    Level &l = lvl[j.level];
    Slot &s = l.slots[j.slot];
    if (has(skipped_jobs, i))
    {
      sweepSkipped |= j.swept ? 1u << j.level : 0;
      continue;
    }
    G_ASSERTF(!j.swept || !(sweepSkipped & (1u << j.level)), "rtss: level %d built a sweep job after a skipped one", j.level);
    if (j.rect == wholeSlot())
    {
      s.page = j.page;
      s.bit = j.bit;
      s.zStart = j.zStart;
      s.zEnd = j.zEnd;
      s.state = VALID;
      slotTab[j.level * W * W + j.slot] = {rtss_pack_page(int2(s.page.x, s.page.y)), RTSS_SLOT_VALID | (s.bit ? RTSS_SLOT_BIT : 0),
        float(s.zStart - l.xform[s.bit].z0), float(s.zEnd - l.xform[s.bit].z0)};
      slotTabDirty = true;
    }
    else if (s.dirty == j.rect)
      s.state = VALID;
    if (has(provisional_jobs, i))
      addDirty(s, wholeSlot());
    if (j.kind == SWEEP_PAGE)
    {
      ++l.swept;
      l.front = key(l, j.page);
    }
  }
  slotTabChanged = eastl::exchange(slotTabDirty, false);
}

void RtssScheduler::globals(IPoint4 &params_i, Point4 &params, Point4 &origin) const
{
  params_i = IPoint4(set.levels, set.windowPages, set.slotTexels, set.pageTexels());
  params =
    Point4(float(1 / lvl[0].extent), belowHorizon ? 1.f : 0.f, float(lvl[0].cur), set.seamBand > 0.f ? 1.f / set.seamBand : 1e30f);
  const float sy0 = lvl[0].xform[lvl[0].cur].sy;
  float syMin = sy0;
  for (const Level &l : lvl)
    for (int b = 0; b < 2; ++b)
      if (l.live & (1 << b))
        syMin = eastl::min(syMin, l.xform[b].sy);
  origin = Point4::xyzV(rtssOrigin, syMin / sy0);
}

bool RtssScheduler::isReady(const BBox3 &box, uint32_t level_mask) const
{
  for (int k = 0; k < set.levels; ++k)
  {
    const Level &l = lvl[k];
    if (!(level_mask & (1u << k)))
      continue;
    if (!l.live || (l.sweeping() && l.fast))
      return false;
    DPoint3 lo, hi;
    ls_bounds(l.xform[l.cur], box, lo, hi);
    if (hi.z < l.camLs[l.cur].z - l.extent || lo.z > l.camLs[l.cur].z + l.extent)
      continue;
    const IBBox2 &cube = l.cube[l.cur];
    const IPoint2 p0(eastl::max(ifloor(lo.x / l.pageWorld), cube.lim[0].x), eastl::max(ifloor(lo.y / l.pageWorld), cube.lim[0].y));
    const IPoint2 p1(eastl::min(ifloor(hi.x / l.pageWorld), cube.lim[1].x), eastl::min(ifloor(hi.y / l.pageWorld), cube.lim[1].y));
    for (int y = p0.y; y <= p1.y; ++y)
      for (int x = p0.x; x <= p1.x; ++x)
      {
        const Slot &s = l.slots[slotIndex(IPoint2(x, y))];
        if (s.state == EMPTY || s.page != IPoint2(x, y) || !depthOk(l, s))
          return false;
      }
  }
  return true;
}

void RtssScheduler::getStats(RtssSchedulerStats &out) const
{
  out = jobStats;
  out.sunBelowHorizon = belowHorizon;
  for (const Level &l : lvl)
  {
    RtssSchedulerLevelStats &o = out.levels.push_back();
    o.cube[0] = l.cube[0];
    o.cube[1] = l.cube[1];
    o.u = l.u;
    o.validSlots = o.dirtySlots = o.emptySlots = 0;
    int unswept = 0;
    for (const Slot &s : l.slots)
    {
      ++(s.state == VALID ? o.validSlots : s.state == DIRTY ? o.dirtySlots : o.emptySlots);
      unswept += s.state != EMPTY && s.bit == l.cur;
    }
    const int pages = l.swept + (l.sweeping() ? unswept : 0);
    o.sweepProgress = l.sweeping() && pages ? float(l.swept) / pages : 1.f;
    o.sunErrorDeg = l.live ? float(angle_between(sunDir, -l.xform[l.newestBit()].d) / RAD_PER_DEG) : 0.f;
    o.toleranceDeg = float(l.tolRad / RAD_PER_DEG);
    o.fastLimitDeg = float(l.fastLimitRad / RAD_PER_DEG);
    o.sweepSeconds = float(l.duration);
    o.lastSweepSeconds = float(l.lastSweepSeconds);
    o.lastPlannedSeconds = float(l.lastPlannedSeconds);
    const int h = sweepHead(l);
    o.headSlot = h < 0 ? -1 : l.sweepList[h].slot;
    o.sweeps = l.sweeps;
    o.sweepPages = pages;
    o.liveMask = l.live;
    o.sweeping = l.sweeping();
    o.fastSweep = l.fast;
    o.pretraceBlocked = l.pretraceBlockedNow;
    o.requiredPending = l.requiredPending;
  }
}
