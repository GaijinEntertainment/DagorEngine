//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_assert.h>
#include <util/dag_stdint.h>

// sample usage
// class EntityIdDummy {};
// typedef GenerationRefId<8, EntityIdDummy> EntityId;// 8 bits for generation
// see also generationReferenceData.h

// class T is for strong typing of different GenerationRefId with same amount of bits
template <unsigned generation_bits, class T>
class GenerationRefId
{
  G_STATIC_ASSERT(generation_bits <= 16);

public:
  constexpr GenerationRefId() = default;
  constexpr explicit GenerationRefId(uint32_t h) : handle(h) {}
  constexpr explicit operator uint32_t() const { return handle; }
  constexpr explicit operator bool() const { return handle != INVALID_ID; }
  constexpr bool operator==(const GenerationRefId &rhs) const { return handle == rhs.handle; }
  constexpr bool operator!=(const GenerationRefId &rhs) const { return handle != rhs.handle; }
  constexpr bool operator<(const GenerationRefId &rhs) const { return handle < rhs.handle; }
  constexpr bool operator>(const GenerationRefId &rhs) const { return handle > rhs.handle; }
  constexpr void reset() { handle = INVALID_ID; }
  constexpr unsigned index() const { return handle & INDEX_MASK; }
  constexpr unsigned generation() const { return (handle >> INDEX_BITS); }
  enum : uint32_t
  {
    INVALID_ID = ~0u,
    GENERATION_BITS = generation_bits,
    INDEX_BITS = 32 - generation_bits,
    INDEX_MASK = (1 << INDEX_BITS) - 1,
    GENERATION_MASK = ~INDEX_MASK
  };
  static constexpr GenerationRefId make(uint32_t index, uint32_t gen)
  {
    return GenerationRefId((index & INDEX_MASK) | (gen << INDEX_BITS));
  }

private:
  uint32_t handle = INVALID_ID;
};
