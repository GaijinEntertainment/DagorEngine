// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/detail/autoResTypeNameId.h>


namespace dafg
{

struct AutoResolutionData
{
  AutoResTypeNameId id;
  bool roundUp; // no initializer, daScript builds the data on the stack and needs the trivial constructor
  float multiplier;

  // Bitwise float comparison is intentional.
  bool operator==(const AutoResolutionData &) const = default;
};

} // namespace dafg
