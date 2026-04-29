//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>


namespace sepclientinstance
{

class SepClientInstance;


struct SepClientInstanceConfig
{
public:
  SepClientInstanceConfig(); // calculates `sepUsagePermilleLimit`

  bool isSepEnabled() const { return sepUsageCalculatedPermilleValue < sepUsagePermilleLimit && !sepUrls.empty(); }

public:
  // Probability of using SEP for current game session.
  // Values: [0..1000]
  // 0 - disabled, 1000 - always enabled (100%), 250 - 25% chance to use SEP.
  int sepUsagePermilleLimit = 0;
  int sepUsageCalculatedPermilleValue = -1; // Normal value: [0..999];

  eastl::vector<eastl::string> sepUrls;

  bool supportCompression = true;
  bool verboseLogging = false;

  eastl::string defaultProjectId; // for logging and for UserAgent
};


using AuthTokenProvider = eastl::function<eastl::string()>;


} // namespace sepclientinstance
