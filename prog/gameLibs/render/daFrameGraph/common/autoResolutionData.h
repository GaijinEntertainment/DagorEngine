// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/detail/autoResTypeNameId.h>


namespace dafg
{

struct AutoResolutionData
{
  AutoResTypeNameId id;
  float multiplier;

  // Bitwise float comparison is intentional.
  bool operator==(const AutoResolutionData &) const = default;
};

} // namespace dafg
