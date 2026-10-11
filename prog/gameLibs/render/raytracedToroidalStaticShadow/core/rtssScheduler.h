// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_hlsl_floatx.h>
#include "../shaders/rtss_common.hlsli"
#include "rtssTransforms.h"
#include "rtssSettings.h"
#include <math/dag_bounds3.h>
#include <math/integer/dag_IBBox2.h>
#include <math/integer/dag_IPoint4.h>
#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <generic/dag_relocatableFixedVector.h>
#include <EASTL/algorithm.h>
#include <math.h>

enum RtssPageClass : uint8_t
{
  RTSS_REQUIRED,
  RTSS_DIRTY,
  RTSS_SWEEP,
  RTSS_PRETRACE,
  RTSS_FILL,
  RTSS_PAGE_CLASS_COUNT
};
static constexpr uint8_t RTSS_JOB_CLASS_MASK = 0x0f;
static constexpr uint8_t RTSS_JOB_CALL = 0x10;      // opens a raster box and pays its call
static constexpr uint8_t RTSS_JOB_SAME_PAGE = 0x20; // the slot already holds this page with this bit
static constexpr uint8_t RTSS_JOB_SWEPT = 0x40;     // takes an entry of the ordered sweep list, also as its replacement
static constexpr uint8_t RTSS_JOB_INNER = 0x80;

inline IPoint2 rtss_raster_scratch_size(const RtssSchedulerSettings &s)
{
  const int side = int(ceilf(sqrtf(float(eastl::clamp(s.rasterBatchPages, 1, 256))))) * s.slotTexels;
  return IPoint2(side, s.rasterTranspose ? eastl::max(side / 2, s.slotTexels) : side);
}

// sweep order: row by row toward -u
inline double rtss_sweep_key(const Point2 &u, const IPoint2 &p)
{
  const double signX = (u.x > 0) - (u.x < 0), signY = (u.y > 0) - (u.y < 0);
  return signY * p.y * 65536 + signX * p.x; // the pages of a level lie within 2W
}

// a raster box of count pages takes page p
inline bool rtss_box_takes(const RtssSchedulerSettings &s, IBBox2 pages, int count, const IPoint2 &p)
{
  const int border = 2 * s.borderTexels, pageTexels = s.pageTexels();
  const IPoint2 scratch = rtss_raster_scratch_size(s);
  pages += p;
  const IPoint2 size = pages.width() + IPoint2(1, 1);
  return size.x * size.y <= eastl::clamp(s.rasterBoxPagesPerJob, 1, 64) * (count + 1) &&
         eastl::max(size.x, size.y) <= eastl::max((scratch.x - border) / pageTexels, 1) &&
         eastl::min(size.x, size.y) <= eastl::max((scratch.y - border) / pageTexels, 1);
}

struct RtssFramePlan
{
  dag::Vector<RtssJob> jobs;
  dag::Vector<uint8_t> jobClass; // RtssPageClass | RTSS_JOB_CALL | RTSS_JOB_SAME_PAGE | RTSS_JOB_SWEPT | RTSS_JOB_INNER
  dag::Vector<uint32_t> groups;  // jobIndex << 16 | firstTexel / RTSS_BUILD_GROUP
};

struct RtssSchedulerLevelStats
{
  IBBox2 cube[2]; // pages that cube receivers use in the transform of bit b
  Point2 u;
  float sweepProgress, sunErrorDeg, toleranceDeg, fastLimitDeg;
  float sweepSeconds, lastSweepSeconds, lastPlannedSeconds;
  int validSlots, dirtySlots, emptySlots, sweeps, sweepPages;
  int headSlot;        // slot of the next sweep page after this frame's jobs, -1 for none
  int requiredPending; // REQUIRED pages that this frame's budget did not take
  uint8_t liveMask;
  bool sweeping, fastSweep, pretraceBlocked;
};

struct RtssSchedulerStats
{
  double pages[RTSS_PAGE_CLASS_COUNT], cost[RTSS_PAGE_CLASS_COUNT]; // cost in the units of the budgets
  double pagesEdge;                                                 // REQUIRED pages past the +u edge
  uint32_t rays;                                                    // texels of the traced jobs, not of the raster ones
  bool sunBelowHorizon;
  dag::RelocatableFixedVector<RtssSchedulerLevelStats, RTSS_MAX_LEVELS, false> levels;
};

class RtssScheduler
{
public:
  explicit RtssScheduler(const RtssSchedulerSettings &s);
  const RtssSchedulerSettings &getSettings() const { return set; }
  // keeps slots; page budgets apply at the next plan(), timing from the next sweep
  void setBudget(const RtssSchedulerBudget &budget) { static_cast<RtssSchedulerBudget &>(set) = budget; }
  // a level whose raster changes is invalidated, true when one does; a change drops the plan of the last plan()
  bool setRecipes(const RtssSchedulerRecipes &recipes);

  void setWorldBox(const BBox3 &casters_box); // a changed box invalidates all slots and drops the plan of the last plan()
  // angular_velocity in rad/s
  void setSun(const Point3 &dir_to_sun, const Point3 &angular_velocity);
  // invalidate and invalidateBoxes drop the plan of the last plan(), as setRecipes, setWorldBox and reset do
  void invalidate(); // every non-empty slot DIRTY
  void invalidateBoxes(const BBox3 *world_boxes, uint32_t cnt, uint32_t level_mask = ~0u);
  void reset(); // device reset: every slot EMPTY, and the plan of the last plan() dropped

  // camera_velocity in m/s, zero above maxCameraSpeed; absolute time in s; a caller sets rasterTile only; the plan stays
  // readable until the next plan(), commit() builds nothing of a dropped plan; an idle scheduler skips a plan of unchanged inputs
  RtssFramePlan &plan(const DPoint3 &camera_pos, const Point3 &camera_velocity, double time_sec);
  // skipped_jobs: indices of plan().jobs not built; after a skipped sweep job, also the later ones of its level;
  // a provisional job is built, and its slot stays DIRTY for a rebuild; both lists sorted; call it once for each plan()
  void commit(dag::ConstSpan<uint32_t> skipped_jobs, dag::ConstSpan<uint32_t> provisional_jobs = {});

  // the transforms, the window and the camera of the last plan(); a bit that is not live gives the current transform
  const RtssLightXform &xform(int level, int bit) const { return lvl[level].liveXform(bit); }
  const DPoint3 &cameraPos() const { return camera; } // the window center: the camera, its height clamped to the world box
  float centerY() const { return float(camera.y - rtssOrigin.y); } // over rtss_origin, for the level selection
  IPoint2 windowOrigin(int level) const { return lvl[level].origin; }
  double texel(int level) const { return lvl[level].texel; }
  double pageWorld(int level) const { return lvl[level].pageWorld; }
  double extent(int level) const { return lvl[level].extent; }

  dag::ConstSpan<RtssLevel> levelTable() const { return levelTab; }      // after plan()
  void globals(IPoint4 &params_i, Point4 &params, Point4 &origin) const; // after plan()
  dag::ConstSpan<RtssSlot> slotTable() const { return slotTab; }         // after commit()
  bool slotTableChanged() const { return slotTabChanged; }               // by the last commit()
  bool isReady(const BBox3 &box, uint32_t level_mask = ~0u) const;
  void getStats(RtssSchedulerStats &out) const; // the jobs of the last plan(), the slots as they are now

private:
  enum SlotState : uint8_t
  {
    EMPTY,
    VALID,
    DIRTY
  };
  enum JobKind : uint8_t
  {
    NEW_PAGE,
    SAME_PAGE,
    SWEEP_PAGE,
    DIRTY_RECT
  };
  struct Rect
  {
    uint8_t x0, y0, x1, y1;
    int area() const { return (x1 - x0) * (y1 - y0); }
    bool operator==(const Rect &r) const { return x0 == r.x0 && y0 == r.y0 && x1 == r.x1 && y1 == r.y1; }
  };
  struct Slot
  {
    IPoint2 page = IPoint2(0, 0);
    uint8_t bit = 0, state = EMPTY;
    uint32_t jobFrame = ~0u;
    double zStart = 0, zEnd = 0;
    Rect dirty = {0, 0, 0, 0};
  };
  struct SweepEntry
  {
    int slot;
    double key;
  };
  struct EdgePage
  {
    IPoint2 page;
    double key;
  };
  struct Pretrace
  {
    double tStart = 0, tMove = 0;
    int dir = 0;
    bool blocked = false;
  };
  struct Level
  {
    RtssLightXform xform[2];
    uint8_t live = 0;
    int cur = 0;
    bool fast = false;
    bool sweeping() const { return live == 3; }
    int newestBit() const { return sweeping() ? cur ^ 1 : cur; }
    const RtssLightXform &liveXform(int bit) const { return xform[live >> bit & 1 ? bit : cur]; }
    Point2 u = Point2(1, 0);
    DPoint3 pivot = DPoint3(0, 0, 0);
    double t0 = 0, duration = 0, lastSweepSeconds = 0, lastPlannedSeconds = 0;
    double front = 0;
    float tolRad = 0, fastLimitRad = 0;
    int sweeps = 0;
    dag::Vector<SweepEntry> sweepList;
    int swept = 0;
    IPoint2 origin = IPoint2(0, 0);
    Pretrace pretrace[2];
    dag::Vector<Slot> slots;
    double texel = 0, pageWorld = 0, extent = 0;

    DPoint3 camLs[2];
    IBBox2 cube[2];
    double reqZ[2][2] = {}, boxZ[2][2] = {};
    dag::Vector<EdgePage> edges;
    int edgeNext = 0, head = 0, requiredPending = 0;
    double frontAfter = 0;
    bool headBlocked = false, pretraceBlockedNow = false;
  };
  struct PlannedJob
  {
    int level = 0, slot = 0;
    IPoint2 page = IPoint2(0, 0);
    Rect rect = {0, 0, 0, 0};
    JobKind kind = SAME_PAGE;
    uint8_t cls = 0, bit = 0;
    bool swept = false;
    double zStart = 0, zEnd = 0;
    int box = -1;
    bool paysCall = false;
  };
  struct RasterBox
  {
    IBBox2 pages;
    int level, bit, count;
    bool paid;
  };
  struct QueueItem
  {
    double order;
    int level, slot;
    IPoint2 page;
    uint8_t cls;
    bool inner;
  };

  static bool innerLevelOrder(const QueueItem &a, const QueueItem &b);
  int slotIndex(const IPoint2 &p) const { return rtss_slot_index(int2(p.x, p.y), set.windowPages); }
  Rect wholeSlot() const { return {0, 0, uint8_t(set.slotTexels), uint8_t(set.slotTexels)}; }
  IPoint2 windowPage(const Level &l, int slot) const;
  bool incomingPage(const Level &l, const IPoint2 &p, IPoint2 &in) const;
  bool inUse(const Level &l, const IPoint2 &p) const { return (l.cube[l.cur] & p) || (l.sweeping() && (l.cube[l.cur ^ 1] & p)); }
  bool inUse(const Level &l, const Slot &s) const { return s.state != EMPTY && inUse(l, s.page); }
  double camDist2(const Level &l, const IPoint2 &p) const
  {
    return (DPoint2(p.x + 0.5, p.y + 0.5) * l.pageWorld - DPoint2::xy(l.camLs[l.cur])).lengthSq();
  }
  bool edgeFree(const Level &l, const Slot &s, const IPoint2 &p) const { return s.jobFrame != frame && (s.page == p || !inUse(l, s)); }
  bool edgeClaims(const Level &l, int slot, const IPoint2 &except) const;
  bool depthOk(const Level &l, const Slot &s) const;
  bool isInner(int level, const IPoint2 &p) const;
  double key(const Level &l, const IPoint2 &p) const { return rtss_sweep_key(l.u, p); }
  double budgetCap() const { return eastl::max(eastl::min(set.updatePagesPerFrame, set.validityBurstPagesPerFrame), 1); }
  double pageCost(int level) const { return eastl::min(double(set.recipes[level].pageCost), budgetCap()); }
  double callCost(int level) const
  {
    return set.recipes[level].raster == RtssRaster::None
             ? 0.0
             : eastl::clamp(double(set.rasterCallCost), 0.0, budgetCap() - pageCost(level));
  }
  double &requiredLeft(int level) { return level == set.levels - 1 ? burstLeft : updateLeft; }
  double jobCost(int level, int area) const { return area * pageCost(level) / double(set.slotTexels * set.slotTexels); }
  uint8_t jobBit(const Level &l, const Slot &s, const IPoint2 &page, JobKind kind) const;
  int joinBox(int level, int bit, const IPoint2 &p) const; // boxes.size() for a new box
  double hOverE(const Level &l, double duration) const;

  void invalidateLevel(Level &l);
  void updateSweep(int level);
  void startSweep(Level &l, const Point3 &dir_end, double t0, double duration, double h_over_e, bool fast);
  void updateWindow(int level);
  void updatePretrace(int level, int axis);
  bool addJob(int level, int slot, const IPoint2 &page, JobKind kind, uint8_t cls, double &budget, bool swept = false);
  int edgesNeeded(int level, double above_key) const;
  bool requireEdges(int level, double above_key);
  int sweepHead(const Level &l) const;
  double headKey(const Level &l) const; // a new page above it takes the target bit
  bool takeSweepHead(int level, double &budget, uint8_t cls);
  double accrueUpdate(); // the batch
  void planRequired();
  void fillBox(size_t from, double horizon, double &budget);
  void planQueue();
  void buildJobs();
  void buildLevelTable();
  void addDirty(Slot &s, const Rect &r);

  RtssSchedulerSettings set;
  dag::Vector<Level> lvl;
  Point3 sunDir = Point3(0, 1, 0), sunAngVel = Point3(0, 0, 0);
  bool belowHorizon = false;
  BBox3 worldBox;
  bool hasWorldBox = false;
  DPoint3 camera = DPoint3(0, 0, 0);
  Point3 cameraVel = Point3(0, 0, 0), rtssOrigin = Point3(0, 0, 0); // constraint: x87 may skip the float rounding of point3()
  double now = 0, frameDt = 0;
  uint32_t frame = 0;
  double burstLeft = 0, updateLeft = 0, updateBalance = 0;
  bool idle = false; // the last plan had nothing to build, nor to wait for
  DPoint3 lastCameraPos = DPoint3(0, 0, 0);
  Point3 lastCameraVel = Point3(0, 0, 0), lastSunDir = Point3(0, 0, 0), lastSunAngVel = Point3(0, 0, 0);

  RtssFramePlan framePlan;
  dag::Vector<PlannedJob> planned;
  dag::Vector<RasterBox> boxes;
  dag::Vector<QueueItem> queue, required;
  dag::Vector<RtssLevel> levelTab;
  dag::Vector<RtssSlot> slotTab;
  bool slotTabDirty = true, slotTabChanged = false;
  RtssSchedulerStats jobStats = {};
};
