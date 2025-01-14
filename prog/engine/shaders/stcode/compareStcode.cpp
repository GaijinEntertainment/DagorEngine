// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compareStcode.h"
#include <shaders/dag_stcode.h>
#include <math/dag_mathBase.h>
#include <debug/dag_assert.h>

#include <EASTL/array.h>
#include <EASTL/string.h>
#include <util/dag_stlqsort.h>
#include <cmath>

namespace stcode::dbg
{

// @NOTE: this code is basically throwaway (since it compares new stcode to the old one, and
//        not to some objective metric of correctness), thus it is not really good, and should be removed
//        once a decision is made which version of stcode to proceed with.


template <class... TArgs>
auto strf(const char *fmt, TArgs &&...args) -> eastl::string
{
  return eastl::string{eastl::string::CtorSprintf{}, fmt, args...};
};

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
  };

  Type type;
  ShaderStage stage;
  int reg;
  int size;
  bool isFloat;
  union
  {
    uint32_t v[4 * 4];
    d3d::SamplerInfo sInfo;
  } val = {};

  static constexpr float MAX_DIFF = 0.001f;
  static constexpr float MAX_REL_DIFF = 0.0001f; // For high values

  bool isSame(const ObservedEffect &other) const
  {
    if (type != other.type || stage != other.stage || reg != other.reg || size != other.size)
      return false;

    if (memcmp(&val, &other.val, sizeof(val)) == 0)
      return true;

    if (isFloat)
    {
      float *v1 = (float *)val.v;
      float *v2 = (float *)other.val.v;
      for (int i = 0; i < size / sizeof(float); ++i)
      {
        if (!is_relative_equal_float(v1[i], v2[i], MAX_DIFF, MAX_REL_DIFF))
          return false;
      }

      return true;
    }

    return false;
  }

  bool isPackedMatrixEffect(const ObservedEffect *others, size_t others_cnt) const
  {
    if (reg != others->reg)
      return false;

    if (others_cnt != 4 || type != SET_CONST || size != 64 || !isFloat)
      return false;

    for (int i = 0; i < others_cnt; ++i)
    {
      const ObservedEffect &other = others[i];
      if (other.type != SET_CONST || other.stage != stage || other.reg != reg + i || other.size != 16 || !other.isFloat)
        return false;

      float *v1 = (float *)val.v;
      float *v2 = (float *)other.val.v;

      for (int j = 0; j < 4; ++j)
      {
        if (!is_relative_equal_float(v1[j + 4 * i], v2[j], MAX_DIFF, MAX_REL_DIFF))
          return false;
      }
    }

    return true;
  }

  void prettyLog() const
  {
    const char *stageName;
    switch (stage)
    {
      case STAGE_VS: stageName = "vs"; break;
      case STAGE_PS: stageName = "ps"; break;
      case STAGE_CS: stageName = "cs"; break;
      default: stageName = "<invalid stage>"; break;
    }

    switch (type)
    {
      case SET_CONST:
      {
        eastl::string msg = strf("SET_CONST (%s) : (", stageName);
        const int elemCnt = size / sizeof(float);
        for (int i = 0; i < elemCnt; ++i)
        {
          if (isFloat)
            msg += strf(i == elemCnt - 1 ? "%f" : "%f, ", ((float *)val.v)[i]);
          else
            msg += strf(i == elemCnt - 1 ? "%d" : "%d, ", ((int *)val.v)[i]);
        }
        msg += strf(") -> %d", reg);
        logwarn(msg.c_str());
      }
      break;
      case SET_TEX: logwarn("    SET_TEX (%s): %p -> %d", stageName, *(BaseTexture **)(val.v), reg); break;
      case SET_SAMPLER: logwarn("    SET_SAMPLER (%s): %llu -> %d", stageName, *(uint64_t *)(val.v), reg); break;
      case SET_BUFFER: logwarn("    SET_BUFFER (%s): %p -> %d", stageName, *(Sbuffer **)(val.v), reg); break;
      case SET_CBUFFER: logwarn("    SET_CBUFFER (%s): %p -> %d", stageName, *(Sbuffer **)(val.v), reg); break;
      case SET_RWTEX: logwarn("    SET_RWTEX (%s): %p -> %d", stageName, *(BaseTexture **)(val.v), reg); break;
      case SET_RWBUF: logwarn("    SET_RWBUF (%s): %p -> %d", stageName, *(Sbuffer **)(val.v), reg); break;
      case CREATE_SAMPLER:
        logwarn("    CREATE_SAMPLER (%s): mip_map_mode=%u filter_mode=%u address_mode_u=%u address_mode_v=%u address_mode_w=%u "
                "border_color=%u anisotropic_max=%f mip_map_bias=%f",
          stageName, (uint32_t)val.sInfo.mip_map_mode, (uint32_t)val.sInfo.filter_mode, (uint32_t)val.sInfo.address_mode_u,
          (uint32_t)val.sInfo.address_mode_v, (uint32_t)val.sInfo.address_mode_w, (uint32_t)val.sInfo.border_color,
          (float)val.sInfo.anisotropic_max, (float)val.sInfo.mip_map_bias, reg);
        break;
      case SET_TLAS: logwarn("    SET_TLAS (%s): %p -> %d", stageName, *(RaytraceTopAccelerationStructure **)(val.v), reg); break;
    }
  }
};

class StcodeEffects
{
public:
  template <class TVal, ObservedEffect::Type record_type>
  void recordEffect(int stage, cpp::uint reg, TVal val)
  {
    ObservedEffect &record = effects[cnt++];
    record.type = record_type;
    record.stage = (ShaderStage)stage;
    record.reg = reg;
    record.size = sizeof(val);
    memcpy(&record.val, &val, record.size);
  }

  void recordSetConst(int stage, unsigned int id, cpp::float4 *val, int elem_cnt)
  {
    ObservedEffect &record = effects[cnt++];
    record.type = ObservedEffect::SET_CONST;
    record.stage = (ShaderStage)stage;
    record.reg = id;
    record.size = elem_cnt * sizeof(*val);
    record.isFloat = true; // @TODO: Need to discriminate between int and float consts to properly test ints
    memcpy(&record.val, val, record.size);
  }

  void sort() { stlsort::sort(effects.begin(), effects.begin() + cnt, comp); }
  void reset()
  {
    effects.fill({});
    cnt = 0;
  }

  static bool compare(StcodeEffects &first, StcodeEffects &second);

private:
  eastl::array<ObservedEffect, 10000> effects = {};
  int cnt;

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

static StcodeEffects referenceEffects, cppEffects;

bool StcodeEffects::compare(StcodeEffects &first, StcodeEffects &second)
{
  first.sort();
  second.sort();

  auto f = first.effects.cbegin();
  auto s = second.effects.cbegin();
  auto fend = f + first.cnt;
  auto send = s + second.cnt;
  for (; f != fend && s != send; ++f, ++s)
  {
    if (f->isPackedMatrixEffect(s, min<long long>(eastl::distance(s, send), 4ll)))
      s += 3;
    else if (!f->isSame(*s))
    {
      logwarn("Stcode effects differ at #%ld (first) | #%ld (second)", eastl::distance(first.effects.cbegin(), f),
        eastl::distance(second.effects.cbegin(), s));
      logwarn("First:");
      f->prettyLog();
      logwarn("Second:");
      s->prettyLog();
      return false;
    }
  }

  if (!(f == fend && s == send))
    logwarn("Stcode effects length mismatch: %ld left in first, %ld left in second", fend - f, send - s);

  return f == fend && s == send;
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

void record_set_const(RecordType type, int stage, unsigned int id, cpp::float4 *val, int cnt)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    choose_effects(type).recordSetConst(stage, id, val, cnt);
}

void validate_accumulated_records(int routine_id, const char *shname)
{
  if (execution_mode() == ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
  {
    if (!StcodeEffects::compare(cppEffects, referenceEffects))
    {
      logerr("CPP statecode shows different effects from reference, shader: %s, global_routine_id: %d", shname, routine_id);
      G_DEBUG_BREAK;
    }
  }
}

} // namespace stcode::dbg
