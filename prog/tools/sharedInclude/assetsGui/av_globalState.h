#pragma once

#include <dag/dag_vector.h>

class DataBlock;

class AssetSelectorGlobalState
{
public:
  static void load(const DataBlock &block);
  static void save(DataBlock &block);

  static const dag::Vector<String> &getFavorites() { return favorites; }
  static void addFavorite(const String &asset);

  // Returns true if the asset has been removed.
  static bool removeFavorite(const String &asset);

  static const dag::Vector<String> &getRecentlyUsed() { return recentlyUsed; }

  // Returns with true if the recently used list has been modified.
  static bool addRecentlyUsed(const char *asset);

  static bool getShowHierarchyInFavorites() { return showHierarchyInFavorites; }
  static void setShowHierarchyInFavorites(bool show) { showHierarchyInFavorites = show; }

  static int getFavoritesGenerationId() { return favoritesGenerationId; }
  static int getRecentlyUsedGenerationId() { return recentlyUsedGenerationId; }

private:
  static dag::Vector<String> favorites;
  static dag::Vector<String> recentlyUsed;
  static int favoritesGenerationId;
  static int recentlyUsedGenerationId;
  static bool showHierarchyInFavorites;

  static const int MAXIMUM_RECENT_COUNT = 50;
};
