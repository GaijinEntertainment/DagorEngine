// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/detail/resNameId.h>
#include <render/daBfg/detail/nodeNameId.h>

#include <id/idIndexedFlags.h>


namespace dabfg
{

struct ValidityInfo
{
  IdIndexedFlags<ResNameId> resourceValid;
  IdIndexedFlags<NodeNameId> nodeValid;
};

} // namespace dabfg
