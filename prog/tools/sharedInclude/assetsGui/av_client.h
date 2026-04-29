#pragma once


#include <generic/dag_span.h>
#include <util/dag_stdint.h>

namespace PropPanel
{
class IMenu;
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

class IAssetBrowserHost
{
public:
  virtual void assetBrowserFill() = 0;
  virtual bool assetBrowserIsOpen() const = 0;
  virtual void assetBrowserSetOpen(bool open) = 0;
  virtual void assetBrowserOnAssetSelected(DagorAsset *asset, bool double_clicked) = 0;
  virtual void assetBrowserOnContextMenu(DagorAsset *asset) = 0;
};

class IAssetSelectorContextMenuHandler
{
public:
  virtual bool onAssetSelectorContextMenu(PropPanel::IMenu &menu, DagorAsset *asset, DagorAssetFolder *asset_folder) = 0;
};

class IAssetSelectorFavoritesRecentlyUsedHost
{
public:
  virtual dag::ConstSpan<int> getAllowedTypes() const = 0;
  virtual void addAssetToFavorites(const DagorAsset &asset) = 0;
  virtual void goToAsset(const DagorAsset &asset) = 0;
  virtual void onSelectionChanged(const DagorAsset *asset) = 0;
  virtual void onSelectionDoubleClicked(const DagorAsset *asset) = 0;
  virtual IAssetBrowserHost &getAssetBrowserHost() = 0;
  virtual IAssetSelectorContextMenuHandler &getAssetSelectorContextMenuHandler() = 0;
};
