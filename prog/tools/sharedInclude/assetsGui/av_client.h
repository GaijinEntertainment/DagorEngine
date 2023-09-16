#pragma once


#include <util/dag_stdint.h>

//==============================================================================
// IAssetBaseViewClient
//==============================================================================
class IAssetBaseViewClient
{
public:
  virtual void onAvClose() = 0;
  virtual void onAvAssetDblClick(const char *asset_name) = 0;
  virtual void onAvSelectAsset(const char *asset_name) = 0;
  virtual void onAvSelectFolder(const char *asset_folder_name) = 0;
};
