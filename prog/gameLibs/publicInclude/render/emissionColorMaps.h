//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resPtr.h>

class EmissionColorMaps
{
  UniqueBufHolder decodeMap;
  // TODO: SbufferIDHolder encodeMap;

  void upload();

public:
  EmissionColorMaps();
  void render();
  void onReset();
};
