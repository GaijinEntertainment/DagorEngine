// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_consts.h>

namespace shaders_internal
{

class ConstSetter
{
public:
  virtual ~ConstSetter() = default;

  virtual bool setConst(ShaderStage stage, unsigned reg_base, const void *data, unsigned num_regs) = 0;

  bool setVsConst(unsigned reg_base, const void *data, unsigned num_regs) { return setConst(STAGE_VS, reg_base, data, num_regs); }
  bool setPsConst(unsigned reg_base, const void *data, unsigned num_regs) { return setConst(STAGE_PS, reg_base, data, num_regs); }
  bool setCsConst(unsigned reg_base, const void *data, unsigned num_regs) { return setConst(STAGE_CS, reg_base, data, num_regs); }
};

} // namespace shaders_internal
