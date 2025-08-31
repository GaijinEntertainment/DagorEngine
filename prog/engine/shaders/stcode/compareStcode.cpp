// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compareStcode.h"
#include "../shadersBinaryData.h"
#include <shaders/dag_stcode.h>
#include <math/dag_mathBase.h>
#include <debug/dag_assert.h>

#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include <util/dag_stlqsort.h>
#include <cmath>

namespace stcode::dbg
{

// @NOTE: this code is basically throwaway (since it compares new stcode to the old one, and
//        not to some objective metric of correctness), thus it is not really good, and should be removed
//        once a decision is made which version of stcode to proceed with.

static bool allow_float_epsilon_mismatch = true;

void require_exact_record_matches() { allow_float_epsilon_mismatch = false; }

template <class... TArgs>
static auto strf(const char *fmt, TArgs &&...args) -> eastl::string
{
  return eastl::string{eastl::string::CtorSprintf{}, fmt, args...};
};

static bool equal_as_floats_inexact(const auto &fdata1, const auto &fdata2)
{
  static_assert(sizeof(fdata1) == sizeof(float) && sizeof(fdata2) == sizeof(float));
  return is_relative_equal_float(*(const float *)&fdata1, *(const float *)&fdata2);
}

static bool numbers_are_equal(const auto &data1, const auto &data2)
{
  static_assert(eastl::is_same_v<decltype(data1), decltype(data2)>);
  if (memcmp(&data1, &data2, sizeof(decltype(data1))) == 0)
    return true;
  if (!allow_float_epsilon_mismatch)
    return false;
  return equal_as_floats_inexact(data1, data2);
}

static bool vectors_are_equal(const auto &data1, const auto &data2)
{
  static_assert(eastl::is_same_v<decltype(data1), decltype(data2)>);
  return numbers_are_equal(data1.r, data2.r) && numbers_are_equal(data1.g, data2.g) && numbers_are_equal(data1.b, data2.b) &&
         numbers_are_equal(data1.a, data2.a);
}

static const char *stage_to_name(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_VS: return "vs";
    case STAGE_PS: return "ps";
    case STAGE_CS: return "cs";
    default: return "<invalid stage>";
  }
}

struct ObservedEffect
{
  enum Type
  {
    SET_CONST = 0,
    SET_TEX,
    SET_SAMPLER,
    SET_BUFFER,
    SET_CBUFFER,
    SET_RWTEX,
    SET_RWBUF,
    CREATE_SAMPLER,
    SET_TLAS,
    REG_BINDLESS,
    SET_STATIC_TEX,
    SUPPORT_MULTIDRAW,
  };

  Type type;
  ShaderStage stage;
  int reg;
  int size;
  bool isFloat;
  uint32_t *val = nullptr;

  bool isSame(const ObservedEffect &other) const
  {
    if (type != other.type || stage != other.stage || reg != other.reg || size != other.size)
      return false;

    if (memcmp(val, other.val, size) == 0)
      return true;

    if (isFloat)
    {
      const uint32_t *v1 = val;
      const uint32_t *v2 = other.val;
      for (int i = 0; i < size / sizeof(uint32_t); ++i)
      {
        if (!numbers_are_equal(v1[i], v2[i]))
          return false;
      }

      return true;
    }

    return false;
  }

  void prettyLog() const
  {
    const char *stageName = stage_to_name(stage);

    switch (type)
    {
      case SET_CONST:
      {
        eastl::string msg = strf("SET_CONST (%s) : (", stageName);
        const int elemCnt = size / sizeof(float);
        for (int i = 0; i < elemCnt; ++i)
        {
          if (isFloat)
            msg += strf(i == elemCnt - 1 ? "%f" : "%f, ", ((float *)val)[i]);
          else
            msg += strf(i == elemCnt - 1 ? "%d" : "%d, ", ((int *)val)[i]);
        }
        msg += strf(") -> %d", reg);
        logwarn(msg.c_str());
      }
      break;
      case SET_TEX: logwarn("    SET_TEX (%s): %p -> %d", stageName, *(const BaseTexture **)val, reg); break;
      case SET_SAMPLER: logwarn("    SET_SAMPLER (%s): %llu -> %d", stageName, *(const uint64_t *)val, reg); break;
      case SET_BUFFER: logwarn("    SET_BUFFER (%s): %p -> %d", stageName, *(const Sbuffer **)val, reg); break;
      case SET_CBUFFER: logwarn("    SET_CBUFFER (%s): %p -> %d", stageName, *(const Sbuffer **)val, reg); break;
      case SET_RWTEX: logwarn("    SET_RWTEX (%s): %p -> %d", stageName, *(const BaseTexture **)val, reg); break;
      case SET_RWBUF: logwarn("    SET_RWBUF (%s): %p -> %d", stageName, *(const Sbuffer **)(val), reg); break;
      case CREATE_SAMPLER:
      {
        const d3d::SamplerInfo &sInfo = *(const d3d::SamplerInfo *)val;
        logwarn("    CREATE_SAMPLER (%s): mip_map_mode=%u filter_mode=%u address_mode_u=%u address_mode_v=%u address_mode_w=%u "
                "border_color=%u anisotropic_max=%f mip_map_bias=%f",
          stageName, (uint32_t)sInfo.mip_map_mode, (uint32_t)sInfo.filter_mode, (uint32_t)sInfo.address_mode_u,
          (uint32_t)sInfo.address_mode_v, (uint32_t)sInfo.address_mode_w, (uint32_t)sInfo.border_color, (float)sInfo.anisotropic_max,
          (float)sInfo.mip_map_bias, reg);
      }
      break;
      case SET_TLAS: logwarn("    SET_TLAS (%s): %p -> %d", stageName, *(RaytraceTopAccelerationStructure **)(val), reg); break;
      case REG_BINDLESS: logwarn("    REG_BINDLESS: %u -> %d", cpp::uint(*(const TEXTUREID *)val), reg); break;
      case SET_STATIC_TEX: logwarn("    SET_STATIC_TEX (%s): %u -> %d", stageName, *(const unsigned *)(val), reg); break;
      case SUPPORT_MULTIDRAW: logwarn("    SUPPORT_MULTIDRAW: %s", *(const bool *)(val) ? "true" : "false"); break;
    }
  }
};

class StcodeEffects
{
public:
  template <class TVal, ObservedEffect::Type record_type>
  void recordEffect(int stage, cpp::uint reg, TVal val)
  {
    ObservedEffect &record = effects.emplace_back();
    record.type = record_type;
    record.stage = (ShaderStage)stage;
    record.reg = reg;
    record.size = sizeof(val);
    record.val = valueStorage.data() + valueStorage.size();
    valueStorage.resize(valueStorage.size() + record.size);
    memcpy(record.val, &val, record.size);
  }

  void recordSetConst(int stage, unsigned int id, const cpp::float4 *val, int elem_cnt)
  {
    ObservedEffect &record = effects.emplace_back();
    record.type = ObservedEffect::SET_CONST;
    record.stage = (ShaderStage)stage;
    record.reg = id;
    record.size = elem_cnt * sizeof(*val);
    record.isFloat = true; // @TODO: Need to discriminate between int and float consts to properly test ints
    record.val = valueStorage.data() + valueStorage.size();
    valueStorage.resize(valueStorage.size() + record.size);
    memcpy(record.val, val, record.size);
  }

  void sort() { stlsort::sort(effects.begin(), effects.end(), comp); }
  void reset()
  {
    effects.clear();
    valueStorage.clear();
  }

  static bool compare(StcodeEffects &first, StcodeEffects &second, const shader_layout::StcodeConstValidationMask &const_mask);

private:
  eastl::fixed_vector<ObservedEffect, 1000> effects = {};
  eastl::fixed_vector<uint32_t, (sizeof(cpp::float4) * 1000) / sizeof(uint32_t)> valueStorage = {};

  static bool comp(const ObservedEffect &e1, const ObservedEffect &e2)
  {
    if (e1.type != e2.type)
      return e1.type < e2.type;
    else if (e1.stage != e2.stage)
      return e1.stage < e2.stage;
    else
      return e1.reg < e2.reg;
  };
};

static thread_local StcodeEffects referenceEffects, cppEffects;

bool StcodeEffects::compare(StcodeEffects &first, StcodeEffects &second, const shader_layout::StcodeConstValidationMask &const_mask)
{
  bool failed = false;

  first.sort();
  second.sort();

  dag::ConstSpan<ObservedEffect> fsp{first.effects};
  dag::ConstSpan<ObservedEffect> ssp{second.effects};

  auto splitEffectsRange = [](auto &effects) {
    auto pivot = eastl::find_if(effects.cbegin(), effects.cend(),
      [](const ObservedEffect &effect) { return effect.type != ObservedEffect::SET_CONST; });
    return eastl::make_pair(dag::ConstSpan<ObservedEffect>{effects.cbegin(), pivot - effects.cbegin()},
      dag::ConstSpan<ObservedEffect>{pivot, effects.cend() - pivot});
  };

  auto [firstConstEffects, firstOtherEffects] = splitEffectsRange(first.effects);
  auto [secondConstEffects, secondOtherEffects] = splitEffectsRange(second.effects);

  // Compare non set-const effects -- they should be 1 to 1
  if (firstOtherEffects.size() != secondOtherEffects.size())
  {
    logwarn("Stcode non-numeric effects length mismatch: %ld in first, %ld in second", firstOtherEffects.size(),
      secondOtherEffects.size());
    return false;
  }
  for (size_t i = 0; i < firstOtherEffects.size(); ++i)
  {
    const auto &f = firstOtherEffects[i];
    const auto &s = secondOtherEffects[i];
    if (!f.isSame(s))
    {
      logwarn("Stcode non-numeric effects differ at #%ld", i);
      logwarn("First:");
      f.prettyLog();
      logwarn("Second:");
      s.prettyLog();
      failed = true;
    }
  }

  if (failed)
    return false;

  if (firstConstEffects.size() == 0 && secondConstEffects.size() == 0)
    return true;
  else if (firstConstEffects.size() == 0 || secondConstEffects.size() == 0)
  {
    logwarn("Stcode numeric effects mismatch: first %s, second %s",
      firstConstEffects.size() > 0 ? strf("has %ld", firstConstEffects.size()).c_str() : "has none",
      secondConstEffects.size() > 0 ? strf("has %ld", secondConstEffects.size()).c_str() : "has none");
    return false;
  }

  // Compare set-const effects -- replay them into an imaginary buffer & compare
  eastl::array<dag::Vector<cpp::float4>, STAGE_MAX> fbuffers{}, sbuffers{};

  auto replay = [&](auto effects, auto &buffers) {
    for (const ObservedEffect &effect : effects)
    {
      G_ASSERT(effect.type == ObservedEffect::SET_CONST);
      G_ASSERT(effect.size % sizeof(cpp::float4) == 0);

      int regsize = effect.size / sizeof(cpp::float4);
      auto &buf = buffers[effect.stage];
      if (buf.size() < effect.reg + regsize)
        buf.resize(effect.reg + regsize, cpp::float4{420.69f}); // deadbeef equivalent

      for (int i = 0; i < regsize; ++i)
      {
        const int regId = effect.reg + i;
        const cpp::float4 *srcData = (const cpp::float4 *)effect.val;
        float *dst[4] = {&buf[regId].r, &buf[regId].g, &buf[regId].b, &buf[regId].a};
        const float src[4] = {srcData[i].r, srcData[i].g, srcData[i].b, srcData[i].a};
        for (int j = 0; j < 4; ++j)
        {
          if (const_mask.test(regId, j, effect.stage))
            *dst[j] = src[j];
        }
      }
    }
  };

  replay(firstConstEffects, fbuffers);
  replay(secondConstEffects, sbuffers);

  for (ShaderStage stage : {STAGE_PS, STAGE_CS, STAGE_VS})
  {
    const auto &fb = fbuffers[stage];
    const auto &sb = sbuffers[stage];

    if (fb.size() != sb.size())
    {
      logwarn("Stcode const effects length mismatch for stage %s: %ld left in first, %ld left in second", stage_to_name(stage),
        fb.size(), sb.size());
      failed = true;
    }
    else
    {
      for (size_t i = 0; i < fb.size(); ++i)
      {
        if (memcmp(&fb[i], &sb[i], sizeof(cpp::float4)) != 0 && !vectors_are_equal(fb[i], sb[i]))
        {
          logwarn("Stcode const effects differ at reg #%ld for stage %s", i, stage_to_name(stage));
          logwarn("First = (%f, %f, %f, %f)", fb[i].r, fb[i].g, fb[i].b, fb[i].a);
          logwarn("Second = (%f, %f, %f, %f)", sb[i].r, sb[i].g, sb[i].b, sb[i].a);
          failed = true;
        }
      }
    }
  }

  return !failed;
}

void reset()
{
  referenceEffects.reset();
  cppEffects.reset();
}

static StcodeEffects &choose_effects(RecordType type) { return type == RecordType::CPP ? cppEffects : referenceEffects; }

void record_set_tex(RecordType type, int stage, cpp::uint reg, BaseTexture *tex)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordEffect<BaseTexture *, ObservedEffect::SET_TEX>(stage, reg, tex);
}

void record_set_buf(RecordType type, int stage, cpp::uint reg, Sbuffer *buf)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordEffect<Sbuffer *, ObservedEffect::SET_BUFFER>(stage, reg, buf);
}

void record_set_const_buf(RecordType type, int stage, cpp::uint reg, Sbuffer *buf)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordEffect<Sbuffer *, ObservedEffect::SET_CBUFFER>(stage, reg, buf);
}

void record_set_tlas(RecordType type, int stage, cpp::uint reg, RaytraceTopAccelerationStructure *tlas)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordEffect<RaytraceTopAccelerationStructure *, ObservedEffect::SET_TLAS>(stage, reg, tlas);
}

void record_set_sampler(RecordType type, int stage, cpp::uint reg, d3d::SamplerHandle handle)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordEffect<d3d::SamplerHandle, ObservedEffect::SET_SAMPLER>(stage, reg, handle);
}

void record_set_rwtex(RecordType type, int stage, cpp::uint reg, BaseTexture *tex)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordEffect<BaseTexture *, ObservedEffect::SET_RWTEX>(stage, reg, tex);
}

void record_set_rwbuf(RecordType type, int stage, cpp::uint reg, Sbuffer *buf)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordEffect<Sbuffer *, ObservedEffect::SET_RWBUF>(stage, reg, buf);
}

void record_request_sampler(RecordType type, const d3d::SamplerInfo &base_info)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordEffect<d3d::SamplerInfo, ObservedEffect::CREATE_SAMPLER>(STAGE_PS, -1, base_info);
}

void record_set_const(RecordType type, int stage, unsigned int id, const void *val, int cnt)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordSetConst(stage, id, (const cpp::float4 *)val, cnt);
}

void record_reg_bindless(RecordType type, unsigned int id, TEXTUREID tid)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordEffect<TEXTUREID, ObservedEffect::REG_BINDLESS>(STAGE_VS, id, tid);
}

void record_set_static_tex(RecordType type, int stage, cpp::uint reg, TEXTUREID tid)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordEffect<TEXTUREID, ObservedEffect::SET_STATIC_TEX>(stage, reg, tid);
}

void record_multidraw_support(RecordType type, bool is_enabled)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordEffect<bool, ObservedEffect::SUPPORT_MULTIDRAW>(STAGE_VS, 0, is_enabled);
}

void validate_accumulated_records(int cpp_routine_id, int ref_routine_id, const char *shname, bool dynamic)
{
  G_ASSERT(shBinDumpOwner().getDumpV3());

  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
  {
    const auto &mask = shBinDumpOwner().getDumpV3()->stcodeConstValidationMasks[ref_routine_id];
    if (!StcodeEffects::compare(cppEffects, referenceEffects, mask))
    {
      logerr("CPP %s shows different effects from reference, shader: %s, cpp routine id (%s): %d, reference routine id: %d",
        dynamic ? "dynstcode" : "stblkcode", shname, stcode::USE_BRANCHED_DYNAMIC_ROUTINES ? "branched" : "branchless", cpp_routine_id,
        ref_routine_id);
      G_DEBUG_BREAK;
    }
  }
}

} // namespace stcode::dbg
