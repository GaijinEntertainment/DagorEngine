// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_outlinerModel.h"
#include <Shlwapi.h> // StrStrIA

namespace Outliner
{

static bool matches_search_text(const char *haystack, const char *needle)
{
  if (needle == nullptr || needle[0] == 0)
    return true;

  return haystack && StrStrIA(haystack, needle) != nullptr;
}

OutlinerTreeItem::~OutlinerTreeItem() { outlinerModel.onOutlinerTreeItemDeleted(*this); }

void OutlinerTreeItem::setExpandedRecursive(bool in_expanded)
{
  setExpanded(in_expanded);

  const int childCount = getChildCount();
  for (int childIndex = 0; childIndex < childCount; ++childIndex)
    getChild(childIndex)->setExpandedRecursive(in_expanded);
}

void OutlinerTreeItem::setSelected(bool in_selected)
{
  if (in_selected == selected)
    return;

  selected = in_selected;
  outlinerModel.onTreeItemSelectionChanged(*this);
}

ObjectTreeItem::~ObjectTreeItem()
{
  G_ASSERT(layerTreeItem);
  if (layerTreeItem)
    layerTreeItem->unlinkObjectFromLayer(*this);

  // To make the per type selection counter correct. Uses a virtual call cannot be in ~OutlinerTreeItem().
  setSelected(false);
}

void OutlinerTreeItem::updateSearchMatch(IOutliner &tree_interface)
{
  setMatchesSearchText(matches_search_text(getLabel(), outlinerModel.textToSearch));

  const int childCount = getChildCount();
  for (int childIndex = 0; childIndex < childCount; ++childIndex)
    getChild(childIndex)->updateSearchMatch(tree_interface);
}

void ObjectTreeItem::updateSearchMatch(IOutliner &tree_interface)
{
  bool matches = false;

  // Do not show the sample object in the Outliner.
  if (tree_interface.isSampleObject(*renderableEditableObject))
  {
    if (objectAssetNameTreeItem)
      objectAssetNameTreeItem->setMatchesSearchText(false);
  }
  else
  {
    if (objectAssetNameTreeItem)
    {
      matches = matches_search_text(objectAssetNameTreeItem->getLabel(), outlinerModel.textToSearch);
      objectAssetNameTreeItem->setMatchesSearchText(matches);
    }

    // In case of ObjectTreeItem we must include its child in the match result because its parent (LayerTreeItem)
    // relies solely on objectsMatchingFilterCount to show the object or not (and does not use the default
    // matchesSearchTextRecursive implementation).
    matches = matches || matches_search_text(getLabel(), outlinerModel.textToSearch);
  }

  if (matches == getMatchesSearchText())
    return;

  setMatchesSearchText(matches);

  G_ASSERT(layerTreeItem);
  if (layerTreeItem)
  {
    if (matches)
      ++layerTreeItem->objectsMatchingFilterCount;
    else
      --layerTreeItem->objectsMatchingFilterCount;
    G_ASSERT(layerTreeItem->objectsMatchingFilterCount >= 0);
  }
}

} // namespace Outliner