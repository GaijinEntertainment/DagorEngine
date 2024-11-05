// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

enum CloudsChangeFlags
{
  CLOUDS_NO_CHANGE,
  CLOUDS_INVALIDATED = 1,
  CLOUDS_INCREMENTAL = 2
};

#define CLOUDS_ESRAM_ONLY 0 // TEXCF_ESRAM_ONLY
