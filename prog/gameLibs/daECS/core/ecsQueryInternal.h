#pragma once
#include <util/dag_stdint.h>
#include <generic/dag_span.h>
#include <EASTL/vector.h>
#include <EASTL/bitvector.h>
#include <EASTL/fixed_vector.h>
#include <daECS/core/dataComponent.h> //for component_index_t


DAG_DECLARE_RELOCATABLE(::ecs::ArchetypesQuery);

namespace ecs
{

constexpr uint32_t MAX_RESERVED_EID_IDX_CONST = USHRT_MAX; // To consider: unsatisfied/weak link time dep in order be able configure it
                                                           // per game?

struct ScheduledArchetypeComponentTrack // sizeof == 4, can be effectively hashed
{
  union
  {
    struct
    {
      archetype_t archetype;
      component_index_t cidx;
    };
    uint32_t combined;
  };
  ScheduledArchetypeComponentTrack() {} // intentionally uninitialized, for performance
  ScheduledArchetypeComponentTrack(archetype_t arch, component_index_t cidx_) : archetype(arch), cidx(cidx_) {}
  operator uint32_t() const
  {
    G_STATIC_ASSERT(sizeof(combined) == sizeof(cidx) + sizeof(archetype));
    return combined;
  }
};

static constexpr int MAX_OPTIONAL_PARAMETERS = 128;
typedef eastl::bitset<MAX_OPTIONAL_PARAMETERS> OptionalMask;

struct memfree_deleter
{
  void operator()(void *p) { memfree_anywhere(p); }
};

// todo: convert that to SoA + offsets instead of pointers
struct alignas(16) ArchetypesQuery
{
  // store pointers separately from counters. That limit ArchetypesQuery to just 48bytes in 64bit platform.
  // can be made 32 bytes, by moving out most rarely/most often used components to SoA arrays, and replacing two compType/compSize
  // pointers with offsets
  uint32_t getComponentsCount() const { return rwCount + roCount; }
  __forceinline bool hasComponents() const
  {
    G_STATIC_ASSERT(offsetof(ArchetypesQuery, rwCount) == offsetof(ArchetypesQuery, roCount) + 2);
    G_STATIC_ASSERT(sizeof(rwCount) == 2 && sizeof(roCount) == 2 && sizeof(roRW) == 4);
    return roRW != 0;
  }

  union
  {
    struct
    {
      uint16_t roCount, rwCount;
    };
    uint32_t roRW;
  };
  archetype_t queriesCount = 0;

  component_index_t trackedChangesCount = 0;
  archetype_t archSubQueriesCount = 0;

  archetype_t firstArch = 0, secondArch = 0, thirdArch = 0;
  // this is array of all archetypes that fit into this query
  // needed for non-eid queries only
  union
  {
    archetype_t *queries = nullptr; // if queriesCount > 7
    archetype_t arches[4];          // other inplace archetypes, starting from fourth, if queriesCount < 7
  };

  typedef uint16_t offset_type_t;
  static constexpr int INVALID_OFFSET = eastl::numeric_limits<offset_type_t>::max();
  union
  {
    offset_type_t *allComponentsArchOffsets = nullptr; // if allocated externally, i.e. total amount of offsets is bigger than 4
    offset_type_t offsets[16]; // if allocated inline, i.e. maximum 8 offsets (2 archetype with 4 components, 4 with 2 or 8 archetypes
                               // with 1 component max)
  };
  eastl::unique_ptr<ScheduledArchetypeComponentTrack[], memfree_deleter> trackedChanges; // todo:move out, used only if tracked changes
                                                                                         // count is not 0. optimize 80% of cases where
                                                                                         // it is not tracked, by allowing more data to
                                                                                         // be stored inline
  //-SoA

  size_t memUsage() const;

  uint32_t getQueriesCount() const { return queriesCount; }

  static constexpr int inplace_offsets_count_bits = 4; // 1<<bits max, and 0 is impossible, so we check bits for cnt-1
  static __forceinline bool isInplaceOffsets(int cnt) { return ((cnt - 1) >> inplace_offsets_count_bits) == 0; }
  __forceinline bool isInplaceOffsets() const { return isInplaceOffsets(getAllComponentsArchOffsetsCount()); }

  static constexpr int inplace_arch_count_bits = 3; // 7 total, so we check 3 bits
  static __forceinline bool isInplaceQueries(int cnt) { return (cnt >> inplace_arch_count_bits) == 0; }

  __forceinline bool isInplaceQueries() const { return isInplaceQueries(queriesCount); }
  uint32_t getAllComponentsArchOffsetsCount() const { return getQueriesCount() * getComponentsCount(); }

  __forceinline archetype_t *queriesInplace() { return &firstArch; }
  __forceinline const archetype_t *queriesInplace() const { return &firstArch; }
  __forceinline const archetype_t *queriesBegin() const { return isInplaceQueries() ? queriesInplace() : queries; }
  const archetype_t *queriesEnd() const { return queriesBegin() + queriesCount; }
  const ScheduledArchetypeComponentTrack *trackedBegin() const { return trackedChanges.get(); }
  const ScheduledArchetypeComponentTrack *trackedEnd() const { return trackedChanges.get() + trackedChangesCount; }

  const offset_type_t *getArchetypeOffsetsPtrInplace() const { return offsets; }
  offset_type_t *getArchetypeOffsetsPtrInplace() { return offsets; }
  const offset_type_t *getArchetypeOffsetsPtr() const
  {
    return isInplaceOffsets() ? getArchetypeOffsetsPtrInplace() : allComponentsArchOffsets;
  }

  uint32_t getArchetypesRelated() const { return queriesCount; }
  void reset()
  {
    // allComponentsId.resize(0);
    if (!isInplaceQueries())
      memfree_default(queries);
    queries = nullptr;
    if (!isInplaceOffsets())
      memfree_default(allComponentsArchOffsets);
    allComponentsArchOffsets = nullptr;
    roCount = 0, rwCount = 0;
    trackedChangesCount = 0;
    queriesCount = 0;
    firstArch = archSubQueriesCount = 0;
    trackedChanges.reset(nullptr);
  }
  ~ArchetypesQuery() { reset(); }
  ArchetypesQuery() = default;
  ArchetypesQuery(ArchetypesQuery &&a) :
    allComponentsArchOffsets(a.allComponentsArchOffsets),
    trackedChanges(eastl::move(a.trackedChanges)),
    roRW(a.roRW),
    trackedChangesCount(a.trackedChangesCount),
    queriesCount(a.queriesCount),
    archSubQueriesCount(a.archSubQueriesCount),
    queries(a.queries),
    firstArch(a.firstArch),
    secondArch(a.secondArch),
    thirdArch(a.thirdArch)
  {
    a.queries = nullptr;
    a.queriesCount = 0;
    a.allComponentsArchOffsets = nullptr;
  }
};

inline size_t ArchetypesQuery::memUsage() const
{
  return (isInplaceQueries() ? 0 : sizeof(archetype_t) * getQueriesCount()) +
         (isInplaceOffsets() ? 0 : sizeof(offset_type_t) * getAllComponentsArchOffsetsCount()) +
         (sizeof(uint16_t)) * getComponentsCount() + sizeof(ScheduledArchetypeComponentTrack) * trackedChangesCount +
         sizeof(archetype_t) * archSubQueriesCount;
}

struct alignas(16) ArchetypesEidQuery
{
  // actually, this pointer can be duplicated for some queries. Many
  // queries can have exact same archetypes fitting, so we'd better
  // store just non-owning pointer/offset here
  uint32_t archSubQueriesAt = ~0u;
  uint32_t componentsSizesAt = ~0u;
  void reset() { archSubQueriesAt = componentsSizesAt = ~0u; }
};

struct ResolvedQueryDesc // resolved query won't change ever (if it was successfully resolved)
{
  typedef uint8_t required_components_count_t;
  // one fixed_vector + ranges. is more cache friendly
  // struct Range { uint16_t start = 0, cnt = 0; bool isInside(uint16_t a) const{return a>=start && a - start < cnt;}};
  struct Range
  {
    uint16_t start, cnt;
    bool isInside(uint16_t a) const { return a >= start && a - start < cnt; }
  };
  typedef dag::RelocatableFixedVector<component_index_t, 7, true, MidmemAlloc, uint16_t> components_vec_t;

private:
  union
  {
    struct
    {
      required_components_count_t requiredComponentsCnt;
      uint8_t rwCnt, roCnt, rqCnt;
    };
    uint64_t ranges = 0;
  };
  OptionalMask optionalMask;
  // todo:
  components_vec_t components; // descriptors of components that this ES needs. can and should be replaced with uint16_t start, count;
public:
  required_components_count_t requiredComponentsCount() const { return requiredComponentsCnt; }
  void setRequiredComponentsCount(required_components_count_t s) { requiredComponentsCnt = s; }
  OptionalMask &getOptionalMask() { return optionalMask; }
  const OptionalMask &getOptionalMask() const { return optionalMask; }
  const components_vec_t &getComponents() const { return components; }
  components_vec_t &getComponents() { return components; }

  Range getNO() const { return Range{getNoStart(), getNoCnt()}; }
  Range getRQ() const { return Range{uint16_t(rwCnt + roCnt), rqCnt}; }
  Range getRO() const { return Range{rwCnt, roCnt}; }
  Range getRW() const { return Range{0, rwCnt}; }
  uint16_t getNoStart() const { return uint16_t(rwCnt + roCnt + rqCnt); }
  uint8_t getNoCnt() const { return uint8_t(components.size() - getNoStart()); }
  uint8_t getRqCnt() const { return rqCnt; }
  uint8_t getRwCnt() const { return rwCnt; }
  uint8_t getRoCnt() const { return roCnt; }
  uint8_t &getRqCnt() { return rqCnt; }
  uint8_t &getRwCnt() { return rwCnt; }
  uint8_t &getRoCnt() { return roCnt; }
  void reset()
  {
    components.clear();
    optionalMask.reset();
    ranges = 0;
  }
  bool isEmpty() const { return components.empty(); }
  bool operator==(const ResolvedQueryDesc &b)
  {
    if (ranges != b.ranges)
      return false;
    if (optionalMask != b.optionalMask)
      return false;
    return components.size() == b.components.size() &&
           memcmp(components.data(), b.components.data(), components.size() * sizeof(component_index_t)) == 0;
  }
  size_t memUsage() const;
};

inline size_t ResolvedQueryDesc::memUsage() const { return sizeof(component_index_t) * components.capacity() + sizeof(optionalMask); }

}; // namespace ecs
