// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_common.h>

class AllAssetsTree;
class DagorAssetMgr;

class AvTree
{
public:
  static void loadIcons(DagorAssetMgr &asset_mgr);
  static bool markExportedTree(AllAssetsTree &tree, PropPanel::TLeafHandle parent, int flags);
  static bool isFolderExportable(const AllAssetsTree &tree, PropPanel::TLeafHandle parent, bool *exported = nullptr);
};
