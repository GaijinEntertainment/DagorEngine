#pragma once

#include <util/dag_stdint.h>
#include <EASTL/numeric_limits.h>

namespace ecs
{
typedef uint32_t components_masks_t[2]; // first rw, second ro. should be removed, I assume

typedef uint32_t component_type_t; // this is hash of name
typedef uint32_t component_t;      // this is hash of name

typedef uint16_t type_index_t; // limit types to 64k
static constexpr type_index_t INVALID_COMPONENT_TYPE_INDEX = EASTL_LIMITS_MAX_U(type_index_t);

typedef uint16_t component_index_t; // limit different components 64k
static constexpr component_index_t INVALID_COMPONENT_INDEX = EASTL_LIMITS_MAX_U(component_index_t);

typedef uint16_t template_t; // limit different templates to 64k
static constexpr template_t INVALID_TEMPLATE_INDEX = EASTL_LIMITS_MAX_U(template_t);

typedef uint16_t archetype_component_id;
static constexpr archetype_component_id INVALID_ARCHETYPE_COMPONENT_ID = EASTL_LIMITS_MAX_U(archetype_component_id);

typedef uint32_t ecs_query_handle_t;
static constexpr ecs_query_handle_t INVALID_QUERY_HANDLE_VAL = EASTL_LIMITS_MAX_U(ecs_query_handle_t);

static constexpr uint32_t max_alignment_bits = 4; // if we wil be working in AVX, it will have to be 256 bits, i.e. 32. Currently
                                                  // assume it is 16
static constexpr uint32_t max_alignment = 1 << max_alignment_bits; // if we wil be working in AVX, it will have to be 256 bits,
                                                                   // i.e. 32. Currently assume it is 16

// all data components to be stored in a very simple way.
// one chunk (persumably allocated per template) have all of data components in linear order, separated by capacity

typedef uint8_t chunk_type_t;
typedef uint16_t id_in_chunk_type_t;
// #define ECS_MAX_CHUNK_ID_BITS 16
static constexpr int ECS_MAX_CHUNK_ID_BITS = sizeof(id_in_chunk_type_t) * 8;
static constexpr int MAX_CHUNKS_COUNT = eastl::numeric_limits<chunk_type_t>::max();

typedef uint16_t archetype_t;
static constexpr archetype_t INVALID_ARCHETYPE = EASTL_LIMITS_MAX_U(archetype_t);


typedef uint16_t component_flags_t;

struct FastGetInfo
{
  component_index_t cidx = INVALID_COMPONENT_INDEX;
  component_index_t getCidx() const { return cidx; }
  bool canBeTracked() const { return canTrack; }
  bool isValid() const { return valid; }

protected:
  bool canTrack = true;
  bool valid = true;
  friend struct LTComponentList;
  friend class EntityManager;
  friend struct DataComponents;
};

#ifndef DAECS_EXTENSIVE_CHECKS
#ifdef _DEBUG_TAB_
#define DAECS_EXTENSIVE_CHECKS (DAGOR_DBGLEVEL > 0)
#else
#define DAECS_EXTENSIVE_CHECKS 0
#endif
#endif

#if DAGOR_DBGLEVEL > 0
#define DAECS_RELEASE_INLINE inline
#else
#define DAECS_RELEASE_INLINE __forceinline
#endif

}; // namespace ecs
