//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <util/dag_hash.h>

namespace ecs
{
typedef uint32_t entity_id_t;

const unsigned ENTITY_INDEX_BITS = 22;

const unsigned ENTITY_GENERATION_BITS = 10;
const unsigned ENTITY_GENERATION_MASK = (1 << ENTITY_GENERATION_BITS) - 1;
const entity_id_t ECS_INVALID_ENTITY_ID_VAL = 0;
#define INVALID_ENTITY_ID EntityId(ecs::ECS_INVALID_ENTITY_ID_VAL)

class EntityManager;

class EntityId
{
public:
  constexpr EntityId() = default;
  explicit constexpr EntityId(entity_id_t h) : handle(h) {}
  constexpr EntityId(const EntityId &) = default;
  constexpr EntityId &operator=(const EntityId &) = default;
  explicit constexpr operator entity_id_t() const { return handle; }
  explicit constexpr operator bool() const { return handle != ECS_INVALID_ENTITY_ID_VAL; }
  constexpr bool operator==(const EntityId &rhs) const { return handle == rhs.handle; }
  constexpr bool operator!=(const EntityId &rhs) const { return handle != rhs.handle; }
  constexpr bool operator<(const EntityId &rhs) const { return handle < rhs.handle; }
  constexpr void reset() { handle = ECS_INVALID_ENTITY_ID_VAL; }
  constexpr entity_id_t value() const { return handle; }
  constexpr unsigned index() const { return handle >> ENTITY_GENERATION_BITS; } // Note: trims generation bits

private:
  friend class ecs::EntityManager;
  friend unsigned get_generation(const EntityId);
  entity_id_t handle = ECS_INVALID_ENTITY_ID_VAL;
  constexpr unsigned generation() const { return handle & ENTITY_GENERATION_MASK; }
};

static_assert(EntityId().value() == ECS_INVALID_ENTITY_ID_VAL, "default constructed EntityId must be invalid");

struct EidHash
{
  size_t operator()(ecs::EntityId val) const { return static_cast<size_t>(static_cast<ecs::entity_id_t>(val)); }
};
struct EidHashFNV1a
{
  size_t operator()(ecs::EntityId val) const { return fnv1a_step<32>(ecs::entity_id_t(val)); }
};

}; // namespace ecs

template <typename T>
struct DebugConverter;
template <>
struct DebugConverter<ecs::EntityId>
{
  static ecs::entity_id_t getDebugType(const ecs::EntityId &v) { return v.value(); }
};
