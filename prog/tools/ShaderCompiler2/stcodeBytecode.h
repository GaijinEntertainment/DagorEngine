// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shaderTab.h"
#include "commonUtils.h"
#include "shErrorReporting.h"
#include <shaders/dag_shaderVarType.h>
#include <debug/dag_debug.h>
#include <EASTL/optional.h>
#include <EASTL/array.h>
#include <EASTL/vector_map.h>

class Register;

class StcodeVMRegisterAllocator
{
  // shadervar id, opcode -> register
  eastl::vector_map<eastl::pair<int, int>, Register> stVarToReg{};
  eastl::array<int, 256> usedRegs{};
  int maxregsize = 0;

  Parser &parser; // for allocation failure reporting

  friend class Register;

public:
  explicit StcodeVMRegisterAllocator(Parser &parser) : parser{parser} {}

  int requiredRegCount() const { return maxregsize; }

  Register _add_reg_word32(int num, bool aligned);
  Register add_vec_reg(int num = 1);
  Register add_reg();
  Register add_reg(int type);
  Register add_resource_reg();
  eastl::optional<Register> registerStvarGetter(int var_id, int getter_opcode, Register new_reg);

  void manuallyReleaseRegister(int reg);
  void dropCachedStvars();
};

class Register
{
  int startReg = -1;
  int count = 0;
  int currentReg = -1;
  StcodeVMRegisterAllocator *owner = nullptr;

  friend class StcodeVMRegisterAllocator;

  void acquireRegs(int r, int num)
  {
    startReg = r;
    count = num;
    currentReg = r;
    for (int i = startReg; i < startReg + count; i++)
      owner->usedRegs[i]++;
  }
  void releaseRegs()
  {
    for (int i = startReg; i < startReg + count; i++)
    {
      G_ASSERT(owner->usedRegs[i] > 0);
      owner->usedRegs[i]--;
    }
    eastl::move(*this).release();
  }

  Register(int reg, StcodeVMRegisterAllocator &owner) : startReg(reg), count(4), currentReg(reg), owner(&owner) {}

  Register(int num, bool aligned, StcodeVMRegisterAllocator &owner) : count(num), owner(&owner)
  {
    for (int i = 0; i <= owner.usedRegs.size() - num; i += (aligned ? 4 : 1))
    {
      if (eastl::all_of(&owner.usedRegs[i], &owner.usedRegs[i + num], [](int used) { return used == 0; }))
      {
        acquireRegs(i, num);
        return;
      }
    }
    report_error(owner.parser, nullptr, "Unable to allocate registers");
  }

  void swap(Register &r)
  {
    eastl::swap(startReg, r.startReg);
    eastl::swap(currentReg, r.currentReg);
    eastl::swap(count, r.count);
    eastl::swap(owner, r.owner);
  }

public:
  Register() = default;
  Register(const Register &r) : owner(r.owner)
  {
    acquireRegs(r.startReg, r.count);
    currentReg = r.currentReg;
  }
  Register &operator=(const Register &r)
  {
    Register re(r);
    re.swap(*this);
    return *this;
  }
  Register(Register &&r) noexcept { r.swap(*this); }
  Register &operator=(Register &&r) noexcept
  {
    Register re(eastl::move(r));
    re.swap(*this);
    return *this;
  }
  ~Register() { releaseRegs(); }

  int release() &&
  {
    int r = currentReg;
    startReg = -1;
    currentReg = -1;
    count = 0;
    return r;
  }

  void reset(int r, int num)
  {
    releaseRegs();
    G_ASSERT(eastl::all_of(&owner->usedRegs[r], &owner->usedRegs[r + num], [](int used) { return used > 0; }));
    acquireRegs(r, num);
  }

  explicit operator int() const
  {
    G_ASSERT(startReg != -1 && count > 0 && currentReg != -1);
    return currentReg;
  }
};

inline Register StcodeVMRegisterAllocator::_add_reg_word32(int num, bool aligned)
{
  Register reg(num, aligned, *this);
  if (maxregsize < int(reg) + num)
    maxregsize = int(reg) + num;
  return reg;
}

inline Register StcodeVMRegisterAllocator::add_vec_reg(int num) { return _add_reg_word32(4 * num, true); }
inline Register StcodeVMRegisterAllocator::add_reg() { return _add_reg_word32(1, false); }
inline Register StcodeVMRegisterAllocator::add_resource_reg() { return _add_reg_word32(2, false); }

inline Register StcodeVMRegisterAllocator::add_reg(int type)
{
  switch (type)
  {
    case SHVT_BUFFER:
    case SHVT_TLAS:
    case SHVT_TEXTURE: return add_resource_reg();
    case SHVT_INT4:
    case SHVT_COLOR4: return add_vec_reg();
    case SHVT_INT: return add_reg();
    case SHVT_FLOAT4X4: return add_vec_reg(4);
    default: G_ASSERT(0);
  }
  return {};
}

inline eastl::optional<Register> StcodeVMRegisterAllocator::registerStvarGetter(int var_id, int getter_opcode, Register new_reg)
{
  auto [it, inserted] = stVarToReg.emplace(eastl::make_pair(var_id, getter_opcode), new_reg);
  if (!inserted)
    return it->second;
  else
    return eastl::nullopt;
}

inline void StcodeVMRegisterAllocator::manuallyReleaseRegister(int reg) { Register raiiReg{reg, *this}; }

inline void StcodeVMRegisterAllocator::dropCachedStvars() { stVarToReg.clear(); }

struct StcodeBytecodeAccumulator
{
  Tab<int> stcode;
  Tab<int> stblkcode;
  eastl::unique_ptr<StcodeVMRegisterAllocator> regAllocator;

  explicit StcodeBytecodeAccumulator(Parser &parser) : regAllocator{new StcodeVMRegisterAllocator{parser}} {}

  void push_stcode(int a) { stcode.push_back(a); }
  void push_stblkcode(int a) { stblkcode.push_back(a); }
  void append_stcode(dag::ConstSpan<int> c) { append_items(stcode, c.size(), c.data()); }
  void append_stblkcode(dag::ConstSpan<int> c) { append_items(stblkcode, c.size(), c.data()); }

  void push_alt_stcode(bool dyn, int a)
  {
    if (dyn)
      push_stcode(a);
    else
      push_stblkcode(a);
  }
  void append_alt_stcode(bool dyn, dag::ConstSpan<int> c)
  {
    if (dyn)
      append_stcode(c);
    else
      append_stblkcode(c);
  }
  Tab<int> &get_alt_curstcode(bool dyn) { return dyn ? stcode : stblkcode; }

  void reset()
  {
    stcode.clear();
    stblkcode.clear();
  }
};

class StcodeBytecodeCache
{
  Tab<TabStcode> stcode{};
  Tab<TabStcode> stblkcode{};

public:
  StcodeBytecodeCache() = default;
  ~StcodeBytecodeCache() { debug("[stat] stcode.count=%d  stblk.size()=%d", stcode.size(), stblkcode.size()); }

  PINNED_TYPE(StcodeBytecodeCache);

  struct Refs
  {
    dag::ConstSpan<int> stcode;
    dag::ConstSpan<int> stblkcode;
  };

  Refs findOrPost(StcodeBytecodeAccumulator &&a_code)
  {
    return {findOrPost(eastl::move(a_code.stcode), true), findOrPost(eastl::move(a_code.stblkcode), false)};
  }

private:
  dag::ConstSpan<int> findOrPost(Tab<int> &&a_routine, bool dyn);
};
