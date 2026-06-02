// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>

struct ResourcePaths
{
  String defaultShadersPath;
  String defaultTexgenPath;
  String shaderIncludesDir; // base dir for `include _foo.blk` directives in the shader-list BLK
  String mainGraphsDir;     // root of user top-level graphs (*.json / *.blk); drives the file open/save dialogs
  String subgraphsDir; // root of *.subgraph.blk reusable subgraph components; scanned by appendSubgraphTemplatesToBaseNodes and the
                       // compile-time expander
};
