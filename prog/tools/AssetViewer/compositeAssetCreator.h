#pragma once

#include <util/dag_string.h>

class DagorAsset;

class CompositeAssetCreator
{
public:
  static String pickName();
  static DagorAsset *create(const String &asset_name, const String &asset_full_path);
};
