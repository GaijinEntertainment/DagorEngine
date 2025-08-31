// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shLog.h"
#include "commonUtils.h"

#include <generic/dag_enumerate.h>
#include <dag/dag_vector.h>
#include <util/dag_globDef.h>
#include <debug/dag_assert.h>
#include <generic/dag_expected.h>
#include <generic/dag_tab.h>

#include <EASTL/array.h>
#include <EASTL/string.h>
#include <EASTL/optional.h>

#include <cstdint>

enum HlslRegisterSpace : uint8_t
{
  HLSL_RSPACE_T = 0,
  HLSL_RSPACE_S,
  HLSL_RSPACE_C,
  HLSL_RSPACE_U,
  HLSL_RSPACE_B,

  HLSL_RSPACE_COUNT,
  HLSL_RSPACE_INVALID
};

inline constexpr eastl::array<HlslRegisterSpace, HLSL_RSPACE_COUNT> HLSL_RSPACE_ALL_LIST = {
  HLSL_RSPACE_T,
  HLSL_RSPACE_S,
  HLSL_RSPACE_C,
  HLSL_RSPACE_U,
  HLSL_RSPACE_B,
};

inline constexpr eastl::array<const char *, HLSL_RSPACE_COUNT> HLSL_RSPACE_ALL_NAMES = {
  "texture",
  "sampler",
  "constant",
  "uav",
  "constbuffer",
};

inline constexpr eastl::array<char, HLSL_RSPACE_COUNT> HLSL_RSPACE_ALL_SYMBOLS = {
  't',
  's',
  'c',
  'u',
  'b',
};

inline void for_each_hlsl_reg_space(auto &&cb)
{
  for (HlslRegisterSpace space : HLSL_RSPACE_ALL_LIST)
    cb(space);
}

inline HlslRegisterSpace symbol_to_hlsl_reg_space(char sym)
{
  for (auto [id, c] : enumerate(HLSL_RSPACE_ALL_SYMBOLS))
    if (c == sym)
      return HlslRegisterSpace(id);
  return HLSL_RSPACE_INVALID;
}

enum class HlslSlotSemantic : uint8_t
{
  ALLOCATED,
  HARDCODED,

  RESERVED,
  RESERVED_FOR_PREDEFINES,
  RESERVED_FOR_IMPLICIT_CONST_CBUF,
  RESERVED_FOR_GLOBAL_CONST_CBUF,
  RESERVED_FOR_MATERIAL_PARAMS_CBUF,
};

struct HlslRegRange
{
  int32_t min = INT32_MAX, cap = -1;
};

inline HlslRegRange update_range(HlslRegRange range, int32_t reg)
{
  G_ASSERT(reg >= 0);
  return {min(range.min, reg), max(range.cap, reg + 1)};
}

inline HlslRegRange update_range(HlslRegRange range, HlslRegRange other)
{
  return {min(range.min, other.min), max(range.cap, other.cap)};
}

class HlslRegAllocator
{
  using allocator_scan_routine_t = int32_t (*)(HlslRegAllocator &, uint32_t);

public:
  static constexpr const char *SLOT_SEMANTIC_DESCS[]{
    "allocated",                // ALLOCATED
    "hardcoded",                // HARDCODED
    "supp blk",                 // RESERVED
    "predefined",               // RESERVED_FOR_PREDEFINES
    "implicit const buf",       // RESERVED_FOR_IMPLICIT_CONST_CBUF
    "global const buf",         // RESERVED_FOR_GLOBAL_CONST_CBUF
    "material params const buf" // RESERVED_FOR_MATERIAL_PARAMS_CBUF
  };

  static const allocator_scan_routine_t DEFAULT_SCAN, BACKWARDS_SCAN;
  struct Policy
  {
    uint32_t base = 0, cap = 0;
    allocator_scan_routine_t scanner = DEFAULT_SCAN;
  };

private:
  struct Slot
  {
    bool used : 1 = false;
    HlslSlotSemantic semantic : 7 = HlslSlotSemantic::ALLOCATED;
  };

  static_assert(sizeof(Slot) == 1);

  dag::Vector<Slot> slots{};
  uint32_t minUsed, usedCap; // min >= cap means none are used
  Policy policy{};

public:
  HlslRegAllocator() = default;
  explicit HlslRegAllocator(Policy policy) : policy{policy}, minUsed{policy.base + policy.cap}, usedCap{policy.base} {}

  HlslRegRange maxAllowedRange() const { return {int32_t(policy.base), int32_t(policy.cap)}; }

  HlslRegRange getRange() const
  {
    if (hasRegs())
      return {int32_t(minUsed), int32_t(usedCap)};
    else
      return {0, 0};
  }
  HlslRegRange getRange(HlslSlotSemantic semantic) const
  {
    return getRangeImpl([semantic](Slot s) { return s.used && s.semantic == semantic; });
  }

  bool hasRegs() const { return usedCap > minUsed; }
  bool hasRegs(HlslSlotSemantic semantic) const
  {
    auto [min, cap] = getRange(semantic);
    return cap > min;
  }

  int32_t allocate(uint32_t count = 1)
  {
    G_ASSERT(count > 0);
    int32_t reg = (*policy.scanner)(*this, count);
    if (reg >= 0)
    {
      G_ASSERT(reg >= policy.base);
      G_ASSERT(reg + count <= policy.cap);
      for (auto i = reg; i < reg + count; ++i)
      {
        G_ASSERT(!slots[i].used);
        slots[i].used = true;
        slots[i].semantic = HlslSlotSemantic::ALLOCATED;
      }
      updateRange(reg, count);
    }
    return reg;
  }

  // @TODO: do the void specialization for Expected!
  struct ReserveSuccess
  {};
  struct ReserveFailure
  {
    bool outOfRange = false;
    int32_t conflictReg = -1;
    HlslSlotSemantic conflictSemantic;
  };

  eastl::optional<HlslSlotSemantic> peekSlotSemantic(int32_t reg) const
  {
    if (reg >= slots.size() || !slots[reg].used)
      return eastl::nullopt;
    return slots[reg].semantic;
  }

  dag::Expected<ReserveSuccess, ReserveFailure> reserve(HlslSlotSemantic semantic, int32_t reg, uint32_t count = 1)
  {
    G_ASSERT(reg >= 0);
    G_ASSERTF(semantic != HlslSlotSemantic::ALLOCATED, "Can't reserve a register as if it were allocated");
    if ((reg < policy.base || reg > policy.cap - count) && semantic != HlslSlotSemantic::RESERVED_FOR_PREDEFINES)
      return dag::Unexpected{ReserveFailure{.outOfRange = true}};
    for (int i = reg; i < min(uint32_t(reg) + count, uint32_t(slots.size())); ++i)
    {
      Slot &dest = slots[i];
      if (dest.used)
      {
        // @NOTE: this allows aliasing hardcoding registers, which is a real use-case
        if (dest.semantic == HlslSlotSemantic::ALLOCATED || dest.semantic != semantic)
          return dag::Unexpected{ReserveFailure{.conflictReg = i, .conflictSemantic = dest.semantic}};
      }
    }
    if (reg + count > slots.size())
      slots.resize(reg + count);
    for (int i = reg; i < reg + count; ++i)
    {
      slots[i].used = true;
      slots[i].semantic = semantic;
    }
    updateRange(reg, count);
    return ReserveSuccess{};
  }

  dag::Expected<ReserveSuccess, Tab<ReserveFailure>> reserveAllFrom(const HlslRegAllocator &supp)
  {
    G_ASSERT(policy.base == supp.policy.base);
    G_ASSERT(policy.cap == supp.policy.cap);
    if (slots.size() < supp.slots.size())
      slots.resize(supp.slots.size());
    Tab<ReserveFailure> failedReserves{};
    for (auto [id, slot] : enumerate(supp.slots))
    {
      if (slot.used)
      {
        if (auto result = reserve(max(HlslSlotSemantic::RESERVED, slot.semantic), id); !result)
          failedReserves.push_back(result.error());
      }
    }
    if (failedReserves.empty())
      return ReserveSuccess{};
    else
      return dag::Unexpected{eastl::move(failedReserves)};
  }

  // @HACK: in order to undo __static_cbuf in shaders if no buffered consts are used
  bool unreserveIfUsed(HlslSlotSemantic semantic, int32_t reg, uint32_t count = 1)
  {
    G_ASSERT(reg >= 0);
    if (semantic <= HlslSlotSemantic::HARDCODED)
      return false;
    if (reg < policy.base || reg > policy.cap - count)
      return false;
    const int cap = min(uint32_t(reg) + count, uint32_t(slots.size()));
    for (int i = reg; i < cap; ++i)
    {
      if (slots[i].used && slots[i].semantic == semantic)
        slots[i].used = false;
    }
    recalculateRange();
    return true;
  }

  void dumpUsage(HlslRegisterSpace space, auto &&logger, auto &&info_provider) const
  {
    auto [min, cap] = maxAllowedRange();
    logger("Used registers in allowed range [%d, %d]:", min, cap - 1);
    int skip = 0;
    for (auto [ind, slot] : enumerate(slots))
    {
      if (skip > 0)
        --skip;
      else if (slot.used)
      {
        auto [name, size] = info_provider(ind);
        logger("  %c%-3d %-5s: %s, %s", HLSL_RSPACE_ALL_SYMBOLS[space], ind, size == 1 ? "" : string_f("[%d]", size).c_str(),
          name.c_str(), SLOT_SEMANTIC_DESCS[size_t(slot.semantic)]);
        skip = size - 1;
      }
    }
  }

  template <class TPred>
  HlslRegRange getRangeImpl(TPred &&pred) const
  {
    uint32_t begin = 0, cap = 0;
    for (uint32_t i = 0; i < slots.size(); ++i)
      if (pred(slots[i]))
      {
        begin = i;
        break;
      }
    for (uint32_t i = slots.size() - 1; i != uint32_t(-1); --i)
      if (pred(slots[i]))
      {
        cap = i + 1;
        break;
      }

    return {int32_t(begin), int32_t(cap)};
  }

  void updateRange(int32_t reg, uint32_t count = 1)
  {
    G_ASSERT(reg >= 0 && count > 0);
    minUsed = min(uint32_t(reg), minUsed);
    usedCap = max(uint32_t(reg) + count, usedCap);
  }
  void recalculateRange()
  {
    auto [min, cap] = getRangeImpl([](Slot s) { return s.used; });
    minUsed = uint32_t(min);
    usedCap = uint32_t(cap);
  }
};

eastl::array<HlslRegAllocator, HLSL_RSPACE_COUNT> make_default_hlsl_reg_allocators(int max_const_count);
HlslRegAllocator make_default_cbuf_reg_allocator();

inline eastl::string get_reg_alloc_dump(const HlslRegAllocator &alloc, HlslRegisterSpace rspace, auto &&info_provider)
{
  eastl::string usageDump{};
  alloc.dumpUsage(
    rspace,
    [&usageDump]<class... Ts>(const char *fmt, Ts &&...args) {
      usageDump.append_sprintf(fmt, eastl::forward<Ts>(args)...);
      usageDump.push_back('\n');
    },
    info_provider);
  return usageDump;
}

inline void report_reg_reserve_failed(const char *name, int reg, int count, HlslRegisterSpace rspace,
  HlslSlotSemantic desired_semantic, HlslRegAllocator::ReserveFailure failure, const HlslRegAllocator &alloc, auto &&info_provider)
{
  eastl::string collisionInfo{};
  if (failure.outOfRange)
  {
    auto [minAllowed, capAllowed] = alloc.maxAllowedRange();
    collisionInfo.sprintf("out of range [%d, %d] allowed for space '%c'", minAllowed, capAllowed - 1, HLSL_RSPACE_ALL_SYMBOLS[rspace]);
  }
  else
  {
    auto [name, _] = info_provider(failure.conflictReg);
    const eastl::string nameRef = name.empty() ? "" : string_f(" (%s)", name.c_str());
    collisionInfo.sprintf("already occupied by a %s param%s", HlslRegAllocator::SLOT_SEMANTIC_DESCS[size_t(failure.conflictSemantic)],
      nameRef.c_str());
  }

  const eastl::string nameRef = name ? string_f(" '%s'", name) : "";
  sh_debug(SHLOG_ERROR, "Trying to reserve a %s param%s with register '%c%d' of size %d, which is %s\n%s",
    HlslRegAllocator::SLOT_SEMANTIC_DESCS[size_t(desired_semantic)], nameRef.c_str(), HLSL_RSPACE_ALL_SYMBOLS[rspace], reg, count,
    collisionInfo.c_str(), get_reg_alloc_dump(alloc, rspace, info_provider).c_str());
}