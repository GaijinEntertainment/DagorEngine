// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/asset.h>
#include <assets/assetFolder.h>

#include <propPanel/control/dragAndDropHandler.h>


constexpr char ASSET_DRAG_AND_DROP_TYPE[] = "Asset";

struct AssetDragData
{
  DagorAsset *asset;
  DagorAssetFolder *folder;
};

namespace PropPanel
{
class TreeBaseWindow;
}

class AssetTreeDragHandler : public PropPanel::ITreeDragHandler
{
public:
  virtual void onBeginDrag(PropPanel::TLeafHandle leaf);

  PropPanel::TreeBaseWindow *tree;
};
