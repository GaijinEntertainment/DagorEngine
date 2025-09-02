// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/detail/resNameId.h>
#include <render/daFrameGraph/detail/nodeNameId.h>

#include <id/idIndexedFlags.h>


namespace dafg
{

struct ValidityInfo
{
  IdIndexedFlags<ResNameId> resourceValid;
  IdIndexedFlags<NodeNameId> nodeValid;
};

} // namespace dafg
