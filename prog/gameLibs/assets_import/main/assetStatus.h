// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

enum class AssetLoadingStatus
{
  NotLoaded,
  Loading,
  Loaded,
  LoadedWithErrors
};

AssetLoadingStatus get_texture_status(const char *name);
AssetLoadingStatus get_asset_status(DagorAsset &asset, bool update_status);
