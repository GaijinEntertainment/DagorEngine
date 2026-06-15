// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <debug/dag_assert.h>
#include <shaders/dag_refinedBlock.h>
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

} // namespace refined_block
