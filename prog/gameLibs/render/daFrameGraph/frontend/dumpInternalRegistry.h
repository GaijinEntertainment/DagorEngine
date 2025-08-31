// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_framemem.h>
#include <generic/dag_fixedMoveOnlyFunction.h>

#include <frontend/internalRegistry.h>
#include <id/idIndexedFlags.h>


namespace dafg
{

using NodeValidCb = dag::FixedMoveOnlyFunction<8, bool(NodeNameId) const>;
using ResValidCb = dag::FixedMoveOnlyFunction<8, bool(ResNameId) const>;

void dump_internal_registry(const InternalRegistry &registry);
void dump_internal_registry(const InternalRegistry &registry, NodeValidCb nodeValid, ResValidCb resourceValid);

} // namespace dafg
