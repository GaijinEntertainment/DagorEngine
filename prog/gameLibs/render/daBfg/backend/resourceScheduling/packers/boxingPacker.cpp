// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <backend/resourceScheduling/packer.h>

#include <EASTL/priority_queue.h>
#include <EASTL/numeric.h>
#include <EASTL/map.h>
#include <EASTL/stack.h>
#include <EASTL/vector_multiset.h>

#include <debug/dag_assert.h>
#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>
#include <util/dag_stlqsort.h>
#include <util/dag_globDef.h>
#include <dag/dag_vectorMap.h>
#include <math/dag_bits.h>


#if __has_include(<UnitTest++/UnitTestPP.h>)
#include <math/random/dag_random.h>
#include <UnitTest++/UnitTestPP.h>
#define ENABLE_UNIT_TESTS
#endif


// This packer is based on the "OPT versus LOAD in Dynamic Storage Allocation"
// article by Buchsbaum, Karloff, Kenyon, Reingold and Thorup.

// The article is extremely math heavy and the algorithm is hellishly
// complicated. Proceed with caution.

// ========= ABANDON HOPE ALL YE WHO ENTER HERE =========


namespace // internal linkage
{

using TimePoint = uint32_t;
using TimeInterval = uint32_t;

using MemoryOffset = uint64_t;
using MemorySize = uint64_t;

using JobIndex = uint32_t;
using JobCount = uint32_t;

using BoxIndex = uint32_t;
using BoxCount = uint32_t;

template <class T>
using FmemVector = dag::Vector<T, framemem_allocator>;
template <class K, class V>
using FmemVectorMap = dag::VectorMap<K, V, eastl::less<K>, framemem_allocator>;

template <class Container>
struct CleanupReversed : Container
{
  using Container::Container;

  ~CleanupReversed()
  {
    while (!Container::empty())
      Container::erase(eastl::prev(Container::end()));
  }
};

template <class T>
using FmemBucketVector = CleanupReversed<dag::Vector<dag::Vector<T, framemem_allocator>, framemem_allocator>>;

// Job of height 1 with time interval
struct Job
{
  TimePoint left;
  TimePoint right;
};

constexpr BoxIndex UNRESOLVED_BOX_ID = static_cast<BoxIndex>(-1);

template <class T, class Comp = eastl::less<T>>
using FmemPrioQueue = eastl::priority_queue<T, FmemVector<T>, Comp>;


// Transforms the jobs by shifting the coordinate system so that
// comparing l or r for jobs means comparing how far they are from the
// time point t.
FmemVector<Job> transform_jobs_alive_at_t(eastl::span<const Job> jobs, TimePoint t, TimeInterval timepoint_count)
{
  // Basically, we transform this
  // r
  // ^
  // |_________________
  // |        |     / |
  // |   A    |   /   |
  // |        | /  B  |
  // |________t_______|
  // |      / |       |
  // |    /   |       |
  // |  /  C  |       |
  // |/_______|________> l
  // Into this
  // r
  // ^
  // |_________________
  // |              / |
  // |            /   |
  // |          /  C  |
  // |        /_______|
  // |      / |       |
  // |    /   |       |
  // |  /  B  |   A   |
  // t/_______|________> l
  FmemVector<Job> result;
  result.reserve(jobs.size());

  // If t is zero, we never use minust, so it's ok that it's not really
  // mod timepoint_count.
  const auto minust = timepoint_count - t; // -t mod timepoint_count
  for (auto [l, r] : jobs)
  {
    if (l > t) // case B
      result.push_back({l - t, r - t});
    else if (r < t) // case C
      result.push_back({l + minust, r + minust});
    else // case A
      result.push_back({l + minust, r - t});
  }
  return result;
}

// For a set of jobs `jobs` all alive at time point t, boxes them
// into a set of boxes (each box with height box_height), returning box
// index for each job, but possibly skipping some jobs (returning -1
// for them).
// The resulting boxing is guaranteed to waste <= 4*eps*LOAD(jobs, u)
// space at each time point u, and the amount of unresolved (skipped)
// jobs is guaranteed to be <= 2*box_height*ceil(1/eps^2).
// In other words, the less space you want to waste with this boxing,
// the more jobs you will have to throw away.
// TIME COMPLEXITY: O(n log n)
FmemVector<BoxIndex> box_unit_jobs_all_alive_at_t(eastl::span<const Job> jobs, TimePoint t, TimeInterval timepoint_count,
  MemorySize box_height, MemorySize inv_eps)
{
  static constexpr BoxIndex UNDETERMINED = static_cast<BoxIndex>(-2);
  FmemVector<BoxIndex> result(jobs.size(), UNDETERMINED);

  FRAMEMEM_VALIDATE;

  G_ASSERTF_RETURN(box_height > 0 && inv_eps > 0, result, "Precondition failed! World is broken!");

  // We look at a peculiar coordinate system where x is left end of
  // jobs and y is right end of jobs. Here, a job is a single point
  // on the l-r plane in the picture below. The fact that it's alive
  // at t means that it's either in square A (no wraparound), in
  // triangle B (left side wraparound), or in triangle C (right side
  // wraparound).
  // r
  // ^
  // |_________________
  // |        |     / |
  // |   A    |   /   |
  // |        | /  B  |
  // |________t_______|
  // |      / |       |
  // |    /   |       |
  // |  /  C  |       |
  // |/_______|________> l

  // We want to grab groups of jobs with l and r farthest away from t.
  // This notion is well-defined, as for all jobs [l, r) contains t.
  // For this, we need to transform the jobs a little bit.

  const auto transformedJobs = transform_jobs_alive_at_t(jobs, t, timepoint_count);

  auto xComp = [&transformedJobs](JobIndex lhs, JobIndex rhs) { return transformedJobs[lhs].left > transformedJobs[rhs].left; };
  auto yComp = [&transformedJobs](JobIndex lhs, JobIndex rhs) { return transformedJobs[lhs].right < transformedJobs[rhs].right; };

  FmemPrioQueue<JobIndex, decltype(xComp)> xHeap(xComp);
  xHeap.get_container().reserve(jobs.size());
  FmemPrioQueue<JobIndex, decltype(yComp)> yHeap(yComp);
  yHeap.get_container().reserve(jobs.size());

  for (JobIndex i = 0; i < jobs.size(); ++i)
  {
    xHeap.push(i);
    yHeap.push(i);
  }

  static constexpr JobIndex WAS_EMPTY = -1;
  auto takeOne = [&result](auto &heap) {
    JobIndex job;
    do
    {
      if (heap.empty())
        return WAS_EMPTY;
      job = heap.top();
      heap.pop();
    } while (result[job] != UNDETERMINED);
    return job;
  };

  auto takeSeveral = [&takeOne](auto &heap, JobCount count) {
    FmemVector<JobIndex> result;
    result.reserve(count);
    for (JobCount i = 0; i < count; ++i)
    {
      auto job = takeOne(heap);
      if (job == WAS_EMPTY)
        break;
      result.push_back(job);
    }
    return result;
  };

  const JobCount toThrowAway = static_cast<JobCount>(eastl::min<MemorySize>(box_height * inv_eps * inv_eps, jobs.size()));

  // First H*ceil(1/eps^2) elements by x coord will be unresolved
  // (they are "too far to the left")
  {
    auto vertUnresolvedStrip = takeSeveral(xHeap, toThrowAway);
    for (auto job : vertUnresolvedStrip)
      result[job] = UNRESOLVED_BOX_ID;
  }

  // First H*ceil(1/eps^2) elements by y coord will be unresolved too
  // (they are "too far to the right")
  {
    auto horUnresolvedStrip = takeSeveral(yHeap, toThrowAway);
    for (auto job : horUnresolvedStrip)
      result[job] = UNRESOLVED_BOX_ID;
  }

  BoxIndex boxIndexCounter = 0;

  auto boxStrip = [&result, &boxIndexCounter, box_height](auto &strip) {
    const auto stripSize = strip.size();
    for (uint32_t i = 0; i < stripSize; i += box_height, ++boxIndexCounter)
      for (uint32_t j = 0; j < box_height && i + j < stripSize; ++j)
        result[strip[stripSize - 1 - i - j]] = boxIndexCounter;
  };

  const JobCount stripSize = static_cast<JobCount>(eastl::min<MemorySize>(box_height * inv_eps, jobs.size()));
  while (!xHeap.empty() || !yHeap.empty())
  {

    {
      // Take a vertical strip with smallest x-coords
      // box_height * ceil(1/eps) that contains
      auto vertStrip = takeSeveral(xHeap, stripSize);

      // Take jobs in "box_height" groups by decreasing Y-coord and
      // call each group a box
      stlsort::sort(vertStrip.begin(), vertStrip.end(), yComp);
      boxStrip(vertStrip);
    }

    {
      // Take a horizontal strip with smallest y-coords that contains
      // box_height * ceil(1/eps) jobs
      auto horStrip = takeSeveral(yHeap, stripSize);

      // Take jobs in "box_height" groups by increasing X-coord and
      // call each group a box
      stlsort::sort(horStrip.begin(), horStrip.end(), xComp);
      boxStrip(horStrip);
    }
  }

  return result;
}

template <class T>
T size_of_mapping(eastl::span<const T> mapping, T invalid)
{
  T result = invalid;
  for (auto i : mapping)
    if (i != invalid && (result == invalid || i > result))
      result = i;
  return result == invalid ? 0 : result + 1;
}

template <class T>
static T size_of_mapping(eastl::span<const T> mapping)
{
  return mapping.empty() ? 0 : 1 + *eastl::max_element(mapping.begin(), mapping.end());
}

FmemVector<JobCount> calculate_loads(eastl::span<const Job> jobs, TimeInterval timepoint_count)
{
  JobCount wraparoundCount = 0;
  FmemVector<JobCount> loads(timepoint_count, 0);
  for (auto [l, r] : jobs)
  {
    ++loads[l];
    --loads[r]; // overflows are ok here, they will get canceled out
    if (r <= l)
      ++wraparoundCount;
  }
  eastl::partial_sum(loads.begin(), loads.end(), loads.begin());
  for (auto &l : loads)
    l += wraparoundCount;
  return loads;
}

// Reinterprets a boxing as new jobs
FmemVector<Job> boxes_to_jobs(eastl::span<const Job> jobs, TimeInterval timepoint_count, eastl::span<const BoxIndex> boxing)
{
  const BoxCount boxCount = size_of_mapping(boxing, UNRESOLVED_BOX_ID);

  FmemVector<Job> result(boxCount, {0, 0});

  FRAMEMEM_VALIDATE;

  FmemVector<MemorySize> boxSizes(boxCount);
  for (JobIndex i = 0; i < jobs.size(); ++i)
    if (auto box = boxing[i]; box != UNRESOLVED_BOX_ID)
      ++boxSizes[box];
  FmemBucketVector<Job> boxedJobs(boxCount);
  for (BoxIndex i = 0; i < boxCount; ++i)
    boxedJobs[i].reserve(boxSizes[i]);
  for (JobIndex i = 0; i < jobs.size(); ++i)
    if (auto box = boxing[i]; box != UNRESOLVED_BOX_ID)
      boxedJobs[box].push_back(jobs[i]);

  const auto length = [timepoint_count](Job job) { //
    return (job.left >= job.right) * timepoint_count + job.right - job.left;
  };

  // TODO: this seems to be sub-optimal, but most likely correct.
  for (BoxIndex box = 0; box < boxCount; ++box)
  {
    G_ASSERT(!boxedJobs[box].empty());

    const auto loads = calculate_loads(boxedJobs[box], timepoint_count);

    const auto isZero = [](JobCount j) { return j == 0; };
    const auto firstNonZero = eastl::find_if_not(loads.begin(), loads.end(), isZero);
    const auto afterLastNonZero = eastl::find_if_not(loads.rbegin(), loads.rend(), isZero).base();

    result[box] = Job{static_cast<TimePoint>(firstNonZero - loads.begin()),
      afterLastNonZero == loads.end() ? 0 : static_cast<TimePoint>(afterLastNonZero - loads.begin())};

    for (auto it = firstNonZero; it != afterLastNonZero;)
    {
      const auto runStart = eastl::find_if(it, afterLastNonZero, isZero);
      const auto runEnd = eastl::find_if_not(runStart, afterLastNonZero, isZero);

      // X stands for non-zero:
      //   ...contender... )         [  ...contender...
      // 000 xxxxxxxxxxxxx 000000000 xxxxxxxxxxxxx 00000
      //     ^             ^         ^             ^
      // firstNonZero   runStart   runEnd    afterLastNonZero
      const Job contender{static_cast<TimePoint>(runEnd - loads.begin()), static_cast<TimePoint>(runStart - loads.begin())};
      if (length(contender) < length(result[box]))
        result[box] = contender;

      it = runEnd;
    }
  }

  return result;
}

} // namespace

#ifdef ENABLE_UNIT_TESTS

SUITE(BoxUnitJobsAliveAtT)
{
  FmemVector<Job> gen_random_jobs_alive_at(JobCount count, TimePoint t, TimeInterval max_time)
  {
    FmemVector<Job> result;
    result.reserve(count);
    for (JobIndex i = 0; i < count; ++i)
    {
      // The distr is a bit non-uniform here, but I don't care honestly
      const auto type = dagor_random::rnd_int(0, 2);
      if (type == 0)
      {
        const TimePoint l = dagor_random::rnd_int(0, t);
        const TimePoint r = dagor_random::rnd_int(t + 1, max_time - 1);
        result.push_back(Job{l, r});
      }
      else if (type == 1)
      {
        const TimePoint l = dagor_random::rnd_int(0, t);
        const TimePoint r = dagor_random::rnd_int(0, l);
        result.push_back(Job{l, r});
      }
      else
      {
        const TimePoint r = dagor_random::rnd_int(t + 1, max_time - 1);
        const TimePoint l = dagor_random::rnd_int(r, max_time - 1);
        result.push_back(Job{l, r});
      }
    }

    return result;
  }

  void check(const eastl::span<const Job> jobs, const TimeInterval timepoint_count, const eastl::span<const BoxIndex> boxing,
    const MemorySize box_height, const float eps)
  {
    FmemVector<Job> resolvedJobs;
    resolvedJobs.reserve(jobs.size());
    for (JobIndex jobIdx = 0; jobIdx < jobs.size(); ++jobIdx)
      if (boxing[jobIdx] != UNRESOLVED_BOX_ID)
        resolvedJobs.push_back(jobs[jobIdx]);

    CHECK_CLOSE(jobs.size(), resolvedJobs.size(), 2 * box_height * ceil((1 / eps) * (1 / eps)));


    const auto boxJobs = boxes_to_jobs(jobs, timepoint_count, boxing);

    // Sanity-check for boxes_to_jobs: both points of each job must be
    // contained within it's box
    for (JobIndex i = 0; i < jobs.size(); ++i)
    {
      if (boxing[i] == UNRESOLVED_BOX_ID)
        continue;

      const auto job = jobs[i];
      const auto box = boxJobs[boxing[i]];
      if (job.left == job.right)
      {
        CHECK(box.left == box.right);
        continue;
      }
      if (box.left == box.right)
        continue; // spanning boxes contain everything

      // Now both job and box are not spanning
      if (box.left < box.right) // non-wrap-around box
        CHECK(box.left <= job.left && job.left < job.right && job.right <= box.right);
      else if (job.left < job.right) // non-wrap-around job
        CHECK(job.right <= box.right || box.left <= job.left);
      else // both are wraparound
        CHECK(job.right <= box.right && box.left <= job.left);
    }

    FmemVector<JobCount> boxSizes(boxJobs.size(), 0);
    for (JobIndex job = 0; job < jobs.size(); ++job)
      if (auto box = boxing[job]; box != UNRESOLVED_BOX_ID)
        ++boxSizes[box];

    for (auto size : boxSizes)
      CHECK(size <= box_height);

    const auto initialLoads = calculate_loads(jobs, timepoint_count);
    const auto resolvedLoads = calculate_loads(resolvedJobs, timepoint_count);
    const auto boxLoads = calculate_loads(boxJobs, timepoint_count);

    for (TimePoint t = 0; t < timepoint_count; ++t)
      CHECK_CLOSE(resolvedLoads[t], boxLoads[t] * box_height, 4 * eps * initialLoads[t]);
  }

  void random_test(int seed, TimeInterval timepoint_count, JobCount job_count, MemorySize box_height, float in_eps)
  {
    const MemorySize invEps = static_cast<MemorySize>(ceil(1 / in_eps));
    dagor_random::set_rnd_seed(seed);
    const auto aliveAt = dagor_random::rnd_int(0, timepoint_count - 1);
    auto jobs = gen_random_jobs_alive_at(job_count, aliveAt, timepoint_count);
    auto boxing = box_unit_jobs_all_alive_at_t(jobs, aliveAt, timepoint_count, box_height, invEps);
    check(jobs, timepoint_count, boxing, box_height, 1.f / invEps);
  }

  TEST(SmallTest1) { random_test(42, 100, 200, 5, 0.2f); }
  TEST(SmallTest2) { random_test(43, 100, 200, 5, 0.2f); }
  TEST(SmallTest3) { random_test(44, 100, 200, 5, 0.2f); }
  TEST(SmallTest4) { random_test(45, 100, 200, 5, 0.2f); }

  TEST(ManyJobsBigEps) { random_test(42, 500, 1000, 100, 0.9f); }
  TEST(ManyJobsSmallEps) { random_test(43, 500, 1000, 100, 0.4f); }
  TEST(ManyJobsTinyEps) { random_test(44, 500, 1000, 100, 0.1f); }
}

#endif


namespace
{

__forceinline TimeInterval circular_distance(TimePoint p1, TimePoint p2, TimeInterval timepoint_count)
{
  return (p1 > p2) * timepoint_count + p2 - p1;
}

// Returns timepoint_count for identical points
__forceinline TimeInterval circular_length(TimePoint p1, TimePoint p2, TimeInterval timepoint_count)
{
  return (p1 >= p2) * timepoint_count + p2 - p1;
}

__forceinline TimeInterval circular_offset(TimePoint base, TimeInterval offset, TimeInterval timepoint_count)
{
  return base + offset - (base + offset >= timepoint_count) * timepoint_count;
}

__forceinline TimeInterval circular_midpoint(TimePoint p1, TimePoint p2, TimeInterval timepoint_count)
{
  return circular_offset(p1, circular_distance(p1, p2, timepoint_count) / 2, timepoint_count);
}

__forceinline bool circular_arcs_intersect(TimePoint l1, TimePoint r1, TimePoint l2, TimePoint r2, TimeInterval timepoint_count)
{
  return circular_distance(l1, l2, timepoint_count) < circular_length(l1, r1, timepoint_count) ||
         circular_distance(l2, l1, timepoint_count) < circular_length(l2, r2, timepoint_count);
}

// Algorithm for UCSDA or rather circular arc graph coloring. (NP-complete btw)
// See "Coloring a Family of Circular Arcs" by Alan Tucker for details.
// Should never be used for interval graphs, as it often gives a worse
// answer than interval graph coloring.
// Answer goodness is dependent on the minimal amount l of jobs needed to cover
// the entire circle (which is well-defined when input graph is not an interval one):
//   For l >= 3, it gives a <= (l-1)/(l-2)*LOAD answer
//   For other l, it gives a <= 2*LOAD answer
// Note however that for small l, it is possible to give an instance of
// circular arc graph coloring of n elements that requires n colors, but
// the LOAD is n/2. So a better bound than 2*LOAD is not generally
// possible.
// TIME COMPLEXITY: O(n^2)
FmemVector<MemoryOffset> color_circular_arc_graph(eastl::span<const Job> jobs, TimeInterval timepoint_count, TimePoint max_load_point)
{
  static constexpr MemoryOffset NO_COLOR = static_cast<MemoryOffset>(-1);
  FmemVector<MemoryOffset> coloring(jobs.size(), NO_COLOR);

  if (jobs.empty())
    return coloring;

  // Our node deletes do not nest with node insertion, so we need
  // to force-free the allocated nodes from the framemem stack.
  // No allocations except for node insertion happen below.
  FRAMEMEM_REGION;
  eastl::multimap<TimePoint, JobIndex, eastl::less<TimePoint>, framemem_allocator> sortedJobs;
  for (JobIndex i = 0; i < jobs.size(); ++i)
    sortedJobs.emplace(jobs[i].left, i);

  const auto wraparound = [&sortedJobs](auto it) { return it == sortedJobs.end() ? sortedJobs.begin() : it; };

  MemoryOffset currentLayer = 0;
  TimePoint currentLayerStart;
  TimePoint currentLayerEnd;

  // Start from any job that is alive at max load point and extends
  // the lest to the left from it.
  {
    auto iter = sortedJobs.upper_bound(max_load_point);
    iter = eastl::prev(iter == sortedJobs.begin() ? sortedJobs.end() : iter);
    const JobIndex startJob = iter->second;
    sortedJobs.erase(iter);
    coloring[startJob] = currentLayer;
    currentLayerStart = jobs[startJob].left;
    currentLayerEnd = jobs[startJob].right;
  }

  struct Hole
  {
    TimePoint left;
    TimePoint right;
    MemoryOffset layer;
  };
  FmemVector<Hole> holes; // stays on top of the framemem stack

  while (!sortedJobs.empty())
  {
    // We get the first job who's left end is to the right of current
    // layer area, i.e. first element of sortedJobs that's greater or
    // equal to currentLayerEnd, which is precisely what lower_bound does.
    const auto it = wraparound(sortedJobs.lower_bound(currentLayerEnd));
    const auto next = it->second;
    sortedJobs.erase(it);

    // If next job intersects the current layer, we move on to the next layer
    if (circular_arcs_intersect(currentLayerStart, currentLayerEnd, jobs[next].left, jobs[next].right, timepoint_count))
    {
      // But first, try to find a hole that fits
      auto holeIt = holes.begin();
      while (holeIt != holes.end() && circular_distance(holeIt->left, jobs[next].left, timepoint_count) +
                                          circular_length(jobs[next].left, jobs[next].right, timepoint_count) +
                                          circular_distance(jobs[next].right, holeIt->right, timepoint_count) >
                                        circular_length(holeIt->left, holeIt->right, timepoint_count))
        ++holeIt;

      if (holeIt != holes.end())
      {
        // We can reuse the hole!
        coloring[next] = holeIt->layer;
        holeIt->left = jobs[next].right;
        if (holeIt->left == holeIt->right)
          holes.erase(holeIt);
      }
      else
      {
        // Tough luck, move onto the next layer
        if (currentLayerStart != currentLayerEnd)
          holes.push_back(Hole{currentLayerEnd, currentLayerStart, currentLayer});
        coloring[next] = ++currentLayer;
        currentLayerStart = jobs[next].left;
        currentLayerEnd = jobs[next].right;
      }
    }
    else
    {
      coloring[next] = currentLayer;
      currentLayerEnd = jobs[next].right;
    }
  }

  return coloring;
}

} // namespace

#ifdef ENABLE_UNIT_TESTS

FmemVector<Job> gen_random_cyclic_jobs(uint32_t count, uint32_t max_time)
{
  FmemVector<Job> result;
  result.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
  {
    const TimePoint l = dagor_random::rnd_int(0, max_time - 1);
    const TimePoint r = dagor_random::rnd_int(0, max_time - 1);
    result.push_back(Job{l, r});
  }

  return result;
}

SUITE(CircularArcGraphColoring)
{
  TEST(Correctness)
  {
    dagor_random::set_rnd_seed(42);
    for (uint32_t i = 0; i < 100; ++i)
    {
      const TimeInterval maxTime = dagor_random::rnd_int(1, 100);
      const JobCount jobCount = dagor_random::rnd_int(0, 1000);

      const auto jobs = gen_random_cyclic_jobs(jobCount, maxTime);

      const auto loads = calculate_loads(jobs, maxTime);
      const TimePoint maxLoadPoint = eastl::max_element(loads.begin(), loads.end()) - loads.begin();
      const auto coloring = color_circular_arc_graph(jobs, maxTime, maxLoadPoint);

      MemoryOffset colorCount = 0;
      for (JobIndex i = 0; i < jobs.size(); ++i)
      {
        colorCount = eastl::max(colorCount, coloring[i]);
        for (JobIndex j = i + 1; j < jobs.size(); ++j)
          if (coloring[i] == coloring[j])
            CHECK(!circular_arcs_intersect(jobs[i].left, jobs[i].right, jobs[j].left, jobs[j].right, maxTime));
      }
      if (!coloring.empty())
        ++colorCount;

      const auto load = loads[maxLoadPoint];
      // Resulting coloring should be no farther away from load than load itself
      CHECK_CLOSE(load, colorCount, load);
    }
  }
}

#endif

namespace
{

// Traditional greedy interval graph coloring algorithm, generalized a bit
// to work with arcs. zero_load_point has to be the point on the circle
// that is not intersected by any arc, making the intersection graph an
// interval one.
// TIME COMPLEXITY: O(n log Chi), where n is job count and Chi is color count
FmemVector<MemoryOffset> color_interval_graph(eastl::span<const Job> jobs, TimeInterval timepoint_count, TimePoint zero_load_point)
{
  static constexpr MemoryOffset NO_COLOR = static_cast<MemoryOffset>(-1);
  FmemVector<MemoryOffset> coloring(jobs.size(), NO_COLOR);

  FRAMEMEM_VALIDATE;

  FmemVector<JobIndex> sortedJobs(jobs.size());
  eastl::iota(sortedJobs.begin(), sortedJobs.end(), 0);
  stlsort::sort(sortedJobs.begin(), sortedJobs.end(), //
    [&jobs](JobIndex fst, JobIndex snd) { return jobs[fst].left < jobs[snd].left; });
  const auto startIt = eastl::upper_bound(sortedJobs.begin(), sortedJobs.end(), zero_load_point,
    [&jobs](TimePoint s, JobIndex idx) { return s < jobs[idx].left; });
  eastl::rotate(sortedJobs.begin(), startIt, sortedJobs.end());

  const auto fixLeft = [zero_load_point, timepoint_count, &jobs](JobIndex i) -> TimePoint //
  { return (jobs[i].left < zero_load_point) * timepoint_count + jobs[i].left - zero_load_point; };
  const auto fixRight = [zero_load_point, timepoint_count, &jobs](JobIndex i) -> TimePoint //
  { return (jobs[i].right <= zero_load_point) * timepoint_count + jobs[i].right - zero_load_point; };

  // Stays on top of framemem stack, so will be blazingly fast
  FmemPrioQueue<eastl::pair<TimePoint, MemoryOffset>, eastl::greater<>> currentColoring;
  for (const auto jobIdx : sortedJobs)
  {
    // Try to use the color that hasn't been used for the longest, i.e. top of the heap
    if (!currentColoring.empty() && currentColoring.top().first <= fixLeft(jobIdx))
    {
      coloring[jobIdx] = currentColoring.top().second;
      currentColoring.pop();
    }
    else
    {
      coloring[jobIdx] = currentColoring.size();
    }

    currentColoring.push({fixRight(jobIdx), coloring[jobIdx]});
  }

  return coloring;
}

} // namespace

#ifdef ENABLE_UNIT_TESTS

FmemVector<Job> gen_random_separable_jobs(uint32_t count, uint32_t max_time)
{
  const auto offset = dagor_random::rnd_int(0, max_time - 1);
  const auto wrapOffset = [max_time, offset](TimePoint x) //
  { return x + offset - (x + offset >= max_time) * max_time; };

  FmemVector<Job> result;
  result.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
  {
    const auto right = dagor_random::rnd_int(1, max_time - 1);
    const auto left = dagor_random::rnd_int(0, right - 1);
    result.push_back(Job{wrapOffset(left), wrapOffset(right)});
  }

  return result;
}

SUITE(IntervalGraphColoring)
{
  TEST(Correctness)
  {
    dagor_random::set_rnd_seed(42);
    for (uint32_t i = 0; i < 100; ++i)
    {
      const TimeInterval maxTime = dagor_random::rnd_int(1, 100);
      const JobCount jobCount = dagor_random::rnd_int(0, 1000);

      const auto jobs = gen_random_separable_jobs(jobCount, maxTime);

      const auto loads = calculate_loads(jobs, maxTime);
      const auto zeroLoadPoint = eastl::find(loads.begin(), loads.end(), 0) - loads.begin();
      G_ASSERT(zeroLoadPoint != maxTime);
      const auto coloring = color_interval_graph(jobs, maxTime, zeroLoadPoint);

      MemoryOffset colorCount = 0;
      for (JobIndex i = 0; i < jobs.size(); ++i)
      {
        colorCount = eastl::max(colorCount, coloring[i]);
        for (JobIndex j = i + 1; j < jobs.size(); ++j)
          if (coloring[i] == coloring[j])
            CHECK(!circular_arcs_intersect(jobs[i].left, jobs[i].right, jobs[j].left, jobs[j].right, maxTime));
      }
      ++colorCount;

      const auto load = *eastl::max_element(loads.begin(), loads.end());
      // Resulting coloring should be optimal
      CHECK_EQUAL(load, colorCount);
    }
  }
}

#endif


namespace
{

struct BoxEmbedding
{
  BoxIndex box;
  MemoryOffset offset;
};

// For a set of unit-height jobs `jobs`, boxes them into a set of boxes
// (each box with height box_height), returning box index for each job,
// The resulting boxing is guaranteed to waste
// <= 4 * eps * LOAD(jobs, u)
//    + O( box_height * log2(box_height) / (eps*eps) * log2(1 / eps))
// space at each time moment u.
//
// Note that this class has a funky order of fields/methods due to
// encapsulating an algorithm. This is intentional, it's easier to read
// this way.
class UnitJobBoxer
{
  const eastl::span<const Job> jobs;
  const TimeInterval timepointCount;
  const MemorySize boxHeight;
  const MemorySize invEps;

  FmemVector<BoxEmbedding> boxing;
  BoxIndex boxCounter = 0;


  static constexpr BoxIndex NOT_YET_BOXED = static_cast<BoxIndex>(-1);

public:
  UnitJobBoxer(eastl::span<const Job> in_jobs, TimeInterval in_timepoint_count, MemorySize in_box_height, MemorySize in_inv_eps) :
    jobs{in_jobs},
    timepointCount{in_timepoint_count},
    boxHeight{in_box_height},
    invEps{in_inv_eps},
    boxing(jobs.size(), {NOT_YET_BOXED, 0})
  {
    G_ASSERTF(boxHeight > 0 && invEps > 0, "Precondition failed! World is broken!");
  }


private:
  // See the article to understand where these magic expressions come from.
  // Both free space count and critical point count are guaranteed to be O(PBound) at all times.
  // NOTE: we use log2(x + 1) here to avoid this becoming zero, which would ruin all our preallocation tricks
  const uint64_t PBound = static_cast<uint64_t>(ceil(boxHeight * log2(boxHeight + 1) * invEps * invEps * log2(invEps + 1)));
  const uint32_t jobCount = static_cast<uint32_t>(jobs.size());
  // There cannot be more crit points than job endpoint count + 2.
  // The constant 3 is taken from corollary 4 in the article, which proves
  // that crit points count <= 3*PBound by induction.
  const uint32_t boundForCritPoints = static_cast<uint32_t>(eastl::min<uint64_t>(3 * PBound, 2 * jobCount + 2));
  // There cannot be more free spaces than job count times 3, as worst
  // case is 1 job per 1 box, which will lead to 3 holes in each box.
  // The constant 6 here is taken from lemma 7 in the article:
  // 2 from proposition (1) plus 4 from proposition (2).
  const uint32_t boundForFreeSpaces = static_cast<uint32_t>(eastl::min<uint64_t>(6 * PBound, 3 * jobCount));
  // Unresolved job count is bounded by job count, as well as
  // the other peculiar expression (explicitly proven in lemma 5).
  const uint32_t boundForUnresolvedJobs = //
    static_cast<uint32_t>(eastl::min<uint64_t>(2 * boxHeight * invEps * invEps, jobCount));

  // General segment, not necessarily a job
  struct Segment
  {
    TimePoint left, right;

    // Important: this does not include end points!
    bool openIntervalContains(TimePoint x) const { return (left < x && x < right) || (left >= right && (x < right || left < x)); }

    friend __forceinline bool operator==(const Segment &fst, const Segment &snd)
    {
      return fst.left == snd.left && fst.right == snd.right;
    }
    friend __forceinline bool operator!=(const Segment &fst, const Segment &snd) { return !(fst == snd); };
  };

  // A hole of height `height` and horizontal bounds [left, right) in box `where.box` at offset `where.offset`
  struct Hole
  {
    Segment area;
    BoxEmbedding where;
    MemorySize height;
  };

  // Invariant: crit points always go in clockwise order, starting and ending
  // in the "border" around the current iteration's working are. Furthermore,
  // all free spaces are allowed to begin/end only in crit points.
  struct IterationArguments
  {
    // We store internal critical points implicitly inside the freeSpaces,
    // except for the leftest/rightest ones, these are the work area for
    // this iteration. Read the paper thoroughly to understand this.
    Segment workArea;

    // Holes in boxes allocated on previous iterations
    // TODO: replacing Hole::height with a "hole->height" map here might improve perf
    FmemVector<Hole> freeSpaces;

    // Jobs alive within the workArea
    FmemVector<JobIndex> jobIdxs;
  };

  auto choseJobs(eastl::span<const JobIndex> which) const
  {
    FmemVector<Job> result;
    result.reserve(which.size());
    for (auto idx : which)
      result.push_back(jobs[idx]);
    return result;
  };

  auto reconstructCriticalPoints(IterationArguments &args) const
  {
    FmemVector<TimePoint> result;
    result.reserve(2 * args.freeSpaces.size() + 2 + timepointCount);

    FRAMEMEM_VALIDATE;

    const auto currJobs = choseJobs(args.jobIdxs);
    // guaranteed to be non-zero somewhere in the work area
    const auto loads = calculate_loads(currJobs, timepointCount);

    // We want to skip a bunch of zeros at the beginning and end of work area.
    // Shrinking work are requires shrinking holes too.
    {
      const auto next = [this](TimePoint t) { return t + 1 == timepointCount ? 0 : t + 1; };
      const auto prev = [this](TimePoint t) { return t == 0 ? timepointCount - 1 : t - 1; };

      // This is correct due to loads not being 0 at some point in work area.
      TimePoint realStart = args.workArea.left;
      while (loads[realStart] == 0)
        realStart = next(realStart);
      TimePoint realEnd = args.workArea.right;
      while (loads[prev(realEnd)] == 0)
        realEnd = prev(realEnd);
      const auto leftPadding = circular_length(args.workArea.left, realStart, timepointCount);
      const auto rightPadding = circular_length(realEnd, args.workArea.right, timepointCount);
      // Holes that are completely outside the new work are need to be thrown out
      args.freeSpaces.erase(eastl::remove_if(args.freeSpaces.begin(), args.freeSpaces.end(),
                              [&](const Hole &hole) {
                                return circular_distance(args.workArea.left, hole.area.right, timepointCount) <= leftPadding ||
                                       circular_distance(hole.area.left, args.workArea.right, timepointCount) <= rightPadding;
                              }),
        args.freeSpaces.end());
      for (auto &hole : args.freeSpaces)
      {
        if (circular_distance(args.workArea.left, hole.area.left, timepointCount) <= leftPadding)
          hole.area.left = realStart;
        if (circular_distance(hole.area.right, args.workArea.right, timepointCount) <= rightPadding)
          hole.area.right = realEnd;
      }
      args.workArea.left = realStart;
      args.workArea.right = realEnd;
    }

    FRAMEMEM_VALIDATE;

    for (const auto &hole : args.freeSpaces)
    {
      result.push_back(hole.area.left);
      result.push_back(hole.area.right);
    }
    result.push_back(args.workArea.left);
    result.push_back(args.workArea.right);

    // This is a hack. Consider the following load plot:
    // ███    A
    // ██████_____███
    // We want to add a crit point at A in order to split the work area
    // on next iterations into load-connected components.
    {
      const auto offset = [this, r = args.workArea.left](TimePoint p) { //
        return circular_offset(p, r, timepointCount);
      };
      const TimeInterval steps = circular_length(args.workArea.left, args.workArea.right, timepointCount);
      for (TimePoint i = 0; i < steps - 1; ++i)
        if (const auto next = offset(i + 1); loads[offset(i)] > 0 && loads[next] == 0)
          result.push_back(next);
    }

    stlsort::sort(result.begin(), result.end());
    result.erase(eastl::unique(result.begin(), result.end()), result.end());

    const auto start = eastl::binary_search_i(result.begin(), result.end(), args.workArea.left);
    eastl::rotate(result.begin(), start, result.end());

    // Edge-case on the very first iteration when we've broken the circle
    if (args.workArea.left == args.workArea.right)
      result.push_back(args.workArea.right);

    return result;
  }

  static __forceinline bool jobAliveAt(Job job, TimePoint x)
  {
    return (job.left <= x && x < job.right) || (job.left >= job.right && (x < job.right || job.left <= x));
  }

  void boxAliveAtT(const eastl::span<const JobIndex> alive_job_idxs, const TimePoint crit_point, FmemVector<JobIndex> &out_unresolved)
  {
    const auto alive = choseJobs(alive_job_idxs);
    const auto subBoxing = box_unit_jobs_all_alive_at_t(alive, crit_point, timepointCount, boxHeight, invEps);
    const auto boxCount = size_of_mapping<BoxIndex>(subBoxing, UNRESOLVED_BOX_ID);

    FmemVector<MemorySize> perBoxOffsets(boxCount, 0);

    for (JobIndex idxInAlive = 0; idxInAlive < alive.size(); ++idxInAlive)
      if (subBoxing[idxInAlive] != UNRESOLVED_BOX_ID)
        boxing[alive_job_idxs[idxInAlive]] = BoxEmbedding{boxCounter + subBoxing[idxInAlive], perBoxOffsets[subBoxing[idxInAlive]]++};
      else
        out_unresolved.push_back(alive_job_idxs[idxInAlive]);
    boxCounter += boxCount;
  }

  // Resolves most jobs that intersect the critical points of the current
  // iteration, splitting other jobs between next iteration arguments.
  // Returns the set of unresolved job indices.
  auto resolveCritPointIntersectors(const eastl::span<const TimePoint> criticalPoints, const IterationArguments &args,
    const eastl::span<IterationArguments> nextArgs)
  {
    G_ASSERT(criticalPoints.size() == nextArgs.size() + 1);

    FmemVector<JobIndex> unresolvedIdxs;
    // At most boundForUnresolvedJobs will come from every crit point,
    // and crit points are bounded by 2 + jobcount*2
    unresolvedIdxs.reserve(boundForUnresolvedJobs * (2 + 2 * args.jobIdxs.size()));

    FRAMEMEM_VALIDATE;

    const auto midpoint = [](auto left, auto right) { return left + (right - left) / 2; };

    FmemVector<eastl::pair<uint32_t, uint32_t>> critPointStack;
    critPointStack.reserve(criticalPoints.size());
    critPointStack.push_back({0, criticalPoints.size() - 1});
    nextArgs[0].jobIdxs.assign(args.jobIdxs.begin(), args.jobIdxs.end());
    // Pack intersectors of critical points in binary-search-like order
    while (!critPointStack.empty())
    {
      auto [l, r] = critPointStack.back();

      const auto m = midpoint(l, r);
      critPointStack.pop_back();
      if (m - l > 1)
        critPointStack.push_back({l, m});
      if (r - m > 1)
        critPointStack.push_back({m, r});
      const TimePoint currCritPoint = criticalPoints[m];


      auto &currJobs = nextArgs[l].jobIdxs;
      {
        const auto aliveStart = eastl::partition(currJobs.begin(), currJobs.end(),
          [this, currCritPoint](JobIndex j) { return !jobAliveAt(jobs[j], currCritPoint); });

        boxAliveAtT({aliveStart, currJobs.end()}, currCritPoint, unresolvedIdxs);
        currJobs.erase(aliveStart, currJobs.end());
      }

      const auto distFromL = [this, p1 = criticalPoints[l]](TimePoint p2) { //
        return circular_length(p1, p2, timepointCount);
      };

      const auto rightStart = eastl::partition(currJobs.begin(), currJobs.end(),
        [this, distFromL, separator = distFromL(currCritPoint)](JobIndex j) { return distFromL(jobs[j].right) <= separator; });

      eastl::move(rightStart, currJobs.end(), eastl::back_insert_iterator(nextArgs[m].jobIdxs));
      currJobs.erase(rightStart, currJobs.end());
    }

    return unresolvedIdxs;
  }

  auto boxUnresolvedJobs(FmemVector<Hole> &free_spaces, Segment work_area, const eastl::span<const JobIndex> unresolved_idxs,
    const eastl::span<const MemoryOffset> coloring)
  {
    const MemorySize colorCount = size_of_mapping<MemoryOffset>(coloring);

    FmemVector<Hole> holes;
    // Not sure whether we can use PBound here, but this amount is definitely enough
    // (every unresolved job might add a hole to the left of it, as well
    // as a single job in each color to the right. Then we'll have previous free spaces
    // returned as holes, and finally 1 more hole to fill the last box)
    holes.reserve(unresolved_idxs.size() + coloring.size() + free_spaces.size() + 1);

    FRAMEMEM_VALIDATE;

    FmemVector<JobCount> perColorJobCount(colorCount, 0);
    for (uint32_t idxInUnresolved = 0; idxInUnresolved < unresolved_idxs.size(); ++idxInUnresolved)
      ++perColorJobCount[coloring[idxInUnresolved]];

    FmemBucketVector<JobIndex> bucketedUnresolvedIdxs(colorCount);
    for (uint32_t color = 0; color < colorCount; ++color)
      bucketedUnresolvedIdxs[color].reserve(perColorJobCount[color]);

    for (uint32_t idxInUnresolved = 0; idxInUnresolved < unresolved_idxs.size(); ++idxInUnresolved)
      bucketedUnresolvedIdxs[coloring[idxInUnresolved]].push_back(unresolved_idxs[idxInUnresolved]);

    for (auto &bucket : bucketedUnresolvedIdxs)
    {
      stlsort::sort(bucket.begin(), bucket.end(), //
        [this](JobIndex fst, JobIndex snd) { return jobs[fst].left < jobs[snd].left; });
      const auto firstIt = eastl::lower_bound(bucket.begin(), bucket.end(), work_area.left, //
        [this](JobIndex j, TimePoint t) { return jobs[j].left < t; });
      eastl::rotate(bucket.begin(), firstIt, bucket.end());
    }

    const auto packLayerIntoBox =                        //
      [this, &bucketedUnresolvedIdxs, &holes, work_area] //
      (uint32_t color, BoxEmbedding where) {
        const auto &bucket = bucketedUnresolvedIdxs[color];

        for (auto i : bucket)
          boxing[i] = where;

        const auto tryAdd = [&holes, &where](TimePoint left, TimePoint right) {
          if (left != right)
            holes.push_back(Hole{{left, right}, where, 1});
        };

        tryAdd(work_area.left, jobs[bucket.front()].left);
        for (uint32_t i = 0; i < bucket.size() - 1; ++i)
          tryAdd(jobs[bucket[i]].right, jobs[bucket[i + 1]].left);
        tryAdd(jobs[bucket.back()].right, work_area.right);
      };

    const auto isNotSpanning = [work_area](const Hole &hole) { //
      return hole.area != work_area;
    };
    const auto startOfSpanningHoles = eastl::partition(free_spaces.begin(), free_spaces.end(), isNotSpanning);

    // Prioritize placing unresolved jobs at low offsets. Holes that span
    // an entire box will be discarded later, this ensures that they stay box-spanning.
    stlsort::sort(startOfSpanningHoles, free_spaces.end(),
      [](const Hole &fst, const Hole &snd) { return fst.where.offset > snd.where.offset; });

    const MemorySize spanningHoleCount = eastl::accumulate(startOfSpanningHoles, free_spaces.end(), MemorySize{0},
      [](MemorySize curr, const Hole &hole) { return curr + hole.height; });
    const MemorySize layersToPackIntoHoles = eastl::min(colorCount, spanningHoleCount);

    for (uint32_t color = 0; color < layersToPackIntoHoles; ++color)
    {
      auto &space = free_spaces.back();
      packLayerIntoBox(color, space.where);
      ++space.where.offset;
      if (--space.height == 0)
        free_spaces.pop_back();
    }
    const MemorySize layersToPackIntoNewBoxes = colorCount - layersToPackIntoHoles;
    MemorySize lastBoxFullness = 0;
    for (uint32_t i = 0; i < layersToPackIntoNewBoxes; ++i)
    {
      packLayerIntoBox(layersToPackIntoHoles + i, BoxEmbedding{boxCounter, lastBoxFullness});

      if (++lastBoxFullness >= boxHeight)
      {
        lastBoxFullness = 0;
        ++boxCounter;
      }
    }

    // If we created a new box but didn't completely fill it, need
    // to remember the holes that come from this empty space.
    if (0 < lastBoxFullness)
    {
      holes.push_back(Hole{work_area, {boxCounter, lastBoxFullness}, boxHeight - lastBoxFullness});
      ++boxCounter;
    }

    return holes;
  }

  void distributeHolesToNextIterations(const eastl::span<const TimePoint> criticalPoints,
    const eastl::span<IterationArguments> nextArgs, FmemVector<Hole> &&holes) const // taking holes by ref is
                                                                                    // important here
  {
    FRAMEMEM_VALIDATE;

    const bool critsAreCyclic = criticalPoints.front() == criticalPoints.back();
    eastl::span<const TimePoint> crits(criticalPoints.data(), criticalPoints.size() - (critsAreCyclic ? 1 : 0));

    FmemVectorMap<TimePoint, decltype(crits)::const_iterator> critPointLookup;
    critPointLookup.reserve(crits.size() + 1);
    for (auto it = crits.begin(); it != crits.end(); ++it)
      critPointLookup.emplace(*it, it);
    critPointLookup.emplace(timepointCount, eastl::min_element(crits.begin(), crits.end()));
    const auto lookup = [&critPointLookup](TimePoint t) { return critPointLookup.upper_bound(t)->second; };

    const auto nextIt = [&crits, critsAreCyclic](auto it) {
      const bool onLastElement = it + 1 == crits.end();
      G_UNUSED(critsAreCyclic);
      G_ASSERT(critsAreCyclic || !onLastElement);
      return onLastElement ? crits.begin() : it + 1;
    };

    const auto prevIt = [&crits, critsAreCyclic](auto it) {
      const bool onFirstElement = it == crits.begin();
      G_UNUSED(critsAreCyclic);
      G_ASSERT(critsAreCyclic || !onFirstElement);
      return onFirstElement ? crits.end() - 1 : it - 1;
    };

    // Distribute holes between new iterations
    while (!holes.empty())
    {
      auto &hole = holes.back();

      auto it = lookup(hole.area.left);
      while (hole.area.openIntervalContains(*it))
      {
        nextArgs[prevIt(it) - crits.begin()].freeSpaces.push_back(
          Hole{Segment{eastl::exchange(hole.area.left, *it), *it}, hole.where, hole.height});

        it = nextIt(it);
      }
      nextArgs[prevIt(it) - crits.begin()].freeSpaces.push_back(hole);

      holes.pop_back();
    }
  }

  TimePoint minLoadPoint(const eastl::span<const Job> chosen_jobs) const
  {
    const auto loads = calculate_loads(chosen_jobs, timepointCount);
    return static_cast<TimePoint>(eastl::min_element(loads.begin(), loads.end()) - loads.begin());
  }

  TimePoint maxLoadPoint(const eastl::span<const Job> chosen_jobs) const
  {
    const auto loads = calculate_loads(chosen_jobs, timepointCount);
    return static_cast<TimePoint>(eastl::max_element(loads.begin(), loads.end()) - loads.begin());
  }

public:
  eastl::span<const BoxEmbedding> operator()()
  {
    FRAMEMEM_VALIDATE;

    // This is an damn fine hack that allows to preserve framemem_allocator
    // state between iterations and not thrash that stack. The invariant
    // here is that allocations inside iteration arguments for the "next"
    // iteration are always at the top of the framemem stack.
    IterationArguments args;
    args.freeSpaces.reserve(boundForFreeSpaces);
    args.jobIdxs.reserve(jobs.size());
    FmemVector<TimePoint> criticalPoints;
    criticalPoints.reserve(boundForCritPoints);

    eastl::stack<IterationArguments, FmemVector<IterationArguments>> iterationStack;
    // Reserve absolute worst case amount of space in the stack -- guarantees no relocations
    iterationStack.get_container().reserve(2 * jobCount);

    // First iteration of the algorithm is manual and special, it splits the circle.
    {
      // If it turns out that load is 0 at this point, we won't do anything
      // in this 0th iteration and simply go on to work exactly as described
      // in the article.
      const auto initialCritPoint = minLoadPoint(jobs);

      auto &initialArgs = iterationStack.emplace();
      // reserved data should be on top of the framemem stack when iterations start
      initialArgs.freeSpaces.reserve(boundForFreeSpaces);
      initialArgs.jobIdxs.reserve(jobs.size());

      FRAMEMEM_VALIDATE;

      initialArgs.workArea = Segment{initialCritPoint, initialCritPoint};

      initialArgs.jobIdxs.resize(jobs.size());
      eastl::iota(initialArgs.jobIdxs.begin(), initialArgs.jobIdxs.end(), 0);

      const auto startOfAlive = eastl::partition(initialArgs.jobIdxs.begin(), initialArgs.jobIdxs.end(),
        [this, initialCritPoint](JobIndex j) { return !jobAliveAt(jobs[j], initialCritPoint); });

      FmemVector<JobIndex> unresolvedIdxs;
      unresolvedIdxs.reserve(eastl::distance(startOfAlive, initialArgs.jobIdxs.end()));
      boxAliveAtT({startOfAlive, initialArgs.jobIdxs.end()}, initialCritPoint, unresolvedIdxs);
      initialArgs.jobIdxs.erase(startOfAlive, initialArgs.jobIdxs.end());

      // We repeat logic of boxUnresolvedJobs a little bit here, as it's different
      // enough that trying to generalize will only make it worse
      MemorySize lastBoxFullness = 0;
      for (JobIndex i : unresolvedIdxs)
      {
        boxing[i] = {boxCounter, lastBoxFullness};
        if (jobs[i].right != jobs[i].left)
          initialArgs.freeSpaces.push_back(Hole{{jobs[i].right, jobs[i].left}, {boxCounter, lastBoxFullness}, 1});

        if (++lastBoxFullness >= boxHeight)
        {
          lastBoxFullness = 0;
          ++boxCounter;
        }
      }

      if (0 < lastBoxFullness)
      {
        initialArgs.freeSpaces.push_back(Hole{initialArgs.workArea, {boxCounter, lastBoxFullness}, boxHeight - lastBoxFullness});
        ++boxCounter;
      }
    }

    while (!iterationStack.empty())
    {
      // Yes, copy, this is intentional.
      args = iterationStack.top();
      // Here, we destroy the allocations inside the iterationStack.top(),
      // which is at the top of the framemem_stack currently.
      iterationStack.pop();

      if (args.jobIdxs.empty())
        continue;

      {
        const auto cps = reconstructCriticalPoints(args);
        criticalPoints.assign(cps.begin(), cps.end());
      }

      // Edge-case: after reconstructing CPs, we might have sub-iterations
      // where there are no intermediate crit points. Need to find a new
      // intermediate crit point in such a case. Note that the way we
      // reconstruct CPs guarantees that the midpoint will be contained
      // in a job.
      if (criticalPoints.size() == 2)
        criticalPoints.insert(criticalPoints.begin() + 1, circular_midpoint(args.workArea.left, args.workArea.right, timepointCount));

      // We now put preemptively reserve data for new iterations on top
      // of the framemem_stack. After this iteration completes, the data
      // for the new iteration will be at the top of the stack, preserving
      // the invariant.
      const uint32_t iterationsToLaunch = criticalPoints.size() - 1;
      for (uint32_t i = 0; i < iterationsToLaunch; ++i)
      {
        auto &nextArgs = iterationStack.emplace();
        nextArgs.workArea = Segment{criticalPoints[i], criticalPoints[i + 1]};
        nextArgs.freeSpaces.reserve(boundForFreeSpaces);
        nextArgs.jobIdxs.reserve(args.jobIdxs.size());
      }

      FRAMEMEM_VALIDATE;

      const eastl::span<IterationArguments> nextArgs(iterationStack.get_container().end() - iterationsToLaunch,
        iterationStack.get_container().end());

      // Step 1: box most jobs that cross critical points using a simple algorithm,
      // but do it in a binary search fashion. What couldn't be boxed is called "unresolved"
      const auto unresolvedIdxs = resolveCritPointIntersectors(criticalPoints, args, nextArgs);

      // Step 2: color jobs that couldn't be boxed on the previous iteration with IGC.
      const auto unresolved = choseJobs(unresolvedIdxs);
      const auto coloring = color_interval_graph(unresolved, timepointCount, minLoadPoint(unresolved));

      // Step 3: box the unresolved jobs into spanning freeSpaces that come from previous
      // iterations. If we don't have enough spanning free spaces, we create brand-new boxes
      // and split them into spanning free spaces for use here.
      // This results in some new holes, as the packing into free is not necessarily tight.
      auto holes = boxUnresolvedJobs(args.freeSpaces, args.workArea, unresolvedIdxs, coloring);

      // If any free space remains, we forward it to next iteration as holes.
      holes.insert(holes.end(), args.freeSpaces.begin(), args.freeSpaces.end());

      // Step 4: "cut" holes up using critical points as delimiters and
      // distribute them to next iterations, who's work areas will be delimited
      // exactly by the current critical points.
      distributeHolesToNextIterations(criticalPoints, nextArgs, eastl::move(holes));
    }

    return boxing;
  }
};

} // namespace

#ifdef ENABLE_UNIT_TESTS

SUITE(BoxUnitJobs)
{
  void check(eastl::span<const Job> jobs, TimeInterval timepoint_count, eastl::span<const BoxEmbedding> boxing, MemorySize box_height,
    float eps)
  {
    for (const auto &emb : boxing)
    {
      REQUIRE CHECK(emb.box != UNRESOLVED_BOX_ID);
      CHECK(emb.offset < box_height);
    }

    FmemVector<BoxIndex> onlyBoxIndices;
    onlyBoxIndices.reserve(boxing.size());
    for (const auto &emb : boxing)
      onlyBoxIndices.push_back(emb.box);
    const auto boxJobs = boxes_to_jobs(jobs, timepoint_count, onlyBoxIndices);

    const auto boxLoads = calculate_loads(boxJobs, timepoint_count);
    const auto initialLoads = calculate_loads(jobs, timepoint_count);

    // Per theorem 2, resulting box load should not at any time exceed
    // (1+ 4*eps)*L + O(H lg H * 1/eps^2 * lg 1/eps),
    // where H is box_height and L is the original load at the same point.
    // The constant inside O should be somewhere around 6, I think.
    const float error = box_height * log2(box_height + 1) / (eps * eps) * log2(1.f / eps);
    const float constant = 6;
    for (TimePoint t = 0; t < timepoint_count; ++t)
      CHECK(boxLoads[t] <= (1 + 4 * eps) * initialLoads[t] + constant * error);

    FmemVector<JobCount> boxSizes(boxJobs.size(), 0);
    for (JobIndex job = 0; job < jobs.size(); ++job)
      ++boxSizes[boxing[job].box];

    FmemBucketVector<JobIndex> boxedJobs(boxJobs.size());
    for (BoxIndex box = 0; box < boxedJobs.size(); ++box)
      boxedJobs[box].reserve(boxSizes[box]);
    for (JobIndex job = 0; job < jobs.size(); ++job)
      boxedJobs[boxing[job].box].push_back(job);

    for (BoxIndex box = 0; box < boxedJobs.size(); ++box)
    {
      const auto &jobsInBox = boxedJobs[box];

      // Check that the chosen offsets within each box lead to a proper coloring within the box
      for (auto i = jobsInBox.begin(); i != jobsInBox.end(); ++i)
        for (auto j = i + 1; j != jobsInBox.end(); ++j)
          if (boxing[*i].offset == boxing[*j].offset)
            CHECK(!circular_arcs_intersect(jobs[*i].left, jobs[*i].right, jobs[*j].left, jobs[*j].right, timepoint_count));
    }
  }

  TEST(SameJobsSmallEps)
  {
    constexpr TimeInterval timepoint_count = 2;
    constexpr TimeInterval box_height = 5;
    constexpr float eps = 0.0001;
    FmemVector<Job> jobs(10, Job{0, 1});
    UnitJobBoxer boxer(jobs, timepoint_count, box_height, static_cast<MemorySize>(ceil(1.f / eps)));
    const auto boxing = boxer();
    check(jobs, timepoint_count, boxing, box_height, eps);
  }

  TEST(SameJobsBigEps)
  {
    constexpr TimeInterval timepoint_count = 2;
    constexpr TimeInterval box_height = 5;
    constexpr float eps = 0.5;
    FmemVector<Job> jobs(100, Job{0, 1});
    UnitJobBoxer boxer(jobs, timepoint_count, box_height, static_cast<MemorySize>(ceil(1.f / eps)));
    const auto boxing = boxer();
    check(jobs, timepoint_count, boxing, box_height, eps);
  }

  TEST(SameJobsWraparound)
  {
    constexpr TimeInterval timepoint_count = 2;
    constexpr TimeInterval box_height = 5;
    constexpr float eps = 0.5;
    FmemVector<Job> jobs(10, Job{1, 0});
    UnitJobBoxer boxer(jobs, timepoint_count, box_height, static_cast<MemorySize>(ceil(1.f / eps)));
    const auto boxing = boxer();
    check(jobs, timepoint_count, boxing, box_height, eps);
  }

  TEST(SameJobsAlwaysAlive)
  {
    constexpr TimeInterval timepoint_count = 2;
    constexpr TimeInterval box_height = 5;
    constexpr float eps = 0.5;
    FmemVector<Job> jobs(10, Job{0, 0});
    UnitJobBoxer boxer(jobs, timepoint_count, box_height, static_cast<MemorySize>(ceil(1.f / eps)));
    const auto boxing = boxer();
    check(jobs, timepoint_count, boxing, box_height, eps);
  }

  TEST(PeskySnake)
  {
    constexpr TimeInterval timepoint_count = 100;
    constexpr TimeInterval box_height = 5;
    constexpr float eps = 0.7;
    FmemVector<Job> jobs;
    for (JobIndex i = 0; i < timepoint_count; ++i)
      jobs.push_back(Job{i, i});
    UnitJobBoxer boxer(jobs, timepoint_count, box_height, static_cast<MemorySize>(ceil(1.f / eps)));
    const auto boxing = boxer();
    check(jobs, timepoint_count, boxing, box_height, eps);
  }

  TEST(Empty)
  {
    constexpr TimeInterval timepoint_count = 100;
    constexpr TimeInterval box_height = 5;
    constexpr float eps = 0.7;
    FmemVector<Job> jobs;
    UnitJobBoxer boxer(jobs, timepoint_count, box_height, static_cast<MemorySize>(ceil(1.f / eps)));
    const auto boxing = boxer();
    check(jobs, timepoint_count, boxing, box_height, eps);
  }

  void random_test(int seed, JobCount job_count, TimePoint timepoint_count)
  {
    dagor_random::set_rnd_seed(seed);
    const MemorySize box_height = dagor_random::rnd_int(1, job_count - 1);
    const float eps = dagor_random::rnd_float(0.0001, 0.9999);
    const auto jobs = gen_random_cyclic_jobs(job_count, timepoint_count);
    UnitJobBoxer boxer(jobs, timepoint_count, box_height, static_cast<MemorySize>(ceil(1.f / eps)));
    const auto boxing = boxer();
    check(jobs, timepoint_count, boxing, box_height, eps);
  }

  TEST(SmallRandom1) { random_test(42, 1, 100); }
  TEST(SmallRandom2a) { random_test(42, 2, 100); }
  TEST(SmallRandom2b) { random_test(44, 2, 100); }
  TEST(SmallRandom3a) { random_test(42, 3, 100); }
  TEST(SmallRandom3b) { random_test(45, 3, 100); }
  TEST(SmallRandom4) { random_test(42, 4, 100); }
  TEST(SmallRandom5a) { random_test(42, 5, 100); }
  TEST(SmallRandom5b) { random_test(43, 5, 100); }
  TEST(SmallRandom5c) { random_test(44, 5, 100); }
  TEST(SmallRandom10a) { random_test(42, 10, 100); }
  TEST(SmallRandom10b) { random_test(777, 10, 100); }

  TEST(BigRandom)
  {
    for (uint32_t i = 0; i < 1000; ++i)
      random_test(777, 100, 100);
  }

  TEST(HugeRandom)
  {
    for (uint32_t i = 0; i < 100; ++i)
      random_test(777, 1000, 500);
  }
}

#endif

namespace
{

// For a set of jobs `jobs` with heights `job_heights`, boxes them into
// a set of boxes (each box with height `box_height`), returning box
// index and offset within that box for each resource.
// The resulting boxing is guaranteed to waste
// <= 9 * eps * LOAD(jobs, u)
//    + O( box_height * log2(box_height/min(job_heights))^2 / eps^4 )
// space at each time moment u.
FmemVector<BoxEmbedding> box_jobs(eastl::span<const Job> jobs, eastl::span<const MemorySize> job_heights, TimeInterval timepoint_count,
  MemorySize box_height, MemorySize inv_eps)
{
  G_ASSERT(jobs.size() == job_heights.size());
  FmemVector<BoxEmbedding> result(jobs.size());

  FRAMEMEM_VALIDATE;

  // We bucket the jobs by the integer part of their height logarithm base (1+eps)

  FmemVector<JobIndex> sortedJobs(jobs.size());
  eastl::iota(sortedJobs.begin(), sortedJobs.end(), 0);
  stlsort::sort(sortedJobs.begin(), sortedJobs.end(),
    [&job_heights](JobIndex fst, JobIndex snd) { return job_heights[fst] < job_heights[snd]; });

  // Pre-condition for the algorithm to be applicable.
  G_ASSERT((job_heights.empty() ? 0 : job_heights[sortedJobs.back()]) * inv_eps <= box_height);

  // Pre-compute floor((1+eps)^i) for all required powers i
  FmemVector<MemorySize> bucketBoundaries;
  {
    const float base = 1.f + 1.f / static_cast<float>(inv_eps);
    float current = 1;
    for (auto it = sortedJobs.begin(); it != sortedJobs.end();)
    {
      while (floor(current) < job_heights[*it])
        current *= base;
      bucketBoundaries.push_back(static_cast<MemorySize>(floor(current)));
      // Skip all jobs that fit into this bucket boundary
      while (it != sortedJobs.end() && job_heights[*it] <= floor(current))
        ++it;
    }
  }

  FmemVectorMap<MemorySize, JobCount> bucketCounts;
  {
    uint32_t bucketIndex = 0;
    for (auto jobIdx : sortedJobs)
    {
      while (bucketBoundaries[bucketIndex] < job_heights[jobIdx])
        ++bucketIndex;
      ++bucketCounts[bucketBoundaries[bucketIndex]];
    }
  }
  CleanupReversed<FmemVectorMap<MemorySize, FmemVector<JobIndex>>> bucketedJobIndxs;
  bucketedJobIndxs.reserve(bucketCounts.size());
  {
    for (auto [size, count] : bucketCounts)
      bucketedJobIndxs[size].reserve(count);

    uint32_t bucketIndex = 0;
    for (auto jobIdx : sortedJobs)
    {
      while (bucketBoundaries[bucketIndex] < job_heights[jobIdx])
        ++bucketIndex;
      bucketedJobIndxs[bucketBoundaries[bucketIndex]].push_back(jobIdx);
    }
  }

  const auto choseJobs = [&jobs](eastl::span<const JobIndex> which) {
    FmemVector<Job> result;
    result.reserve(which.size());
    for (auto idx : which)
      result.push_back(jobs[idx]);
    return result;
  };

  BoxIndex boxCounter = 0;
  for (const auto &[roundedJobHeight, jobIdxsInThisBucket] : bucketedJobIndxs)
  {
    const auto jobsInThisBucket = choseJobs(jobIdxsInThisBucket);

    // Edge-case: if we have a single job with height equal to box_height,
    // roundedJobHeight will be larger than box_height, so we'll get 0.
    // Use box-height 1 instead, as it's ok in this particular case.
    const auto boxHeight = box_height / roundedJobHeight;
    UnitJobBoxer boxer(jobsInThisBucket, timepoint_count, boxHeight > 0 ? boxHeight : 1, inv_eps);
    const auto bucketBoxing = boxer();

    for (JobIndex i = 0; i < jobIdxsInThisBucket.size(); ++i)
    {
      const auto jobIdx = jobIdxsInThisBucket[i];

      result[jobIdx] = BoxEmbedding{boxCounter + bucketBoxing[i].box, bucketBoxing[i].offset * roundedJobHeight};
    }

    BoxCount count = 0;
    for (auto [box, _] : bucketBoxing)
      count = eastl::max(count, box);
    if (!bucketBoxing.empty())
      ++count;

    boxCounter += count;
  }

  return result;
}

} // namespace


#ifdef ENABLE_UNIT_TESTS

SUITE(BoxJobs)
{
  void check(eastl::span<const Job> jobs, eastl::span<const MemorySize> job_heights, TimeInterval timepoint_count,
    eastl::span<const BoxEmbedding> boxing, MemorySize box_height, float eps)
  {
    for (const auto &emb : boxing)
      REQUIRE CHECK(emb.box != UNRESOLVED_BOX_ID);

    for (JobIndex i = 0; i < jobs.size(); ++i)
      CHECK(boxing[i].offset + job_heights[i] <= box_height);

    FmemVector<BoxIndex> onlyBoxIndices;
    onlyBoxIndices.reserve(boxing.size());
    for (const auto &emb : boxing)
      onlyBoxIndices.push_back(emb.box);
    const auto boxJobs = boxes_to_jobs(jobs, timepoint_count, onlyBoxIndices);

    const auto boxLoads = calculate_loads(boxJobs, timepoint_count);
    const auto initialLoads = calculate_loads(jobs, timepoint_count);

    // Per corollary 15, resulting box load should not at any time exceed
    // (1 + 9*eps)*L + O(H lg^2 (H/h_min) / eps^4),
    // where H is box_height, L is the original load at the same point,
    // and h_min is the minimal job height.
    // The constant inside O should be the same as in unit job boxing, but maybe a bit bigger?
    const float min_h = static_cast<float>(*eastl::min_element(job_heights.begin(), job_heights.end()));
    const float invEps = 1.f / eps;
    const float epsToMinus4Power = invEps * invEps * invEps * invEps;
    const float logTerm = log2(box_height / min_h);
    const float error = box_height * logTerm * logTerm * epsToMinus4Power;
    const float constant = 6;
    for (TimePoint t = 0; t < timepoint_count; ++t)
      CHECK(boxLoads[t] <= (1 + 9 * eps) * initialLoads[t] + constant * error);

    FmemVector<JobCount> boxSizes(boxJobs.size(), 0);
    for (JobIndex job = 0; job < jobs.size(); ++job)
      ++boxSizes[boxing[job].box];

    FmemBucketVector<JobIndex> boxedJobs(boxJobs.size());
    for (BoxIndex box = 0; box < boxedJobs.size(); ++box)
      boxedJobs[box].reserve(boxSizes[box]);
    for (JobIndex job = 0; job < jobs.size(); ++job)
      boxedJobs[boxing[job].box].push_back(job);

    for (BoxIndex box = 0; box < boxedJobs.size(); ++box)
    {
      const auto &jobsInBox = boxedJobs[box];

      REQUIRE CHECK(!jobsInBox.empty());

      // Check that the chosen offsets within each box lead to a proper coloring within the box
      for (auto i = jobsInBox.begin(); i != jobsInBox.end(); ++i)
        for (auto j = i + 1; j != jobsInBox.end(); ++j)
          if (boxing[*j].offset <= boxing[*i].offset && boxing[*i].offset < boxing[*j].offset + job_heights[*j] ||
              boxing[*i].offset <= boxing[*j].offset && boxing[*j].offset < boxing[*i].offset + job_heights[*i])
            CHECK(!circular_arcs_intersect(jobs[*i].left, jobs[*i].right, jobs[*j].left, jobs[*j].right, timepoint_count));
    }
  }

  FmemVector<MemorySize> gen_random_heights(JobCount count, MemorySize min, MemorySize max)
  {
    FmemVector<MemorySize> result(count);
    for (auto &val : result)
      val = dagor_random::rnd_int(min, max);
    return result;
  }

  void random_test(int seed, JobCount job_count, TimePoint timepoint_count)
  {
    dagor_random::set_rnd_seed(seed);
    const MemorySize box_height = dagor_random::rnd_int(1, job_count - 1);
    const float inEps = dagor_random::rnd_float(0.0000001, 0.9999);

    const MemorySize invEps = static_cast<MemorySize>(ceil(1.f / inEps));

    const MemorySize max_job_height = box_height / invEps;
    const MemorySize min_job_height = dagor_random::rnd_int(1, max_job_height);

    G_ASSERTF(min_job_height <= max_job_height, "Bad test seed!");

    const auto jobs = gen_random_cyclic_jobs(job_count, timepoint_count);
    const auto job_heights = gen_random_heights(job_count, min_job_height, max_job_height);
    const auto boxing = box_jobs(jobs, job_heights, timepoint_count, box_height, invEps);
    check(jobs, job_heights, timepoint_count, boxing, box_height, 1.f / invEps);
  }

  TEST(Small1) { random_test(42, 10, 10); }
  TEST(Small2) { random_test(46, 15, 10); }
  TEST(Medium) { random_test(42, 100, 500); }
  TEST(Big1) { random_test(42, 1000, 500); }
  TEST(Big2) { random_test(43, 1000, 500); }
}

#endif


// For a set of jobs `jobs` with heights `job_heights`, solves the
// cyclic dynamic storage allocation problem, such that the resulting
// memory used is
// <= (1.5 + c*eps) * L + O(h_max / eps^6)
FmemVector<MemoryOffset> cdsa(eastl::span<const Job> in_jobs, eastl::span<const MemorySize> in_job_heights,
  TimeInterval timepoint_count, float in_eps)
{
  G_ASSERT(in_jobs.size() == in_job_heights.size());
  FmemVector<MemoryOffset> result(in_jobs.size(), 0);

  FRAMEMEM_VALIDATE;

  // 1/eps should be integral for the algorithm to work
  MemorySize invEps = static_cast<MemorySize>(ceil(1.f / in_eps));

  // Consider a forest, where every node is a job, and all non-root nodes
  // have an "offset" value attached.
  // We operate iteratively on this forest, initially setting it to input jobs.
  // Iteration:
  //    Grab some jobs with smallest heights and box them.
  //    Add boxes to the forest as roots and attach the jobs they contain
  //    to them, writing respective offset on each grabbed ex-root.
  //    Invariant is preserved, but the forest now has less trees.
  //    Repeat until there's a single root.
  // Note that actual implementation is a bit different at the end of the
  // iterative process, read the code to understand how.

  // The hope is that not much boxes will contain few jobs, so the resulting
  // tree will be at least binary.
  const JobCount maximumTreeNodeCount = 2 * in_jobs.size();

  // We will need to keep track of new job-node dimensions
  FmemVector<Job> jobs;
  jobs.reserve(maximumTreeNodeCount);
  jobs.assign(in_jobs.begin(), in_jobs.end());
  FmemVector<MemorySize> jobHeights;
  jobHeights.reserve(maximumTreeNodeCount);
  jobHeights.assign(in_job_heights.begin(), in_job_heights.end());

  const auto choseJobs = [&jobs](eastl::span<const JobIndex> which) {
    FmemVector<Job> result;
    result.reserve(which.size());

    for (auto idx : which)
      result.push_back(jobs[idx]);
    return result;
  };
  const auto choseHeights = [&jobHeights](eastl::span<const JobIndex> which) {
    FmemVector<MemorySize> result;
    result.reserve(which.size());
    for (auto idx : which)
      result.push_back(jobHeights[idx]);
    return result;
  };

  static constexpr JobIndex ROOT = static_cast<JobIndex>(-1);

  struct Edge
  {
    JobIndex parent = ROOT;
    MemoryOffset offset{0};
  };

  FmemVector<Edge> parents;
  parents.reserve(maximumTreeNodeCount);
  parents.resize(in_jobs.size(), Edge{});

  const auto comp = [&jobHeights](JobIndex fst, JobIndex snd) { return jobHeights[fst] > jobHeights[snd]; };
  using ForestRootStorage =
    eastl::vector_multiset<JobIndex, decltype(comp), framemem_allocator, dag::Vector<JobIndex, framemem_allocator>>;
  ForestRootStorage forestRoots(comp);
  // Will never expand past in_jobs.size()
  forestRoots.reserve(in_jobs.size());
  for (JobIndex i = 0; i < in_jobs.size(); ++i)
    forestRoots.emplace(i);

  const auto hMax = in_job_heights[forestRoots.front()];

  JobIndex freeJobIdx = in_jobs.size();

  bool onLastIteration;
  do // At least a single iteration will always occur
  {
    FRAMEMEM_VALIDATE;
    // Do NOT ask me to explain these values. It's discrete maths MAGIC.
    // LITERAL MAGIC. These values work, and the proof completes itself.
    // Other values do not. Read theorem 16, if you want to understand this.
    const auto hMin = jobHeights[forestRoots.back()];
    const float ratio = static_cast<float>(hMax) / hMin;
    const float logRatio = log2(ratio);

    onLastIteration = logRatio * logRatio < invEps;

    const MemorySize invMu = onLastIteration ? invEps : static_cast<MemorySize>(ceil(invEps * logRatio * logRatio));
    const MemorySize boxHeight =
      onLastIteration ? hMax * invEps
                      : static_cast<MemorySize>(ceil(hMax / (invMu * invMu * invMu * invMu * invMu * logRatio * logRatio)));
    const MemorySize jobCutoff = onLastIteration ? hMax : boxHeight / invMu;

    const float nextRatioBound = static_cast<float>(hMax * invMu) / boxHeight;

    // We want the ratio to decrease by a square root. If this does not
    // happen, we skip to the last iteration by decreasing epsilon.
    if (!onLastIteration && nextRatioBound * nextRatioBound > ratio)
    {
      invEps = static_cast<MemorySize>(ceil(logRatio * logRatio) + 1);
      continue;
    }

    const auto smallJobsBegin =
      eastl::upper_bound(forestRoots.rbegin(), forestRoots.rend(), jobCutoff, [&jobHeights](MemorySize v, JobIndex j) {
        return v < jobHeights[j];
      }).base();
    eastl::span<const JobIndex> smallJobIdxs(smallJobsBegin, forestRoots.end());
    const auto smallJobs = choseJobs(smallJobIdxs);
    const auto smallHeights = choseHeights(smallJobIdxs);

    const auto boxing = box_jobs(smallJobs, smallHeights, timepoint_count, boxHeight, invMu);

    for (JobIndex inSmallIdx = 0; inSmallIdx < smallJobIdxs.size(); ++inSmallIdx)
      parents[smallJobIdxs[inSmallIdx]] = Edge{freeJobIdx + boxing[inSmallIdx].box, boxing[inSmallIdx].offset};

    BoxCount boxCount;
    {
      // TODO: this is ugly, boxes_to_jobs should work with embeddings too
      FmemVector<BoxIndex> onlyBoxIndices;
      onlyBoxIndices.reserve(boxing.size());
      eastl::transform(boxing.begin(), boxing.end(), eastl::back_inserter(onlyBoxIndices),
        [](const BoxEmbedding &e) { return e.box; });
      const auto boxJobs = boxes_to_jobs(smallJobs, timepoint_count, onlyBoxIndices);
      jobs.insert(jobs.end(), boxJobs.begin(), boxJobs.end());
      boxCount = boxJobs.size();
    }

    {
      FmemVector<MemorySize> boxJobHeights(boxCount, 0);
      for (JobIndex inSmallIdx = 0; inSmallIdx < smallJobIdxs.size(); ++inSmallIdx)
      {
        auto [box, offset] = boxing[inSmallIdx];
        auto &boxHeight = boxJobHeights[box];
        boxHeight = eastl::max(boxHeight, offset + smallHeights[inSmallIdx]);
      }
      jobHeights.insert(jobHeights.end(), boxJobHeights.begin(), boxJobHeights.end());
    }

    parents.insert(parents.end(), boxCount, Edge{});

    forestRoots.erase(smallJobsBegin, forestRoots.end());
    for (BoxIndex i = 0; i < boxCount; ++i)
      forestRoots.emplace(freeJobIdx + i);

    freeJobIdx += boxCount;
  } while (!onLastIteration);

  // We've now finished growing the trees.
  // Root nodes are in `forestRoots`, edges are in `parents`.
  // What is left is to pack the remaining roots using CAGC and accumulate
  // offsets for leaves by traversing the trees.

  G_ASSERT(!forestRoots.empty());

  const auto rootJobs = choseJobs(forestRoots);
  const auto rootLoads = calculate_loads(rootJobs, timepoint_count);
  const TimePoint rootLoadMaxTime = static_cast<TimePoint>(eastl::max_element(rootLoads.begin(), rootLoads.end()) - rootLoads.begin());
  const auto rootColoring = color_circular_arc_graph(rootJobs, timepoint_count, rootLoadMaxTime);

  {
    const auto colorCount = size_of_mapping<MemoryOffset>(rootColoring);
    // We chose a different height for each color
    FmemVector<MemorySize> rootColorHeights(colorCount, 0);
    for (JobIndex inRootsIdx = 0; inRootsIdx < forestRoots.size(); ++inRootsIdx)
    {
      auto &rootHeight = rootColorHeights[rootColoring[inRootsIdx]];
      rootHeight = eastl::max(rootHeight, jobHeights[forestRoots[inRootsIdx]]);
    }
    FmemVector<MemorySize> rootColorOffsets;
    rootColorOffsets.reserve(colorCount + 1);
    rootColorOffsets.push_back(0);
    eastl::partial_sum(rootColorHeights.begin(), rootColorHeights.end(), eastl::back_inserter(rootColorOffsets));

    for (JobIndex inRootsIdx = 0; inRootsIdx < forestRoots.size(); ++inRootsIdx)
      parents[forestRoots[inRootsIdx]].offset = rootColorOffsets[rootColoring[inRootsIdx]];
  }

  // This is correct, as we create nodes in a bottom-up manner.
  // A parent always has a higher index than a child.
  for (auto it = parents.rbegin(), end = parents.rend(); it != end; ++it)
    if (it->parent != ROOT)
      it->offset += parents[it->parent].offset;

  for (JobIndex i = 0; i < result.size(); ++i)
    result[i] = parents[i].offset;

  return result;
}

FmemVector<MemorySize> calculate_loads(eastl::span<const Job> jobs, eastl::span<const MemorySize> job_heights,
  TimeInterval timepoint_count)
{
  MemorySize wraparoundLoad = 0;
  FmemVector<MemorySize> loads(timepoint_count, 0);
  for (JobIndex i = 0; i < jobs.size(); ++i)
  {
    loads[jobs[i].left] += job_heights[i];
    loads[jobs[i].right] -= job_heights[i]; // overflows are ok here, they will get canceled out
    if (jobs[i].right <= jobs[i].left)
      wraparoundLoad += job_heights[i];
  }
  eastl::partial_sum(loads.begin(), loads.end(), loads.begin());
  for (auto &l : loads)
    l += wraparoundLoad;
  return loads;
}


// Range minimum query segment tree with segment updates.
// https://cp-algorithms.com/data_structures/segment_tree.html#range-updates-lazy-propagation
class TopProfileTracker
{
  using NodeIdx = uint32_t;

  inline static constexpr NodeIdx ROOT = 1;
  inline static constexpr MemoryOffset UP_TO_DATE = ~MemoryOffset{0};

  static NodeIdx leftChild(NodeIdx v) { return 2 * v; }
  static NodeIdx rightChild(NodeIdx v) { return 2 * v + 1; }

  static TimeInterval calcLogSize(TimeInterval v) { return v < 2 ? 0 : __bsr_unsafe(v - 1) + 1; }

  eastl::pair<TimePoint, TimePoint> segmentForNode(NodeIdx v) const
  {
    const auto k = __bsr_unsafe(v);
    const auto u = v - (1 << k);
    const auto p = 1 << (logSize - k);
    return {u * p, (u + 1) * p};
  }

  bool isLeaf(NodeIdx node) const { return node >= size; }

  void pushDefer(NodeIdx node)
  {
    const auto defer = eastl::exchange(deferred[node], UP_TO_DATE);
    if (defer == UP_TO_DATE)
      return;

    current[node] = defer;
    if (!isLeaf(node))
    {
      deferred[leftChild(node)] = defer;
      deferred[rightChild(node)] = defer;
    }
  }

public:
  TopProfileTracker(TimeInterval timepoint_count) :
    logSize(calcLogSize(timepoint_count)), size(1 << logSize), current(2 * size, 0), deferred(2 * size, UP_TO_DATE)
  {
    G_ASSERT(logSize < 32);
  }

  MemoryOffset rangeMax(const TimePoint l, const TimePoint r)
  {
    FRAMEMEM_VALIDATE;

    if (r <= l)
      return eastl::max(l != size ? rangeMax(l, size) : 0, 0 != r ? rangeMax(0, r) : 0);

    MemoryOffset result = {};

    FmemVector<NodeIdx> stack;
    stack.emplace_back(ROOT);
    while (!stack.empty())
    {
      const auto node = stack.back();
      stack.pop_back();

      pushDefer(node);

      const auto [cl, cr] = segmentForNode(node);
      if (l <= cl && cr <= r)
        result = eastl::max(current[node], result);
      else
      {
        TimePoint center = cl + (cr - cl) / 2;
        if (l < center)
          stack.push_back(leftChild(node));
        if (center < r)
          stack.push_back(rightChild(node));
      }
    }

    return result;
  }

  void rangeUpdate(TimePoint l, TimePoint r, MemoryOffset value)
  {
    FRAMEMEM_VALIDATE;

    if (r <= l)
    {
      if (l != size)
        rangeUpdate(l, size, value);
      if (0 != r)
        rangeUpdate(0, r, value);
      return;
    }

    FmemVector<NodeIdx> stack;
    stack.emplace_back(ROOT);
    while (!stack.empty())
    {
      const auto node = stack.back();
      stack.pop_back();

      pushDefer(node);

      const auto [cl, cr] = segmentForNode(node);

      if (l <= cl && cr <= r)
        deferred[node] = value;
      else
      {
        current[node] = eastl::max(current[node], value);
        TimePoint center = cl + (cr - cl) / 2;
        if (l < center)
          stack.push_back(leftChild(node));
        if (center < r)
          stack.push_back(rightChild(node));
      }
    }
  }

private:
  const uint32_t logSize;
  const TimeInterval size;
  FmemVector<MemoryOffset> current;
  FmemVector<MemoryOffset> deferred;
};


#ifdef ENABLE_UNIT_TESTS

SUITE(TopProfileTracking)
{
  TEST(Simple1)
  {
    TopProfileTracker tracker(100);
    tracker.rangeUpdate(25, 50, 42);
    for (int i = 25; i < 49; ++i)
      CHECK_EQUAL(42, tracker.rangeMax(i, i + 1));
  }

  TEST(Simple2)
  {
    TopProfileTracker tracker(100);
    tracker.rangeUpdate(25, 50, 42);
    CHECK_EQUAL(42, tracker.rangeMax(39, 88));
  }

  TEST(Simple3)
  {
    TopProfileTracker tracker(100);
    tracker.rangeUpdate(25, 50, 42);
    CHECK_EQUAL(42, tracker.rangeMax(7, 31));
  }

  TEST(Stress)
  {
    dagor_random::set_rnd_seed(5);

    constexpr TimeInterval TIMELINE_SIZE = 1000;

    TopProfileTracker tracker(TIMELINE_SIZE);
    FmemVector<MemoryOffset> naive(TIMELINE_SIZE, 0);

    const auto checkRangeMax = [&](TimePoint l, TimePoint r) {
      const auto expected = *eastl::max_element(naive.begin() + l, naive.begin() + r);
      const auto actual = tracker.rangeMax(l, r);
      CHECK_EQUAL(expected, actual);
    };

    const auto rangeUpdate = [&](TimePoint l, TimePoint r, MemoryOffset value) {
      tracker.rangeUpdate(l, r, value);
      for (; l != r; ++l)
        naive[l] = eastl::max(naive[l], value);
    };

    for (int i = 0; i < 5; ++i)
    {
      const auto l = dagor_random::rnd_int(0, TIMELINE_SIZE - 2);
      const auto r = dagor_random::rnd_int(l, TIMELINE_SIZE - 1);
      if (dagor_random::rnd_int(0, 1) == 0)
        rangeUpdate(l, r, dagor_random::rnd_int(0, 10000));
      else
        checkRangeMax(l, r);
    }
  }
}

#endif


namespace dabfg
{

struct BoxingPacker
{
  // Uses our cdsa algorithm to pack resources in (1.5 + O((h_max/L)^1/7))*LOAD
  // by picking a good epsilon.
  PackerOutput operator()(PackerInput input)
  {
    const auto timepointCount = input.timelineSize;

    calculatedOffsets.assign(input.resources.size(), PackerOutput::NOT_ALLOCATED);

    FRAMEMEM_VALIDATE;

    FmemVector<Job> jobs;
    jobs.reserve(input.resources.size());
    FmemVector<MemorySize> jobHeights;
    jobHeights.reserve(input.resources.size());
    FmemVector<uint32_t> initialIndices;
    initialIndices.reserve(input.resources.size());

    for (uint32_t i = 0; i < input.resources.size(); ++i)
    {
      const auto &res = input.resources[i];
      if (res.size == 0 || res.align == 0)
        continue;

      jobs.push_back(Job{res.start, res.end});
      jobHeights.push_back(res.size + res.align - 1);
      initialIndices.push_back(i);
    }

    if (jobs.empty())
      return PackerOutput{calculatedOffsets, 0};

    const auto loads = calculate_loads(jobs, jobHeights, timepointCount);
    const auto maxLoad = *eastl::max_element(loads.begin(), loads.end());
    const auto maxHeight = *eastl::max_element(jobHeights.begin(), jobHeights.end());
    const auto eps = pow(static_cast<float>(maxHeight) / static_cast<float>(maxLoad), 1.f / 7.f);
    const auto rawResult = cdsa(jobs, jobHeights, timepointCount, eps);

    {
      FRAMEMEM_VALIDATE;

      FmemVector<JobIndex> sorted(rawResult.size());
      eastl::iota(sorted.begin(), sorted.end(), 0);
      eastl::sort(sorted.begin(), sorted.end(), [&rawResult](JobIndex fst, JobIndex snd) { return rawResult[fst] < rawResult[snd]; });

      TopProfileTracker tracker(timepointCount);
      for (auto idx : sorted)
      {
        const auto &res = input.resources[initialIndices[idx]];
        const auto compressedOffset = res.doAlign(tracker.rangeMax(res.start, res.end));

        if (compressedOffset + res.size <= input.maxHeapSize)
        {
          calculatedOffsets[initialIndices[idx]] = compressedOffset;
          tracker.rangeUpdate(res.start, res.end, compressedOffset + res.size);
        }
        else
        {
          calculatedOffsets[initialIndices[idx]] = PackerOutput::NOT_SCHEDULED;
        }
      }
    }

    PackerOutput output;
    output.heapSize = 0;
    for (uint32_t i = 0; i < input.resources.size(); ++i)
      if (calculatedOffsets[i] != PackerOutput::NOT_SCHEDULED && calculatedOffsets[i] != PackerOutput::NOT_ALLOCATED)
        output.heapSize = eastl::max(output.heapSize, calculatedOffsets[i] + input.resources[i].size);
    output.offsets = calculatedOffsets;
    return output;
  }

  dag::Vector<uint64_t> calculatedOffsets;
};


Packer make_boxing_packer() { return BoxingPacker{}; }

} // namespace dabfg

// AD-HOC EXPERIMENTS

FmemVector<MemoryOffset> adhoc_cdsa(eastl::span<const Job> in_jobs, eastl::span<const MemorySize> in_job_heights,
  TimeInterval timepoint_count)
{
  G_ASSERT(in_jobs.size() == in_job_heights.size());
  FmemVector<MemoryOffset> result(in_jobs.size(), 0);

  FRAMEMEM_VALIDATE;

  const JobCount maximumTreeNodeCount = 4 * in_jobs.size();

  // We will need to keep track of new job-node dimensions
  FmemVector<Job> jobs;
  jobs.reserve(maximumTreeNodeCount);
  jobs.assign(in_jobs.begin(), in_jobs.end());
  FmemVector<MemorySize> jobHeights;
  jobHeights.reserve(maximumTreeNodeCount);
  jobHeights.assign(in_job_heights.begin(), in_job_heights.end());

  const auto choseJobs = [&jobs](eastl::span<const JobIndex> which) {
    FmemVector<Job> result;
    result.reserve(which.size());

    for (auto idx : which)
      result.push_back(jobs[idx]);
    return result;
  };

  static constexpr JobIndex ROOT = static_cast<JobIndex>(-1);

  struct Edge
  {
    JobIndex parent = ROOT;
    MemoryOffset offset{0};
  };

  FmemVector<Edge> parents;
  parents.reserve(maximumTreeNodeCount);
  parents.resize(in_jobs.size(), Edge{});

  // Will never expand past in_jobs.size()
  const auto comp = [&jobHeights](JobIndex fst, JobIndex snd) { return jobHeights[fst] > jobHeights[snd]; };
  using ForestRootStorage =
    eastl::vector_multiset<JobIndex, decltype(comp), framemem_allocator, dag::Vector<JobIndex, framemem_allocator>>;
  ForestRootStorage forestRoots(comp);
  forestRoots.reserve(in_jobs.size());
  for (JobIndex i = 0; i < in_jobs.size(); ++i)
    forestRoots.emplace(i);

  JobIndex freeJobIdx = in_jobs.size();

  while (jobHeights[forestRoots.front()] != jobHeights[forestRoots.back()])
  {
    FRAMEMEM_VALIDATE;

    const auto [smallBeg, smallEnd] = forestRoots.equal_range(forestRoots.back());
    eastl::span<const JobIndex> smallJobIdxs(smallBeg, smallEnd);
    const auto smallJobs = choseJobs(smallJobIdxs);

    const auto currHeight = jobHeights[forestRoots.back()];
    const auto targetJobIt = eastl::lower_bound(forestRoots.rbegin(), forestRoots.rend(), 2 * currHeight,
      [&jobHeights](JobIndex idx, MemorySize sz) { return jobHeights[idx] < sz; });
    const auto boxHeight = targetJobIt != forestRoots.rend() ? jobHeights[*targetJobIt] : jobHeights[forestRoots.front()];

    // MemorySize invEps;
    // {
    //   const auto loads = calculate_loads(smallJobs, timepoint_count);

    //   MemorySize l = 1;
    //   MemorySize r = 1000;

    //   const auto b = 6 * boxHeight * log2(static_cast<double>(boxHeight));
    //   const auto a = *eastl::max_element(loads.begin(), loads.end());
    //   while (r - l > 1)
    //   {
    //     invEps = l + (r - l)/2;

    //     const auto derivative = 4*a - (2 * b * log2(static_cast<double>(invEps)) + b)*invEps / log(2.);

    //     if (derivative > 0)
    //       r = invEps;
    //     else
    //       l = invEps;
    //   }
    // }

    UnitJobBoxer boxer(smallJobs, timepoint_count, boxHeight, 1);
    const auto boxing = boxer();

    for (JobIndex inSmallIdx = 0; inSmallIdx < smallJobIdxs.size(); ++inSmallIdx)
      parents[smallJobIdxs[inSmallIdx]] = Edge{freeJobIdx + boxing[inSmallIdx].box, boxing[inSmallIdx].offset};

    BoxCount boxCount;
    {
      // TODO: this is ugly, boxes_to_jobs should work with embeddings too
      FmemVector<BoxIndex> onlyBoxIndices;
      onlyBoxIndices.reserve(boxing.size());
      eastl::transform(boxing.begin(), boxing.end(), eastl::back_inserter(onlyBoxIndices),
        [](const BoxEmbedding &e) { return e.box; });
      const auto boxJobs = boxes_to_jobs(smallJobs, timepoint_count, onlyBoxIndices);
      jobs.insert(jobs.end(), boxJobs.begin(), boxJobs.end());
      boxCount = boxJobs.size();
    }

    {
      G_ASSERT(smallBeg != forestRoots.begin()); // sanity check
      FmemVector<MemorySize> boxJobHeights(boxCount, jobHeights[*eastl::prev(smallBeg)]);
      for (JobIndex inSmallIdx = 0; inSmallIdx < smallJobIdxs.size(); ++inSmallIdx)
      {
        auto [box, offset] = boxing[inSmallIdx];
        auto &boxHeight = boxJobHeights[box];
        boxHeight = eastl::max(boxHeight, (offset + 1) * currHeight);
      }
      jobHeights.insert(jobHeights.end(), boxJobHeights.begin(), boxJobHeights.end());
    }

    FRAMEMEM_VALIDATE;

    parents.insert(parents.end(), boxCount, Edge{});

    G_ASSERT(boxCount <= smallJobIdxs.size());

    forestRoots.erase(smallBeg, smallEnd);
    for (BoxIndex i = 0; i < boxCount; ++i)
      forestRoots.emplace(freeJobIdx + i);

    freeJobIdx += boxCount;
  }

  // We've now finished growing the trees.
  // Root nodes are in `forestRoots`, edges are in `parents`.
  // What is left is to pack the remaining roots using CAGC and accumulate
  // offsets for leaves by traversing the trees.

  G_ASSERT(!forestRoots.empty());

  const auto rootJobs = choseJobs(forestRoots);
  const auto rootLoads = calculate_loads(rootJobs, timepoint_count);
  const TimePoint rootLoadMaxTime = static_cast<TimePoint>(eastl::max_element(rootLoads.begin(), rootLoads.end()) - rootLoads.begin());
  const auto rootColoring = color_circular_arc_graph(rootJobs, timepoint_count, rootLoadMaxTime);

  {
    const auto colorCount = size_of_mapping<MemoryOffset>(rootColoring);
    // We chose a different height for each color
    FmemVector<MemorySize> rootColorHeights(colorCount, 0);
    for (JobIndex inRootsIdx = 0; inRootsIdx < forestRoots.size(); ++inRootsIdx)
    {
      auto &rootHeight = rootColorHeights[rootColoring[inRootsIdx]];
      rootHeight = eastl::max(rootHeight, jobHeights[forestRoots[inRootsIdx]]);
    }
    FmemVector<MemorySize> rootColorOffsets;
    rootColorOffsets.reserve(colorCount + 1);
    rootColorOffsets.push_back(0);
    eastl::partial_sum(rootColorHeights.begin(), rootColorHeights.end(), eastl::back_inserter(rootColorOffsets));

    for (JobIndex inRootsIdx = 0; inRootsIdx < forestRoots.size(); ++inRootsIdx)
      parents[forestRoots[inRootsIdx]].offset = rootColorOffsets[rootColoring[inRootsIdx]];
  }

  // This is correct, as we create nodes in a bottom-up manner.
  // A parent always has a higher index than a child.
  for (auto it = parents.rbegin(), end = parents.rend(); it != end; ++it)
    if (it->parent != ROOT)
      it->offset += parents[it->parent].offset;

#if 0
  {
    logwarn("Final hierarchical boxing (l, r, s, p, o):");
    dag::Vector<JobCount> childCount(parents.size(), 0);
    for (const auto &p : parents)
      if (p.parent != ROOT)
        ++childCount[p.parent];

    for (JobIndex i = 0; i < jobs.size(); ++i)
      if (childCount[i] != 1)
        logwarn("(%d, %d, %d, %d)", jobs[i].left, jobs[i].right, jobHeights[i], parents[i].offset);
  }
#endif

  for (JobIndex i = 0; i < result.size(); ++i)
    result[i] = parents[i].offset;

  return result;
}

namespace dabfg
{

struct AdhocBoxingPacker
{
  // Uses our cdsa algorithm to pack resources in (1.5 + O((h_max/L)^1/7))*LOAD
  // by picking a good epsilon.
  PackerOutput operator()(PackerInput input)
  {
    const auto timepointCount = input.timelineSize;

    calculatedOffsets.assign(input.resources.size(), PackerOutput::NOT_ALLOCATED);

    FRAMEMEM_VALIDATE;

    FmemVector<Job> jobs;
    jobs.reserve(input.resources.size());
    FmemVector<MemorySize> jobHeights;
    jobHeights.reserve(input.resources.size());
    FmemVector<uint32_t> initialIndices;
    initialIndices.reserve(input.resources.size());

    for (uint32_t i = 0; i < input.resources.size(); ++i)
    {
      const auto &res = input.resources[i];
      if (res.size == 0 || res.align == 0)
        continue;

      jobs.push_back(Job{res.start, res.end});
      jobHeights.push_back(res.size + res.align - 1);
      initialIndices.push_back(i);
    }

    if (jobs.empty())
      return PackerOutput{calculatedOffsets, 0};

    const auto rawResult = adhoc_cdsa(jobs, jobHeights, timepointCount);

    {
      FRAMEMEM_VALIDATE;

      FmemVector<JobIndex> sorted(rawResult.size());
      eastl::iota(sorted.begin(), sorted.end(), 0);
      eastl::sort(sorted.begin(), sorted.end(), [&rawResult](JobIndex fst, JobIndex snd) { return rawResult[fst] < rawResult[snd]; });

      TopProfileTracker tracker(timepointCount);
      for (auto idx : sorted)
      {
        const auto &res = input.resources[initialIndices[idx]];
        const auto compressedOffset = res.doAlign(tracker.rangeMax(res.start, res.end));

        if (compressedOffset + res.size <= input.maxHeapSize)
        {
          calculatedOffsets[initialIndices[idx]] = compressedOffset;
          tracker.rangeUpdate(res.start, res.end, compressedOffset + res.size);
        }
        else
        {
          calculatedOffsets[initialIndices[idx]] = PackerOutput::NOT_SCHEDULED;
        }
      }
    }

    PackerOutput output;
    output.heapSize = 0;
    for (uint32_t i = 0; i < input.resources.size(); ++i)
      if (calculatedOffsets[i] != PackerOutput::NOT_SCHEDULED && calculatedOffsets[i] != PackerOutput::NOT_ALLOCATED)
        output.heapSize = eastl::max(output.heapSize, calculatedOffsets[i] + input.resources[i].size);
    output.offsets = calculatedOffsets;
    return output;
  }

  dag::Vector<uint64_t> calculatedOffsets;
};


Packer make_adhoc_boxing_packer() { return AdhocBoxingPacker{}; }

} // namespace dabfg
