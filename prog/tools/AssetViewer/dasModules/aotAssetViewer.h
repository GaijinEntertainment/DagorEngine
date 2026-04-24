// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_appwnd.h"
#include <de3_interface.h>

namespace bind_dascript
{

inline void select_asset(const char *assetName)
{
  DagorAsset *asset = DAEDITOR3.getAssetByName(assetName);
  if (!asset)
    return;
  get_app().selectAsset(*asset);
}

} // namespace bind_dascript