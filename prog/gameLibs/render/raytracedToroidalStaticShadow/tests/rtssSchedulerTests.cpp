// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "rtssTestSim.h"
#include <core/rtssRasterPlan.h>

static constexpr double DEG = M_PI / 180;
static constexpr double DAY24_RATE = 2 * M_PI / (24 * 60);

static void check_invariants(const RtssSim &sim, int skip = -1)
{
  for (int i = 0; i < RTSS_INVARIANTS; ++i)
    if (i != skip)
    {
      INFO("I" << i + 1 << " " << sim.firstViolation[i].c_str());
      CHECK(sim.violations[i] == 0);
    }
}


static Point3 day_axis(double tilt_deg) { return Point3(0, float(cos(tilt_deg * DEG)), float(sin(tilt_deg * DEG))); }

static Point3 sun_at(double elevation_deg, double azimuth_deg)
{
  const double e = elevation_deg * DEG, a = azimuth_deg * DEG;
  return Point3(float(cos(e) * cos(a)), float(sin(e)), float(cos(e) * sin(a)));
}

static Point3 turned(const Point3 &sun, double step_deg)
{
  return rtss_rotate(sun, normalize(sun % Point3(0.3f, 0.1f, 1.f)), step_deg * DEG);
}

static int level_of_class(const RtssSim &sim, uint8_t cls, int level)
{
  int n = 0;
  for (int i = 0; i < (int)sim.jobs.size(); ++i)
    n += sim.jobLevel(sim.jobs[i]) == level && (sim.jobClass[i] & RTSS_JOB_CLASS_MASK) == cls;
  return n;
}

static int required_in_window(const RtssSim &sim, int level)
{
  int n = 0;
  for (int i = 0; i < (int)sim.jobs.size(); ++i)
    n += sim.jobLevel(sim.jobs[i]) == level && (sim.jobClass[i] & RTSS_JOB_CLASS_MASK) == RTSS_REQUIRED &&
         sim.inWindow(level, IPoint2(sim.jobs[i].page.x, sim.jobs[i].page.y));
  return n;
}

static int cube_pages(const RtssSim &sim, int level)
{
  const IPoint2 w = sim.stats.levels[level].cube[sim.curBit(level)].width();
  return (w.x + 1) * (w.y + 1);
}

static int finer_cube_pages(const RtssSim &sim)
{
  int n = 0;
  for (int k = 0; k + 1 < sim.settings.levels; ++k)
    n += cube_pages(sim, k);
  return n;
}

static int update_frames(const RtssSchedulerSettings &s, double cost) { return int(ceil(cost / s.updatePagesPerFrame)); }

static void check_day_sun_error(RtssSim &sim, double rate, const eastl::function<void()> &each_frame)
{
  sim.sun = sun_at(45, 30);
  sim.sunAngVel = day_axis(40) * float(rate);
  double worstError[RTSS_MAX_LEVELS] = {};
  for (int i = 0; i < 3600; ++i)
  {
    sim.step();
    if (sim.frame <= 600)
      continue;
    each_frame();
    for (int k = 0; k < sim.settings.levels; ++k)
      worstError[k] = eastl::max(worstError[k], double(sim.stats.levels[k].sunErrorDeg / sim.stats.levels[k].toleranceDeg));
  }
  const double pathRate = length(day_axis(40) % sim.sun);
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    INFO("level " << k << " sun error / tolerance " << worstError[k] << ", sun path rate / |angular velocity| " << pathRate);
    CHECK(worstError[k] <= 1.05 * pathRate);
  }
}

TEST_CASE("T2 static sun and camera settle with no jobs")
{
  RtssSim sim;
  int lastJobFrame = 0;
  double fillPages = 0;
  for (int i = 0; i < 600; ++i)
  {
    sim.step();
    fillPages += sim.stats.pages[RTSS_FILL];
    if (!sim.jobs.empty())
      lastJobFrame = sim.frame;
  }
  const int W = sim.windowPages(),
            settled = update_frames(sim.settings, sim.settings.levels * W * W - cube_pages(sim, sim.settings.levels - 1));
  INFO("last frame with jobs " << lastJobFrame << ", the update budget needs " << settled << ", FILL pages " << fillPages);
  CHECK(lastJobFrame <= settled + 1);
  CHECK(fillPages > 0);
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    CHECK(sim.levelComplete(k));
    CHECK(sim.stats.levels[k].validSlots == sim.windowPages() * sim.windowPages());
    CHECK_FALSE(sim.stats.levels[k].pretraceBlocked);
  }
  CHECK(sim.selectionMisses(256) == 0);
  check_invariants(sim);
}

TEST_CASE("T3 a 24-minute day with a static camera builds about 20 pages for each frame")
{
  RtssSim sim;
  sim.selectionSamples = 16;
  double sum = 0, worstMax = 0;
  int frames = 0;
  check_day_sun_error(sim, DAY24_RATE, [&]() {
    sum += sim.otherPages() + sim.stats.pages[RTSS_REQUIRED];
    worstMax = eastl::max(worstMax, sim.otherPages());
    ++frames;
  });
  const double avg = sum / frames;
  INFO("pages for each frame " << avg << ", most " << worstMax);
  CHECK(avg > 19);
  CHECK(avg < 21);
  CHECK(worstMax <= sim.settings.updatePagesPerFrame);
  INFO("selection misses " << sim.selectionMissCount);
  CHECK(sim.selectionMissCount == 0);
  check_invariants(sim);
}

TEST_CASE("T56 setRecipes invalidates only the level whose raster changes, and a cost change invalidates nothing")
{
  RtssSim sim;
  sim.run(60);
  sim.sched.getStats(sim.stats);
  const int built1 = sim.stats.levels[1].validSlots + sim.stats.levels[1].dirtySlots;
  sim.settings.recipes[1].raster = RtssRaster::Augment;
  sim.settings.recipes[2].pageCost = 2;
  CHECK(sim.sched.setRecipes(sim.settings));
  CHECK_FALSE(sim.sched.setRecipes(sim.settings));
  sim.sched.getStats(sim.stats);
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    INFO("level " << k);
    CHECK(sim.stats.levels[k].dirtySlots == (k == 1 ? built1 : 0));
  }
  sim.run(120);
  sim.sched.getStats(sim.stats);
  CHECK(sim.stats.levels[1].dirtySlots == 0);
  check_invariants(sim);
}

struct WalkResult
{
  double pretrace = 0, required = 0;
  int requiredFrames = 0, requiredUnexplained = 0, pretraceFrames = 0;
};

template <typename Velocity>
static WalkResult walk(RtssSim &sim, int frames, Velocity velocity)
{
  WalkResult r;
  int counted = 0;
  sim.selectionSamples = 16;
  for (int i = 0; i < frames; ++i)
  {
    sim.cameraVel = velocity(sim);
    sim.step();
    if (i < 300)
      continue;
    ++counted;
    r.pretrace += sim.stats.pages[RTSS_PRETRACE];
    r.pretraceFrames += sim.stats.pages[RTSS_PRETRACE] > 0;
    r.required += sim.stats.pages[RTSS_REQUIRED];
    for (int k = 0; k < sim.settings.levels; ++k)
      if (required_in_window(sim, k) > 0)
      {
        ++r.requiredFrames;
        r.requiredUnexplained += !sim.stats.levels[k].pretraceBlocked;
      }
  }
  r.pretrace /= counted;
  r.required /= counted;
  return r;
}

TEST_CASE("T4 a walk at 6 m/s pre-traces about one page for each frame")
{
  const double expectedStraight = 16 * 6.0 / (62 * 0.05) * (1 + 0.5 + 0.25 + 0.125) / 60;
  SECTION("straight")
  {
    RtssSim sim;
    sim.sunAngVel = day_axis(40) * float(DAY24_RATE);
    sim.run(5);
    const Point3 dir = sim.row(0, sim.curBit(0), 0);
    const WalkResult r = walk(sim, 1500, [&](RtssSim &) { return dir * 6.f; });
    INFO("PRETRACE " << r.pretrace << " (expected " << expectedStraight << ") in " << r.pretraceFrames << " of 1200 frames, REQUIRED "
                     << r.required << ", " << r.requiredFrames << " level frames with window pages, " << r.requiredUnexplained
                     << " without a blocked pre-trace");
    CHECK(fabs(r.pretrace / expectedStraight - 1) < 0.15);
    CHECK(r.pretraceFrames * 3 <= 1200);
    CHECK(r.requiredUnexplained == 0);
    CHECK(sim.selectionMissCount == 0);
    check_invariants(sim);
  }
  SECTION("diagonal")
  {
    RtssSim sim;
    sim.sunAngVel = day_axis(40) * float(DAY24_RATE);
    sim.run(5);
    const Point3 dir = sim.row(0, sim.curBit(0), 0) + sim.row(0, sim.curBit(0), 1);
    const WalkResult r = walk(sim, 1500, [&](RtssSim &) { return dir * 6.f; });
    const double expected = expectedStraight * (2 * 16 - 1) / 16;
    INFO("PRETRACE " << r.pretrace << " (expected " << expected << " for 6 m/s on each page axis), REQUIRED " << r.required << " in "
                     << r.requiredFrames << " level frames, " << r.requiredUnexplained << " unexplained");
    CHECK(fabs(r.pretrace / expected - 1) < 0.15);
    CHECK(r.requiredUnexplained == 0);
    CHECK(sim.selectionMissCount == 0);
    check_invariants(sim);
  }
  SECTION("circle")
  {
    RtssSim sim;
    sim.sunAngVel = day_axis(40) * float(DAY24_RATE);
    sim.run(5);
    const Point3 ex = sim.row(0, sim.curBit(0), 0), ey = sim.row(0, sim.curBit(0), 1);
    const WalkResult r = walk(sim, 2400, [&](RtssSim &s) {
      const float a = float(s.time * 6 / 60);
      return (ey * cosf(a) - ex * sinf(a)) * 6.f;
    });
    INFO("PRETRACE " << r.pretrace << ", REQUIRED " << r.required << ", " << r.requiredFrames << " level frames, "
                     << r.requiredUnexplained << " unexplained");
    CHECK(r.pretrace < 2.2);
    CHECK(r.required < 0.02 * r.pretrace); // a turn back re-requests early pre-traces
    CHECK(sim.selectionMissCount == 0);
    check_invariants(sim);
  }
}

TEST_CASE("T5 a flight at 50 m/s makes the sweeps late, lowers the fast limit with H, and the stats show it")
{
  RtssSim sim;
  sim.sunAngVel = day_axis(40) * float(DAY24_RATE);
  sim.run(5);
  const Point3 dir = sim.row(0, sim.curBit(0), 0);
  double incoming = 0, pretrace = 0, longestRatio = 0;
  int frames = 0;
  const float staticFastLimit = sim.stats.levels[0].fastLimitDeg;
  float flightFastLimit = FLT_MAX;
  sim.cameraVel = dir * 50.f;
  for (int i = 0; i < 900; ++i)
  {
    sim.step();
    if (i < 300)
      continue;
    ++frames;
    flightFastLimit = eastl::min(flightFastLimit, sim.stats.levels[0].fastLimitDeg);
    pretrace += sim.stats.pages[RTSS_PRETRACE];
    incoming += sim.stats.pages[RTSS_PRETRACE] + sim.stats.pages[RTSS_REQUIRED] - sim.stats.pagesEdge;
    for (const RtssSchedulerLevelStats &l : sim.stats.levels)
      longestRatio = eastl::max(longestRatio, double(l.lastSweepSeconds / l.lastPlannedSeconds));
  }
  const double expected = 16 * 50.0 / (62 * 0.05) * (1 + 0.5 + 0.25 + 0.125) / 60;
  INFO("incoming pages " << incoming / frames << " (PRETRACE " << pretrace / frames << ", expected " << expected
                         << "), longest sweep / planned " << longestRatio << ", level 0 fast limit " << staticFastLimit
                         << " deg still, " << flightFastLimit << " in the flight");
  CHECK(fabs(incoming / frames / expected - 1) < 0.15);
  CHECK(longestRatio > 1);
  CHECK(flightFastLimit < staticFastLimit);
  check_invariants(sim);
}

TEST_CASE("T6 a teleport of 1 km and a device reset rebuild the coarsest level in one frame, then level 0 first and all levels at "
          "updatePagesPerFrame")
{
  RtssSim sim;
  sim.run(60);
  const int last = sim.settings.levels - 1;
  const int level0Frames = update_frames(sim.settings, cube_pages(sim, 0)),
            allFrames = update_frames(sim.settings, finer_cube_pages(sim));
  auto coarsestMisses = [&]() {
    const RtssCpuTables t = sim.tables();
    const float zr = float(eastl::min(1.0, sim.settings.maxCasterReach / sim.extent(last)));
    int misses = 0;
    for (int i = 0; i < 256; ++i)
    {
      const Point3 p(_rnd_float(sim.seed, -1, 1), _rnd_float(sim.seed, -1, 1), _rnd_float(sim.seed, -zr, zr));
      misses += rtss_cpu_lookup(t, sim.cubePoint(last, sim.curBit(last), p), Point3(0, 0, 0), 0.f, 0.5f, last) != last;
    }
    return misses;
  };
  sim.camera += DPoint3(1000, 0, 0);
  sim.coverageFrom = sim.frame + 5;
  sim.step();
  CHECK(sim.stats.pages[RTSS_REQUIRED] > 0);
  CHECK(sim.levelComplete(last));
  CHECK(coarsestMisses() == 0);
  sim.run(level0Frames - 1);
  CHECK(sim.levelComplete(0));
  sim.run(allFrames - level0Frames);
  for (int k = 0; k < sim.settings.levels; ++k)
    CHECK(sim.levelComplete(k));

  sim.reset();
  sim.sched.getStats(sim.stats);
  for (int k = 0; k < sim.settings.levels; ++k)
    CHECK(sim.stats.levels[k].emptySlots == sim.windowPages() * sim.windowPages());
  sim.coverageFrom = sim.frame + 5;
  sim.step();
  CHECK(sim.levelComplete(last));
  CHECK(coarsestMisses() == 0);
  sim.run(level0Frames - 1);
  CHECK(sim.levelComplete(0));
  sim.run(allFrames - level0Frames);
  for (int k = 0; k < sim.settings.levels; ++k)
    CHECK(sim.levelComplete(k));
  sim.run(60);
  CHECK(sim.jobs.empty());
  check_invariants(sim);
}

static bool near_plus_u_face(const RtssCoverageMiss &m, double u_shift_in_cubes)
{
  for (int a = 0; a < 2; ++a)
    if ((m.u[a] > 0 ? m.cubePos[a] : -m.cubePos[a]) >= 1 - fabsf(m.u[a]) * u_shift_in_cubes)
      return true;
  return false;
}

static double plus_u_shift(double step_deg) { return (cos(step_deg * DEG) - 1) + 2.5 * sin(step_deg * DEG); }

struct LargeStep
{
  int sweepFrames = 0, missFrames = 0, farMisses = 0, lateMisses = 0;
  int finishFrame[RTSS_MAX_LEVELS] = {};
  bool allFast = true;
};

static LargeStep large_step(RtssSim &sim, double step_deg, int frames)
{
  LargeStep r;
  const double uShift = plus_u_shift(step_deg);
  sim.strictCoverage = false;
  sim.misses.clear();
  sim.sun = turned(sim.sun, step_deg);
  const int start = sim.frame + 1;
  int lastSweepFrame = start;
  for (int i = 0; i < frames; ++i)
  {
    sim.step();
    bool any = false;
    for (int k = 0; k < sim.settings.levels; ++k)
    {
      if (sim.frame == start)
        r.allFast &= sim.sweeping(k) && sim.stats.levels[k].fastSweep;
      if (sim.sweeping(k))
      {
        any = true;
        r.finishFrame[k] = sim.frame + 1;
      }
    }
    if (any)
      lastSweepFrame = sim.frame;
  }
  r.sweepFrames = lastSweepFrame - start + 1;
  int lastMissFrame = -1;
  for (const RtssCoverageMiss &m : sim.misses)
  {
    r.missFrames += m.frame != lastMissFrame;
    lastMissFrame = m.frame;
    r.farMisses += !near_plus_u_face(m, uShift);
    r.lateMisses += m.frame > lastSweepFrame;
  }
  return r;
}

TEST_CASE("T7 a step of 7.5 deg sweeps all levels fast, the coarsest first")
{
  RtssSim sim;
  sim.run(60);
  const LargeStep r = large_step(sim, 7.5, 60);
  INFO("sweep frames " << r.sweepFrames << ", frames with misses " << r.missFrames << ", misses " << sim.misses.size()
                       << ", far from a +u face " << r.farMisses << ", after the sweep " << r.lateMisses);
  CHECK(r.allFast);
  for (int k = 0; k + 1 < sim.settings.levels; ++k)
    CHECK(r.finishFrame[sim.settings.levels - 1] <= r.finishFrame[k]);
  const int W = sim.windowPages();
  CHECK(r.sweepFrames <= update_frames(sim.settings, (sim.settings.levels - 1) * W * W) + 1);
  CHECK(r.missFrames <= r.sweepFrames);
  CHECK(r.farMisses == 0);
  CHECK(r.lateMisses == 0);
  check_invariants(sim);
}

TEST_CASE("T8 a sun change of 1 deg with no angular velocity makes one sweep of staticSweepSeconds")
{
  RtssSim sim;
  sim.run(60);
  int sweepsBefore[RTSS_MAX_LEVELS];
  for (int k = 0; k < sim.settings.levels; ++k)
    sweepsBefore[k] = sim.stats.levels[k].sweeps;
  sim.sun = turned(sim.sun, 1);
  sim.step();
  int pages[RTSS_MAX_LEVELS], allPages = 0;
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    CHECK(sim.sweeping(k));
    CHECK_FALSE(sim.stats.levels[k].fastSweep);
    CHECK(sim.stats.levels[k].sweepSeconds == sim.settings.staticSweepSeconds);
    pages[k] = sim.stats.levels[k].sweepPages;
    allPages += pages[k];
  }
  double finerEdges = 0;
  for (int i = 0; i < 240; ++i)
  {
    sim.step();
    for (int k = 0; k + 1 < sim.settings.levels; ++k)
      finerEdges += level_of_class(sim, RTSS_REQUIRED, k);
  }
  const double ppf = sim.settings.updatePagesPerFrame, longest = (ceil((allPages + finerEdges) / ppf) + 1) * sim.dt;
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    const double shortest = eastl::max(double(sim.settings.staticSweepSeconds), ceil(pages[k] / ppf) * sim.dt);
    const double measured = sim.stats.levels[k].lastSweepSeconds;
    INFO("level " << k << " measured sweep " << measured << " s, bounds " << shortest << " .. " << longest << " s");
    CHECK(measured >= shortest - sim.dt);
    CHECK(measured <= longest + 1e-6); // constraint: lastSweepSeconds is a float
    CHECK_FALSE(sim.sweeping(k));
    CHECK(sim.stats.levels[k].sweeps == sweepsBefore[k] + 1);
    CHECK(sim.stats.levels[k].sunErrorDeg < 0.01f);
  }
  CHECK(sim.jobs.empty());
  check_invariants(sim);
}

TEST_CASE("T9 no sweep starts below the horizon, and the first sweep of each level after the sunrise is fast")
{
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 128;
  RtssSim sim(s);
  const double tilt = 45.5;
  const Point3 axis = day_axis(tilt);
  sim.sunAngVel = axis * float(DEG);
  sim.sun = rtss_rotate(Point3(0, 1, 0), axis, 150 * DEG);
  int sweeps[RTSS_MAX_LEVELS] = {}, belowStarts = 0, belowFrames = 0, flagMismatch = 0, firstAfterRise = 0, slowFirst = 0;
  bool risen = false, wasBelow = false, seen[RTSS_MAX_LEVELS] = {};
  sim.strictCoverage = false;
  for (int i = 0; i < 4200; ++i)
  {
    sim.step();
    const bool below = sim.stats.sunBelowHorizon;
    belowFrames += below;
    risen |= wasBelow && !below;
    wasBelow = below;
    flagMismatch += (sim.tables().params.y != 0.f) != (normalize(sim.sun).y < sim.settings.minSunY);
    for (int k = 0; k < s.levels; ++k)
    {
      if (sim.stats.levels[k].sweeps != sweeps[k])
      {
        belowStarts += below;
        if (risen && !eastl::exchange(seen[k], true))
        {
          ++firstAfterRise;
          slowFirst += !sim.stats.levels[k].fastSweep;
        }
      }
      sweeps[k] = sim.stats.levels[k].sweeps;
    }
  }
  INFO("frames below the horizon " << belowFrames << ", sweeps started below " << belowStarts << ", first sweeps after the sunrise "
                                   << firstAfterRise << " (" << slowFirst << " not fast)");
  CHECK(belowFrames > 600);
  CHECK(risen);
  CHECK(belowStarts == 0);
  CHECK(firstAfterRise == s.levels);
  CHECK(slowFirst == 0);
  CHECK(flagMismatch == 0);
  check_invariants(sim);
}

// bit -1: the transform of the bit of the slot
static void expected_dirty_rect(const RtssSim &sim, int slot_idx, const BBox3 &box, int rect[4], int bit = -1)
{
  const int W = sim.windowPages(), S = sim.settings.slotTexels, B = sim.settings.borderTexels, level = slot_idx / (W * W);
  const RtssSlot &sl = sim.slots()[slot_idx];
  bit = bit >= 0 ? bit : int(rtss_slot_bit(sl.flags));
  const IPoint2 page = rtss_unpack_page(sl.page, sim.sched.windowOrigin(level));
  const RtssLevel &t = sim.levelTab(level);
  for (int a = 0; a < 2; ++a)
  {
    const float4 &r = t.xform[bit * 3 + a];
    const float origin = float(((a ? page.y - t.pageOrigin.y : page.x - t.pageOrigin.x) * (S - 2 * B) - B));
    float lo = FLT_MAX, hi = -FLT_MAX;
    for (int c = 0; c < 8; ++c)
    {
      const Point3 rel = point3(dpoint3(box.point(c)) - sim.camera);
      const float v = (r.x * rel.x + r.y * rel.y + r.z * rel.z + r.w) / float(sim.texel(level)) - origin;
      lo = eastl::min(lo, v);
      hi = eastl::max(hi, v);
    }
    rect[a] = eastl::clamp(int(floorf(lo)), 0, S);
    rect[a + 2] = eastl::clamp(int(ceilf(hi)), 0, S);
  }
}

TEST_CASE("T10 invalidateBoxes makes clipped DIRTY rectangles in the slots that two boxes touch, their union where both clip one "
          "slot, with a border of 1 and 3")
{
  RtssSchedulerSettings s;
  s.borderTexels = GENERATE(1, 3);
  RtssSim sim(s);
  sim.run(60);
  const Point3 c = point3(sim.camera) + Point3(4.f, 0.5f, -3.f);
  const BBox3 boxes[2] = {
    BBox3(c - Point3(10, 10, 10), c + Point3(-1, 10, 10)), BBox3(c + Point3(1, -10, -10), c + Point3(10, 10, 10))};
  dag::Vector<int> expected(sim.slots().size() * 4);
  int expectedSlots = 0, sharedSlots = 0;
  for (int i = 0; i < (int)sim.slots().size(); ++i)
  {
    int a[4], b[4], *r = &expected[i * 4];
    expected_dirty_rect(sim, i, boxes[0], a);
    expected_dirty_rect(sim, i, boxes[1], b);
    const bool hasA = a[0] < a[2] && a[1] < a[3], hasB = b[0] < b[2] && b[1] < b[3];
    for (int e = 0; e < 4; ++e)
      r[e] = !hasB ? a[e] : !hasA ? b[e] : e < 2 ? eastl::min(a[e], b[e]) : eastl::max(a[e], b[e]);
    expectedSlots += hasA || hasB;
    sharedSlots += hasA && hasB;
  }
  sim.sched.invalidateBoxes(boxes, 2);
  const uint32_t whole = sim.wholeRect();
  int dirtyJobs = 0, subSlotJobs = 0, unexpected = 0, worstEdge = 0, exactEdges = 0, frames = 0;
  for (; frames < 200; ++frames)
  {
    sim.step();
    for (int i = 0; i < (int)sim.jobs.size(); ++i)
    {
      const RtssJob &j = sim.jobs[i];
      if ((sim.jobClass[i] & RTSS_JOB_CLASS_MASK) != RTSS_DIRTY)
        continue;
      const int *r = &expected[sim.jobSlotIndex(j) * 4];
      ++dirtyJobs;
      subSlotJobs += j.rect != whole;
      unexpected += !(r[0] < r[2] && r[1] < r[3]);
      for (int e = 0; e < 4; ++e)
      {
        const int err = abs(int((j.rect >> (e * 8)) & 0xff) - r[e]);
        worstEdge = eastl::max(worstEdge, err);
        exactEdges += err == 0;
      }
    }
    int dirty = 0;
    for (const RtssSchedulerLevelStats &l : sim.stats.levels)
      dirty += l.dirtySlots;
    if (dirty == 0 && sim.jobs.empty())
      break;
  }
  INFO("border " << s.borderTexels << ", DIRTY jobs " << dirtyJobs << " (" << subSlotJobs << " sub-slot) for " << expectedSlots
                 << " slots, " << sharedSlots << " of them clipped by both boxes, in " << frames
                 << " frames, worst rectangle edge error " << worstEdge << " texels, exact edges " << exactEdges << " of "
                 << dirtyJobs * 4);
  CHECK(dirtyJobs == expectedSlots);
  CHECK(sharedSlots > 0);
  CHECK(unexpected == 0);
  CHECK(subSlotJobs > 0);
  CHECK(worstEdge <= 1);
  CHECK(exactEdges >= dirtyJobs * 4 * 0.95);
  CHECK(frames < 200);

  const BBox3 tiny(c - Point3(0.05f, 0.05f, 0.05f), c + Point3(0.05f, 0.05f, 0.05f));
  sim.sched.invalidateBoxes(&tiny, 1);
  int levelMask = 0;
  for (int i = 0; i < 20; ++i)
  {
    sim.step();
    for (const RtssJob &j : sim.jobs)
      levelMask |= 1 << sim.jobLevel(j);
  }
  INFO("levels touched by a 10 cm box " << levelMask);
  CHECK((levelMask & 1) != 0);
  CHECK((levelMask & ~1) == 0);
  check_invariants(sim);
}

TEST_CASE("T24 during a sweep the sweep job replaces the DIRTY job of the head slot")
{
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 64;
  s.staticSweepSeconds = 2;
  RtssSim sim(s);
  sim.run(60);
  sim.sun = turned(sim.sun, 1);
  sim.step();
  REQUIRE(sim.sweeping(0));
  sim.sched.invalidate();
  const int W = s.windowPages;
  int dirtyOnHead = 0, dirtyJobs = 0, sweepJobs = 0;
  for (int i = 0; i < 180; ++i)
  {
    int head[RTSS_MAX_LEVELS];
    for (int k = 0; k < s.levels; ++k)
      head[k] = sim.stats.levels[k].headSlot;
    sim.step();
    for (int j = 0; j < (int)sim.jobs.size(); ++j)
    {
      const int k = sim.jobLevel(sim.jobs[j]), cls = sim.jobClass[j] & RTSS_JOB_CLASS_MASK;
      dirtyJobs += cls == RTSS_DIRTY;
      sweepJobs += cls == RTSS_SWEEP;
      const int slot = sim.jobSlotIndex(sim.jobs[j]) % (W * W);
      dirtyOnHead += cls == RTSS_DIRTY && (slot == head[k] || slot == sim.stats.levels[k].headSlot);
    }
  }
  INFO("DIRTY jobs " << dirtyJobs << ", sweep jobs " << sweepJobs << ", DIRTY jobs on the sweep head " << dirtyOnHead);
  CHECK(dirtyJobs > 0);
  CHECK(sweepJobs > 0);
  CHECK(dirtyOnHead == 0);
  for (int k = 0; k < s.levels; ++k)
  {
    CHECK_FALSE(sim.sweeping(k));
    CHECK(sim.stats.levels[k].dirtySlots == 0);
  }
  check_invariants(sim);
}

TEST_CASE("T11 a budget of 4 pages keeps the sun error of the coarsest level below the fast limit plus the tolerance, and a finer "
          "level misses only in its fast sweeps")
{
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 4;
  RtssSim sim(s);
  sim.strictCoverage = false;
  sim.sun = sun_at(60, 30);
  sim.sunAngVel = day_axis(40) * float(8 * DAY24_RATE);
  double worst[RTSS_MAX_LEVELS] = {}, worstLate[RTSS_MAX_LEVELS] = {};
  int fastSweeps = 0, otherMisses = 0;
  bool wasFast[RTSS_MAX_LEVELS] = {};
  for (int i = 0; i < 4800; ++i)
  {
    sim.misses.clear();
    sim.step();
    for (const RtssCoverageMiss &m : sim.misses)
      otherMisses += m.level == s.levels - 1 || !sim.stats.levels[m.level].fastSweep;
    for (int k = 0; k < s.levels; ++k)
    {
      const RtssSchedulerLevelStats &l = sim.stats.levels[k];
      fastSweeps += l.fastSweep && !wasFast[k];
      wasFast[k] = l.fastSweep;
      if (i < 600)
        continue;
      const double bound = l.fastLimitDeg + l.toleranceDeg;
      worst[k] = eastl::max(worst[k], double(l.sunErrorDeg / bound));
      if (i >= 3600)
        worstLate[k] = eastl::max(worstLate[k], double(l.sunErrorDeg / bound));
    }
  }
  INFO("fast sweeps " << fastSweeps << ", misses outside a fast sweep of a finer level " << otherMisses);
  CHECK(fastSweeps > 0);
  CHECK(otherMisses == 0);
  for (int k = 0; k < s.levels; ++k)
    UNSCOPED_INFO("level " << k << " sun error / (fast limit + tolerance) " << worst[k] << ", in the last 20 s " << worstLate[k]);
  CHECK(worst[s.levels - 1] <= 1);
  check_invariants(sim);
}

TEST_CASE("T12 a camera move of 50 m along the sun axis with a world box needs no jobs")
{
  RtssSim sim;
  sim.setWorldBox(BBox3(point3(sim.camera) - Point3(1000, 1000, 1000), point3(sim.camera) + Point3(1000, 1000, 1000)));
  sim.run(60);
  int jobs = 0;
  for (int i = 0; i < 600; ++i)
  {
    sim.cameraVel = sim.sun * ((i / 300) % 2 ? -10.f : 10.f);
    sim.step();
    jobs += sim.jobs.size();
  }
  CHECK(jobs == 0);
  check_invariants(sim);
}

TEST_CASE("T69 a level cube deeper than maxCasterReach keeps a depth margin of its extent, with and without a world box: the first "
          "rebuild of its deepest level comes within a step past a move of the extent along the sun")
{
  RtssSchedulerSettings s;
  s.maxCasterReach = 50;
  for (int withBox = 0; withBox < 2; ++withBox)
  {
    INFO((withBox ? "with" : "without") << " a world box");
    RtssSim sim(s);
    const int last = s.levels - 1;
    const double extent = sim.extent(last);
    REQUIRE(extent > s.maxCasterReach);
    if (withBox)
    {
      const Point3 span = Point3(1, 1, 1) * float(3 * extent + s.maxCasterReach);
      sim.setWorldBox(BBox3(point3(sim.camera) - span, point3(sim.camera) + span));
    }
    sim.run(60);
    REQUIRE(sim.jobs.empty());
    const double stepM = 0.5;
    sim.cameraVel = sim.sun * float(-stepM / sim.dt);
    double moved = 0;
    for (; moved < 2 * extent; moved += stepM)
    {
      sim.step();
      if (eastl::any_of(sim.jobs.begin(), sim.jobs.end(), [&](const RtssJob &j) { return sim.jobLevel(j) == last; }))
        break;
    }
    CHECK(moved + stepM >= extent);
    CHECK(moved + stepM < extent + stepM + 1e-6);
    check_invariants(sim);
  }
}

TEST_CASE("T13 without a world box a move along the sun axis past maxCasterReach makes slots REQUIRED again")
{
  RtssSim sim;
  sim.run(60);
  const DPoint3 start = sim.camera, down = dpoint3(-sim.sun);
  const double stepM = 20;
  double firstRequired[RTSS_MAX_LEVELS];
  for (double &d : firstRequired)
    d = -1;
  int requiredFrames = 0, readyWhileRequired = 0;
  sim.beforeCommit = [&](dag::Vector<uint32_t> &, dag::Vector<uint32_t> &) {
    int required = 0;
    for (int k = 0; k < sim.settings.levels; ++k)
      required += level_of_class(sim, RTSS_REQUIRED, k);
    if (!required)
      return;
    ++requiredFrames;
    readyWhileRequired += sim.sched.isReady(BBox3(point3(sim.camera), 1.f));
  };
  for (int i = 1; i <= 120; ++i)
  {
    sim.camera = start + down * (i * stepM);
    sim.step();
    for (int k = 0; k < sim.settings.levels; ++k)
      if (firstRequired[k] < 0 && level_of_class(sim, RTSS_REQUIRED, k) > 0)
        firstRequired[k] = i * stepM;
  }
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    const double expected = sim.settings.maxCasterReach - sim.extent(k);
    INFO("level " << k << " REQUIRED again after " << firstRequired[k] << " m, expected " << expected << " m");
    CHECK(firstRequired[k] >= expected);
    CHECK(firstRequired[k] < expected + stepM + 1e-6);
  }
  INFO("frames with REQUIRED depth rebuilds " << requiredFrames << ", ready in " << readyWhileRequired << " of them");
  CHECK(requiredFrames > 0);
  CHECK(readyWhileRequired == 0);
  sim.beforeCommit = nullptr;
  sim.run(10);
  CHECK(sim.sched.isReady(BBox3(point3(sim.camera), 1.f)));
  check_invariants(sim);
  sim.sched.invalidate();
  CHECK(sim.sched.isReady(BBox3(point3(sim.camera), 1.f)));
}

TEST_CASE("T14 every sweep during a walk finishes and entering slots join the sweep")
{
  RtssSim sim;
  sim.sunAngVel = day_axis(40) * float(DAY24_RATE);
  sim.selectionSamples = 16;
  sim.run(5);
  const Point3 ex = sim.row(0, sim.curBit(0), 0), ey = sim.row(0, sim.curBit(0), 1);
  double started[RTSS_MAX_LEVELS], planned[RTSS_MAX_LEVELS] = {}, longest[RTSS_MAX_LEVELS] = {};
  int sweeps[RTSS_MAX_LEVELS], pagesAtStart[RTSS_MAX_LEVELS] = {}, grown = 0, finished = 0;
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    started[k] = -1;
    sweeps[k] = sim.stats.levels[k].sweeps;
  }
  for (int i = 0; i < 3000; ++i)
  {
    const float a = float(sim.time * 6 / 60);
    sim.cameraVel = (ey * cosf(a) - ex * sinf(a)) * 6.f;
    sim.step();
    for (int k = 0; k < sim.settings.levels; ++k)
    {
      const RtssSchedulerLevelStats &l = sim.stats.levels[k];
      const bool restarted = l.sweeps != sweeps[k];
      if (started[k] >= 0)
        longest[k] = eastl::max(longest[k], sim.time - started[k] - planned[k]);
      if (started[k] >= 0 && (restarted || !l.sweeping))
      {
        ++finished;
        started[k] = -1;
      }
      if (restarted)
      {
        started[k] = sim.time;
        planned[k] = l.sweepSeconds;
        pagesAtStart[k] = l.sweepPages;
        sweeps[k] = l.sweeps;
      }
      if (l.sweeping && l.sweepPages > pagesAtStart[k])
      {
        ++grown;
        pagesAtStart[k] = l.sweepPages;
      }
    }
  }
  INFO("sweeps finished " << finished << ", sweep lists grown " << grown << " times, selection misses " << sim.selectionMissCount);
  CHECK(finished > 100);
  CHECK(grown > 0);
  CHECK(sim.selectionMissCount == 0);
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    INFO("level " << k << " longest delay after the planned end " << longest[k] << " s");
    CHECK(longest[k] < 0.15);
  }
  check_invariants(sim);
}

TEST_CASE("T15 after a teleport the coarsest level goes first, then non-inner pages before inner ones, fewer with a wider seam band")
{
  auto innerPages = [](float seam_band) {
    RtssSchedulerSettings s;
    s.seamBand = seam_band;
    RtssSim sim(s);
    sim.run(60);
    sim.camera += DPoint3(700, 0, -700);
    sim.coverageFrom = sim.frame + 6;
    int inner = 0, finerInner = 0, outerAfterInner = 0, coarsestLate = 0;
    for (int i = 0, frames = update_frames(sim.settings, finer_cube_pages(sim)); i < frames; ++i)
    {
      sim.step();
      int finer = 0;
      for (int j = 0; j < (int)sim.jobs.size(); ++j)
      {
        if ((sim.jobClass[j] & RTSS_JOB_CLASS_MASK) != RTSS_REQUIRED)
          continue;
        const bool coarsest = sim.jobLevel(sim.jobs[j]) == s.levels - 1, isInner = sim.jobClass[j] & RTSS_JOB_INNER;
        inner += isInner;
        coarsestLate += coarsest && finer > 0;
        outerAfterInner += !coarsest && !isInner && finerInner > 0;
        finer += !coarsest;
        finerInner += !coarsest && isInner;
      }
      coarsestLate += finer > 0 && !sim.levelComplete(s.levels - 1);
    }
    CHECK(coarsestLate == 0);
    CHECK(outerAfterInner == 0);
    check_invariants(sim);
    return inner;
  };
  const int inner = innerPages(0.1f), innerWide = innerPages(0.5f);
  INFO("inner REQUIRED pages " << inner << ", with a seam band of 0.5 " << innerWide);
  CHECK(inner >= 3 * 16);
  CHECK(innerWide < inner);
}

TEST_CASE("T70 a validity burst smaller than the coarsest level builds its pages nearest the camera first, inner ones too")
{
  RtssSchedulerSettings s;
  s.validityBurstPagesPerFrame = 64;
  RtssSim sim(s);
  sim.run(60);
  sim.camera += DPoint3(3000, 0, -3000);
  sim.coverageFrom = sim.frame + 1000;
  sim.step();
  const int last = s.levels - 1, bit = sim.curBit(last);
  const DPoint3 c = sim.sched.xform(last, bit).toLs(sim.camera) / sim.sched.pageWorld(last);
  auto dist2 = [&](const IPoint2 &p) { return (p.x + 0.5 - c.x) * (p.x + 0.5 - c.x) + (p.y + 0.5 - c.y) * (p.y + 0.5 - c.y); };
  double farthestBuilt = 0, nearestLeft = DBL_MAX;
  int built = 0;
  const IBBox2 &cube = sim.stats.levels[last].cube[bit];
  for (int y = cube[0].y; y <= cube[1].y; ++y)
    for (int x = cube[0].x; x <= cube[1].x; ++x)
      if (sim.holds(last, IPoint2(x, y), bit))
        farthestBuilt = eastl::max(farthestBuilt, dist2(IPoint2(x, y))), ++built;
      else
        nearestLeft = eastl::min(nearestLeft, dist2(IPoint2(x, y)));
  INFO(built << " pages built in the first frame, the farthest at " << sqrt(farthestBuilt) << " pages, the nearest left at "
             << sqrt(nearestLeft));
  CHECK(built == s.validityBurstPagesPerFrame + s.updatePagesPerFrame * s.batchFrames);
  CHECK(sim.holds(last, IPoint2(int(floor(c.x)), int(floor(c.y))), bit));
  CHECK(farthestBuilt <= nearestLeft + 1e-9);
  check_invariants(sim);
}

TEST_CASE("T16 a sweep with u against the camera motion blocks the release and keeps the coverage")
{
  RtssSchedulerSettings s;
  s.staticSweepSeconds = 4;
  s.updatePagesPerFrame = 96;
  RtssSim sim(s);
  sim.run(60);
  sim.sun = turned(sim.sun, 2.5);
  sim.step();
  REQUIRE(sim.sweeping(0));
  const Point2 u = sim.stats.levels[0].u;
  const Point3 dir = -(sim.row(0, sim.curBit(0), 0) * u.x + sim.row(0, sim.curBit(0), 1) * u.y);
  sim.cameraVel = dir * 6.f;
  int blockedFrames = 0, moves = 0, worstMoveRequired = 0;
  int2 origin[RTSS_MAX_LEVELS];
  for (int k = 0; k < s.levels; ++k)
    origin[k] = sim.levelTab(k).pageOrigin;
  for (int i = 0; i < 240; ++i)
  {
    sim.step();
    for (int k = 0; k < s.levels; ++k)
    {
      blockedFrames += sim.stats.levels[k].pretraceBlocked;
      const int2 &o = sim.levelTab(k).pageOrigin;
      if (o.x != origin[k].x || o.y != origin[k].y)
      {
        ++moves;
        worstMoveRequired = eastl::max(worstMoveRequired, required_in_window(sim, k));
      }
      origin[k] = o;
    }
  }
  INFO("frames with a blocked release " << blockedFrames << ", window moves " << moves << ", most REQUIRED pages at a move "
                                        << worstMoveRequired);
  CHECK(blockedFrames > 0);
  CHECK(moves > 4);
  CHECK(worstMoveRequired == 0);
  check_invariants(sim);
}

TEST_CASE("T17 a step above the window limit changes no slot page twice and keeps a page that both cubes need")
{
  const double stepDeg = 7.5, uShift = plus_u_shift(stepDeg);
  int worstChanges = 0, changed = 0, kept = 0, farMisses = 0, misses = 0, minSweepFrames = INT_MAX;
  RtssSchedulerSettings s;
  s.validityBurstPagesPerFrame = 16;
  for (int offset = 0; offset < 4; ++offset)
  {
    RtssSim sim(s);
    sim.camera += DPoint3(offset * 0.25 * 62 * 0.4, 0, offset * 0.1 * 62 * 0.4);
    sim.coverageFrom = 120;
    sim.run(150);
    REQUIRE(sim.jobs.empty());
    dag::Vector<uint8_t> changes(sim.slots().size(), 0);
    sim.strictCoverage = false;
    sim.sun = turned(sim.sun, stepDeg);
    int sweepFrames = 0;
    for (int i = 0; i < 200; ++i)
    {
      sim.step();
      for (const RtssJob &j : sim.jobs)
      {
        const int idx = sim.jobSlotIndex(j);
        const RtssSlot &old = sim.prevSlots[idx];
        if (sim.sweeping(sim.jobLevel(j)) && (old.flags & RTSS_SLOT_VALID) && old.page != rtss_pack_page(j.page))
        {
          ++changed;
          worstChanges = eastl::max(worstChanges, int(++changes[idx]));
        }
      }
      bool any = false;
      for (int k = 0; k < s.levels; ++k)
      {
        if (!sim.sweeping(k))
          continue;
        any = true;
        const IBBox2 &c = sim.stats.levels[k].cube[sim.curBit(k) ^ 1];
        for (int y = c.lim[0].y; y <= c.lim[1].y; ++y)
          for (int x = c.lim[0].x; x <= c.lim[1].x; ++x)
          {
            const RtssSlot &sl = sim.slot(k, IPoint2(x, y));
            const IPoint2 r = rtss_unpack_page(sl.page, IPoint2(x, y));
            kept += !sim.inWindow(k, IPoint2(x, y)) && (sl.flags & RTSS_SLOT_VALID) && r != IPoint2(x, y) && sim.inUse(k, r);
          }
      }
      sweepFrames += any;
    }
    minSweepFrames = eastl::min(minSweepFrames, sweepFrames);
    misses += sim.misses.size();
    for (const RtssCoverageMiss &m : sim.misses)
      farMisses += !near_plus_u_face(m, uShift);
    check_invariants(sim);
  }
  INFO("shortest sweep " << minSweepFrames << " frames, page changes " << changed << " (most for one slot " << worstChanges
                         << "), past-edge pages whose slot kept a needed page " << kept << ", misses " << misses
                         << ", far from a +u face " << farMisses);
  CHECK(minSweepFrames >= update_frames(s, (s.levels - 1) * s.windowPages * s.windowPages));
  CHECK(worstChanges <= 1);
  CHECK(changed > 0);
  CHECK(kept > 0);
  CHECK(farMisses == 0);
}

TEST_CASE("T19 a clamped tolerance at W = 32 keeps a normal animated sun out of fast sweeps")
{
  RtssSchedulerSettings s;
  s.windowPages = 32;
  s.updatePagesPerFrame = 96;
  RtssSim sim(s);
  sim.coverageFrom = 60;
  sim.sun = sun_at(45, 30);
  sim.sunAngVel = day_axis(40) * float(DAY24_RATE);
  int fastFrames = 0, windowRequired = 0, sweeps = 0;
  double longestLate = 0;
  for (int i = 0; i < 3600; ++i)
  {
    sim.step();
    if (sim.frame <= 600)
      continue;
    for (int k = 0; k < s.levels; ++k)
    {
      const RtssSchedulerLevelStats &l = sim.stats.levels[k];
      fastFrames += l.fastSweep;
      windowRequired += required_in_window(sim, k);
      longestLate = eastl::max(longestLate, double(l.lastSweepSeconds - l.sweepSeconds));
    }
  }
  for (int k = 0; k < s.levels; ++k)
    sweeps += sim.stats.levels[k].sweeps;
  const RtssSchedulerLevelStats &top = sim.stats.levels[s.levels - 1];
  INFO("sweeps " << sweeps << ", longest sweep end after the plan " << longestLate << " s, fast sweep frames " << fastFrames
                 << ", REQUIRED window pages " << windowRequired << ", level " << s.levels - 1 << " tolerance " << top.toleranceDeg
                 << " deg, fast limit " << top.fastLimitDeg << " deg");
  CHECK(sweeps > 100);
  CHECK(longestLate >= sim.dt * 0.99);
  CHECK(fastFrames == 0);
  CHECK(windowRequired == 0);
  CHECK(top.toleranceDeg <= 0.5f * top.fastLimitDeg + 1e-4f);
  check_invariants(sim);
}

TEST_CASE("T18 random moves, teleports, sun steps, invalidations and resets keep the invariants")
{
  int allMisses = 0;
  for (int run = 0; run < 24; ++run)
  {
    RtssSchedulerSettings s;
    static const int windows[] = {4, 8, 16, 32};
    s.windowPages = windows[run % 4];
    s.slotTexels = 32 << (run % 3);
    s.borderTexels = 1 + run % 4;
    s.levels = 1 + run % 6;
    s.updatePagesPerFrame = 8 + (run % 5) * 12;
    s.validityBurstPagesPerFrame = run % 2 ? 64 : 256;
    s.recipes[run % 3].raster = RtssRaster::All;
    s.recipes[1].pageCost = 1 + (run % 3) * 0.5f;
    RtssSim sim(s);
    sim.seed = 1000 + run * 77;
    sim.strictCoverage = false;
    sim.sunAngVel = day_axis(30 + run * 2) * float(DAY24_RATE * (1 + run % 3));
    int ev = 1000 + run, heldInFast = 0, heldElse = 0, quietMissing = 0, lastEvent = 0, misses = 0;
    for (int i = 0; i < 2000; ++i)
    {
      const float r = _frnd(ev);
      if (r < 0.01f)
      {
        const float a = _frnd(ev) * 6.28f;
        sim.cameraVel = Point3(cosf(a), 0, sinf(a)) * (_frnd(ev) < 0.3f ? 30.f : 6.f);
      }
      else if (r < 0.012f)
      {
        sim.camera += DPoint3(_frnd(ev) * 2000 - 1000, 0, _frnd(ev) * 2000 - 1000);
        lastEvent = i;
      }
      else if (r < 0.015f)
      {
        sim.sun = turned(sim.sun, 1 + _frnd(ev) * 9);
        lastEvent = i;
      }
      else if (r < 0.02f)
      {
        const Point3 c = point3(sim.camera) + Point3(_frnd(ev) * 60 - 30, 0, _frnd(ev) * 60 - 30);
        const BBox3 box(c - Point3(3, 3, 3), c + Point3(3, 3, 3));
        sim.sched.invalidateBoxes(&box, 1);
      }
      else if (r < 0.021f)
      {
        sim.reset();
        lastEvent = i;
      }
      else if (r < 0.023f)
        sim.sched.invalidate();
      if (sim.sun.y < 0.2f)
      {
        sim.sun = sun_at(50, _frnd(ev) * 360);
        lastEvent = i;
      }
      sim.misses.clear();
      sim.step();
      misses += sim.misses.size();
      for (const RtssCoverageMiss &m : sim.misses)
        if (!m.pageMissing)
          ++(sim.stats.levels[m.level].fastSweep ? heldInFast : heldElse);
        else
          quietMissing += i - lastEvent > 60 && !sim.stats.levels[m.level].fastSweep &&
                          s.recipes[m.level].pageCost <=
                            sim.settings.updatePagesPerFrame - sim.updateCost +
                              (m.level == sim.settings.levels - 1 ? sim.settings.validityBurstPagesPerFrame - sim.burstCost : 0.0);
    }
    INFO("run " << run << ": misses " << misses << ", with the pages held " << heldInFast << " in fast sweeps and " << heldElse
                << " else, with a page missing in quiet frames " << quietMissing);
    allMisses += misses;
    CHECK(heldElse == 0);
    CHECK(quietMissing == 0);
    check_invariants(sim, RTSS_I7_COVERAGE);
  }
  CHECK(allMisses > 0); // T11 has the misses of the held pages, in fast sweeps
}

TEST_CASE("T20 setBudget keeps every slot valid, the next sweep uses the new budget, and the stats keep the costs of the last plan")
{
  RtssSim sim;
  sim.run(60);
  sim.settings.updatePagesPerFrame = 8;
  sim.sched.setBudget(sim.settings);
  sim.run(30);
  CHECK(sim.jobs.empty());
  for (int k = 0; k < sim.settings.levels; ++k)
    CHECK(sim.stats.levels[k].validSlots == sim.windowPages() * sim.windowPages());
  sim.sun = turned(sim.sun, 1);
  double maxPages = 0;
  for (int i = 0; i < 300; ++i)
  {
    sim.step();
    maxPages = eastl::max(maxPages, sim.otherPages());
  }
  CHECK(maxPages == 8);
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    CHECK_FALSE(sim.sweeping(k));
    CHECK(sim.stats.levels[k].validSlots == sim.windowPages() * sim.windowPages());
  }
  sim.settings.recipes[0].pageCost = 4;
  sim.sched.setRecipes(sim.settings);
  sim.sched.invalidate();
  sim.step();
  const RtssSchedulerStats planned = sim.stats;
  sim.settings.updatePagesPerFrame = 1;
  sim.sched.setBudget(sim.settings);
  sim.sched.getStats(sim.stats);
  CHECK(planned.cost[RTSS_DIRTY] > planned.pages[RTSS_DIRTY]);
  CHECK(sim.stats.cost[RTSS_DIRTY] == planned.cost[RTSS_DIRTY]);
  check_invariants(sim);
}

TEST_CASE("T21 in the seam band a dither below the band selects the next level, and the coarsest level fades to lit")
{
  RtssSim sim;
  sim.run(120);
  const RtssCpuTables t = sim.tables();
  const int levels = sim.settings.levels;
  for (int k = 0; k < levels; ++k)
  {
    INFO("level " << k);
    const bool coarsest = k + 1 == levels;
    const int next = coarsest ? k : k + 1;
    float fade = -1.f;
    const Point3 pos = sim.cubePoint(0, sim.curBit(0), Point3(0.95f * float(1 << k), 0, 0));
    CHECK(rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 0.51f, -1, &fade) == k);
    CHECK(fabsf(fade - (coarsest ? 0.5f : 0.f)) < 0.01f);
    CHECK(rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 0.49f) == next);
    const Point3 edge = sim.cubePoint(0, sim.curBit(0), Point3(0.999f * float(1 << k), 0, 0));
    CHECK(rtss_cpu_lookup(t, edge, Point3(0, 0, 0), 0.f, 0.9f, -1, &fade) == next);
    CHECK(fabsf(fade - (coarsest ? 0.99f : 0.f)) < 0.01f);
  }
  check_invariants(sim);
}

TEST_CASE("T22 invalidateBoxes takes every box and only the levels of the mask")
{
  RtssSim sim;
  sim.run(120);
  const Point3 c = point3(sim.camera);
  const BBox3 boxes[2] = {BBox3(c + Point3(-12, 0, 0), 2.f), BBox3(c + Point3(12, 0, 0), 2.f)};
  int alone[2][3];
  for (int b = 0; b < 2; ++b)
  {
    sim.sched.invalidateBoxes(&boxes[b], 1);
    sim.sched.getStats(sim.stats);
    for (int k = 0; k < 3; ++k)
      alone[b][k] = sim.stats.levels[k].dirtySlots;
    sim.run(60);
  }
  sim.sched.invalidateBoxes(boxes, 2, ~2u);
  sim.sched.getStats(sim.stats);
  for (int k = 0; k < 3; ++k)
  {
    INFO("level " << k << ": " << alone[0][k] << " and " << alone[1][k] << " slots alone");
    CHECK(alone[0][k] > 0);
    CHECK(alone[1][k] > 0);
    CHECK(sim.stats.levels[k].dirtySlots == (k == 1 ? 0 : alone[0][k] + alone[1][k]));
  }
  CHECK(sim.stats.levels[3].dirtySlots > 0);
  sim.run(60);
  for (int k = 0; k < sim.settings.levels; ++k)
    CHECK(sim.stats.levels[k].dirtySlots == 0);
  check_invariants(sim);
}

TEST_CASE("T23 skipped jobs change no slot, and a sweep with skipped jobs still finishes")
{
  RtssSim sim;
  sim.run(60);
  const int W = sim.windowPages(), sweepsBefore = sim.stats.levels[0].sweeps;
  int keep = 0, skipFrames = 0;
  sim.beforeCommit = [&](dag::Vector<uint32_t> &skipped, dag::Vector<uint32_t> &) {
    int n = 0;
    for (int i = 0; i < (int)sim.jobs.size(); ++i)
      if (sim.jobLevel(sim.jobs[i]) == 0 && n++ >= keep)
        skipped.push_back(i);
    skipFrames += n > keep;
  };
  sim.sun = turned(sim.sun, 1);
  sim.step();
  CHECK(sim.sweeping(0));
  const dag::Vector<RtssSlot> before(sim.slots().begin(), sim.slots().begin() + W * W);
  skipFrames = 0;
  sim.run(20);
  CHECK(skipFrames == 20);
  CHECK(memcmp(before.data(), sim.slots().data(), W * W * sizeof(RtssSlot)) == 0);
  CHECK(sim.sweeping(0));
  keep = 1;
  int changedMax = 0, changedSum = 0;
  for (int f = 0; f < 20; ++f)
  {
    sim.step();
    int changed = 0;
    for (int i = 0; i < W * W; ++i)
      changed += memcmp(&sim.slots()[i], &sim.prevSlots[i], sizeof(RtssSlot)) != 0;
    changedMax = eastl::max(changedMax, changed);
    changedSum += changed;
  }
  CHECK(changedMax == 1);
  CHECK(changedSum > 1);
  sim.beforeCommit = nullptr;
  sim.run(300);
  CHECK_FALSE(sim.sweeping(0));
  CHECK(sim.stats.levels[0].sweeps == sweepsBefore + 1);
  CHECK(sim.levelComplete(0));
  check_invariants(sim);
}

TEST_CASE("T25 the normal offset moves the receiver along its normal by 1 - NoL")
{
  RtssSim sim;
  sim.run(120);
  const RtssCpuTables t = sim.tables();
  const RtssLevel &l = sim.levelTab(0);
  const int bit = sim.curBit(0);
  const Point3 d = sim.row(0, bit, 2);
  const IPoint2 page(l.pageOrigin.x + int(floorf(l.xform[bit * 3].w * l.invPage)),
    l.pageOrigin.y + int(floorf(l.xform[bit * 3 + 1].w * l.invPage)));
  const float bias = 2, farZ = sim.slot(0, page).maxDepth - l.xform[bit * 3 + 2].w;
  const float nearZ = sim.slot(0, page).minDepth - l.xform[bit * 3 + 2].w;
  auto texelAt = [&](float z) { return t.texel0 * fmaxf(fabsf(z) * t.params.x, 1.f); };
  const float offset = bias * texelAt(farZ);
  const Point3 pos = d * (farZ - 0.5f * offset);
  CHECK(rtss_cpu_lookup(t, pos, -d, bias, 0.5f, 0) == 0);
  CHECK(rtss_cpu_lookup(t, pos + d * offset, -d, bias, 0.5f, 0) == -1);
  CHECK(rtss_cpu_lookup(t, pos, d, bias, 0.5f, 0) == -1);
  const Point3 n = -d * 0.6f + sim.row(0, bit, 0) * 0.8f;
  const Point3 nearPos = d * (nearZ + 0.3f * texelAt(nearZ));
  CHECK(rtss_cpu_lookup(t, nearPos, Point3(0, 0, 0), bias, 0.5f, 0) == 0);
  CHECK(rtss_cpu_lookup(t, nearPos, n, bias, 0.5f, 0) == -1);
  const RtssLevel &l1 = sim.levelTab(1);
  const int bit1 = sim.curBit(1);
  const Point3 d1 = sim.row(1, bit1, 2), n1 = -d1 * 0.6f + sim.row(1, bit1, 0) * 0.8f;
  const IPoint2 page1(l1.pageOrigin.x + int(floorf(l1.xform[bit1 * 3].w * l1.invPage)),
    l1.pageOrigin.y + int(floorf(l1.xform[bit1 * 3 + 1].w * l1.invPage)));
  const float nearZ1 = sim.slot(1, page1).minDepth - l1.xform[bit1 * 3 + 2].w;
  const Point3 nearPos1 = d1 * (nearZ1 + 0.7f * texelAt(nearZ1));
  CHECK(rtss_cpu_lookup(t, nearPos1, n1, bias, 0.5f, 1) == 1);
  check_invariants(sim);
}

static void run_to_idle(RtssSim &sim, int max_frames, const eastl::function<void()> &each_frame = {})
{
  for (int i = 0; i < max_frames; ++i)
  {
    bool sweeping = false;
    for (int k = 0; k < sim.settings.levels; ++k)
      sweeping |= sim.sweeping(k);
    if (!sweeping)
      return;
    sim.step();
    if (each_frame)
      each_frame();
  }
  FAIL("a level still sweeps after " << max_frames << " frames");
}

TEST_CASE("T26 a teleport makes the running sweep fast")
{
  RtssSim sim;
  sim.run(60);
  sim.sun = turned(sim.sun, 1);
  sim.run(2);
  REQUIRE(sim.sweeping(0));
  REQUIRE_FALSE(sim.stats.levels[0].fastSweep);
  const int sweeps = sim.stats.levels[0].sweeps;
  sim.camera += DPoint3(1000, 0, 0);
  sim.strictCoverage = false;
  sim.step();
  CHECK(sim.sweeping(0));
  CHECK(sim.stats.levels[0].fastSweep);
  CHECK(sim.stats.levels[0].sweeps == sweeps);
  run_to_idle(sim, 120);
  auto complete = [&] {
    for (int k = 0; k < sim.settings.levels; ++k)
      if (!sim.levelComplete(k))
        return false;
    return true;
  };
  for (int i = 0; i < 60 && !complete(); ++i)
    sim.step();
  sim.strictCoverage = true;
  sim.run(60);
  CHECK_FALSE(sim.sweeping(0));
  CHECK(sim.levelComplete(0));
  check_invariants(sim);
}

TEST_CASE("T27 a 180 deg step sweeps every level fast with u = (1, 0)")
{
  RtssSchedulerSettings s;
  s.minSunY = -1.1f;
  RtssSim sim(s);
  sim.run(60);
  sim.sun = turned(sim.sun, 1);
  sim.run(60);
  for (int k = 0; k < s.levels; ++k)
    REQUIRE_FALSE((sim.stats.levels[k].u == Point2(1, 0)));
  sim.sun = -sim.sun;
  sim.strictCoverage = false;
  sim.step();
  for (int k = 0; k < s.levels; ++k)
  {
    INFO("level " << k);
    CHECK(sim.sweeping(k));
    CHECK(sim.stats.levels[k].fastSweep);
    CHECK((sim.stats.levels[k].u == Point2(1, 0)));
  }
  run_to_idle(sim, 300);
  sim.strictCoverage = true;
  sim.run(60);
  for (int k = 0; k < s.levels; ++k)
  {
    CHECK_FALSE(sim.sweeping(k));
    CHECK(sim.levelComplete(k));
  }
  check_invariants(sim);
}

TEST_CASE("T28 texel0 below 0.001 or NaN is clamped to 0.001, above 1000 to 1000, and 0.001 stays")
{
  RtssSchedulerSettings s;
  s.texel0 = 0.0005f;
  CHECK(RtssScheduler(s).getSettings().texel0 == 0.001f);
  s.texel0 = NAN;
  CHECK(RtssScheduler(s).getSettings().texel0 == 0.001f);
  s.texel0 = FLT_MAX;
  CHECK(RtssScheduler(s).getSettings().texel0 == 1000.f);
  s.texel0 = 0.001f;
  CHECK(RtssScheduler(s).getSettings().texel0 == 0.001f);
}

TEST_CASE("T29 a 240-minute day clamps the sweep duration and keeps the sun error of T3")
{
  RtssSim sim;
  bool clamped = false;
  check_day_sun_error(sim, DAY24_RATE / 10, [&]() {
    for (int k = 0; k < sim.settings.levels; ++k)
      clamped |= sim.sweeping(k) && sim.stats.levels[k].sweepSeconds == sim.settings.maxSweepSeconds;
  });
  CHECK(clamped);
  check_invariants(sim);
}

TEST_CASE("T30 setBudget gives the next sweep the new timing")
{
  RtssSim sim;
  sim.run(60);
  sim.settings.staticSweepSeconds = 1;
  sim.sched.setBudget(sim.settings);
  sim.sun = turned(sim.sun, 1);
  sim.step();
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    CHECK(sim.sweeping(k));
    CHECK(sim.stats.levels[k].sweepSeconds == 1.f);
  }
  check_invariants(sim);
}

TEST_CASE("T31 anisotropic texels at a low sun: a step in one scale sweeps in order, a new scale sweeps fast")
{
  RtssSchedulerSettings s;
  s.maxAnisotropy = 8;
  RtssSim sim(s);
  sim.sun = sun_at(14, 30);
  sim.run(60);
  CHECK(fabsf(length(sim.row(0, sim.curBit(0), 1)) - 4) < 1e-5f);
  for (int k = 0; k < s.levels; ++k)
    CHECK(sim.levelComplete(k));

  sim.sun = sun_at(14.3, 30);
  sim.step();
  CHECK(sim.sweeping(0));
  CHECK_FALSE(sim.stats.levels[0].fastSweep);
  CHECK(sim.stats.levels[0].fastLimitDeg < 0.7f);
  run_to_idle(sim, 600);
  check_invariants(sim);

  int innerChecked = 0, innerWrong = 0;
  auto checkInner = [&]() {
    const double band = 1 - sim.sched.getSettings().seamBand;
    for (int j = 0; j < (int)sim.jobs.size(); ++j)
    {
      const RtssJob &job = sim.jobs[j];
      const int k = sim.jobLevel(job);
      if (k == 0)
        continue;
      const RtssLightXform &x = sim.sched.xform(k, sim.curBit(k)), &xf = sim.sched.xform(k - 1, sim.curBit(k - 1));
      if (x.sy == xf.sy)
        continue;
      const double pw = sim.sched.pageWorld(k), h = band * sim.extent(k - 1) - sim.sched.pageWorld(k - 1), hy = h * x.sy / xf.sy;
      const DPoint3 c = x.toLs(sim.camera);
      const bool inner = job.page.x * pw >= c.x - h && (job.page.x + 1) * pw <= c.x + h && job.page.y * pw >= c.y - hy &&
                         (job.page.y + 1) * pw <= c.y + hy;
      ++innerChecked;
      innerWrong += inner != ((sim.jobClass[j] & RTSS_JOB_INNER) != 0);
    }
  };
  sim.sun = sun_at(17.1, 30);
  sim.strictCoverage = false;
  sim.step();
  checkInner();
  run_to_idle(sim, 600, checkInner);
  sim.strictCoverage = true;
  sim.run(60);
  sim.sun = sun_at(17.4, 30);
  sim.strictCoverage = false;
  sim.step();
  CHECK(sim.stats.levels[0].fastSweep);
  checkInner();
  run_to_idle(sim, 600, checkInner);
  sim.strictCoverage = true;
  INFO(innerChecked << " jobs of a level with another sy than the finer level, " << innerWrong << " with a wrong INNER bit");
  CHECK(innerChecked > 0);
  CHECK(innerWrong == 0);
  sim.run(60);
  CHECK(fabsf(length(sim.row(0, sim.curBit(0), 1)) - 2 * sqrtf(2)) < 1e-5f);
  for (int k = 0; k < s.levels; ++k)
    CHECK(sim.levelComplete(k));
  const WalkResult r = walk(sim, 600, [](RtssSim &) { return Point3(10, 0, 6); });
  INFO(r.requiredFrames << " level frames with REQUIRED window pages, " << r.requiredUnexplained << " without a blocked pre-trace");
  CHECK(r.requiredUnexplained == 0);
  CHECK(sim.selectionMissCount == 0);
  check_invariants(sim);
}

TEST_CASE("T32 a border of 3 texels makes smaller pages and keeps the invariants")
{
  RtssSchedulerSettings s;
  s.borderTexels = 3;
  RtssSim sim(s);
  sim.run(60);
  IPoint4 paramsI;
  Point4 params, origin;
  sim.sched.globals(paramsI, params, origin);
  CHECK(paramsI.z == s.slotTexels);
  CHECK(paramsI.w == s.slotTexels - 6);
  CHECK(fabs(sim.levelTab(1).invPage * sim.sched.pageWorld(1) - 1) < 1e-6);
  CHECK(fabsf(params.w * s.seamBand - 1) < 1e-6f);
  sim.sun = turned(sim.sun, 1);
  sim.step();
  run_to_idle(sim, 600);
  sim.cameraVel = Point3(10, 0, 6);
  sim.run(240);
  sim.cameraVel = Point3(0, 0, 0);
  sim.run(60);
  for (int k = 0; k < s.levels; ++k)
    CHECK(sim.levelComplete(k));
  check_invariants(sim);
}

TEST_CASE("T33 settings out of range are clamped, and windowPages and slotTexels round up to a power of two")
{
  RtssSchedulerSettings s;
  s.borderTexels = 0;
  s.maxAnisotropy = 0.5f;
  CHECK(RtssScheduler(s).getSettings().borderTexels == 1);
  CHECK(RtssScheduler(s).getSettings().maxAnisotropy == 1.f);
  s.borderTexels = 5;
  s.maxAnisotropy = 16.f;
  CHECK(RtssScheduler(s).getSettings().borderTexels == 4);
  CHECK(RtssScheduler(s).getSettings().maxAnisotropy == 8.f);
  s.recipes[0].pageCost = 0.f;
  s.recipes[1].pageCost = 1000.f;
  CHECK(RtssScheduler(s).getSettings().recipes[0].pageCost == 0.1f);
  CHECK(RtssScheduler(s).getSettings().recipes[1].pageCost == 100.f);
  RtssScheduler withRecipes{RtssSchedulerSettings()};
  withRecipes.setRecipes(s);
  CHECK(withRecipes.getSettings().recipes[0].pageCost == 0.1f);
  CHECK(withRecipes.getSettings().recipes[1].pageCost == 100.f);

  s = RtssSchedulerSettings();
  s.levels = 0;
  s.windowPages = 12;
  s.slotTexels = 256;
  RtssSchedulerSettings c = RtssScheduler(s).getSettings();
  CHECK(c.levels == 1);
  CHECK(c.windowPages == 16);
  CHECK(c.slotTexels == 128);
  s.levels = RTSS_MAX_LEVELS + 1;
  s.windowPages = 2;
  s.slotTexels = 48;
  c = RtssScheduler(s).getSettings();
  CHECK(c.levels == RTSS_MAX_LEVELS);
  CHECK(c.windowPages == 4);
  CHECK(c.slotTexels == 64);

  RtssSim sim(c);
  sim.run(60);
  CHECK(sim.stats.levels.size() == RTSS_MAX_LEVELS);
  for (int k = 0; k < RTSS_MAX_LEVELS; ++k)
    CHECK(sim.levelComplete(k));
  check_invariants(sim);
}

TEST_CASE("T34 seamBand sets the width of the band, and 0 is a step")
{
  RtssSchedulerSettings s;
  s.seamBand = 0.25f;
  RtssSim sim(s);
  sim.run(120);
  const RtssCpuTables t = sim.tables();
  const Point3 pos = sim.cubePoint(0, sim.curBit(0), Point3(0.875f, 0, 0));
  CHECK(rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 0.51f) == 0);
  CHECK(rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 0.49f) == 1);
  const Point3 inner = sim.cubePoint(0, sim.curBit(0), Point3(0.74f, 0, 0));
  CHECK(rtss_cpu_lookup(t, inner, Point3(0, 0, 0), 0.f, 0.f) == 0);
  sim.settings.seamBand = 0.f;
  sim.sched.setBudget(sim.settings);
  const Point3 edge = sim.cubePoint(0, sim.curBit(0), Point3(0.9995f, 0, 0));
  CHECK(rtss_cpu_lookup(sim.tables(), edge, Point3(0, 0, 0), 0.f, 0.f) == 0);
  check_invariants(sim);
}

TEST_CASE("T35 an All recipe makes every DIRTY job of the T10 box whole-slot, at a cost of 1")
{
  RtssSchedulerSettings s;
  for (RtssLevelRecipe &r : s.recipes)
    r.raster = RtssRaster::All;
  RtssSim sim(s);
  sim.run(60);
  const Point3 c = point3(sim.camera) + Point3(4.f, 0.5f, -3.f);
  const BBox3 box(c - Point3(10, 10, 10), c + Point3(10, 10, 10));
  dag::Vector<uint8_t> expected(sim.slots().size());
  int expectedSlots = 0;
  for (int i = 0; i < (int)sim.slots().size(); ++i)
  {
    int r[4];
    expected_dirty_rect(sim, i, box, r);
    expected[i] = r[0] < r[2] && r[1] < r[3];
    expectedSlots += expected[i];
  }
  sim.sched.invalidateBoxes(&box, 1);
  int dirtyJobs = 0, subSlotJobs = 0, unexpected = 0, frames = 0;
  double dirtyPages = 0;
  for (; frames < 200; ++frames)
  {
    sim.step();
    for (int i = 0; i < (int)sim.jobs.size(); ++i)
      if ((sim.jobClass[i] & RTSS_JOB_CLASS_MASK) == RTSS_DIRTY)
      {
        ++dirtyJobs;
        subSlotJobs += sim.jobs[i].rect != sim.wholeRect();
        unexpected += !expected[sim.jobSlotIndex(sim.jobs[i])];
      }
    dirtyPages += sim.stats.pages[RTSS_DIRTY];
    int dirty = 0;
    for (const RtssSchedulerLevelStats &l : sim.stats.levels)
      dirty += l.dirtySlots;
    if (dirty == 0 && sim.jobs.empty())
      break;
  }
  INFO("DIRTY jobs " << dirtyJobs << " (" << subSlotJobs << " sub-slot, " << unexpected << " outside the box) for " << expectedSlots
                     << " slots, " << dirtyPages << " pages, in " << frames << " frames");
  CHECK(dirtyJobs == expectedSlots);
  CHECK(subSlotJobs == 0);
  CHECK(unexpected == 0);
  CHECK(dirtyPages == double(dirtyJobs));
  CHECK(frames < 200);
  check_invariants(sim);
}

TEST_CASE("T36 provisional jobs take the page and the bit, stay DIRTY and are rebuilt whole, and a provisional sweep finishes")
{
  RtssSim sim;
  sim.run(60);
  const int W = sim.windowPages(), sweepsBefore = sim.stats.levels[1].sweeps;
  dag::Vector<uint8_t> touched(W * W, 0);
  int sweptJobs = 0;
  sim.beforeCommit = [&](dag::Vector<uint32_t> &, dag::Vector<uint32_t> &provisional) {
    for (int i = 0; i < (int)sim.jobs.size(); ++i)
      if (sim.jobLevel(sim.jobs[i]) == 1)
      {
        provisional.push_back(i);
        touched[sim.jobSlotIndex(sim.jobs[i]) - W * W] = 1;
        sweptJobs += (sim.jobClass[i] & RTSS_JOB_SWEPT) != 0;
      }
  };
  sim.sun = turned(sim.sun, 1);
  int wrongSlots = 0;
  float progressFirst = -1;
  for (int f = 0; f < 10; ++f)
  {
    sim.step();
    if (progressFirst < 0)
      progressFirst = sim.stats.levels[1].sweepProgress;
    for (uint32_t i : sim.provisional)
      wrongSlots += !RtssSim::holdsJob(sim.slots()[sim.jobSlotIndex(sim.jobs[i])], sim.jobs[i]);
  }
  sim.beforeCommit = nullptr;
  int touchedSlots = 0;
  for (uint8_t t : touched)
    touchedSlots += t;
  int dirtyAfter = -1, partialJobs = 0, cleanFrames = 0;
  float progressAfter = 0;
  for (; cleanFrames < 200; ++cleanFrames)
  {
    sim.step();
    if (dirtyAfter < 0)
    {
      dirtyAfter = sim.stats.levels[1].dirtySlots;
      progressAfter = sim.stats.levels[1].sweepProgress;
    }
    for (const RtssJob &j : sim.jobs)
      partialJobs += sim.jobLevel(j) == 1 && j.rect != sim.wholeRect();
    if (sim.stats.levels[1].dirtySlots == 0)
      break;
  }
  run_to_idle(sim, 600);
  INFO("provisional slots " << touchedSlots << " (" << sweptJobs << " sweep jobs), DIRTY after " << dirtyAfter << ", clean after "
                            << cleanFrames << " frames, sweep progress " << progressFirst << " to " << progressAfter);
  CHECK(sweptJobs > 0);
  CHECK(wrongSlots == 0);
  CHECK(dirtyAfter == touchedSlots);
  CHECK(partialJobs == 0);
  CHECK(cleanFrames < 200);
  CHECK(progressAfter > progressFirst);
  CHECK(sim.stats.levels[1].sweeps == sweepsBefore + 1);
  CHECK(sim.levelComplete(1));
  check_invariants(sim);
}

TEST_CASE("T37 rtss_raster_plan groups jobs by level and bit, packs the cull group regions into a scratch of 1 to 256 slots, "
          "turned by 90 deg only where they do not fit")
{
  const int batchPages = GENERATE(1, 15, 16, 256);
  const bool transpose = GENERATE(false, true);
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 512;
  s.rasterBatchPages = batchPages;
  s.rasterTranspose = transpose;
  RtssSim sim(s);
  sim.run(60);
  sim.sun = turned(sim.sun, 1);
  sim.sched.invalidate();
  dag::Vector<RtssJob> jobs;
  for (int f = 0; f < 60 && jobs.empty(); ++f)
  {
    sim.step();
    bool level[RTSS_MAX_LEVELS] = {}, bit[2] = {};
    for (const RtssJob &j : sim.jobs)
      level[sim.jobLevel(j)] = bit[j.flags & RTSS_JOB_BIT] = true;
    if (eastl::count(level, level + RTSS_MAX_LEVELS, true) >= 3 && bit[0] && bit[1] && sim.jobs.size() > 256)
      jobs.assign(sim.jobs.begin(), sim.jobs.end());
  }
  REQUIRE(!jobs.empty());
  RtssRasterPlan rp;
  rtss_raster_plan(make_span(jobs), ~0u, sim.sched, rp);
  const int n = (int)jobs.size(), S = s.slotTexels, P = S - 2 * s.borderTexels;
  dag::Vector<uint32_t> sorted(rp.order.begin(), rp.order.end());
  eastl::sort(sorted.begin(), sorted.end());
  bool permutation = (int)sorted.size() == n;
  for (int i = 0; permutation && i < n; ++i)
    permutation = sorted[i] == uint32_t(i);
  int next = 0, badGroups = 0, prevKey = -1, badRegions = 0, overlaps = 0, badWindows = 0, multiPage = 0, turned = 0;
  for (int gi = 0; gi < (int)rp.cullGroups.size(); ++gi)
  {
    const RtssRasterGroup &g = rp.cullGroups[gi];
    badGroups += g.first != next || g.count <= 0 || g.level * 2 + g.bit < prevKey;
    prevKey = g.level * 2 + g.bit;
    const IPoint2 box = g.pages.width() + IPoint2(1, 1), region = box * P + IPoint2(S - P, S - P);
    multiPage += g.count > 1;
    turned += g.transposed;
    badRegions += g.transposed != (region.x > rp.size.x || region.y > rp.size.y) ||
                  g.size != (g.transposed ? IPoint2(region.y, region.x) : region) || g.origin.x < 0 || g.origin.y < 0 ||
                  g.origin.x + g.size.x > rp.size.x || g.origin.y + g.size.y > rp.size.y || g.batch < 0 || g.batch >= rp.batches ||
                  (gi > 0 && g.batch < rp.cullGroups[gi - 1].batch);
    for (int o = 0; o < gi; ++o)
    {
      const RtssRasterGroup &h = rp.cullGroups[o];
      overlaps += h.batch == g.batch && h.origin.x < g.origin.x + g.size.x && g.origin.x < h.origin.x + h.size.x &&
                  h.origin.y < g.origin.y + g.size.y && g.origin.y < h.origin.y + h.size.y;
    }
    for (int i = g.first; i < g.first + g.count; ++i)
    {
      const RtssJob &j = jobs[rp.order[i]];
      badGroups += sim.jobLevel(j) != g.level || int(j.flags & RTSS_JOB_BIT) != g.bit || !(g.pages & rtss_job_page(j));
      const IPoint2 window(j.rasterTile & (RTSS_TILE_TRANSPOSED - 1), j.rasterTile >> 16);
      const IPoint2 offset = g.transposed ? IPoint2(j.page.y - g.pages[0].y, j.page.x - g.pages[0].x)
                                          : IPoint2(j.page.x - g.pages[0].x, g.pages[1].y - j.page.y);
      badWindows += window != g.origin + offset * P || ((j.rasterTile & RTSS_TILE_TRANSPOSED) != 0) != g.transposed;
    }
    next = g.first + g.count;
  }
  INFO(n << " jobs in " << rp.cullGroups.size() << " groups (" << multiPage << " of more pages, " << turned << " turned), "
         << rp.batches << " fills of " << rp.size.x << " x " << rp.size.y);
  const int side = int(ceil(sqrt(double(batchPages)))) * S;
  CHECK(permutation);
  CHECK(rp.cullGroups.size() >= 3);
  CHECK((batchPages == 1 || multiPage > 0));
  CHECK(badGroups == 0);
  CHECK(next == n);
  CHECK(rp.size.x == side);
  CHECK(rp.size.y == (transpose ? eastl::max(side / 2, S) : side));
  CHECK((transpose || turned == 0));
  CHECK((!transpose || batchPages != 16 || turned > 0));
  CHECK(badRegions == 0);
  CHECK(overlaps == 0);
  CHECK(badWindows == 0);
  check_invariants(sim);
}

TEST_CASE("T38 rtss_raster_skips skips the jobs that are not ready and keep the page, and builds the others as provisional")
{
  RtssSchedulerSettings s;
  s.recipes[0].raster = RtssRaster::All;
  s.recipes[1].raster = RtssRaster::Augment;
  RtssSim sim(s);
  sim.run(60);
  sim.cameraVel = Point3(6, 0, 0);
  sim.sun = turned(sim.sun, 1);
  sim.sched.invalidate();
  const int sweepsBefore = sim.stats.levels[0].sweeps;
  int skippedJobs = 0, provisionalSwept = 0, provisionalOther = 0, wrong = 0;
  sim.beforeCommit = [&](dag::Vector<uint32_t> &skipped, dag::Vector<uint32_t> &provisional) {
    dag::Vector<uint8_t> ready(sim.jobs.size());
    for (int i = 0; i < (int)ready.size(); ++i)
      ready[i] = i % 3 != 1;
    skipped.assign(3, ~0u);
    provisional.assign(3, ~0u);
    RtssRasterPlan rp;
    const uint32_t rasterLevels = rtss_raster_levels(sim.sched);
    rtss_raster_plan(sim.jobs, rasterLevels, sim.sched, rp);
    rtss_raster_skips(rp, make_span_const(sim.jobClass), make_span_const(ready), skipped, provisional);
    dag::Vector<uint32_t> expected[2];
    for (uint32_t i = 0; i < ready.size(); ++i)
      if (!ready[i] && (rasterLevels & (1u << sim.jobLevel(sim.jobs[i]))))
        expected[(sim.jobClass[i] & RTSS_JOB_SAME_PAGE) ? 0 : 1].push_back(i);
    wrong += skipped != expected[0] || provisional != expected[1];
    for (uint32_t i : provisional)
    {
      provisionalSwept += (sim.jobClass[i] & RTSS_JOB_SWEPT) != 0;
      provisionalOther += (sim.jobClass[i] & RTSS_JOB_SWEPT) == 0;
    }
    skippedJobs += skipped.size();
  };
  sim.run(30);
  sim.beforeCommit = nullptr;
  sim.cameraVel = Point3(0, 0, 0);
  run_to_idle(sim, 600);
  sim.run(120);
  INFO("skipped " << skippedJobs << ", provisional sweep jobs " << provisionalSwept << ", other provisional " << provisionalOther
                  << ", wrong " << wrong);
  CHECK(wrong == 0);
  CHECK(skippedJobs > 0);
  CHECK(provisionalSwept > 0);
  CHECK(provisionalOther > 0);
  CHECK(sim.stats.levels[0].sweeps == sweepsBefore + 1);
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    CHECK(sim.stats.levels[k].dirtySlots == 0);
    CHECK(sim.levelComplete(k));
  }
  check_invariants(sim);
}

TEST_CASE("T39 the raster rectangle and matrices put each slot texel at its viewport texel and depth, also 100 km out, for the target "
          "transform of a sweep and in a region turned by 90 deg, and the view is the orthonormal light view")
{
  for (const DPoint3 &camera : {DPoint3(10.3, 2.1, -7.7), DPoint3(100000.3, 2.1, -70000.7)})
  {
    RtssSchedulerSettings s;
    s.maxAnisotropy = 2;
    RtssSim sim(s);
    sim.camera = camera;
    sim.sun = sun_at(20, 30);
    sim.run(60);
    const int S = s.slotTexels, B = s.borderTexels, P = S - 2 * B;
    bool found[2][RTSS_MAX_LEVELS] = {}, swept = false;
    int badRows = 0, levels = 0;
    double worstXY = 0, worstZ = 0, worstTexel = 0, targetShift = 0;
    float sy = 0;
    sim.sched.invalidate();
    for (int f = 0; f < 120 && levels < 2 * s.levels; ++f)
    {
      if (levels == s.levels && !eastl::exchange(swept, true))
        sim.sun = turned(sim.sun, 2);
      sim.step();
      for (const RtssJob &j : sim.jobs)
      {
        const int k = sim.jobLevel(j), bit = j.flags & RTSS_JOB_BIT, target = bit != sim.curBit(k);
        if (found[target][k])
          continue;
        found[target][k] = true;
        ++levels;
        const RtssLightXform &x = sim.sched.xform(k, bit);
        targetShift = eastl::max(targetShift, target ? length(x.c) : 0.0);
        const IPoint2 o = sim.sched.windowOrigin(k);
        const double texel = sim.sched.texel(k), pw = sim.sched.pageWorld(k);
        TMatrix view;
        TMatrix4 proj, turnedProj;
        rtss_raster_matrices(sim.sched, k, bit, rtss_raster_rect(sim.sched, j), false, view, proj);
        rtss_raster_matrices(sim.sched, k, bit, rtss_raster_rect(sim.sched, j), true, view, turnedProj);
        Point4 rows[3];
        rtss_pack_xform(x, sim.camera, o, pw, rows);
        for (int c = 0; c < 3; ++c)
          badRows += view.m[c][0] != rows[0][c] || view.m[c][1] * x.sy != rows[1][c] || view.m[c][2] != rows[2][c];
        sy = x.sy;
        const double z = j.zStart + 0.3 * (j.zEnd - j.zStart);
        const double tolX = 1e-3 + fabs(view.m[3][0]) * FLT_EPSILON / texel;
        const double tolY = 1e-3 + fabs(view.m[3][1]) * sy * FLT_EPSILON / texel;
        const double tolZ = 1e-6 + fabs(view.m[3][2]) * FLT_EPSILON / (j.zEnd - j.zStart);
        for (const IPoint2 &t : {IPoint2(0, S - 1), IPoint2(S / 2, 0), IPoint2(S - 1, S / 2)})
        {
          const DPoint3 w = x.fromLs(DPoint3(((j.page.x - o.x) * P + t.x - B + 0.5) * texel + o.x * pw,
            ((j.page.y - o.y) * P + t.y - B + 0.5) * texel + o.y * pw, z + x.z0));
          const DPoint3 ls = x.toLs(w);
          const double tx = (ls.x - o.x * pw) / texel - ((j.page.x - o.x) * P - B);
          const double ty = (ls.y - o.y * pw) / texel - ((j.page.y - o.y) * P - B);
          const Point3 wf = point3(w); // constraint: the raster callback takes float positions
          const double posErr = 4 * eastl::max(eastl::max(fabs(w.x), fabs(w.y)), fabs(w.z)) * FLT_EPSILON;
          const double errX = tolX + posErr / texel, errY = tolY + posErr * sy / texel;
          float v[3], ndc[3], turnedNdc[3];
          for (int i = 0; i < 3; ++i)
            v[i] = view.m[0][i] * wf.x + view.m[1][i] * wf.y + view.m[2][i] * wf.z + view.m[3][i];
          for (int i = 0; i < 3; ++i)
          {
            ndc[i] = v[0] * proj.m[0][i] + v[1] * proj.m[1][i] + v[2] * proj.m[2][i] + proj.m[3][i];
            turnedNdc[i] = v[0] * turnedProj.m[0][i] + v[1] * turnedProj.m[1][i] + v[2] * turnedProj.m[2][i] + turnedProj.m[3][i];
          }
          worstTexel = eastl::max(worstTexel, fabs(tx - (t.x + 0.5)) + fabs(ty - (t.y + 0.5)));
          worstXY = eastl::max(worstXY, fabs((ndc[0] + 1) * 0.5 * S - tx) / errX);
          worstXY = eastl::max(worstXY, fabs((1 - ndc[1]) * 0.5 * S - (S - ty)) / errY);
          // a region turned by 90 deg reads slot texel t at t.yx, as rtss_raster_texel does
          worstXY = eastl::max(worstXY, fabs((turnedNdc[0] + 1) * 0.5 * S - ty) / errY);
          worstXY = eastl::max(worstXY, fabs((1 - turnedNdc[1]) * 0.5 * S - tx) / errX);
          worstZ = eastl::max(worstZ,
            eastl::max(fabs(ndc[2] - (ls.z - x.z0 - j.zStart) / (j.zEnd - j.zStart)), double(fabsf(turnedNdc[2] - ndc[2]))) /
              (tolZ + posErr / (j.zEnd - j.zStart)));
        }
      }
    }
    INFO("camera " << camera.x << ", " << camera.z << ": levels " << levels << ", sy " << sy << ", worst error / tolerance: texel "
                   << worstXY << ", depth " << worstZ << ", fromLs off the texel center " << worstTexel << ", target shift "
                   << targetShift);
    CHECK(levels == 2 * s.levels);
    CHECK(targetShift > 0);
    CHECK(sy == 2.f);
    CHECK(badRows == 0);
    CHECK(worstTexel < 0.5);
    CHECK(worstXY <= 1);
    CHECK(worstZ <= 1);
    check_invariants(sim);
  }
}

TEST_CASE("T40 RTSS_JOB_SAME_PAGE marks the DIRTY jobs and the depth rebuilds, not the new pages and the sweep jobs")
{
  RtssSim sim;
  sim.run(60);
  int wrong = 0, dirty = 0, dirtyFlagged = 0, teleport = 0, teleportFlagged = 0, swept = 0, sweptFlagged = 0, required = 0,
      depthFlagged = 0;
  auto check = [&]() {
    for (int i = 0; i < (int)sim.jobs.size(); ++i)
    {
      const RtssJob &j = sim.jobs[i];
      const RtssSlot &old = sim.prevSlots[sim.jobSlotIndex(j)];
      const bool same = RtssSim::holdsJob(old, j);
      wrong += same != ((sim.jobClass[i] & RTSS_JOB_SAME_PAGE) != 0);
    }
  };
  auto count = [&](int &jobs, int &flagged, auto pick) {
    for (int i = 0; i < (int)sim.jobs.size(); ++i)
      if (pick(sim.jobClass[i]))
      {
        ++jobs;
        flagged += (sim.jobClass[i] & RTSS_JOB_SAME_PAGE) != 0;
      }
  };
  auto ofClass = [](uint8_t cls) { return [cls](uint8_t jc) { return (jc & RTSS_JOB_CLASS_MASK) == cls; }; };

  const Point3 c = point3(sim.camera) + Point3(4.f, 0.5f, -3.f);
  const BBox3 box(c - Point3(10, 10, 10), c + Point3(10, 10, 10));
  sim.sched.invalidateBoxes(&box, 1);
  for (int f = 0; f < 60; ++f)
  {
    sim.step();
    check();
    count(dirty, dirtyFlagged, ofClass(RTSS_DIRTY));
  }

  sim.camera += DPoint3(1000, 0, 0);
  sim.coverageFrom = sim.frame + 5;
  sim.step();
  check();
  count(teleport, teleportFlagged, [](uint8_t) { return true; });
  sim.run(60);

  sim.sun = turned(sim.sun, 1);
  for (int f = 0; f < 120; ++f)
  {
    sim.step();
    check();
    count(swept, sweptFlagged, [](uint8_t jc) { return (jc & RTSS_JOB_SWEPT) != 0; });
  }

  const DPoint3 start = sim.camera, down = dpoint3(-sim.sun);
  for (int i = 1; i <= 120; ++i)
  {
    sim.camera = start + down * (i * 20.0);
    sim.step();
    check();
    count(required, depthFlagged, ofClass(RTSS_REQUIRED));
  }
  INFO("DIRTY jobs " << dirty << " (" << dirtyFlagged << " flagged), teleport jobs " << teleport << " (" << teleportFlagged
                     << "), sweep jobs " << swept << " (" << sweptFlagged << "), REQUIRED jobs of the move " << required << " ("
                     << depthFlagged << " flagged), flags that do not match the slot " << wrong);
  CHECK(wrong == 0);
  CHECK(dirty > 0);
  CHECK(dirtyFlagged == dirty);
  CHECK(teleport > 0);
  CHECK(teleportFlagged == 0);
  CHECK(swept > 0);
  CHECK(sweptFlagged == 0);
  CHECK(depthFlagged > 0);
  check_invariants(sim);
}

TEST_CASE("T41 rtss_raster_group_rect is the smallest box that holds the rectangle and the depth range of each job of the group")
{
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 64;
  s.maxAnisotropy = 2;
  RtssSim sim(s);
  sim.sun = sun_at(20, 30);
  sim.run(60);
  sim.sun = turned(sim.sun, 1);
  sim.sched.invalidate();
  int groups = 0, targetGroups = 0, outside = 0, notTight = 0, badRows = 0;
  for (int f = 0; f < 60 && (groups < 3 || !targetGroups); ++f)
  {
    sim.step();
    RtssRasterPlan rp;
    rtss_raster_plan(sim.jobs, ~0u, sim.sched, rp);
    for (const RtssRasterGroup &g : rp.cullGroups)
    {
      if (g.count < 2)
        continue;
      ++groups;
      targetGroups += g.bit != sim.curBit(g.level);
      TMatrix view;
      TMatrix4 proj;
      const RtssLightXform &x = sim.sched.xform(g.level, g.bit);
      const RtssRasterRect u = rtss_raster_group_rect(sim.sched, make_span_const(sim.jobs), rp, g);
      rtss_raster_matrices(sim.sched, g.level, g.bit, u, g.transposed, view, proj);
      for (int c = 0; c < 3; ++c)
        badRows += view.m[c][0] != sim.row(g.level, g.bit, 0)[c] || view.m[c][1] * x.sy != sim.row(g.level, g.bit, 1)[c] ||
                   view.m[c][2] != sim.row(g.level, g.bit, 2)[c];
      bool hit[6] = {};
      for (int i = g.first; i < g.first + g.count; ++i)
      {
        const RtssRasterRect r = rtss_raster_rect(sim.sched, sim.jobs[rp.order[i]]);
        outside += r.x0 < u.x0 || r.y0 < u.y0 || r.x1 > u.x1 || r.y1 > u.y1 || r.zStart < u.zStart || r.zEnd > u.zEnd;
        const bool eq[6] = {r.x0 == u.x0, r.y0 == u.y0, r.x1 == u.x1, r.y1 == u.y1, r.zStart == u.zStart, r.zEnd == u.zEnd};
        for (int k = 0; k < 6; ++k)
          hit[k] |= eq[k];
      }
      notTight += eastl::count(hit, hit + 6, false);
      const double texel = sim.sched.texel(g.level);
      const IPoint2 size = g.transposed ? IPoint2(g.size.y, g.size.x) : g.size;
      notTight += fabs((u.x1 - u.x0) / texel - size.x) > 1e-3 || fabs((u.y1 - u.y0) / texel - size.y) > 1e-3;
    }
  }
  INFO(groups << " groups of 2 jobs or more (" << targetGroups << " of a target bit)");
  CHECK(groups >= 3);
  CHECK(targetGroups > 0);
  CHECK(outside == 0);
  CHECK(notTight == 0);
  CHECK(badRows == 0);
  check_invariants(sim);
}

TEST_CASE("T43 the cull groups split each group into runs of one job or of a page box of at most 1, 2 or 4 pages for each job")
{
  const int maxBoxPages = GENERATE(1, 2, 4);
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 64;
  s.rasterBoxPagesPerJob = maxBoxPages;
  RtssSim sim(s);
  sim.run(60);
  sim.sun = turned(sim.sun, 1);
  sim.sched.invalidate();
  int frames = 0, split = 0, badRuns = 0, loose = 0, singles = 0, compact = 0;
  for (int f = 0; f < 60 && frames < 10; ++f)
  {
    sim.step();
    if (sim.jobs.empty())
      continue;
    ++frames;
    RtssRasterPlan rp;
    rtss_raster_plan(sim.jobs, ~0u, sim.sched, rp);
    int next = 0;
    int prevKey = -1;
    for (const RtssRasterGroup &c : rp.cullGroups)
    {
      const int key = c.level * 2 + c.bit;
      badRuns += c.first != next || c.count <= 0 || key < prevKey;
      split += key == prevKey;
      prevKey = key;
      next = c.first + c.count;
      IBBox2 box;
      for (int i = c.first; i < c.first + c.count; ++i)
      {
        const RtssJob &job = sim.jobs[rp.order[i]];
        badRuns += rtss_raster_job_key(job) != key;
        box += rtss_job_page(job);
      }
      const IPoint2 size = box.width() + IPoint2(1, 1);
      singles += c.count == 1;
      compact += c.count > 1;
      loose += c.count > 1 && size.x * size.y > maxBoxPages * c.count;
    }
    badRuns += next != (int)rp.order.size();
  }
  INFO("at most " << maxBoxPages << " pages for each job: " << frames << " frames, " << split << " groups split, " << compact
                  << " cull groups of 2 jobs or more, " << singles << " of 1");
  CHECK(frames >= 10);
  CHECK(badRuns == 0);
  CHECK(loose == 0);
  CHECK(compact > 0);
  CHECK(split > 0);
  check_invariants(sim);
}

TEST_CASE("T46 a whole-slot job spans the caster range of its transform, clipped to the world box")
{
  RtssSim sim;
  sim.run(30);
  const int S = sim.settings.slotTexels;
  const double reach = sim.settings.maxCasterReach;
  const BBox3 box(point3(sim.camera) - Point3(300, 40, 300), point3(sim.camera) + Point3(300, 60, 300));
  for (int withBox = 0; withBox < 2; ++withBox)
  {
    if (withBox)
      sim.setWorldBox(box);
    else
      sim.sched.invalidate();
    int whole = 0;
    double worst = 0;
    for (int f = 0; f < 10; ++f)
    {
      sim.step();
      for (const RtssJob &j : sim.jobs)
      {
        const int x0 = j.rect & 0xFF, y0 = (j.rect >> 8) & 0xFF, x1 = (j.rect >> 16) & 0xFF, y1 = j.rect >> 24;
        if ((x1 - x0) * (y1 - y0) != S * S)
          continue;
        const RtssLightXform &x = sim.sched.xform(sim.jobLevel(j), j.flags & RTSS_JOB_BIT);
        double lo = x.toLs(sim.camera).z - reach, hi = x.toLs(sim.camera).z + reach;
        if (withBox)
        {
          double zmin = DBL_MAX, zmax = -DBL_MAX;
          for (int i = 0; i < 8; ++i)
          {
            zmin = eastl::min(zmin, x.toLs(dpoint3(box.point(i))).z);
            zmax = eastl::max(zmax, x.toLs(dpoint3(box.point(i))).z);
          }
          lo = eastl::max(lo, zmin);
          hi = eastl::min(hi, zmax);
        }
        worst = eastl::max({worst, fabs(j.zStart - (lo - x.z0)), fabs(j.zEnd - (hi - x.z0))});
        ++whole;
      }
    }
    INFO((withBox ? "with" : "without") << " a world box: " << whole << " whole-slot jobs, worst depth error " << worst);
    CHECK(whole > 0);
    CHECK(worst < 1e-3);
  }
  check_invariants(sim);
}

TEST_CASE("T77 at a low maxCasterReach a whole-slot job of a sweeping level spans twice the extent of both transforms, or the reach "
          "where the reach is more (6.4)")
{
  RtssSchedulerSettings s;
  s.maxCasterReach = 50;
  RtssSim sim(s);
  sim.sun = sun_at(45, 30);
  sim.run(60);
  REQUIRE(sim.extent(s.levels - 1) > s.maxCasterReach);
  sim.sun = sun_at(45, 33);
  const double reach = s.maxCasterReach;
  int whole = 0, swept = 0;
  double worst = 0;
  for (int f = 0; f < 300; ++f)
  {
    sim.step();
    for (const RtssJob &j : sim.jobs)
    {
      if (j.rect != sim.wholeRect())
        continue;
      const int k = sim.jobLevel(j), bit = j.flags & RTSS_JOB_BIT, cur = sim.curBit(k);
      const RtssLightXform &x = sim.sched.xform(k, cur), &xj = sim.sched.xform(k, bit);
      const double e = sim.extent(k);
      const DPoint3 dt = dpoint3(sim.sched.xform(k, cur ^ 1).d);
      const double sweepZ = e * (fabs(dt * dpoint3(x.ex)) + fabs(dt * dpoint3(x.ey)) / x.sy + fabs(dt * dpoint3(x.d)));
      const bool sweeps = sim.sweeping(k) && bit != cur;
      const double rz = sweeps ? eastl::max(e, sweepZ) : e, hz = eastl::max(reach, 2 * rz), cz = xj.toLs(sim.camera).z;
      worst = eastl::max({worst, fabs(j.zStart - (cz - hz - xj.z0)), fabs(j.zEnd - (cz + hz - xj.z0))});
      ++whole;
      swept += sweeps && sweepZ > e && e > reach;
    }
  }
  INFO(whole << " whole-slot jobs, " << swept << " of a deep sweep with a range set by the step, worst depth error " << worst);
  CHECK(whole > 0);
  CHECK(swept > 0);
  CHECK(worst < 1e-3);
  check_invariants(sim);
}

TEST_CASE("T47 a new world box invalidates the slots, the same box again does nothing")
{
  RtssSim sim;
  const BBox3 box(point3(sim.camera) - Point3(1000, 1000, 1000), point3(sim.camera) + Point3(1000, 1000, 1000));
  sim.setWorldBox(box);
  sim.run(120);
  sim.setWorldBox(box);
  sim.step();
  CHECK(sim.jobs.empty());
  sim.setWorldBox(BBox3(box[0], box[1] + Point3(0, 100, 0)));
  sim.sched.getStats(sim.stats);
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    INFO("level " << k);
    CHECK(sim.stats.levels[k].validSlots == 0);
    CHECK(sim.stats.levels[k].dirtySlots > 0);
  }
  sim.step();
  CHECK(!sim.jobs.empty());
  sim.run(120);
  check_invariants(sim);
}

TEST_CASE("T48 the CPU lookup takes a slot only when its bit is the bit of the transform it tries, else the next level")
{
  RtssSim sim;
  sim.run(120);
  RtssCpuTables t = sim.tables();
  for (int k = 0; k < t.paramsI.x; ++k)
    REQUIRE(!sim.sweeping(k));
  const RtssLevel &l = sim.levelTab(0);
  const int bit = sim.curBit(0);
  const IPoint2 page(l.pageOrigin.x + int(floorf(l.xform[bit * 3].w * l.invPage)),
    l.pageOrigin.y + int(floorf(l.xform[bit * 3 + 1].w * l.invPage)));
  const float z = 0.5f * (sim.slot(0, page).minDepth + sim.slot(0, page).maxDepth) - l.xform[bit * 3 + 2].w;
  const Point3 pos = sim.row(0, bit, 2) * z;
  REQUIRE(rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 0.5f, 0) == 0);
  dag::Vector<RtssSlot> slots(t.slots.begin(), t.slots.end());
  for (RtssSlot &s : slots)
    s.flags ^= RTSS_SLOT_BIT;
  t.slots = make_span_const(slots);
  CHECK(rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 0.5f, 0) == -1);
  for (int i = 0; i < (int)slots.size(); ++i)
    slots[i].flags = (slots[i].flags ^ RTSS_SLOT_BIT) & (i < t.paramsI.y * t.paramsI.y ? ~RTSS_SLOT_VALID : ~0u);
  CHECK(rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 0.5f, 0) == 1);
  check_invariants(sim);
}

TEST_CASE("T54 only a None level makes sub-slot DIRTY jobs, and the raster plan gives tiles to the raster levels only")
{
  RtssSchedulerSettings s;
  s.recipes[0].raster = RtssRaster::All;
  s.recipes[1].raster = RtssRaster::Augment;
  RtssSim sim(s);
  sim.run(60);
  const Point3 c = point3(sim.camera) + Point3(4.f, 0.5f, -3.f);
  const BBox3 box(c - Point3(10, 10, 10), c + Point3(10, 10, 10));
  sim.sched.invalidateBoxes(&box, 1);
  int subSlot[3] = {}, tiles = 0, wrongTile = 0, wrongRays = 0;
  for (int i = 0; i < 60; ++i)
  {
    sim.step();
    RtssRasterPlan rp;
    rtss_raster_plan(sim.jobs, rtss_raster_levels(sim.sched), sim.sched, rp);
    int rays = 0;
    for (const RtssJob &j : sim.jobs)
    {
      const int level = sim.jobLevel(j);
      subSlot[eastl::min(level, 2)] += j.rect != sim.wholeRect();
      tiles += j.rasterTile != ~0u;
      wrongTile += (level < 2) != (j.rasterTile != ~0u);
      rays += sim.settings.recipes[level].raster == RtssRaster::All ? 0 : int(rtss_rect_area(j.rect));
    }
    wrongRays += rays != int(sim.stats.rays);
  }
  INFO("sub-slot jobs on levels 0, 1, 2+: " << subSlot[0] << ", " << subSlot[1] << ", " << subSlot[2] << "; tiles " << tiles);
  CHECK(subSlot[0] == 0);
  CHECK(subSlot[1] == 0);
  CHECK(subSlot[2] > 0);
  CHECK(tiles > 0);
  CHECK(wrongTile == 0);
  CHECK(wrongRays == 0);
  check_invariants(sim);
}

TEST_CASE("T55 a pageCost of 3 on level 0 keeps the budgets in cost units, and a teleport rebuilds level 0 at a third of "
          "updatePagesPerFrame pages for each frame")
{
  RtssSchedulerSettings s;
  s.recipes[0].pageCost = 3;
  RtssSim sim(s);
  sim.run(60);
  sim.cameraVel = Point3(6, 0, 0);
  sim.run(300);
  sim.cameraVel = Point3(0, 0, 0);
  sim.run(60);
  const int level0Frames = update_frames(sim.settings, cube_pages(sim, 0) * s.recipes[0].pageCost);
  sim.camera += DPoint3(1000, 0, 0);
  sim.coverageFrom = sim.frame + 8;
  sim.step();
  CHECK(sim.levelComplete(s.levels - 1));
  sim.run(level0Frames - 2);
  CHECK_FALSE(sim.levelComplete(0));
  sim.run(1);
  CHECK(sim.levelComplete(0));
  check_invariants(sim);
}

TEST_CASE("T57 a pageCost above updatePagesPerFrame still rebuilds the DIRTY slots of its level, one page for each frame")
{
  RtssSchedulerSettings s;
  s.recipes[0] = {RtssRaster::All, 100.f};
  RtssSim sim(s);
  sim.coverageFrom = 40;
  const int W = sim.windowPages();
  sim.run(W * W + update_frames(sim.settings, (s.levels - 1) * W * W));
  REQUIRE(sim.jobs.empty());
  const BBox3 box(point3(sim.camera), 20.f);
  sim.sched.invalidateBoxes(&box, 1, 1);
  sim.sched.getStats(sim.stats);
  const int dirty = sim.stats.levels[0].dirtySlots;
  REQUIRE(dirty > 0);
  sim.run(dirty + 30);
  sim.sched.getStats(sim.stats);
  CHECK(sim.stats.levels[0].dirtySlots == 0);
  check_invariants(sim);
}

TEST_CASE("T58 an updatePagesPerFrame of 0 builds the coarsest level only and no DIRTY page: the capped page cost stays positive")
{
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 0;
  RtssSim sim(s);
  sim.run(60);
  const int last = s.levels - 1;
  CHECK(sim.levelComplete(last));
  CHECK(sim.stats.levels[0].validSlots == 0);
  const BBox3 box(point3(sim.camera), 20.f);
  sim.sched.invalidateBoxes(&box, 1, 1 << last);
  sim.sched.getStats(sim.stats);
  const int dirty = sim.stats.levels[last].dirtySlots;
  REQUIRE(dirty > 0);
  sim.run(30);
  sim.sched.getStats(sim.stats);
  CHECK(sim.stats.levels[last].dirtySlots == dirty);
  check_invariants(sim);
}

TEST_CASE("T60 invalidateBoxes in a sweep clips a slot of the target bit with the target transform")
{
  RtssSim sim;
  sim.run(60);
  sim.sun = turned(sim.sun, 2);
  sim.run(8);
  REQUIRE(sim.sweeping(0));
  const int W = sim.windowPages(), newest = sim.newestBit(0);
  auto isTarget = [&](int i) {
    const RtssSlot &sl = sim.slots()[i];
    return (sl.flags & RTSS_SLOT_VALID) && rtss_slot_bit(sl.flags) == uint32_t(newest);
  };
  int swept = 0;
  while (swept < W * W && !isTarget(swept))
    ++swept;
  REQUIRE(swept < W * W);
  // 15 m sunward of the camera depth, the two transforms disagree by texels there
  const RtssLightXform &x = sim.sched.xform(0, newest);
  const IPoint2 page = rtss_unpack_page(sim.slots()[swept].page, sim.sched.windowOrigin(0));
  const double pw = sim.sched.pageWorld(0);
  const Point3 c = point3(x.fromLs(DPoint3((page.x + 0.5) * pw, (page.y + 0.5) * pw, x.toLs(sim.camera).z - 15)));
  const BBox3 box(c - Point3(1, 1, 1), c + Point3(1, 1, 1));
  dag::Vector<int> expected(W * W * 4);
  dag::Vector<uint8_t> target(W * W, 0);
  int moved = 0;
  for (int i = 0; i < W * W; ++i)
  {
    int *r = &expected[i * 4], other[4];
    if (!isTarget(i))
      continue;
    expected_dirty_rect(sim, i, box, r);
    expected_dirty_rect(sim, i, box, other, newest ^ 1);
    target[i] = r[0] < r[2] && r[1] < r[3];
    moved += target[i] && (abs(r[0] - other[0]) > 1 || abs(r[1] - other[1]) > 1);
  }
  REQUIRE(moved > 0);
  sim.sched.invalidateBoxes(&box, 1, 1);
  int checked = 0, worstEdge = 0;
  for (int f = 0; f < 60; ++f)
  {
    sim.step();
    for (int i = 0; i < (int)sim.jobs.size(); ++i)
    {
      const RtssJob &j = sim.jobs[i];
      const int slot = sim.jobSlotIndex(j);
      if ((sim.jobClass[i] & RTSS_JOB_CLASS_MASK) != RTSS_DIRTY || slot >= W * W || !target[slot])
        continue;
      ++checked;
      for (int e = 0; e < 4; ++e)
        worstEdge = eastl::max(worstEdge, abs(int((j.rect >> (e * 8)) & 0xff) - expected[slot * 4 + e]));
    }
  }
  INFO("target-bit slots with a moved rectangle " << moved << ", DIRTY jobs checked " << checked << ", worst edge " << worstEdge);
  CHECK(checked > 0);
  CHECK(worstEdge <= 1);
  check_invariants(sim);
}

TEST_CASE("T59 isReady over a box of many pages that reaches past the level 0 cube; false after a reset and with one page missing")
{
  RtssSim sim;
  sim.run(60);
  const BBox3 wide(point3(sim.camera), float(3 * sim.extent(0)));
  CHECK(sim.sched.isReady(wide));
  sim.reset();
  CHECK_FALSE(sim.sched.isReady(wide));
  int held = -1, built = 0;
  sim.beforeCommit = [&](dag::Vector<uint32_t> &skipped, dag::Vector<uint32_t> &) {
    for (int i = 0; i < (int)sim.jobs.size(); ++i)
      if (sim.jobLevel(sim.jobs[i]) == 0)
      {
        if (held < 0)
          held = sim.jobSlotIndex(sim.jobs[i]);
        if (sim.jobSlotIndex(sim.jobs[i]) == held)
          skipped.push_back(i);
        else
          ++built;
      }
  };
  sim.run(60);
  CHECK(built > 0);
  CHECK_FALSE(sim.sched.isReady(wide));
  sim.beforeCommit = nullptr;
  sim.run(60);
  CHECK(sim.sched.isReady(wide));
  check_invariants(sim, RTSS_I7_COVERAGE); // a reset empties the level cubes for its frames
}

TEST_CASE("T50 the lookup finds the pages 300 km out, past 32767 pages of level 0 from the origin")
{
  RtssSim sim;
  sim.camera = DPoint3(300000.3, 2.1, -250000.7);
  sim.run(120);
  REQUIRE(abs(sim.sched.windowOrigin(0).x) + abs(sim.sched.windowOrigin(0).y) > 2 * 32767);
  for (int k = 0; k < sim.settings.levels; ++k)
    CHECK(sim.levelComplete(k));
  check_invariants(sim);
  IPoint4 paramsI;
  Point4 params, origin;
  sim.sched.globals(paramsI, params, origin);
  const DPoint3 w = sim.camera + DPoint3(3.3, 1.2, -2.1), rel = w - DPoint3(origin.x, origin.y, origin.z);
  const RtssLightXform &x = sim.sched.xform(0, sim.curBit(0));
  const IPoint2 o = sim.sched.windowOrigin(0);
  const DPoint3 ls = x.toLs(w) - DPoint3(o.x * sim.sched.pageWorld(0), o.y * sim.sched.pageWorld(0), x.z0);
  for (int r = 0; r < 3; ++r)
  {
    const float4 &v = sim.levelTab(0).xform[sim.curBit(0) * 3 + r];
    CHECK(fabs(v.x * rel.x + v.y * rel.y + v.z * rel.z + v.w - ls[r]) < 1e-3 * sim.texel(0));
  }
}

TEST_CASE("T51 a sub-slot DIRTY job keeps the depth range of its slot, not the caster range of the camera")
{
  RtssSim sim;
  sim.run(60);
  sim.camera += dpoint3(sim.row(0, sim.curBit(0), 2)) * 20.0;
  sim.run(30);
  const Point3 c = point3(sim.camera) + Point3(4.f, 0.5f, -3.f);
  const BBox3 box(c - Point3(10, 10, 10), c + Point3(10, 10, 10));
  sim.sched.invalidateBoxes(&box, 1);
  int subSlot = 0, wrongRange = 0;
  for (int i = 0; i < 60; ++i)
  {
    sim.step();
    for (const RtssJob &j : sim.jobs)
    {
      const RtssSlot &old = sim.prevSlots[sim.jobSlotIndex(j)];
      if (j.rect == sim.wholeRect() || !(old.flags & RTSS_SLOT_VALID))
        continue;
      ++subSlot;
      wrongRange += j.zStart != old.minDepth || j.zEnd != old.maxDepth;
    }
  }
  INFO(subSlot << " sub-slot jobs, " << wrongRange << " with another range");
  CHECK(subSlot > 0);
  CHECK(wrongRange == 0);
  check_invariants(sim);
}

TEST_CASE("T52 a camera past the sunward face of a world box taller than the caster reach plans no depth rebuild and stays ready")
{
  RtssSim sim;
  sim.sun = sun_at(10, 30); // a low sun: the climb past the face stays under the box top
  const float tall = 1.5f * sim.settings.maxCasterReach;
  const double past = tall + 200 * sqrt(3.0) + sim.extent(sim.settings.levels - 1);
  sim.setWorldBox(BBox3(point3(sim.camera) - Point3(1000, 1000, 1000),
    point3(sim.camera) + sim.sun * tall + Point3(100, sim.sun.y * float(past - tall) + 100, 100)));
  sim.run(60);
  sim.strictCoverage = false;
  sim.camera += dpoint3(sim.sun) * past;
  double required = 0;
  for (int f = 0; f < 10; ++f)
  {
    sim.step();
    required += sim.stats.pages[RTSS_REQUIRED];
  }
  CHECK(required == 0);
  CHECK(sim.sched.isReady(BBox3(point3(sim.camera), 1.f)));
  check_invariants(sim);
}

TEST_CASE("T53 isReady is false in a fast sweep; a camera past the world box after an invalidation gets empty caster ranges")
{
  RtssSim sim;
  sim.run(60);
  const BBox3 near(point3(sim.camera), 1.f);
  large_step(sim, 7.5, 1);
  CHECK(sim.stats.levels[0].fastSweep);
  CHECK_FALSE(sim.sched.isReady(near));
  sim.run(60);
  sim.strictCoverage = true;
  CHECK(sim.sched.isReady(near));
  sim.setWorldBox(BBox3(point3(sim.camera) - Point3(1000, 1000, 1000), point3(sim.camera) + Point3(1000, 1000, 1000)));
  sim.run(60);
  check_invariants(sim);
  sim.sched.invalidate();
  const DPoint3 away(-sim.sun.x, 0, -sim.sun.z); // level: under the box the window keeps to its bottom
  sim.camera += away * ((sim.settings.maxCasterReach + 2000) / (away * away));
  sim.step();
  REQUIRE(!sim.jobs.empty());
  int castingJobs = 0;
  for (const RtssJob &j : sim.jobs)
    castingJobs += j.zEnd != j.zStart;
  CHECK(castingJobs == 0);
  sim.run(60);
  check_invariants(sim, RTSS_I7_COVERAGE); // no receiver lives outside the world box
}

TEST_CASE("T61 a raster change between plan() and commit() keeps its level DIRTY")
{
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 100000;
  s.recipes[1].raster = RtssRaster::All;
  RtssSim sim(s);
  sim.run(60);
  sim.sched.invalidate();
  sim.settings.recipes[1].raster = RtssRaster::None;
  sim.beforeCommit = [&](dag::Vector<uint32_t> &, dag::Vector<uint32_t> &) { CHECK(sim.sched.setRecipes(sim.settings)); };
  sim.step();
  sim.sched.getStats(sim.stats);
  CHECK(sim.stats.levels[1].validSlots == 0);
  CHECK(sim.stats.levels[1].dirtySlots > 0);
  check_invariants(sim);
}

TEST_CASE("T63 a fast sweep whose edge pages cost more than a whole burst still finishes (W 32, cost 3, burst 8)")
{
  RtssSchedulerSettings s;
  s.windowPages = 32;
  s.validityBurstPagesPerFrame = 8;
  for (RtssLevelRecipe &r : s.recipes)
    r.pageCost = 3;
  RtssSim sim(s);
  sim.run(3000);
  const LargeStep r = large_step(sim, 7.5, 2500);
  INFO("sweep frames " << r.sweepFrames);
  CHECK(r.allFast);
  for (int k = 0; k < sim.settings.levels; ++k)
    CHECK_FALSE(sim.sweeping(k));
  check_invariants(sim, RTSS_I7_COVERAGE); // a burst of 8 cannot keep the receiver cubes covered in a fast sweep
}

TEST_CASE("T62 a page more than W pages out of the window is emptied, so no old slot passes 65536 pages away; one W out stays")
{
  {
    RtssSim sim;
    sim.run(60);
    sim.coverageFrom = INT_MAX;
    sim.beforeCommit = [&](dag::Vector<uint32_t> &skipped, dag::Vector<uint32_t> &) {
      for (uint32_t i = 0; i < sim.jobs.size(); ++i)
        skipped.push_back(i);
    };
    const int W = sim.windowPages(), x0 = sim.sched.windowOrigin(0).x;
    const double pw = sim.sched.pageWorld(0);
    const DPoint3 ex = dpoint3(sim.sched.xform(0, sim.curBit(0)).ex);
    sim.camera += ex * ((x0 + W / 2 + 0.5) * pw - sim.sched.xform(0, sim.curBit(0)).toLs(sim.camera).x);
    sim.step();
    REQUIRE(sim.sched.windowOrigin(0).x == x0);
    sim.camera += ex * (W * pw);
    sim.step();
    REQUIRE(sim.sched.windowOrigin(0).x == x0 + W);
    sim.sched.getStats(sim.stats);
    CHECK(sim.stats.levels[0].validSlots == W * W);
    check_invariants(sim, RTSS_I7_COVERAGE);
  }
  for (const bool jump : {true, false})
  {
    INFO("jump " << jump);
    RtssSim sim;
    sim.run(60);
    sim.coverageFrom = INT_MAX;
    sim.beforeCommit = [&](dag::Vector<uint32_t> &skipped, dag::Vector<uint32_t> &) {
      for (uint32_t i = 0; i < sim.jobs.size(); ++i)
        skipped.push_back(i);
    };
    const double step = jump ? 65536.0 : sim.windowPages() - 1;
    for (double moved = 0; moved < 65536.0; moved += step)
    {
      sim.camera += dpoint3(sim.sched.xform(0, 0).ex) * (step * sim.sched.pageWorld(0));
      sim.step();
    }
    sim.sched.getStats(sim.stats);
    for (int k = 0; k < sim.settings.levels; ++k)
    {
      INFO("level " << k);
      CHECK(sim.stats.levels[k].validSlots == 0);
      CHECK(sim.stats.levels[k].dirtySlots == 0);
    }
    int published = 0;
    for (const RtssSlot &e : sim.sched.slotTable())
      published += (e.flags & RTSS_SLOT_VALID) != 0;
    CHECK(published == 0);
    check_invariants(sim);
  }
}

TEST_CASE("T65 an invalidate(), invalidateBoxes() or world box change between plan() and commit() keeps every level DIRTY, a reset() "
          "keeps every slot EMPTY")
{
  const int how = GENERATE(0, 1, 2, 3);
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 100000;
  RtssSim sim(s);
  sim.run(60);
  sim.sched.invalidate();
  const BBox3 all(point3(sim.camera), 1e6f), box(point3(sim.camera), 500.f);
  sim.beforeCommit = [&](dag::Vector<uint32_t> &, dag::Vector<uint32_t> &) {
    if (how == 3)
      sim.sched.reset();
    else if (how == 2)
      sim.setWorldBox(box);
    else if (how)
      sim.sched.invalidateBoxes(&all, 1);
    else
      sim.sched.invalidate();
  };
  sim.step();
  sim.sched.getStats(sim.stats);
  const int W = sim.windowPages();
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    INFO("level " << k);
    CHECK(sim.stats.levels[k].validSlots == 0);
    if (how == 3)
      CHECK(sim.stats.levels[k].emptySlots == W * W);
    else
      CHECK(sim.stats.levels[k].dirtySlots > 0);
  }
  check_invariants(sim, how == 3 ? RTSS_I7_COVERAGE : -1);
}

TEST_CASE("T66 a REQUIRED depth rebuild whose pageCost is above the burst still runs (cost 3, burst 2)")
{
  RtssSchedulerSettings s;
  s.validityBurstPagesPerFrame = 2;
  for (RtssLevelRecipe &r : s.recipes)
    r.pageCost = 3;
  RtssSim sim(s);
  sim.run(600);
  REQUIRE(sim.sched.isReady(BBox3(point3(sim.camera), 1.f)));
  sim.camera += dpoint3(-sim.sun) * double(sim.settings.maxCasterReach);
  sim.step();
  CHECK(!sim.sched.isReady(BBox3(point3(sim.camera), 1.f)));
  sim.run(2000);
  CHECK(sim.sched.isReady(BBox3(point3(sim.camera), 1.f)));
  check_invariants(sim, RTSS_I7_COVERAGE); // a burst of 2 cannot keep the receiver cubes covered
}

TEST_CASE("T67 a reset in a sweep makes the first transform again, anchored at the world origin, and leaves no sweep head")
{
  RtssSim sim;
  sim.run(60);
  sim.sun = turned(sim.sun, 1);
  sim.step();
  run_to_idle(sim, 600);
  REQUIRE(length(sim.sched.xform(0, sim.curBit(0)).c) > 0);
  sim.sun = turned(sim.sun, 1);
  sim.step();
  REQUIRE(sim.stats.levels[0].headSlot >= 0);
  sim.reset();
  sim.sched.getStats(sim.stats);
  for (const RtssSchedulerLevelStats &l : sim.stats.levels)
    CHECK(l.headSlot == -1);
  sim.step();
  CHECK(length(sim.sched.xform(0, sim.curBit(0)).c) == 0);
  check_invariants(sim, RTSS_I7_COVERAGE); // the reset empties every slot
}

TEST_CASE("T68 the selection finds a held page of the coarsest level while level 0 has a new anisotropic scale, past the finer cubes "
          "on the coarsest level itself, and the coarsest seam fades in its own sy")
{
  RtssSchedulerSettings s;
  s.maxAnisotropy = 2;
  RtssSim sim(s);
  sim.sun = sun_at(38, 30);
  sim.run(60);
  sim.sunAngVel = normalize(Point3(0, 1, 0) % sim.sun) * float(DAY24_RATE / 4);
  sim.strictCoverage = false;
  const int last = s.levels - 1;
  int mixedFrames = 0, held = 0, outerHeld = 0, outerFiner = 0, coarserSy = 0, pastHeld = 0;
  float worstFade = 0;
  for (int f = 0; f < 6000; ++f)
  {
    if (f == 3000)
      sim.sunAngVel = -sim.sunAngVel; // a rising sun gives level 0 the smaller sy first
    sim.step();
    const float sy0 = sim.sched.xform(0, sim.curBit(0)).sy, syLast = sim.sched.xform(last, sim.curBit(last)).sy;
    if (sy0 == syLast)
      continue;
    ++mixedFrames;
    const RtssCpuTables t = sim.tables();
    const int newest = sim.newestBit(last);
    float fade = -1.f;
    rtss_cpu_lookup(t, sim.cubePoint(last, newest, Point3(0.1f, 0.97f, 0)), Point3(0, 0, 0), 0.f, 0.5f, -1, &fade);
    worstFade = eastl::max(worstFade, fabsf(fade - (1 - 0.03f / s.seamBand)));
    coarserSy += sim.sched.xform(last, newest).sy > sy0;
    for (int i = 0; i < 64; ++i)
    {
      Point3 p(_rnd_float(sim.seed, -0.9f, 0.9f), _rnd_float(sim.seed, -0.9f, 0.9f), _rnd_float(sim.seed, -0.2f, 0.2f));
      const bool outer = i & 1;
      if (outer)
        p.x = p.x < 0 ? p.x / 6 - 0.75f : p.x / 6 + 0.75f; // past the cube of level last - 1
      const Point3 pos = sim.cubePoint(last, sim.curBit(last), p);
      if (rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 0.5f, last) != last)
        continue;
      ++held;
      const int level = rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 0.5f);
      int finest = last;
      for (int k = 0; k < last; ++k)
      {
        const RtssLightXform &xk = sim.sched.xform(k, sim.curBit(k));
        const DPoint3 d = xk.toLs(dpoint3(pos)) - xk.toLs(DPoint3(0, 0, 0));
        if (eastl::max({fabs(d.x), fabs(d.y), fabs(d.z)}) <= 0.95 * sim.extent(k))
        {
          finest = k;
          break;
        }
      }
      if (rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 1.f, finest) == finest)
        pastHeld += rtss_cpu_lookup(t, pos, Point3(0, 0, 0), 0.f, 1.f) > finest;
      outerHeld += outer;
      outerFiner += outer && level != last;
    }
  }
  INFO("frames with two scales " << mixedFrames << ", held coarsest points " << held << ", past a level that holds " << pastHeld
                                 << ", outside the finer cubes " << outerHeld << ", of them a finer level " << outerFiner
                                 << ", frames with a larger coarsest sy " << coarserSy << ", worst seam fade error " << worstFade);
  CHECK(mixedFrames > 0);
  CHECK(held > 0);
  CHECK(pastHeld == 0);
  CHECK(outerHeld > 0);
  CHECK(outerFiner == 0);
  CHECK(coarserSy > 0);
  CHECK(worstFade < 0.02f);
  check_invariants(sim);
}

TEST_CASE("T71 with batchFrames 4 a slow sun builds the same pages in about a quarter of the frames, a batch within 4 budgets")
{
  double pages[2] = {}, worst[2] = {};
  int work[2] = {};
  RtssSchedulerSettings s;
  for (int b = 0; b < 2; ++b)
  {
    s.batchFrames = b ? 4 : 1;
    RtssSim sim(s);
    check_day_sun_error(sim, DAY24_RATE / 6, [&]() {
      const double p = sim.otherPages() + sim.stats.pages[RTSS_REQUIRED];
      pages[b] += p;
      work[b] += p > 0;
      worst[b] = eastl::max(worst[b], sim.stats.pages[RTSS_SWEEP]);
    });
    check_invariants(sim);
  }
  INFO("pages " << pages[0] << " in " << work[0] << " frames, batched " << pages[1] << " in " << work[1] << ", largest sweep batch "
                << worst[1] << " against " << worst[0]);
  CHECK(fabs(pages[1] - pages[0]) < 0.15 * pages[0]);
  CHECK(work[1] * 2.5 <= work[0]);
  CHECK(worst[1] > worst[0]);
}

TEST_CASE("T72 with batchFrames 4 a sun faster than the budget builds the same pages in a quarter of the frames, in debt between them")
{
  double pages[2] = {}, worst[2] = {}, err[2] = {};
  int work[2] = {};
  RtssSchedulerSettings s;
  for (int b = 0; b < 2; ++b)
  {
    s.batchFrames = b ? 4 : 1;
    RtssSim sim(s);
    sim.sun = sun_at(45, 30);
    sim.sunAngVel = day_axis(40) * float(DAY24_RATE * 2);
    for (int i = 0; i < 1800; ++i)
    {
      sim.step();
      if (sim.frame <= 600)
        continue;
      const double p = sim.otherPages() + sim.stats.pages[RTSS_REQUIRED];
      pages[b] += p;
      work[b] += p > 0;
      worst[b] = eastl::max(worst[b], sim.otherPages());
      for (int k = 0; k < s.levels; ++k)
        err[b] = eastl::max(err[b], double(sim.stats.levels[k].sunErrorDeg / sim.stats.levels[k].toleranceDeg));
    }
    check_invariants(sim);
  }
  INFO("pages " << pages[0] << " in " << work[0] << " frames, batched " << pages[1] << " in " << work[1] << ", largest batch "
                << worst[1] << ", worst sun error / tolerance " << err[0] << " batched " << err[1]);
  CHECK(fabs(pages[1] - pages[0]) < 0.1 * pages[0]);
  CHECK(work[1] * 3 <= work[0]);
  CHECK(worst[1] > 2 * worst[0]);
  CHECK(err[1] <= 1.1 * err[0]);
}

TEST_CASE("T73 a reset clears the update debt, so the frame after it builds the finer levels")
{
  RtssSchedulerSettings s;
  s.batchFrames = 4;
  RtssSim sim(s);
  sim.sun = sun_at(45, 30);
  sim.sunAngVel = day_axis(40) * float(DAY24_RATE * 2);
  for (int i = 0; i < 600; ++i)
    sim.step();
  for (int phase = 0; phase < s.batchFrames; ++phase)
  {
    sim.step();
    sim.reset();
    sim.step();
    INFO("phase " << phase << ": REQUIRED pages after the reset " << sim.stats.pages[RTSS_REQUIRED]);
    CHECK(sim.stats.pages[RTSS_REQUIRED] > s.validityBurstPagesPerFrame);
  }
  check_invariants(sim);
}

TEST_CASE("T74 with batchFrames 4 a sweep page is built at most batchFrames + 1 frames ahead of its deadline: the horizon of "
          "batchFrames - 1, the frame of the sweep start and the frame of its commit")
{
  RtssSchedulerSettings s;
  s.batchFrames = 4;
  RtssSim sim(s);
  sim.sun = sun_at(45, 30);
  sim.sunAngVel = day_axis(40) * float(DAY24_RATE / 6);
  int start[RTSS_MAX_LEVELS] = {}, sweeps[RTSS_MAX_LEVELS] = {};
  double lead = 0;
  for (int i = 0; i < 3600; ++i)
  {
    sim.step();
    for (int k = 0; k < s.levels; ++k)
    {
      const RtssSchedulerLevelStats &l = sim.stats.levels[k];
      if (l.sweeps != sweeps[k])
        sweeps[k] = l.sweeps, start[k] = sim.frame;
      if (!l.sweeping || l.fastSweep || sim.frame <= 600)
        continue;
      lead = eastl::max(lead, l.sweepProgress * l.sweepSeconds / sim.dt - (sim.frame - 1 - start[k]));
    }
  }
  check_invariants(sim);
  INFO("worst lead of a sweep page over its deadline " << lead << " frames");
  CHECK(lead > 0);
  CHECK(lead <= s.batchFrames + 1);
}

TEST_CASE("T75 a budget cut forgives the debt beyond the new batch, so the frames without a build after it stay within that batch")
{
  RtssSchedulerSettings s;
  s.batchFrames = 4;
  RtssSim sim(s);
  sim.sun = sun_at(45, 30);
  sim.sunAngVel = day_axis(40) * float(DAY24_RATE * 2);
  for (int i = 0; i < 600; ++i)
    sim.step();
  for (int i = 0; sim.updateCost <= 2 * s.updatePagesPerFrame; ++i)
  {
    REQUIRE(i < 100);
    sim.step();
  }
  sim.settings.updatePagesPerFrame /= 4;
  sim.sched.setBudget(sim.settings);
  int idle = 0;
  do
    sim.step();
  while (sim.updateCost == 0 && ++idle < 20);
  INFO("frames without an update-budget build after the cut " << idle);
  CHECK(idle > 0);
  CHECK(idle <= s.batchFrames + 1);
  check_invariants(sim);
}

TEST_CASE(
  "T76 with batchFrames 4 and a budget of 3 a 6 m/s walk pre-traces the same pages in a third of the frames with jobs or fewer")
{
  double pages[2] = {};
  int work[2] = {};
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 3;
  for (int b = 0; b < 2; ++b)
  {
    s.batchFrames = b ? 4 : 1;
    RtssSim sim(s);
    sim.run(5);
    const Point3 dir = sim.row(0, sim.curBit(0), 0);
    for (int i = 0; i < 1500; ++i)
    {
      sim.cameraVel = dir * 6.f;
      sim.step();
      if (i < 300)
        continue;
      pages[b] += sim.stats.pages[RTSS_PRETRACE];
      work[b] += sim.stats.pages[RTSS_PRETRACE] > 0;
    }
    check_invariants(sim);
  }
  INFO("pre-trace pages " << pages[0] << " in " << work[0] << " frames, batched " << pages[1] << " in " << work[1]);
  CHECK(fabs(pages[1] - pages[0]) < 0.15 * pages[0]);
  CHECK(work[1] * 3 <= work[0]);
}

TEST_CASE(
  "T78 a 7.5 deg step during a sun faster than the budget sweeps fast in debt: batchFrames 4 takes the frames of 1 and a batch")
{
  int frames[2] = {};
  RtssSchedulerSettings s;
  for (int b = 0; b < 2; ++b)
  {
    s.batchFrames = b ? 4 : 1;
    RtssSim sim(s);
    sim.sun = sun_at(45, 30);
    sim.sunAngVel = day_axis(40) * float(DAY24_RATE * 2);
    sim.run(600);
    sim.sunAngVel = Point3(0, 0, 0);
    const LargeStep r = large_step(sim, 7.5, 200);
    INFO("batchFrames " << s.batchFrames << ": sweep frames " << r.sweepFrames << ", far misses " << r.farMisses
                        << ", after the sweep " << r.lateMisses);
    CHECK(r.allFast);
    for (int k = 0; k < s.levels; ++k)
      CHECK_FALSE(sim.sweeping(k));
    CHECK(r.farMisses == 0);
    CHECK(r.lateMisses == 0);
    check_invariants(sim);
    frames[b] = r.sweepFrames;
  }
  INFO("sweep frames " << frames[0] << ", batched " << frames[1]);
  CHECK(frames[1] <= frames[0] + s.batchFrames);
}

TEST_CASE("T79 above the world box the window keeps to its top: a climb plans no jobs, and the lookup still finds every cube")
{
  int jobs[2] = {};
  for (int withBox = 0; withBox < 2; ++withBox)
  {
    RtssSim sim;
    const BBox3 box(point3(sim.camera) - Point3(1000, 50, 1000), point3(sim.camera) + Point3(1000, 20, 1000));
    if (withBox)
      sim.setWorldBox(box);
    sim.camera.y = box[1].y + 30;
    sim.run(120);
    sim.cameraVel = Point3(0, 10, 0);
    for (int i = 0; i < 300; ++i)
    {
      sim.step();
      jobs[withBox] += sim.jobs.size();
    }
    if (withBox)
      CHECK(fabs(sim.sched.cameraPos().y - box[1].y) < 1e-3);
    check_invariants(sim);
  }
  INFO("jobs of a 50 m climb above the box: " << jobs[0] << " without a world box, " << jobs[1] << " with it");
  CHECK(jobs[0] > 0);
  CHECK(jobs[1] == 0);
}

TEST_CASE("T80 a raster group with an empty depth range has finite matrices, for a cull ahead")
{
  RtssSim sim;
  sim.setWorldBox(BBox3(point3(sim.camera) - Point3(1000, 1000, 1000), point3(sim.camera) + Point3(1000, 1000, 1000)));
  const DPoint3 away(-sim.sun.x, 0, -sim.sun.z);
  sim.camera += away * ((sim.settings.maxCasterReach + 2000) / (away * away));
  sim.step();
  RtssRasterPlan rp;
  rtss_raster_plan(sim.jobs, ~0u, sim.sched, rp);
  int empty = 0, notFinite = 0;
  for (const RtssRasterGroup &g : rp.cullGroups)
  {
    const RtssRasterRect r = rtss_raster_group_rect(sim.sched, make_span_const(sim.jobs), rp, g);
    empty += r.zEnd == r.zStart;
    TMatrix view;
    TMatrix4 proj;
    rtss_raster_matrices(sim.sched, g.level, g.bit, r, g.transposed, view, proj);
    const TMatrix4 globtm = TMatrix4(view) * proj;
    for (int i = 0; i < 16; ++i)
      notFinite += !isfinite((&globtm._11)[i]);
  }
  INFO(rp.cullGroups.size() << " groups, " << empty << " with an empty depth range");
  CHECK(empty > 0);
  CHECK(notFinite == 0);
  check_invariants(sim);
}

TEST_CASE("T81 isReady with a level mask: after a teleport the coarsest level is ready before the finer ones")
{
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 2;
  RtssSim sim(s);
  sim.run(60);
  sim.camera += DPoint3(3000, 0, 1700);
  const uint32_t coarsest = 1u << (sim.settings.levels - 1);
  int coarsestFrame = -1, allFrame = -1;
  for (int f = 0; f < 2000 && allFrame < 0; ++f)
  {
    sim.step();
    const BBox3 box(point3(sim.camera) - Point3(2, 2, 2), point3(sim.camera) + Point3(2, 2, 2));
    if (coarsestFrame < 0 && sim.sched.isReady(box, coarsest))
      coarsestFrame = f;
    if (sim.sched.isReady(box))
      allFrame = f;
  }
  INFO("the coarsest level is ready in frame " << coarsestFrame << ", all levels in frame " << allFrame);
  CHECK(coarsestFrame >= 0);
  CHECK(allFrame > coarsestFrame);
  check_invariants(sim);
}

TEST_CASE("T82 a raster call cost makes a walk build more pages for each call, and the plan pays each call that the raster plan draws")
{
  double pagesPerCall[2] = {};
  for (int priced = 0; priced < 2; ++priced)
  {
    RtssSchedulerSettings s;
    for (int k = 0; k < s.levels; ++k)
      s.recipes[k].raster = RtssRaster::All;
    s.updatePagesPerFrame = 12;
    s.rasterCallCost = priced ? 4.f : 0.f;
    RtssSim sim(s);
    sim.run(600);
    sim.cameraVel = Point3(6, 0, 0.7f);
    int calls = 0, pages = 0, unpaid = 0;
    for (int i = 0; i < 600; ++i)
    {
      sim.step();
      RtssRasterPlan rp;
      rtss_raster_plan(sim.jobs, rtss_raster_levels(sim.sched), sim.sched, rp);
      calls += rp.cullGroups.size();
      pages += sim.jobs.size();
      int paid = 0;
      for (uint8_t c : sim.jobClass)
        paid += (c & RTSS_JOB_CALL) != 0;
      unpaid += priced && paid != (int)rp.cullGroups.size();
    }
    pagesPerCall[priced] = double(pages) / eastl::max(calls, 1);
    INFO("call cost " << s.rasterCallCost << ": " << pages << " pages in " << calls << " calls, " << unpaid << " frames not paid");
    CHECK(unpaid == 0);
    check_invariants(sim);
  }
  INFO("pages for each call: " << pagesPerCall[0] << " with no call cost, " << pagesPerCall[1] << " with it");
  CHECK(pagesPerCall[1] >= 2 * pagesPerCall[0]);
}

TEST_CASE("T83 an idle scheduler skips the plans of unchanged inputs, and plans again after each change")
{
  RtssSchedulerSettings s;
  s.recipes[0].raster = RtssRaster::All;
  RtssSim sim(s);
  sim.run(600);
  REQUIRE(sim.jobs.empty());
  auto plansAfter = [&](const char *name, const eastl::function<void()> &change) {
    sim.run(600);
    INFO(name);
    REQUIRE(sim.jobs.empty());
    change();
    sim.step();
    bool sweeps = false;
    for (int k = 0; k < sim.settings.levels; ++k)
      sweeps |= sim.sweeping(k);
    CHECK((!sim.jobs.empty() || sweeps));
  };
  const BBox3 around(point3(sim.camera) - Point3(3, 3, 3), point3(sim.camera) + Point3(3, 3, 3));
  plansAfter("invalidate", [&]() { sim.sched.invalidate(); });
  plansAfter("invalidateBoxes", [&]() { sim.sched.invalidateBoxes(&around, 1); });
  plansAfter("setRecipes", [&]() {
    RtssSchedulerRecipes r = sim.settings;
    r.recipes[1].raster = RtssRaster::All;
    sim.sched.setRecipes(r);
  });
  plansAfter("setWorldBox", [&]() { sim.setWorldBox(BBox3(around[0] - Point3(500, 50, 500), around[1] + Point3(500, 50, 500))); });
  plansAfter("reset", [&]() { sim.reset(); });
  plansAfter("sun", [&]() { sim.sun = turned(sim.sun, 2); });
  plansAfter("camera", [&]() { sim.camera += DPoint3(3 * sim.sched.pageWorld(0), 0, 0); });
  check_invariants(sim);
}

TEST_CASE("T84 idle frames accrue the update budget, so the first change after them builds at once")
{
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 3;
  s.batchFrames = 4;
  RtssSim sim(s);
  sim.run(600);
  const Point3 c = point3(sim.camera);
  const BBox3 small(c - Point3(1, 1, 1), c + Point3(1, 1, 1));
  int late = 0;
  for (float size = 8; size <= 512; size *= 2)
  {
    const BBox3 big(c - Point3(size, 3, size), c + Point3(size, 3, size));
    sim.sched.invalidateBoxes(&big, 1);
    sim.run(600);
    REQUIRE(sim.jobs.empty());
    sim.sched.invalidateBoxes(&small, 1);
    sim.step();
    late += sim.jobs.empty();
  }
  CHECK(late == 0);
  check_invariants(sim);
}

TEST_CASE("T85 a setBudget() or setSun() between plan() and commit() keeps the plan, so no slot of it stays DIRTY")
{
  const bool sun = GENERATE(false, true);
  RtssSchedulerSettings s;
  s.updatePagesPerFrame = 100000;
  RtssSim sim(s);
  sim.run(60);
  sim.sched.invalidate();
  sim.beforeCommit = [&](dag::Vector<uint32_t> &, dag::Vector<uint32_t> &) {
    if (sun)
      sim.sched.setSun(turned(sim.sun, 2), sim.sunAngVel);
    else
    {
      sim.settings.staticSweepSeconds = 1;
      sim.sched.setBudget(sim.settings);
    }
  };
  sim.step();
  REQUIRE(!sim.jobs.empty());
  sim.sched.getStats(sim.stats);
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    INFO("level " << k);
    CHECK(sim.stats.levels[k].dirtySlots == 0);
  }
  check_invariants(sim);
}

TEST_CASE("T86 a sweep page joins a raster call that the frame paid, so a fast sweep at a call cost of 7 and budgets of 14 builds "
          "several pages of a level in one call")
{
  RtssSchedulerSettings s;
  s.windowPages = 8;
  s.slotTexels = 128;
  s.borderTexels = 3;
  for (int k = 0; k < s.levels; ++k)
    s.recipes[k] = {RtssRaster::All, 1.f};
  s.rasterCallCost = 7;
  s.updatePagesPerFrame = s.validityBurstPagesPerFrame = 14;
  s.batchFrames = 1;
  RtssSim sim(s);
  sim.coverageFrom = 60;
  sim.run(600);
  REQUIRE(sim.jobs.empty());
  sim.sun = rtss_rotate(sim.sun, sim.row(0, sim.curBit(0), 1), 6 * DEG);
  sim.strictCoverage = false;
  sim.step();
  int finerPages = 0;
  for (int k = 0; k < s.levels; ++k)
  {
    INFO("level " << k);
    REQUIRE(sim.sweeping(k));
    REQUIRE(sim.stats.levels[k].fastSweep);
    finerPages += k + 1 < s.levels ? sim.stats.levels[k].sweepPages : 0;
  }
  const int onePagePerBudget = eastl::max(finerPages, sim.stats.levels[s.levels - 1].sweepPages);
  int frames = 0, joined = 0, unpaid = 0;
  for (bool sweeping = true; sweeping && frames < 2 * onePagePerBudget;)
  {
    ++frames;
    int swept[RTSS_MAX_LEVELS] = {}, sweptCalls[RTSS_MAX_LEVELS] = {}, paid = 0;
    for (int i = 0; i < (int)sim.jobs.size(); ++i)
    {
      const int k = sim.jobLevel(sim.jobs[i]);
      const bool call = sim.jobClass[i] & RTSS_JOB_CALL;
      paid += call;
      swept[k] += (sim.jobClass[i] & RTSS_JOB_SWEPT) != 0;
      sweptCalls[k] += (sim.jobClass[i] & RTSS_JOB_SWEPT) && call;
    }
    for (int k = 0; k < s.levels; ++k)
      joined += swept[k] > 1 && swept[k] > sweptCalls[k];
    RtssRasterPlan rp;
    rtss_raster_plan(sim.jobs, rtss_raster_levels(sim.sched), sim.sched, rp);
    unpaid += paid != (int)rp.cullGroups.size();
    sweeping = false;
    for (int k = 0; k < s.levels; ++k)
      sweeping |= sim.sweeping(k);
    if (sweeping)
      sim.step();
  }
  INFO(frames << " frames for the sweeps, one page per frame of each budget needs " << onePagePerBudget << "; " << joined
              << " level frames with sweep pages that join a paid call, " << unpaid << " frames not paid");
  CHECK(joined > 0);
  CHECK(unpaid == 0);
  CHECK(2 * frames < onePagePerBudget);
  sim.strictCoverage = true;
  sim.run(60);
  for (int k = 0; k < s.levels; ++k)
  {
    CHECK_FALSE(sim.sweeping(k));
    CHECK(sim.levelComplete(k));
  }
  check_invariants(sim);
}

TEST_CASE("T87 a teleport with no running sweep starts none: a camera velocity above maxCameraSpeed is no velocity")
{
  RtssSim sim;
  sim.sunAngVel = day_axis(40) * float(DAY24_RATE / 100);
  sim.run(600);
  int sweeps[RTSS_MAX_LEVELS] = {};
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    INFO("level " << k << " sun error " << sim.stats.levels[k].sunErrorDeg << ", tolerance " << sim.stats.levels[k].toleranceDeg);
    REQUIRE_FALSE(sim.sweeping(k));
    REQUIRE(sim.levelComplete(k));
    REQUIRE(sim.stats.levels[k].sunErrorDeg > 0);
    REQUIRE(sim.stats.levels[k].sunErrorDeg < 0.4f * sim.stats.levels[k].toleranceDeg);
    sweeps[k] = sim.stats.levels[k].sweeps;
  }
  const Point3 jump(1450, 0, 0);
  sim.cameraVel = jump / float(sim.dt);
  REQUIRE(length(sim.cameraVel) > sim.settings.maxCameraSpeed);
  sim.step();
  sim.cameraVel = Point3(0, 0, 0);
  for (int k = 0; k < sim.settings.levels; ++k)
  {
    INFO("level " << k);
    CHECK_FALSE(sim.sweeping(k));
    CHECK(sim.stats.levels[k].sweeps == sweeps[k]);
  }
  sim.run(60);
  for (int k = 0; k < sim.settings.levels; ++k)
    CHECK(sim.levelComplete(k));
  check_invariants(sim);
}

TEST_CASE("T88 a fast sweep with a diagonal u builds a run of one row in each raster call, as many pages as the scratch box or the "
          "budget allows, at a call cost of 7 and budgets of 15")
{
  RtssSchedulerSettings s;
  s.windowPages = 8;
  s.slotTexels = 128;
  s.borderTexels = 3;
  for (int k = 0; k < s.levels; ++k)
    s.recipes[k] = {RtssRaster::All, 1.f};
  s.rasterCallCost = 7;
  s.updatePagesPerFrame = s.validityBurstPagesPerFrame = 15;
  s.batchFrames = 1;
  s.rasterBatchPages = GENERATE(16, 64);
  const int boxX = (rtss_raster_scratch_size(s).x - 2 * s.borderTexels) / s.pageTexels();
  const double call = s.rasterCallCost, cost = s.recipes[0].pageCost;
  const int run = eastl::min(eastl::min(s.windowPages, boxX), int((s.updatePagesPerFrame - call) / cost));
  RtssSim sim(s);
  sim.coverageFrom = 60;
  sim.run(600);
  REQUIRE(sim.jobs.empty());
  const Point3 axis = normalize(sim.row(0, sim.curBit(0), 0) + sim.row(0, sim.curBit(0), 1));
  sim.sun = rtss_rotate(sim.sun, axis, 6 * DEG);
  sim.strictCoverage = false;
  sim.step();
  int allPages = 0;
  for (int k = 0; k < s.levels; ++k)
  {
    const Point2 u = sim.stats.levels[k].u;
    INFO("level " << k << " u " << u.x << ", " << u.y);
    REQUIRE(sim.sweeping(k));
    REQUIRE(sim.stats.levels[k].fastSweep);
    REQUIRE(fabsf(u.x) > 0.5f);
    REQUIRE(fabsf(u.y) > 0.5f);
    allPages += sim.stats.levels[k].sweepPages;
  }
  int frames = 0, sweptPages = 0, sweepCalls = 0, longestRun = 0, unpaid = 0;
  for (bool sweeping = true; sweeping && frames < allPages;)
  {
    ++frames;
    int paid = 0;
    for (uint8_t c : sim.jobClass)
      paid += (c & RTSS_JOB_CALL) != 0;
    RtssRasterPlan rp;
    rtss_raster_plan(sim.jobs, rtss_raster_levels(sim.sched), sim.sched, rp);
    unpaid += paid != (int)rp.cullGroups.size();
    for (const RtssRasterGroup &g : rp.cullGroups)
    {
      int swept = 0;
      for (int r = g.first; r < g.first + g.count; ++r)
        swept += (sim.jobClass[rp.order[r]] & RTSS_JOB_SWEPT) != 0;
      if (!swept)
        continue;
      sweptPages += swept;
      ++sweepCalls;
      longestRun = eastl::max(longestRun, swept);
    }
    sweeping = false;
    for (int k = 0; k < s.levels; ++k)
      sweeping |= sim.sweeping(k);
    if (sweeping)
      sim.step();
  }
  INFO("a run of " << run << " pages: " << sweptPages << " sweep pages in " << sweepCalls << " calls, the longest run " << longestRun
                   << ", " << frames << " frames for the sweeps of " << allPages << " pages, " << unpaid << " frames not paid");
  CHECK(longestRun == run);
  CHECK(4 * sweptPages >= 3 * run * sweepCalls);
  CHECK(frames <= allPages / run + 4 * s.levels);
  CHECK(unpaid == 0);
  sim.strictCoverage = true;
  sim.run(60);
  for (int k = 0; k < s.levels; ++k)
  {
    CHECK_FALSE(sim.sweeping(k));
    CHECK(sim.levelComplete(k));
  }
  check_invariants(sim);
}

TEST_CASE("T89 past its burst the coarsest level takes the update budget before the finer levels, and within it leaves the update "
          "budget to them")
{
  auto requiredPages = [](const RtssSim &sim, bool coarsest) {
    int pages = 0;
    for (int k = 0; k < sim.settings.levels; ++k)
      pages += (k == sim.settings.levels - 1) == coarsest ? level_of_class(sim, RTSS_REQUIRED, k) : 0;
    return pages;
  };
  RtssSchedulerSettings s;
  s.validityBurstPagesPerFrame = 64;
  RtssSim small(s);
  small.run(60);
  small.camera += DPoint3(3000, 0, -3000);
  small.coverageFrom = small.frame + 1000;
  small.step();
  CHECK(requiredPages(small, false) == 0);
  small.run(200);
  check_invariants(small);

  s.batchFrames = 4;
  RtssSim spent(s);
  spent.run(60);
  for (int jump = 0; jump < 3; ++jump)
  {
    spent.camera += DPoint3(3000, 0, -3000);
    spent.coverageFrom = spent.frame + 1000;
    spent.step();
    if (jump == 1)
      CHECK(requiredPages(spent, true) == s.validityBurstPagesPerFrame + s.updatePagesPerFrame * s.batchFrames);
  }
  CHECK(requiredPages(spent, true) == s.validityBurstPagesPerFrame); // two batches spent the balance
  spent.run(200);
  check_invariants(spent);

  RtssSim big;
  big.coverageFrom = big.frame + 1000;
  big.step();
  const int bigCoarsest = requiredPages(big, true), bigFiner = requiredPages(big, false);
  INFO(bigCoarsest << " pages of the coarsest level, " << bigFiner << " of the finer ones");
  CHECK(big.levelComplete(big.settings.levels - 1));
  CHECK(bigCoarsest < big.settings.validityBurstPagesPerFrame);
  CHECK(bigFiner == big.settings.updatePagesPerFrame * big.settings.batchFrames);
  big.run(200);
  check_invariants(big);
}
