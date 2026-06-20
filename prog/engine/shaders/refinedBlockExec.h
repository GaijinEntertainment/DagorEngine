// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <debug/dag_assert.h>
#include <shaders/dag_refinedBlock.h>
#include <shaders/stcode/scalarTypes.h>
#include <drv/3d/dag_resId.h>
#include <math/dag_color.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_TMatrix4.h>
#include <EASTL/variant.h>
#include <EASTL/vector.h>

#include "constSetter.h"

namespace refined_block
{

class RawBufferConstSetter : public shaders_internal::ConstSetter
{
public:
  RawBufferConstSetter(dag::Span<float> b) : buf(b) {}

  bool setConst(ShaderStage, unsigned reg_base, const void *data, unsigned num_regs) override
  {
    G_ASSERT_RETURN(int(reg_base + num_regs) <= buf.size(), false);
    memcpy(&buf[reg_base], data, num_regs * sizeof(float));
    return true;
  }

private:
  dag::Span<float> buf;
};

void exec_refined_block_stcode(dag::ConstSpan<int> cod, const PassBlockHandle &block, dag::Span<float> out_buf,
  eastl::vector<FlushedVar> &out_vars);

uint32_t get_or_allocate_bindless_tex(BaseTexture *tex);

template <typename T>
T default_value()
{
  if constexpr (eastl::is_same_v<eastl::decay<T>, d3d::SamplerHandle>)
    return d3d::request_sampler({});
  else
    return {};
}

template <typename T>
T stcode_get_from_block(const PassBlockHandle &block, int id)
{
  return block.getByGid<T>(id)
    .or_else([&](auto err) -> dag::Expected<T, GetError> {
      const char *blockName = block.getName();
      const auto &dump = shBinDump();
      const char *varName =
        (unsigned(id) < (unsigned)dump.globVars.size()) ? (const char *)dump.varMap[dump.globVars.getNameId(id)] : "<unknown>";
      switch (err)
      {
        case GetError::VarNotFound:
          G_ASSERTF(0, "RefinedBlock '%s' or it's parent does not contain variable '%s'", blockName, varName); //-V1037
          break;
        case GetError::WrongVarType:
          G_ASSERTF(0, "RefinedBlock '%s' variable '%s' has wrong type", blockName, varName); //-V1037
          break;
      }
      return default_value<T>();
    })
    .value();
}

class ScopedRefinedBlock
{
public:
  explicit ScopedRefinedBlock(const PassBlockHandle &block);
  ~ScopedRefinedBlock();

  template <typename T>
  T getVar(int32_t gid) const
  {
    return stcode_get_from_block<T>(block, gid);
  }

  uint32_t getOrAllocateBindlessTex(BaseTexture *tex) const { return get_or_allocate_bindless_tex(tex); }

private:
  const PassBlockHandle &block;
};

class ScopedFlushedVarsCollector
{
public:
  explicit ScopedFlushedVarsCollector(eastl::vector<FlushedVar> &vars);
  ~ScopedFlushedVarsCollector();
};

stcode::cpp::float4 rb_get_f4(int32_t gid);
stcode::cpp::int4 rb_get_ivec(int32_t gid);
float rb_get_real(int32_t gid);
int32_t rb_get_int(int32_t gid);
void rb_get_mat44(int32_t gid, stcode::cpp::float4x4 *out);
void *rb_get_tex(int32_t gid);
void *rb_get_buf(int32_t gid);
uint64_t rb_get_sampler(int32_t gid);
void *rb_get_tlas(int32_t gid);

uint32_t rb_alloc_bindless(void *tex);
uint32_t rb_alloc_bindless_sampler(uint64_t handle);

void rb_flush_tex(int32_t stage, int32_t slot, void *tex);
void rb_flush_buf(int32_t stage, int32_t slot, void *buf);
void rb_flush_bindless_tex(uint32_t bi, void *tex);
void rb_flush_cbuf(int32_t stage, int32_t slot, void *buf);
void rb_flush_sampler(int32_t stage, int32_t slot, uint64_t handle);
void rb_flush_bindless_sampler(uint32_t bi, uint64_t handle);
void rb_flush_tlas(int32_t stage, int32_t slot, void *tlas);
void rb_flush_rwtex(int32_t stage, int32_t slot, void *tex);
void rb_flush_rwbuf(int32_t stage, int32_t slot, void *buf);

} // namespace refined_block
