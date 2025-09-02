// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_outlinerModel.h"
#include <osApiWrappers/dag_localConv.h>

namespace Outliner
{

static bool matches_search_text(const char *haystack, const char *needle)
{
  if (needle == nullptr || needle[0] == 0)
    return true;

  return haystack && dd_stristr(haystack, needle) != nullptr;
}

OutlinerTreeItem::~OutlinerTreeItem() { outlinerModel.onOutlinerTreeItemDeleted(*this); }

void OutlinerTreeItem::setExpandedRecursive(bool in_expanded)
{
  setExpanded(in_expanded);

  const int childCount = getFilteredChildCount();
  for (int childIndex = 0; childIndex < childCount; ++childIndex)
    getFilteredChild(childIndex)->setExpandedRecursive(in_expanded);
}

void OutlinerTreeItem::setSelected(bool in_selected)
{
  if (in_selected == selected)
    return;

  selected = in_selected;
  outlinerModel.onTreeItemSelectionChanged(*this);
}

bool ObjectAssetNameTreeItem::doesItemMatchSearch(IOutliner &tree_interface) const
{
  // We do not want to duplicate the objectTypeVisible check here, so match checking is done in the parent.
  // (Also ObjectAssetNameTreeItem is always shown if its parent is shown, so it makes more sense to match it there.)
  return false;
}

ObjectTreeItem::~ObjectTreeItem()
{
  G_ASSERT(layerTreeItem);
  if (layerTreeItem)
    layerTreeItem->unlinkObjectFromLayer(*this);

  // To make the per type selection counter correct. Uses a virtual call cannot be in ~OutlinerTreeItem().
  setSelected(false);
}

bool ObjectTreeItem::doesItemMatchSearch(IOutliner &tree_interface) const
{
  OutlinerTreeItem *parentTreeItem = getParent();
  G_ASSERT(parentTreeItem && parentTreeItem->getType() == ItemType::Layer);
  parentTreeItem = parentTreeItem->getParent();
  G_ASSERT(parentTreeItem && parentTreeItem->getType() == ItemType::ObjectType);
  ObjectTypeTreeItem *objectTypeTreeItem = static_cast<ObjectTypeTreeItem *>(parentTreeItem);
  if (!objectTypeTreeItem->objectTypeVisible)
    return false;

  // Do not show the sample object in the Outliner.
  if (tree_interface.isSampleObject(*renderableEditableObject))
    return false;

  if (objectAssetNameTreeItem && matches_search_text(objectAssetNameTreeItem->getLabel(), outlinerModel.textToSearch))
    return true;

  return matches_search_text(getLabel(), outlinerModel.textToSearch);
}

bool LayerTreeItem::doesItemMatchSearch(IOutliner &tree_interface) const
{
  OutlinerTreeItem *parentTreeItem = getParent();
  G_ASSERT(parentTreeItem && parentTreeItem->getType() == ItemType::ObjectType);
  ObjectTypeTreeItem *objectTypeTreeItem = static_cast<ObjectTypeTreeItem *>(parentTreeItem);
  if (!objectTypeTreeItem->objectTypeVisible)
    return false;

  return matches_search_text(getLabel(), outlinerModel.textToSearch);
}

void LayerTreeItem::addToFilteredChildren(OutlinerTreeItem &tree_item)
{
  OutlinerTreeItem::addToFilteredChildren(tree_item);

  G_ASSERT(tree_item.getType() == ItemType::Object);
  ObjectTreeItem &objectTreeItem = static_cast<ObjectTreeItem &>(tree_item);
  RenderableEditableObject *object = objectTreeItem.renderableEditableObject;

  // Check by pointer. Sanity check because of the renames.
  G_ASSERT(eastl::find_if(filteredSortedObjects.begin(), filteredSortedObjects.end(),
             [object](ObjectTreeItem *oti) { return oti->renderableEditableObject == object; }) == filteredSortedObjects.end());

  filteredSortedObjects.insert(&objectTreeItem);
}

void LayerTreeItem::removeFromFilteredChildren(OutlinerTreeItem &tree_item)
{
  OutlinerTreeItem::removeFromFilteredChildren(tree_item);

  G_ASSERT(tree_item.getType() == ItemType::Object);
  ObjectTreeItem &objectTreeItem = static_cast<ObjectTreeItem &>(tree_item);
  G_VERIFY(filteredSortedObjects.erase(&objectTreeItem) == 1);
}

bool ObjectTypeTreeItem::doesItemMatchSearch(IOutliner &tree_interface) const
{
  if (!objectTypeVisible)
    return false;

  return matches_search_text(getLabel(), outlinerModel.textToSearch);
}

void ObjectTypeTreeItem::addToFilteredChildren(OutlinerTreeItem &tree_item)
{
  OutlinerTreeItem::addToFilteredChildren(tree_item);

  G_ASSERT(tree_item.getType() == ItemType::Layer);
  G_ASSERT(eastl::find(filteredLayers.begin(), filteredLayers.end(), &tree_item) == filteredLayers.end());

  auto it = eastl::lower_bound(filteredLayers.begin(), filteredLayers.end(), &tree_item, [](OutlinerTreeItem *a, OutlinerTreeItem *b) {
    LayerTreeItem *layerTreeItemA = static_cast<LayerTreeItem *>(a);
    LayerTreeItem *layerTreeItemB = static_cast<LayerTreeItem *>(b);
    return layerTreeItemA->perTypeLayerIndex < layerTreeItemB->perTypeLayerIndex;
  });

  filteredLayers.insert(it, static_cast<LayerTreeItem *>(&tree_item));
}

void ObjectTypeTreeItem::removeFromFilteredChildren(OutlinerTreeItem &tree_item)
{
  OutlinerTreeItem::removeFromFilteredChildren(tree_item);

  G_ASSERT(tree_item.getType() == ItemType::Layer);
  auto it = eastl::find(filteredLayers.begin(), filteredLayers.end(), &tree_item);
  G_ASSERT(it != filteredLayers.end());
  filteredLayers.erase(it);
}

bool RootTreeItem::doesItemMatchSearch(IOutliner &tree_interface) const
{
  G_ASSERT(false);
  return false;
}

void RootTreeItem::addToFilteredChildren(OutlinerTreeItem &tree_item)
{
  OutlinerTreeItem::addToFilteredChildren(tree_item);

  G_ASSERT(tree_item.getType() == ItemType::ObjectType);
  G_ASSERT(eastl::find(filteredObjectTypes.begin(), filteredObjectTypes.end(), &tree_item) == filteredObjectTypes.end());

  auto it = eastl::lower_bound(filteredObjectTypes.begin(), filteredObjectTypes.end(), &tree_item,
    [](OutlinerTreeItem *a, OutlinerTreeItem *b) {
      ObjectTypeTreeItem *objectTypeTreeItemA = static_cast<ObjectTypeTreeItem *>(a);
      ObjectTypeTreeItem *objectTypeTreeItemB = static_cast<ObjectTypeTreeItem *>(b);
      return objectTypeTreeItemA->objectTypeIndex < objectTypeTreeItemB->objectTypeIndex;
    });

  filteredObjectTypes.insert(it, static_cast<ObjectTypeTreeItem *>(&tree_item));
}

void RootTreeItem::removeFromFilteredChildren(OutlinerTreeItem &tree_item)
{
  OutlinerTreeItem::removeFromFilteredChildren(tree_item);

  G_ASSERT(tree_item.getType() == ItemType::ObjectType);
  auto it = eastl::find(filteredObjectTypes.begin(), filteredObjectTypes.end(), &tree_item);
  G_ASSERT(it != filteredObjectTypes.end());
  filteredObjectTypes.erase(it);
}

} // namespace Outliner