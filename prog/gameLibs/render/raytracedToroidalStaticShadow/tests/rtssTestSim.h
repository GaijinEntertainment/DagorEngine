// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <core/rtssScheduler.h>
#include <core/rtssCpuLookup.h>
#include <math/random/dag_random.h>
#include <EASTL/algorithm.h>
#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <float.h>
#include <string.h>

enum RtssInvariant
{
  RTSS_I1_LIVE_XFORMS,
  RTSS_I2_KEY_ORDER,
  RTSS_I3_WHOLE_SLOT,
  RTSS_I4_ONE_JOB,
  RTSS_I5_BUDGET,
  RTSS_I6_FINITE_SWEEP,
  RTSS_I7_COVERAGE,
  RTSS_I8_KEEP_USED_PAGE,
  RTSS_I9_TABLE_SIGNAL,
  RTSS_I10_PLAN_TOTALS,
  RTSS_I11_STATS,
  RTSS_INVARIANTS
};

struct RtssCoverageMiss
{
  int frame, level;
  Point3 cubePos; // E_k units, level's current transform
  Point2 u;
  bool pageMissing;
};

inline Point3 rtss_rotate(const Point3 &v, const Point3 &axis, double angle)
{
  return point3(rtss_rotate(dpoint3(v), normalize(dpoint3(axis)), angle));
}

// the page with these low 16 bits that is nearest to near
inline IPoint2 rtss_unpack_page(uint32_t p, const IPoint2 &near)
{
  return near + IPoint2(int16_t(uint16_t(p) - uint16_t(near.x)), int16_t(uint16_t(p >> 16) - uint16_t(near.y)));
}

struct RtssSim
{
  RtssSchedulerSettings settings;
  RtssScheduler sched;
  double time = 0, dt = 1.0 / 60;
  int frame = 0, seed = 1;
  DPoint3 camera = DPoint3(10.3, 2.1, -7.7);
  Point3 cameraVel = Point3(0, 0, 0);
  BBox3 worldBox;
  Point3 sun = normalize(Point3(0.35f, 0.8f, 0.3f)), sunAngVel = Point3(0, 0, 0);
  int coverageFrom = 10, selectionSamples = 0;
  bool strictCoverage = true, resetPending = true;
  eastl::function<void(dag::Vector<uint32_t> &skipped_jobs, dag::Vector<uint32_t> &provisional_jobs)> beforeCommit;

  int violations[RTSS_INVARIANTS] = {};
  eastl::string firstViolation[RTSS_INVARIANTS];
  dag::Vector<RtssCoverageMiss> misses;
  int selectionMissCount = 0;

  dag::Span<RtssJob> jobs; // valid until the next step()
  dag::ConstSpan<uint8_t> jobClass;
  dag::Vector<uint32_t> skipped, provisional;
  RtssSchedulerStats stats;
  double burstCost = 0, updateCost = 0;
  dag::Vector<RtssSlot> prevSlots;
  dag::Vector<uint8_t> jobCount;

  explicit RtssSim(const RtssSchedulerSettings &s = RtssSchedulerSettings()) : sched(s) { settings = sched.getSettings(); }

  int windowPages() const { return settings.windowPages; }
  double extent(int level) const { return sched.extent(level); }
  double texel(int level) const { return sched.texel(level); }
  const RtssLevel &levelTab(int level) const { return sched.levelTable()[level]; }
  int curBit(int level) const { return (levelTab(level).flags & RTSS_LEVEL_CURRENT_BIT) ? 1 : 0; }
  int newestBit(int level) const { return levelTab(level).flags & RTSS_LEVEL_NEWEST_BIT; }
  bool sweeping(int level) const { return (levelTab(level).flags & RTSS_LEVEL_SWEEPING) != 0; }
  Point3 row(int level, int bit, int r) const
  {
    const float4 &v = levelTab(level).xform[bit * 3 + r];
    return Point3(v.x, v.y, v.z);
  }
  dag::ConstSpan<RtssSlot> slots() const { return sched.slotTable(); }
  const RtssSlot &slot(int level, const IPoint2 &p) const
  {
    const int W = windowPages();
    return slots()[level * W * W + rtss_slot_index(int2(p.x, p.y), W)];
  }
  bool holds(int level, const IPoint2 &p, int bit) const
  {
    const RtssSlot &s = slot(level, p);
    return (s.flags & RTSS_SLOT_VALID) && s.page == rtss_pack_page(int2(p.x, p.y)) && rtss_slot_bit(s.flags) == uint32_t(bit);
  }
  static bool holdsJob(const RtssSlot &s, const RtssJob &j) // the page of the job with its bit
  {
    return (s.flags & RTSS_SLOT_VALID) && s.page == rtss_pack_page(j.page) && rtss_slot_bit(s.flags) == (j.flags & RTSS_JOB_BIT);
  }
  bool inWindow(int level, const IPoint2 &p) const
  {
    const int2 &o = levelTab(level).pageOrigin;
    return p.x >= o.x && p.y >= o.y && p.x < o.x + windowPages() && p.y < o.y + windowPages();
  }
  bool levelComplete(int level) const
  {
    const IBBox2 &c = stats.levels[level].cube[curBit(level)];
    for (int y = c.lim[0].y; y <= c.lim[1].y; ++y)
      for (int x = c.lim[0].x; x <= c.lim[1].x; ++x)
        if (!holds(level, IPoint2(x, y), curBit(level)))
          return false;
    return true;
  }
  double otherPages() const
  {
    return stats.pages[RTSS_DIRTY] + stats.pages[RTSS_SWEEP] + stats.pages[RTSS_PRETRACE] + stats.pages[RTSS_FILL];
  }
  int jobLevel(const RtssJob &j) const { return j.levelSlot >> 16; }
  uint32_t wholeRect() const { return (uint32_t(settings.slotTexels) << 16) | (uint32_t(settings.slotTexels) << 24); }
  int jobSlotIndex(const RtssJob &j) const
  {
    const int W = windowPages();
    return (jobLevel(j) * W + ((j.levelSlot >> 8) & 0xff)) * W + (j.levelSlot & 0xff);
  }
  RtssCpuTables tables() const
  {
    RtssCpuTables t;
    t.levels = sched.levelTable();
    t.slots = slots();
    sched.globals(t.paramsI, t.params, t.origin);
    t.centerY = sched.centerY();
    t.texel0 = float(sched.texel(0));
    return t;
  }
  Point3 cubePoint(int level, int bit, const Point3 &p) const
  {
    const Point3 ey = row(level, bit, 1);
    return (row(level, bit, 0) * p.x + ey * (p.y / lengthSq(ey)) + row(level, bit, 2) * p.z) * float(extent(level)) +
           Point3(0, sched.centerY(), 0);
  }

  void fail(RtssInvariant i, const eastl::string &what)
  {
    if (violations[i]++ == 0)
      firstViolation[i].sprintf("frame %d: %s", frame, what.c_str());
  }

  void reset()
  {
    sched.reset();
    resetPending = true;
  }

  void setWorldBox(const BBox3 &box)
  {
    worldBox = box;
    sched.setWorldBox(box);
  }

  DPoint3 center() const
  {
    DPoint3 c = camera;
    if (!worldBox.isempty())
      c.y = eastl::clamp(c.y, double(worldBox[0].y), double(worldBox[1].y));
    return c;
  }

  void run(int frames)
  {
    for (int i = 0; i < frames; ++i)
      step();
  }

  void step()
  {
    prevSlots.assign(slots().begin(), slots().end());
    time += dt;
    ++frame;
    camera += dpoint3(cameraVel) * dt;
    if (lengthSq(sunAngVel) > 0)
      sun = normalize(rtss_rotate(sun, sunAngVel, length(sunAngVel) * dt));
    sched.setSun(sun, sunAngVel);
    RtssFramePlan &plan = sched.plan(camera, cameraVel, time);
    jobs = make_span(plan.jobs);
    jobClass = make_span_const(plan.jobClass);
    sched.getStats(stats);
    checkPlan(plan.groups);
    skipped.clear();
    provisional.clear();
    if (beforeCommit)
      beforeCommit(skipped, provisional);
    sched.commit(make_span_const(skipped), make_span_const(provisional));
    const bool differ = memcmp(prevSlots.data(), slots().data(), prevSlots.size() * sizeof(RtssSlot)) != 0;
    bool wholeBuilt = false;
    for (uint32_t i = 0; i < jobs.size(); ++i)
      wholeBuilt |= jobs[i].rect == wholeRect() && !eastl::binary_search(skipped.begin(), skipped.end(), i);
    if (sched.slotTableChanged() ? !(differ || wholeBuilt || resetPending) : differ)
      fail(RTSS_I9_TABLE_SIGNAL,
        eastl::string(eastl::string::CtorSprintf(), "slot table signal %d with %d jobs, %d skipped, reset %d, table changed %d",
          sched.slotTableChanged(), (int)jobs.size(), (int)skipped.size(), resetPending, differ));
    resetPending = false;
    checkTables();
  }

  bool inUse(int level, const IPoint2 &page) const
  {
    const int cur = curBit(level);
    return (stats.levels[level].cube[cur] & page) || (sweeping(level) && (stats.levels[level].cube[cur ^ 1] & page));
  }

  void checkPlan(dag::ConstSpan<uint32_t> groups)
  {
    const int W = windowPages();
    eastl::string s;
    IPoint4 paramsI;
    Point4 params, origin;
    sched.globals(paramsI, params, origin);
    // constraint: one float ulp, x87 compares with no rounding to float
    if (length(DPoint3(origin.x, origin.y, origin.z) - camera) > length(camera) * 1.2e-7 || sched.cameraPos() != center())
      fail(RTSS_I11_STATS, "rtss_origin is not the camera, or cameraPos() is not the window center");
    if (params.z != float(curBit(0)))
      fail(RTSS_I11_STATS, "rtss_params.z is not the current bit of level 0");
    for (int k = 0; k < settings.levels; ++k)
    {
      const uint8_t live = stats.levels[k].liveMask, expect = sweeping(k) ? 3 : 1 << curBit(k);
      if (live != expect)
        fail(RTSS_I1_LIVE_XFORMS, s.sprintf("level %d live mask %d, expected %d", k, live, expect));
      const float d = stats.levels[k].sweepSeconds;
      if (sweeping(k) && !(d >= eastl::min(settings.minSweepSeconds, settings.staticSweepSeconds) &&
                           d <= eastl::max(settings.maxSweepSeconds, settings.staticSweepSeconds)))
        fail(RTSS_I6_FINITE_SWEEP, s.sprintf("level %d sweep duration %g", k, d));
      checkLevelStats(k);
    }
    const RtssSchedulerSettings &set = sched.getSettings();
    const double cap = eastl::max(eastl::min(set.updatePagesPerFrame, set.validityBurstPagesPerFrame), 1);
    double cost[RTSS_PAGE_CLASS_COUNT] = {};
    burstCost = updateCost = 0;
    for (int ji = 0; ji < (int)jobs.size(); ++ji)
    {
      const int level = jobLevel(jobs[ji]), cls = jobClass[ji] & RTSS_JOB_CLASS_MASK;
      const double pageCost = eastl::min(double(set.recipes[level].pageCost), cap);
      const double call = jobClass[ji] & RTSS_JOB_CALL ? eastl::clamp(double(set.rasterCallCost), 0.0, cap - pageCost) : 0.0;
      const double c = rtss_rect_area(jobs[ji].rect) * pageCost / (set.slotTexels * set.slotTexels) + call;
      cost[cls] += c;
      (cls == RTSS_REQUIRED && level == set.levels - 1 ? burstCost : updateCost) += c;
    }
    const double spill = eastl::max(burstCost - set.validityBurstPagesPerFrame, 0.0); // the coarsest level past its burst
    burstCost -= spill;
    updateCost += spill;
    if (updateCost > set.batchFrames * set.updatePagesPerFrame + 1e-6)
      fail(RTSS_I5_BUDGET, s.sprintf("the pages outside the validity burst cost %g", updateCost));
    for (int c = 0; c < RTSS_PAGE_CLASS_COUNT; ++c)
      if (fabs(cost[c] - stats.cost[c]) > 1e-6)
        fail(RTSS_I5_BUDGET, s.sprintf("class %d costs %g, the stats say %g", c, cost[c], stats.cost[c]));

    jobCount.assign(slots().size(), 0);
    const uint32_t whole = wholeRect();
    uint32_t group = 0;
    double lastDist[2][RTSS_MAX_LEVELS] = {};
    for (int ji = 0; ji < (int)jobs.size(); ++ji)
    {
      const RtssJob &j = jobs[ji];
      const int level = jobLevel(j), idx = jobSlotIndex(j);
      const uint32_t texels = rtss_rect_area(j.rect);
      for (uint32_t n = 0; n < (texels + RTSS_BUILD_GROUP - 1) / RTSS_BUILD_GROUP; ++n, ++group)
        if (group >= groups.size() || groups[group] != ((uint32_t(ji) << 16) | n))
          fail(RTSS_I10_PLAN_TOTALS, s.sprintf("group %u of job %d is not in order", n, ji));
      if (++jobCount[idx] > 1)
        fail(RTSS_I4_ONE_JOB, s.sprintf("level %d slot %d has two jobs", level, idx % (W * W)));
      const RtssSlot &old = prevSlots[idx];
      const IPoint2 page(j.page.x, j.page.y), oldPage = rtss_unpack_page(old.page, page);
      const int oldBit = rtss_slot_bit(old.flags);
      const bool valid = old.flags & RTSS_SLOT_VALID;
      if ((!valid || oldPage != page || oldBit != int(j.flags & RTSS_JOB_BIT)) && j.rect != whole)
        fail(RTSS_I3_WHOLE_SLOT, s.sprintf("level %d page %d,%d changes with rect %08x", level, page.x, page.y, j.rect));
      if (valid && oldPage != page && inUse(level, oldPage))
        fail(RTSS_I8_KEEP_USED_PAGE, s.sprintf("level %d page %d,%d (class %d) replaces %d,%d that a cube needs", level, page.x,
                                       page.y, jobClass[ji], oldPage.x, oldPage.y));
      if (j.zEnd < j.zStart)
        fail(RTSS_I10_PLAN_TOTALS, s.sprintf("job %d has a negative ray length", ji));
      if ((jobClass[ji] & RTSS_JOB_CLASS_MASK) == RTSS_DIRTY && !inWindow(level, page) && !inUse(level, page))
        fail(RTSS_I5_BUDGET, s.sprintf("level %d rebuilds page %d,%d that the window left and no cube needs", level, page.x, page.y));
      if ((jobClass[ji] & ~(RTSS_JOB_SAME_PAGE | RTSS_JOB_INNER | RTSS_JOB_CALL)) == RTSS_REQUIRED && inWindow(level, page))
      {
        const DPoint3 c = sched.xform(level, curBit(level)).toLs(center()) / sched.pageWorld(level);
        const double dx = page.x + 0.5 - c.x, dy = page.y + 0.5 - c.y, dist = dx * dx + dy * dy;
        double &last = lastDist[(jobClass[ji] & RTSS_JOB_INNER) != 0][level];
        if (dist < last - 1e-3)
          fail(RTSS_I5_BUDGET,
            s.sprintf("level %d REQUIRED page %d,%d is nearer the camera than the one before it", level, page.x, page.y));
        last = dist;
      }
    }
    if (group != groups.size())
      fail(RTSS_I10_PLAN_TOTALS, s.sprintf("plan has %d groups, its jobs need %d", (int)groups.size(), (int)group));
  }

  void checkLevelStats(int k)
  {
    const RtssSchedulerLevelStats &l = stats.levels[k];
    const DPoint3 d = -dpoint3(row(k, newestBit(k), 2)), sunD = dpoint3(sun);
    const double errorDeg = atan2(length(d % sunD), d * sunD) * (180 / M_PI);
    const float slack = 1.0001f;
    eastl::string s;
    if (fabs(errorDeg - l.sunErrorDeg) > 1e-3)
      fail(RTSS_I11_STATS, s.sprintf("level %d sun error %g, measured %g", k, l.sunErrorDeg, errorDeg));
    if (l.toleranceDeg > settings.toleranceDeg0 * float(1 << k) * slack || l.toleranceDeg > settings.toleranceDegMax * slack ||
        l.toleranceDeg > 0.5f * l.fastLimitDeg * slack || l.fastLimitDeg > settings.fastSweepDeg * slack)
      fail(RTSS_I11_STATS, s.sprintf("level %d tolerance %g, fast limit %g", k, l.toleranceDeg, l.fastLimitDeg));
    const double e = sched.extent(k), pw = sched.pageWorld(k);
    const DPoint3 center = this->center();
    for (int b = 0; b < 2; ++b)
    {
      if (!(l.liveMask >> b & 1))
        continue;
      const DPoint3 c = sched.xform(k, b).toLs(center);
      const IPoint2 lo(int(floor((c.x - e) / pw)), int(floor((c.y - e) / pw))),
        hi(int(floor((c.x + e) / pw)), int(floor((c.y + e) / pw)));
      const IBBox2 &cube = l.cube[b];
      bool ok = b == curBit(k) ? cube.lim[0] == lo && cube.lim[1] == hi : (cube & lo) && (cube & hi);
      const RtssLightXform &x = sched.xform(k, curBit(k));
      for (int i = 0; i < 8 && b != curBit(k); ++i)
      {
        const DPoint3 ls = sched.xform(k, b).toLs(center + dpoint3(x.ex) * ((i & 1) ? e : -e) +
                                                  dpoint3(x.ey) * (((i & 2) ? e : -e) / x.sy) + dpoint3(x.d) * ((i & 4) ? e : -e));
        ok &= cube & IPoint2(int(floor(ls.x / pw)), int(floor(ls.y / pw)));
      }
      if (!ok)
        fail(RTSS_I11_STATS, s.sprintf("level %d bit %d cube is not the pages of the extent around the camera", k, b));
    }
  }

  void checkTables()
  {
    eastl::string s;
    for (int k = 0; k < settings.levels; ++k)
    {
      if (!sweeping(k))
        continue;
      const Point2 u = stats.levels[k].u;
      double maxCur = -DBL_MAX, minTgt = DBL_MAX;
      const int W = windowPages();
      for (int i = 0; i < W * W; ++i)
      {
        const RtssSlot &sl = slots()[k * W * W + i];
        const IPoint2 p = rtss_unpack_page(sl.page, sched.windowOrigin(k));
        if (!(sl.flags & RTSS_SLOT_VALID) || !inWindow(k, p))
          continue;
        const double key = ((u.y > 0) - (u.y < 0)) * p.y * 65536.0 + ((u.x > 0) - (u.x < 0)) * p.x; // the spec key, not rtss_sweep_key
        if (rtss_slot_bit(sl.flags) == uint32_t(curBit(k)))
          maxCur = eastl::max(maxCur, key);
        else
          minTgt = eastl::min(minTgt, key);
      }
      if (maxCur > minTgt)
        fail(RTSS_I2_KEY_ORDER, s.sprintf("level %d current-bit key %g above target-bit key %g", k, maxCur, minTgt));
    }

    if (frame < coverageFrom || stats.sunBelowHorizon)
      return;
    if (selectionSamples)
      selectionMissCount += selectionMisses(selectionSamples);
    const RtssCpuTables t = tables();
    for (int k = 0; k < settings.levels; ++k)
    {
      const float cubeDepth = float(extent(k));
      for (int i = 0; i < 72; ++i)
      {
        const int c = i - 64;
        const Point3 p = c < 0 ? Point3(_rnd_float(seed, -1, 1), _rnd_float(seed, -1, 1), _rnd_float(seed, -1, 1))
                               : Point3(c & 1 ? 0.99f : -0.99f, c & 2 ? 0.99f : -0.99f, c & 4 ? 0.99f : -0.99f);
        const Point3 pos = cubePoint(k, curBit(k), p);
        const int got = rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 0.5f, k);
        if (got == k)
          continue;
        bool pageMissing = true;
        for (int b = 0; b < 2; ++b)
          if (b == curBit(k) || sweeping(k))
          {
            const RtssLevel &l = levelTab(k);
            const float lx = row(k, b, 0) * pos + l.xform[b * 3].w, ly = row(k, b, 1) * pos + l.xform[b * 3 + 1].w;
            const IPoint2 page(l.pageOrigin.x + int(floorf(lx * l.invPage)), l.pageOrigin.y + int(floorf(ly * l.invPage)));
            const RtssSlot &sl = slot(k, page);
            const float camZ = l.xform[b * 3 + 2].w;
            pageMissing &= !holds(k, page, b) || sl.minDepth > camZ - cubeDepth || sl.maxDepth < camZ + cubeDepth;
          }
        if (k < settings.levels - 1 && stats.levels[k].requiredPending > 0 && got > k)
          continue; // a budget delay reads a coarser level
        misses.push_back({frame, k, p, stats.levels[k].u, pageMissing});
        if (strictCoverage)
          fail(RTSS_I7_COVERAGE, s.sprintf("level %d misses %.2f,%.2f,%.2f of its cube, got %d", k, p.x, p.y, p.z, got));
      }
    }
  }

  int selectionMisses(int samples)
  {
    const RtssCpuTables t = tables();
    int missCount = 0;
    for (int k = 0; k < settings.levels; ++k)
      for (int i = 0; i < samples && !(k < settings.levels - 1 && stats.levels[k].requiredPending > 0);)
      {
        const Point3 p(_rnd_float(seed, -0.9f, 0.9f), _rnd_float(seed, -0.9f, 0.9f), _rnd_float(seed, -0.9f, 0.9f));
        const float inner = k ? 0.5f : 0.f;
        if (eastl::max(eastl::max(fabsf(p.x), fabsf(p.y)), fabsf(p.z)) <= inner * 1.01f)
          continue;
        ++i;
        missCount += rtss_cpu_lookup(t, cubePoint(0, curBit(0), p * float(1 << k)), Point3(0, 0, 0), 0.f, 0.5f) != k;
      }
    return missCount;
  }
};
