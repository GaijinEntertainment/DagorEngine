// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/detail/resNameId.h>

#include <id/idIndexedFlags.h>

namespace dafg
{

struct FrontendRecompilationData
{
  IdIndexedFlags<NodeNameId> nodeUnchanged;
  IdIndexedFlags<NodeNameId> resourceRequestsForNodeUnchanged; // ignores history requests
  IdIndexedFlags<ResNameId> resourceResolvedNameUnchanged;
};

} // namespace dafg
