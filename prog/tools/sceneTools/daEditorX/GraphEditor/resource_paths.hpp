// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>

struct ResourcePaths
{
  String defaultShadersPath;
  String defaultTexgenPath;
  String shaderIncludesDir; // base dir for `include _foo.blk` directives in the shader-list BLK
};
