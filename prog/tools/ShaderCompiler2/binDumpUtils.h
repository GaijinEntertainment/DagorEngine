// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <generic/dag_tab.h>

#include "shTargetContext.h"
#include "cppStcodeAssembly.h"

// forwards
class ShaderStateBlock;
class ShaderClass;
namespace mkbindump
{
class GatherNameMap;
}


// binary dump utility functions
namespace bindumphlp
{
void sortShaders(dag::ConstSpan<ShaderStateBlock *> blocks, shc::TargetContext &ctx);
void patchStCode(dag::Span<int> code, dag::ConstSpan<int> remapTable, dag::ConstSpan<int> smpTable);

// builds global variables remapping table
void countRefAndRemapGlobalVars(Tab<int> &dest_ref, dag::ConstSpan<ShaderClass *> shaderClasses, mkbindump::GatherNameMap &varMap,
  shc::TargetContext &ctx);
} // namespace bindumphlp
