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
const unsigned ENTITY_INDEX_MASK = (1 << ENTITY_INDEX_BITS) - 1;

const unsigned ENTITY_GENERATION_BITS = 8;
const unsigned ENTITY_GENERATION_MASK = (1 << ENTITY_GENERATION_BITS) - 1;
const entity_id_t ECS_INVALID_ENTITY_ID_VAL = 0;
#define INVALID_ENTITY_ID EntityId(ecs::ECS_INVALID_ENTITY_ID_VAL)

class EntityManager;

class EntityId
{
public:
  EntityId() = default;
  explicit EntityId(entity_id_t h) : handle(h) {}
  EntityId(const EntityId &) = default;
  EntityId &operator=(const EntityId &) = default;
  explicit operator entity_id_t() const { return handle; }
  explicit operator bool() const { return handle != ECS_INVALID_ENTITY_ID_VAL; }
  bool operator==(const EntityId &rhs) const { return handle == rhs.handle; }
  bool operator!=(const EntityId &rhs) const { return handle != rhs.handle; }
  bool operator<(const EntityId &rhs) const { return handle < rhs.handle; }
  void reset() { handle = ECS_INVALID_ENTITY_ID_VAL; }
  entity_id_t value() const { return handle; }
  unsigned index() const { return handle & ENTITY_INDEX_MASK; } // Note: trim high generation bits

private:
  friend class ecs::EntityManager;
  friend unsigned get_generation(const EntityId);
  entity_id_t handle = ECS_INVALID_ENTITY_ID_VAL;
  unsigned generation() const { return (handle >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK; }
};

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
