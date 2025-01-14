// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assetsGui/av_globalState.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>

dag::Vector<String> AssetSelectorGlobalState::favorites;
dag::Vector<String> AssetSelectorGlobalState::recentlyUsed;
bool AssetSelectorGlobalState::showHierarchyInFavorites = true;
int AssetSelectorGlobalState::favoritesGenerationId = 0;
int AssetSelectorGlobalState::recentlyUsedGenerationId = 0;

void AssetSelectorGlobalState::load(const DataBlock &block)
{
  favorites.clear();
  recentlyUsed.clear();

  const DataBlock *selectorBlock = block.getBlockByName("AssetSelector");
  if (!selectorBlock)
    return;

  const DataBlock *favoritesBlock = selectorBlock->getBlockByName("Favorites");
  if (favoritesBlock)
    for (int i = 0; i < favoritesBlock->paramCount(); ++i)
      if (favoritesBlock->getParamType(i) == DataBlock::TYPE_STRING && strcmp(favoritesBlock->getParamName(i), "Favorite") == 0)
        favorites.emplace_back(String(favoritesBlock->getStr(i)));

  const DataBlock *recentlyUsedBlock = selectorBlock->getBlockByName("RecentlyUsed");
  if (recentlyUsedBlock)
    for (int i = 0; i < recentlyUsedBlock->paramCount(); ++i)
      if (recentlyUsedBlock->getParamType(i) == DataBlock::TYPE_STRING &&
          strcmp(recentlyUsedBlock->getParamName(i), "RecentlyUsed") == 0)
        recentlyUsed.emplace_back(String(recentlyUsedBlock->getStr(i)));

  showHierarchyInFavorites = selectorBlock->getBool("ShowHierarchyInFavorites", showHierarchyInFavorites);
}

void AssetSelectorGlobalState::save(DataBlock &block)
{
  DataBlock *selectorBlock = block.addBlock("AssetSelector");
  G_ASSERT(selectorBlock);
  if (!selectorBlock)
    return;

  // This is needed because in daEditor DeWorkspace loads the existing DataBlock and saves that.
  selectorBlock->clearData();

  DataBlock *favoritesBlock = selectorBlock->addBlock("Favorites");
  G_ASSERT(favoritesBlock);
  if (favoritesBlock)
    for (const String &name : favorites)
      favoritesBlock->addStr("Favorite", name);

  DataBlock *recentlyUsedBlock = selectorBlock->addBlock("RecentlyUsed");
  G_ASSERT(recentlyUsedBlock);
  if (recentlyUsedBlock)
    for (const String &name : recentlyUsed)
      recentlyUsedBlock->addStr("RecentlyUsed", name);

  selectorBlock->setBool("ShowHierarchyInFavorites", showHierarchyInFavorites);
}

void AssetSelectorGlobalState::addFavorite(const String &asset)
{
  if (!asset.empty() && eastl::find(favorites.begin(), favorites.end(), asset) == favorites.end())
  {
    favorites.push_back(asset);
    ++favoritesGenerationId;
  }
}

bool AssetSelectorGlobalState::removeFavorite(const String &asset)
{
  auto it = eastl::find(favorites.begin(), favorites.end(), asset);
  if (it == favorites.end())
    return false;

  favorites.erase(it);
  ++favoritesGenerationId;
  return true;
}

bool AssetSelectorGlobalState::addRecentlyUsed(const char *asset)
{
  if (!asset || *asset == 0)
    return false;

  for (auto it = recentlyUsed.rbegin(); it != recentlyUsed.rend(); ++it)
    if (strcmp(*it, asset) == 0)
    {
      if (it == recentlyUsed.rbegin())
        return false;

      recentlyUsed.erase(it);
      break;
    }

  if (recentlyUsed.size() >= MAXIMUM_RECENT_COUNT)
    recentlyUsed.erase(recentlyUsed.begin());

  recentlyUsed.emplace_back(String(asset));

  ++recentlyUsedGenerationId;
  return true;
}
