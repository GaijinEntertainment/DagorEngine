// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector.h>

struct ImpostorOptions
{
  enum class FolderBlkGenMode
  {
    DISABLED,
    DONT_REPLACE, // does not do anything if there is a file already
    REPLACE       // recreates the file always
  };
  eastl::string appBlk;
  eastl::string changedFileOutput;
  eastl::vector<eastl::string> assetsToBuild;
  eastl::vector<eastl::string> packsToBuild;
  bool classic_dbg = false;
  bool valid = true;
  bool clean = false;
  bool dryMode = false;
  bool skipGen = false;
  bool forceRebake = false;
  FolderBlkGenMode folderBlkGenMode = FolderBlkGenMode::REPLACE;
};
