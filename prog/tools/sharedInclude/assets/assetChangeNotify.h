//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DagorAsset;


// asset base change notification client interface
class IDagorAssetBaseChangeNotify
{
public:
  virtual void onAssetBaseChanged(dag::ConstSpan<DagorAsset *> changed_assets, dag::ConstSpan<DagorAsset *> added_assets,
    dag::ConstSpan<DagorAsset *> removed_assets) = 0;
};

// asset change notification client interface
class IDagorAssetChangeNotify
{
public:
  virtual void onAssetRemoved(int asset_name_id, int asset_type) = 0;
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type) = 0;
};
