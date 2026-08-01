// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/fixed_string.h>
#include <shaders/dag_refinedBlock.h>

namespace dagdp
{

using TmpName = eastl::fixed_string<char, 256>;

inline refined_block::ViewBlockHandle get_dagdp_view_block(const char *view_name)
{
  TmpName name(TmpName::CtorSprintf(), "dagdp_view@%s", view_name);
  return refined_block::get_global().refineBlock(name.c_str());
}

} // namespace dagdp
