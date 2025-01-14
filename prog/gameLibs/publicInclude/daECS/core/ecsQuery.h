//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_generationRefId.h>
#include <util/dag_stdint.h>
#include <EASTL/vector.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_function.h>
#include <generic/dag_span.h>
#include <memory/dag_framemem.h>
#include "entityId.h"
#include "dataComponent.h"
#include "utility/ecsForEach.h"
#include "internal/chunkedVectorMT.h"

namespace ecs
{

typedef uint32_t component_type_t;

// struct QueryIterator;
struct QueryView;

class QueryIdDummy
{};
typedef GenerationRefId<8, QueryIdDummy> QueryId;

enum class QueryCbResult
{
  Stop,
  Continue
};

// Queries are considered perfomance critical therefore it should not allocate memory on execution (hence fixed_function)
typedef eastl::fixed_function<sizeof(void *) * 2, void(const QueryView &)> query_cb_t;
typedef eastl::fixed_function<sizeof(void *) * 2, QueryCbResult(const QueryView &)> stoppable_query_cb_t;
#if DAGOR_DBGLEVEL > 1
#define QUERY_FAST_ASSERT \
  G_FAST_ASSERT // there is no reason to check type correctness, as we generate our queries using codegen. do that only in
                // DAGOR_DBGLEVEL>1
#define ECS_VALIDATE_QUERY_TYPES 1
#else
#define ECS_VALIDATE_QUERY_TYPES 0
#define QUERY_FAST_ASSERT(...)
#endif
#ifdef _MSC_VER
#define DECL_RESTRICT __declspec(restrict)
#else
#define DECL_RESTRICT
#endif
typedef uint32_t QueryIterator;

struct QueryView
{
  struct OneComponentLine;
  typedef OneComponentLine *__restrict ComponentsData;
  QueryIterator begin() const;
  QueryIterator end() const;

  uint16_t getRoCount() const { return roCount; }
  uint16_t getRwCount() const { return rwCount; }
  uint16_t getRoStart() const { return rwCount; }
  uint16_t getRwStart() const { return 0; }

  uint32_t getEntitiesCount() const { return chunkEntitiesEnd - chunkEntitiesStart; }

  DECL_RESTRICT ComponentsData getComponentUntypedData(uint16_t compId) const { return componentData[compId]; }

  template <class T>
  const typename PtrComponentType<T>::cptr_type getComponentRawRO(uint16_t compId) const
  {
    typename PtrComponentType<T>::cptr_type ret = (typename PtrComponentType<T>::cptr_type)getComponentUntypedData(compId);
    return (typename PtrComponentType<T>::cptr_type)ECS_ASSUME_ALIGNED_PTR(T, ret);
  }

  template <class T>
  typename PtrComponentType<T>::ptr_type getComponentRawRW(uint16_t compId) const
  {
    typename PtrComponentType<T>::ptr_type ret = (typename PtrComponentType<T>::ptr_type)getComponentUntypedData(compId);
    return (typename PtrComponentType<T>::ptr_type)ECS_ASSUME_ALIGNED_PTR(T, ret);
  }
  uint32_t getComponentsCount() const { return rwCount + roCount; } //

  void *__restrict getUserData() const { return userData; }

  template <class T, typename U = T>
  auto &getComponentRW(uint16_t compId, uint32_t idInChunk) const
  {
    auto p = getComponentUntypedData(compId);
    QUERY_FAST_ASSERT(p);
    return ref<T, U>(p, idInChunk);
  }
  template <class T, typename U = T>
  auto &getComponentRO(uint16_t compId, uint32_t idInChunk) const
  {
    auto p = getComponentUntypedData(compId);
    QUERY_FAST_ASSERT(p);
    return ref<T, U, /*bConst*/ true>(p, idInChunk);
  }

  template <class T, typename U = T>
  auto &getComponentRODef(uint16_t compId, uint32_t idInChunk, const T &__restrict def) const
  {
    auto p = getComponentUntypedData(compId);
    return p ? ref<T, U, /*bConst*/ true>(p, idInChunk) : def;
  }
  template <class T, typename U = T>
  DECL_RESTRICT const U *__restrict getComponentROOpt(uint16_t compId, uint32_t idInChunk) const
  {
    auto p = getComponentUntypedData(compId);
    return p ? &ref<T, U, /*bConst*/ true>(p, idInChunk) : nullptr;
  }
  template <class T, typename U = T>
  DECL_RESTRICT U *__restrict getComponentRWOpt(uint16_t compId, uint32_t idInChunk) const
  {
    auto p = getComponentUntypedData(compId);
    return p ? &ref<T, U>(p, idInChunk) : nullptr;
  }
  uint32_t getWorkerId() const { return workerId; }
  EntityManager &__restrict manager() const { return *mgr; }
  QueryId getQueryId() const { return id; }

protected:
  template <typename T, typename U = T, bool bConst = false>
  static eastl::conditional_t<bConst, const U &__restrict, U &__restrict> ref(ComponentsData p, uint32_t idInChunk)
  {
    if constexpr (eastl::is_same_v<T, eastl::remove_const_t<U>>)
      return PtrComponentType<T>::ref((typename PtrComponentType<T>::ptr_type)p + idInChunk);
    else
    {
      static_assert(sizeof(T) == sizeof(U), "Not binary compatible types");
      static_assert(!PtrComponentType<T>::is_boxed, "Only value (i.e non boxed) types are allowed to be aliased");
      constexpr size_t max_algn = (eastl::max)(ecs::ComponentTypeInfo<T>::ref_alignment, alignof(U));
      auto ptr = (U *__restrict)p + idInChunk;
      return *(U *__restrict)ASSUME_ALIGNED(ptr, max_algn);
    }
  }

  friend class EntityManager;
  friend struct Query;
  friend class ESJob;
  uint16_t workerId = 0;
  id_in_chunk_type_t chunkEntitiesStart = 0;                // this is for parallel_for
  uint32_t chunkEntitiesEnd = 0;                            // entities to proceed
  const ComponentsData *__restrict componentData = nullptr; // chunk data
  mutable void *__restrict userData = nullptr;
  EntityManager *__restrict mgr = NULL;

  QueryId id;
  union
  {
    struct
    {
      uint8_t roCount, rwCount;
    };
    uint16_t roRW = 0;
  };
  QueryView(class EntityManager &__restrict mgr_, void *user_data = nullptr) : mgr(&mgr_), userData(user_data) {}

  QueryView(EntityManager &__restrict mgr_, uint16_t workerId_, id_in_chunk_type_t chunkEntitiesStart_, uint32_t chunkEntitiesEnd_,
    QueryId id_,
    // uint8_t *__restrict *__restrict componentData_
    const ComponentsData *__restrict componentData_, uint16_t ro_rw, void *__restrict user_data) :
    mgr(&mgr_),
    workerId(workerId_),
    chunkEntitiesStart(chunkEntitiesStart_),
    chunkEntitiesEnd(chunkEntitiesEnd_),
    componentData(componentData_),
    id(id_),
    roRW(ro_rw),
    userData(user_data)
  {}
};
inline QueryIterator QueryView::begin() const { return chunkEntitiesStart; }
inline QueryIterator QueryView::end() const { return chunkEntitiesEnd; }

struct Query
{
  QueryView getView(class EntityManager &__restrict mgr, //-V669
    void *__restrict user_data, uint32_t chunk, id_in_chunk_type_t start, uint32_t count, uint16_t worker = 0) const
  {
    return QueryView(mgr, worker, start, start + count, id, (QueryView::ComponentsData *__restrict)getChunkData(chunk), roRW,
      user_data);
  }
  QueryView getView(class EntityManager &__restrict mgr, void *user_data, uint32_t chunk) const
  {
    return getView(mgr, user_data, chunk, 0, chunkEntitiesCnt[chunk]);
  }
  uint32_t allComponentsCount() const { return roCount + rwCount; }
  uint32_t chunksCount() const { return chunks; }

protected:
  Query(QueryId q) : id(q) {}
  QueryView::ComponentsData *getChunkData(uint32_t chunk) const { return (componentData + allComponentsCount() * chunk); }
  friend class ESJob;
  friend class EntityManager;
  void reset()
  {
    totalSize = 0;
    chunkEntitiesCnt = nullptr;
    componentData = nullptr;
    chunks = 0;
    id.reset();
  }
  uint32_t chunks = 0;
  uint32_t totalSize = 0; // sum of entities in all chunks
  uint32_t *__restrict chunkEntitiesCnt = nullptr;
  QueryView::OneComponentLine *__restrict *__restrict componentData = nullptr;
  QueryId id;
  union
  {
    struct
    {
      uint8_t roCount, rwCount;
    };
    uint16_t roRW = 0;
  };
};

enum
{
  CDF_OPTIONAL = 1, // component is optional and might be absent
};

struct ComponentDesc
{
  component_t name = 0;
  component_type_t type = 0;
  uint32_t flags = 0; // bitmask of CDFlags enum
#if DAGOR_DBGLEVEL > 0
  const char *nameStr = nullptr;
#endif
  constexpr ComponentDesc(const HashedConstString n, component_type_t tp, uint32_t f = 0) :
    name(n.hash),
    type(tp),
    flags(f)
#if DAGOR_DBGLEVEL > 0
    ,
    nameStr(n.str)
#endif
  {}

  template <typename T>
  constexpr ComponentDesc(const HashedConstString n, const ComponentTypeInfo<T> &, uint32_t f = 0) :
    ComponentDesc(n, ComponentTypeInfo<T>::type, f)
  {}

  ComponentDesc() = default;
};
struct BaseQueryDesc
{
  dag::ConstSpan<ComponentDesc> componentsRW; // descriptors of components that this query needs to write
  dag::ConstSpan<ComponentDesc> componentsRO; // descriptors of components that this query needs to read
  dag::ConstSpan<ComponentDesc> componentsRQ; // descriptors of components that this query needs to exist
  dag::ConstSpan<ComponentDesc> componentsNO; // descriptors of components that this query needs to not exist
  BaseQueryDesc(dag::ConstSpan<ComponentDesc> comps_rw, dag::ConstSpan<ComponentDesc> comps_ro, dag::ConstSpan<ComponentDesc> comps_rq,
    dag::ConstSpan<ComponentDesc> comps_no) :
    componentsRW(comps_rw), componentsRO(comps_ro), componentsRQ(comps_rq), componentsNO(comps_no)
  {}
  static inline bool equal_list(dag::ConstSpan<ComponentDesc> a, dag::ConstSpan<ComponentDesc> b)
  {
    if (a.size() != b.size())
      return false;
    for (uint32_t i = 0, e = a.size(); i != e; ++i)
      if (
        a[i].name != b[i].name || a[i].flags != b[i].flags ||
        (a[i].type != b[i].type && a[i].type != ComponentTypeInfo<auto_type>::type && b[i].type != ComponentTypeInfo<auto_type>::type))
        return false;
    return true;
  }
  bool operator==(const BaseQueryDesc &b) const
  {
    return equal_list(componentsRW, b.componentsRW) && equal_list(componentsRO, b.componentsRO) &&
           equal_list(componentsRQ, b.componentsRQ) && equal_list(componentsNO, b.componentsNO);
  }
};

template <unsigned N>
constexpr int comp_hash_index_of(const ComponentDesc (&arr)[N], const component_t n, int i = 0)
{
  return i >= N ? -1 : arr[i].name == n ? i : comp_hash_index_of(arr, n, i + 1);
}

template <int N>
struct ComponentSystemIndex
{
  static constexpr int value = N;
  static_assert(N >= 0, "component not found");
};

#define ECS_QUERY_COMP_INDEX(carr, name) ecs::ComponentSystemIndex<ecs::comp_hash_index_of(carr, ECS_HASH(name).hash)>::value

#define ECS_QUERY_COMP_RO_INDEX(carr, name) ((uint16_t)(ECS_QUERY_COMP_INDEX(carr, name)))
#define ECS_QUERY_COMP_RW_INDEX(carr, name) ((uint16_t)(ECS_QUERY_COMP_INDEX(carr, name)))

#define ECS_QUERY_COMP_RO_PTR(T, carr, name) components.getComponentRawRO<T>(ECS_QUERY_COMP_RO_INDEX(carr, name))

#define ECS_QUERY_COMP_RW_PTR(T, carr, name) components.getComponentRawRW<T>(ECS_QUERY_COMP_RW_INDEX(carr, name))

#define ECS_RW_COMP(carr, cname, T)      components.getComponentRW<T>(ECS_QUERY_COMP_RW_INDEX(carr, cname), comp)
#define ECS_RO_COMP(carr, cname, T)      components.getComponentRO<T>(ECS_QUERY_COMP_RO_INDEX(carr, cname), comp)
#define ECS_RO_COMP_OR(carr, cname, def) components.getComponentRODef(ECS_QUERY_COMP_RO_INDEX(carr, cname), comp, def)
#define ECS_RO_COMP_PTR(carr, cname, T)  components.getComponentROOpt<T>(ECS_QUERY_COMP_RO_INDEX(carr, cname), comp)
#define ECS_RW_COMP_PTR(carr, cname, T)  components.getComponentRWOpt<T>(ECS_QUERY_COMP_RW_INDEX(carr, cname), comp)

//#define ECS_QUERY_COMP_PTR(T, name, RORW, chunk, carr, query)\
//  query.getComponentRaw_##RORW##<T>(ECS_QUERY_COMP_INDEX(carr##_##RORW, name), chunk)

//#define ECS_QUERY_COMP_RO_PTR(T, name, chunk, carr, query)\
//  ECS_QUERY_COMP_PTR(T, name, ro, chunk, carr, query)

//#define ECS_QUERY_COMP_RW_PTR(T, name, chunk, carr, query)\
//  ECS_QUERY_COMP_PTR(T, name, rw, chunk, carr, query)

struct NamedQueryDesc : public BaseQueryDesc
{
  const char *name = NULL; // name of this entity system, must be unique
  NamedQueryDesc(const char *n, dag::ConstSpan<ComponentDesc> comps_rw, dag::ConstSpan<ComponentDesc> comps_ro,
    dag::ConstSpan<ComponentDesc> comps_rq, dag::ConstSpan<ComponentDesc> comps_no) :
    BaseQueryDesc(comps_rw, comps_ro, comps_rq, comps_no), name(n)
  {}

  // this constructor is to be removed, for transition period
  NamedQueryDesc(const char *n, dag::ConstSpan<ComponentDesc> comps_rw, dag::ConstSpan<ComponentDesc> comps_ro) :
    NamedQueryDesc(n, comps_rw, comps_ro, {}, {})
  {}
};
typedef NamedQueryDesc EntitySystemComponentsDesc; // backwards ecs20 compatibility

typedef QueryView EntityComponents; // backwards ecs20 compatibility

class EntityManager;

struct CompileTimeQueryDesc : public NamedQueryDesc
{
  CompileTimeQueryDesc(const char *n, dag::ConstSpan<ComponentDesc> comps_rw, dag::ConstSpan<ComponentDesc> comps_ro,
    dag::ConstSpan<ComponentDesc> comps_rq, dag::ConstSpan<ComponentDesc> comps_no, uint32_t def_quant = 0) :
    NamedQueryDesc(n, comps_rw, comps_ro, comps_rq, comps_no), quant(def_quant)
  {
    next = tail;
    tail = this;
  }
  QueryId getHandle() const { return query; }
  uint32_t getQuant() const { return quant; }
  void setQuant(uint16_t q) { quant = q; }
  template <typename Lambda>
  friend void iterate_compile_time_queries(Lambda fn);

protected:
  QueryId query;
  uint32_t quant = 0;
  friend class EntityManager;
  CompileTimeQueryDesc *next = NULL; // slist
  static CompileTimeQueryDesc *tail;
};

template <typename Lambda>
inline void iterate_compile_time_queries(Lambda fn)
{
  for (CompileTimeQueryDesc *q = CompileTimeQueryDesc::tail; q;)
  {
    CompileTimeQueryDesc *nextQuery = q->next;
    if (fn(q))
      break;
    q = nextQuery;
  }
}


struct QueryStackData
{
  ChunkedVectorSingleThreaded<QueryView::OneComponentLine *, 10> componentData;
  ChunkedVectorSingleThreaded<uint32_t, 8> chunkEntitiesCnt;
  void collapse();
};

#undef QUERY_FAST_ASSERT
#undef DECL_RESTRICT

#if _ECS_CODEGEN
#define ECS_UNUSED(a)                      a
#define ECS_REQUIRE_ONE(a)                 __attribute__((annotate("@require:" #a)))
#define ECS_REQUIRE(...)                   ECS_FOR_EACH(ECS_REQUIRE_ONE, __VA_ARGS__)
#define ECS_REQUIRE_NOT_ONE(a)             __attribute__((annotate("@require_not:" #a)))
#define ECS_REQUIRE_NOT(...)               ECS_FOR_EACH(ECS_REQUIRE_NOT_ONE, __VA_ARGS__)
#define ECS_CAN_PARALLEL_FOR(a, def_quant) __attribute__((annotate("@can_parallel_for:" #def_quant))) a
#else
#define ECS_UNUSED(a)
#define ECS_REQUIRE_ONE(a)
#define ECS_REQUIRE(...)
#define ECS_REQUIRE_NOT_ONE(a)
#define ECS_REQUIRE_NOT(...)
#define ECS_CAN_PARALLEL_FOR(a, def_quant) a
#endif

}; // namespace ecs
// special overload for make_span
inline dag::ConstSpan<ecs::ComponentDesc> empty_span(const ecs::ComponentDesc *) { return {}; }
inline dag::ConstSpan<ecs::ComponentDesc> empty_span() { return {}; }
