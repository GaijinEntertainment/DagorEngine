#pragma once


#include <generic/dag_span.h>
#include <util/dag_stdint.h>

namespace PropPanel
{
class ITreeInterface;
class TreeBaseWindow;
} // namespace PropPanel

class DagorAsset;
class DagorAssetFolder;

//==============================================================================
// IAssetBaseViewClient
//==============================================================================
class IAssetBaseViewClient
{
public:
  virtual void onAvClose() = 0;
  virtual void onAvAssetDblClick(DagorAsset *asset, const char *asset_name) = 0;
  virtual void onAvSelectAsset(DagorAsset *asset, const char *asset_name) = 0;
  virtual void onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name) = 0;
};

class IAssetSelectorContextMenuHandler
{
public:
  virtual bool onAssetSelectorContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) = 0;
};

class IAssetSelectorFavoritesRecentlyUsedHost
{
public:
  virtual dag::ConstSpan<int> getAllowedTypes() const = 0;
  virtual void addAssetToFavorites(const DagorAsset &asset) = 0;
  virtual void goToAsset(const DagorAsset &asset) = 0;
  virtual void onSelectionChanged(const DagorAsset *asset) = 0;
  virtual void onSelectionDoubleClicked(const DagorAsset *asset) = 0;
};
