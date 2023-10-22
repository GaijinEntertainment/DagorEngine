//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/allocator.h>


namespace rendinst::render
{

struct OpaqueGlobalDynVarsPolicy;
template <typename, class>
class RiExtraRendererT;

// Opaque class, should only be passed as a pointer/reference outside of the library.
using RiExtraRenderer = RiExtraRendererT<EASTLAllocatorType, OpaqueGlobalDynVarsPolicy>;

} // namespace rendinst::render
