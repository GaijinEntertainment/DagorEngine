// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_rendEdObject.h>
#include <EditorCore/ec_outlinerInterface.h>
#include <util/dag_string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector_set.h>
#include <ska_hash_map/flat_hash_map2.hpp>

namespace Outliner
{

class LayerTreeItem;
class OutlinerModel;

enum class OutlinerSelectionState
{
  None,
  Partial,
  All,
};

class OutlinerTreeItem
{
public:
  enum class ItemType
  {
    ObjectType,
    Layer,
    Object,
    ObjectAssetName,

    Count,
  };

  explicit OutlinerTreeItem(OutlinerModel &outliner_model) : outlinerModel(outliner_model) {}
  virtual ~OutlinerTreeItem();

  virtual ItemType getType() const = 0;
  virtual const char *getLabel() const = 0;
  virtual int getChildCount() const = 0;
  virtual OutlinerTreeItem *getChild(int index) const = 0;

  virtual bool matchesSearchTextRecursive() const
  {
    if (getMatchesSearchText())
      return true;

    const int childCount = getChildCount();
    for (int childIndex = 0; childIndex < childCount; ++childIndex)
      if (getChild(childIndex)->matchesSearchTextRecursive())
        return true;

    return false;
  }

  virtual void updateSearchMatch(IOutliner &tree_interface);

  bool isExpanded() const { return expanded; }
  void setExpanded(bool in_expanded) { expanded = in_expanded; }
  void setExpandedRecursive(bool in_expanded);
  bool isSelected() const { return selected; }
  void setSelected(bool in_selected);
  bool getMatchesSearchText() const { return matchesSearchText; }
  void setMatchesSearchText(bool matches) { matchesSearchText = matches; }

protected:
  OutlinerModel &outlinerModel;

private:
  bool expanded = false;
  bool selected = false;
  bool matchesSearchText = true;
};

class ObjectAssetNameTreeItem : public OutlinerTreeItem
{
public:
  using OutlinerTreeItem::OutlinerTreeItem;

  ~ObjectAssetNameTreeItem()
  {
    // To make the per type selection counter correct. Uses a virtual call cannot be in ~OutlinerTreeItem().
    setSelected(false);
  }

  virtual ItemType getType() const override { return ItemType::ObjectAssetName; }
  virtual const char *getLabel() const override { return assetName; }
  virtual int getChildCount() const override { return 0; }
  virtual OutlinerTreeItem *getChild(int index) const override { return nullptr; }

  String assetName;
};

class ObjectTreeItem : public OutlinerTreeItem
{
public:
  using OutlinerTreeItem::OutlinerTreeItem;

  ~ObjectTreeItem();

  virtual ItemType getType() const override { return ItemType::Object; }
  virtual const char *getLabel() const override { return renderableEditableObject->getName(); }
  virtual int getChildCount() const override { return objectAssetNameTreeItem.get() ? 1 : 0; }
  virtual OutlinerTreeItem *getChild(int index) const override { return objectAssetNameTreeItem.get(); }

  virtual void updateSearchMatch(IOutliner &tree_interface) override;

  void setAssetName(const char *object_asset_name)
  {
    if (object_asset_name && *object_asset_name)
    {
      if (!objectAssetNameTreeItem)
        objectAssetNameTreeItem.reset(new ObjectAssetNameTreeItem(outlinerModel));

      objectAssetNameTreeItem->assetName = object_asset_name;
    }
    else if (objectAssetNameTreeItem)
    {
      objectAssetNameTreeItem.reset();
    }
  }

  LayerTreeItem *layerTreeItem = nullptr;
  eastl::unique_ptr<ObjectAssetNameTreeItem> objectAssetNameTreeItem;
  RenderableEditableObject *renderableEditableObject = nullptr;
};

struct ObjectTreeItemLess
{
  bool operator()(ObjectTreeItem *a, ObjectTreeItem *b) const
  {
    const int result = strcmp(a->getLabel(), b->getLabel());
    if (result < 0)
      return true;
    else if (result > 0)
      return false;
    else
      return a < b;
  }
};

class LayerTreeItem : public OutlinerTreeItem
{
public:
  using OutlinerTreeItem::OutlinerTreeItem;

  ~LayerTreeItem()
  {
    while (sortedObjects.size() > 0)
    {
      // Deleting the ObjectTreeItem will also unlink it from the layer.
      // sortedObjects is a vector_set, so erasing from the back should be faster.
      delete sortedObjects.getContainer().back();
    }

    // To make the per type selection counter correct. Uses a virtual call cannot be in ~OutlinerTreeItem().
    setSelected(false);
  }

  virtual ItemType getType() const override { return ItemType::Layer; }
  virtual const char *getLabel() const override { return layerName; }
  virtual int getChildCount() const override { return sortedObjects.size(); }
  virtual OutlinerTreeItem *getChild(int index) const override { return sortedObjects[index]; }

  virtual bool matchesSearchTextRecursive() const
  {
    if (getMatchesSearchText())
      return true;

    // A layer can have many children so use our cached value.
    return objectsMatchingFilterCount > 0;
  }

  OutlinerSelectionState getSelectionState() const
  {
    if (selectedObjectCount == 0)
      return OutlinerSelectionState::None;

    return sortedObjects.size() == selectedObjectCount ? OutlinerSelectionState::All : OutlinerSelectionState::Partial;
  }

  OutlinerSelectionState getFullSelectionState() const
  {
    if (isSelected())
    {
      if (selectedObjectCount == 0)
        return OutlinerSelectionState::Partial;

      return sortedObjects.size() == selectedObjectCount ? OutlinerSelectionState::All : OutlinerSelectionState::Partial;
    }
    return selectedObjectCount == 0 ? OutlinerSelectionState::None : OutlinerSelectionState::Partial;
  }

  void linkObjectToLayer(ObjectTreeItem &object_tree_item)
  {
    // Check by pointer. Sanity check because of the renames.
    RenderableEditableObject *object = object_tree_item.renderableEditableObject;
    G_ASSERT(object);
    G_ASSERT(eastl::find_if(sortedObjects.begin(), sortedObjects.end(),
               [object](ObjectTreeItem *oti) { return oti->renderableEditableObject == object; }) == sortedObjects.end());

    sortedObjects.insert(&object_tree_item);

    if (object_tree_item.isSelected())
    {
      ++selectedObjectCount;
      G_ASSERT(selectedObjectCount <= sortedObjects.size());
    }

    if (object_tree_item.getMatchesSearchText())
    {
      ++objectsMatchingFilterCount;
      G_ASSERT(objectsMatchingFilterCount <= sortedObjects.size());
    }

    object_tree_item.layerTreeItem = this;
  }

  void unlinkObjectFromLayer(ObjectTreeItem &object_tree_item)
  {
    object_tree_item.layerTreeItem = nullptr;

    if (object_tree_item.isSelected())
    {
      --selectedObjectCount;
      G_ASSERT(selectedObjectCount >= 0);
    }

    if (object_tree_item.getMatchesSearchText())
    {
      --objectsMatchingFilterCount;
      G_ASSERT(objectsMatchingFilterCount >= 0);
    }

    G_VERIFY(sortedObjects.erase(&object_tree_item) == 1);
    G_ASSERT(selectedObjectCount <= sortedObjects.size());
    G_ASSERT(objectsMatchingFilterCount <= sortedObjects.size());
  }

  String layerName;
  eastl::vector_set<ObjectTreeItem *, ObjectTreeItemLess> sortedObjects;
  int selectedObjectCount = 0;
  int objectsMatchingFilterCount = 0;
};

class ObjectTypeTreeItem : public OutlinerTreeItem
{
public:
  using OutlinerTreeItem::OutlinerTreeItem;

  ~ObjectTypeTreeItem()
  {
    clear_all_ptr_items(layers);

    // To make the per type selection counter correct. Uses a virtual call cannot be in ~OutlinerTreeItem().
    setSelected(false);
  }

  virtual ItemType getType() const override { return ItemType::ObjectType; }
  virtual const char *getLabel() const override { return objectTypeName; }
  virtual int getChildCount() const override { return layers.size(); }
  virtual OutlinerTreeItem *getChild(int index) const override { return layers[index]; }

  OutlinerSelectionState getSelectionState() const
  {
    if (layers.empty())
      return OutlinerSelectionState::None;

    OutlinerSelectionState firstSelectionState = layers[0]->getSelectionState();
    for (int i = 1; i < layers.size(); ++i)
    {
      const OutlinerSelectionState currentSelectionState = layers[i]->getSelectionState();
      if (currentSelectionState != firstSelectionState)
        return OutlinerSelectionState::Partial;
    }

    return firstSelectionState;
  }

  OutlinerSelectionState getFullSelectionState() const
  {
    auto state = isSelected() ? OutlinerSelectionState::All : OutlinerSelectionState::None;
    for (LayerTreeItem *layer : layers)
    {
      const OutlinerSelectionState layerState = layer->getFullSelectionState();
      if (layerState != state)
        return OutlinerSelectionState::Partial;
    }
    return state;
  }

  String objectTypeName;
  dag::Vector<LayerTreeItem *> layers;
};

class OutlinerModel
{
public:
  OutlinerModel()
  {
    for (int i = 0; i < (int)OutlinerTreeItem::ItemType::Count; ++i)
      selectionCountPerItemType[i] = 0;
  }

  ~OutlinerModel() { clear_all_ptr_items(objectTypes); }

  void expandAll(bool expand)
  {
    for (ObjectTypeTreeItem *objectTypeTreeItem : objectTypes)
      objectTypeTreeItem->setExpandedRecursive(expand);
  }

  void fillTypesAndLayers(IOutliner &tree_interface)
  {
    G_ASSERT(objectTypes.empty());

    for (int typeIndex = 0; typeIndex < tree_interface.getTypeCount(); ++typeIndex)
    {
      ObjectTypeTreeItem *objectTypeTreeItem = new ObjectTypeTreeItem(*this);
      objectTypeTreeItem->setExpanded(true);
      objectTypeTreeItem->objectTypeName = tree_interface.getTypeName(typeIndex);
      objectTypes.push_back(objectTypeTreeItem);

      for (int layerIndex = 0; layerIndex < tree_interface.getLayerCount(typeIndex); ++layerIndex)
      {
        LayerTreeItem *layerTreeItem = new LayerTreeItem(*this);
        layerTreeItem->setExpanded(true);
        layerTreeItem->layerName = tree_interface.getLayerName(typeIndex, layerIndex);
        objectTypeTreeItem->layers.push_back(layerTreeItem);
      }
    }
  }

  void onLayerNameChanged(IOutliner &tree_interface)
  {
    for (int typeIndex = 0; typeIndex < tree_interface.getTypeCount(); ++typeIndex)
    {
      ObjectTypeTreeItem *objectTypeTreeItem = objectTypes[typeIndex];

      for (int layerIndex = 0; layerIndex < tree_interface.getLayerCount(typeIndex); ++layerIndex)
      {
        LayerTreeItem *layerTreeItem = objectTypeTreeItem->layers[layerIndex];
        layerTreeItem->layerName = tree_interface.getLayerName(typeIndex, layerIndex);

        // Layer renaming is very rare, so it does not really matter that updateSearchMatch is really sub-optimal
        // here (because it is recursive).
        layerTreeItem->updateSearchMatch(tree_interface);
      }
    }
  }

  void onAddObject(IOutliner &tree_interface, RenderableEditableObject &object)
  {
    // Filter out not supported object types (for example spline points).
    int type;
    int perTypeLayerIndex;
    if (!tree_interface.getObjectTypeAndPerTypeLayerIndex(object, type, perTypeLayerIndex))
      return;

    ObjectTreeItem *objectTreeItem = new ObjectTreeItem(*this);
    objectTreeItem->renderableEditableObject = &object;
    objectTreeItem->setSelected(tree_interface.isObjectSelected(object));
    objectTreeItem->setAssetName(showAssetNameUnderObjects ? tree_interface.getObjectAssetName(object) : nullptr);

    LayerTreeItem *layerTreeItem = objectTypes[type]->layers[perTypeLayerIndex];
    layerTreeItem->linkObjectToLayer(*objectTreeItem);

    objectTreeItem->updateSearchMatch(tree_interface);

    G_ASSERT(objectToTreeItemMap.find(&object) == objectToTreeItemMap.end());
    objectToTreeItemMap.insert({&object, objectTreeItem});
  }

  void onRemoveObject(IOutliner &tree_interface, RenderableEditableObject &object)
  {
    // Filter out not supported object types (for example spline points).
    int type;
    int perTypeLayerIndex;
    if (!tree_interface.getObjectTypeAndPerTypeLayerIndex(object, type, perTypeLayerIndex))
      return;

    auto it = objectToTreeItemMap.find(&object);
    G_ASSERT(it != objectToTreeItemMap.end());
    ObjectTreeItem *objectTreeItem = it->second;
    objectToTreeItemMap.erase(&object);

    // This will also unlink it from the layer.
    delete objectTreeItem;
  }

  void onRenameObject(IOutliner &tree_interface, RenderableEditableObject &object)
  {
    // Filter out not supported object types (for example spline points).
    int type;
    int perTypeLayerIndex;
    if (!tree_interface.getObjectTypeAndPerTypeLayerIndex(object, type, perTypeLayerIndex))
      return;

    auto it = objectToTreeItemMap.find(&object);
    G_ASSERT(it != objectToTreeItemMap.end());
    ObjectTreeItem *objectTreeItem = it->second;

    objectTreeItem->updateSearchMatch(tree_interface);

    // It has been already renamed, so we have to find it by pointer.
    G_ASSERT(objectTreeItem->layerTreeItem);
    eastl::vector_set<ObjectTreeItem *, ObjectTreeItemLess> &sortedObjects = objectTreeItem->layerTreeItem->sortedObjects;
    auto itSortedObjects = eastl::find(sortedObjects.getContainer().begin(), sortedObjects.getContainer().end(), objectTreeItem);
    G_ASSERT(itSortedObjects != sortedObjects.getContainer().end());
    sortedObjects.getContainer().erase(itSortedObjects);

    sortedObjects.insert(objectTreeItem);
  }

  void onObjectSelectionChanged(IOutliner &tree_interface, RenderableEditableObject &object)
  {
    // Filter out not supported object types (for example spline points).
    int type;
    int perTypeLayerIndex;
    if (!tree_interface.getObjectTypeAndPerTypeLayerIndex(object, type, perTypeLayerIndex))
      return;

    auto it = objectToTreeItemMap.find(&object);
    G_ASSERT(it != objectToTreeItemMap.end());
    ObjectTreeItem *objectTreeItem = it->second;

    const bool objectIsSelected = tree_interface.isObjectSelected(object);
    if (objectIsSelected == objectTreeItem->isSelected())
      return;

    objectTreeItem->setSelected(objectIsSelected);

    LayerTreeItem *layerTreeItem = objectTypes[type]->layers[perTypeLayerIndex];
    if (objectIsSelected)
      ++layerTreeItem->selectedObjectCount;
    else
      --layerTreeItem->selectedObjectCount;

    G_ASSERT(layerTreeItem->selectedObjectCount >= 0);
    G_ASSERT(layerTreeItem->selectedObjectCount <= layerTreeItem->sortedObjects.size());
  }

  void onObjectAssetNameChanged(IOutliner &tree_interface, RenderableEditableObject &object)
  {
    auto it = objectToTreeItemMap.find(&object);
    G_ASSERT(it != objectToTreeItemMap.end());
    ObjectTreeItem *objectTreeItem = it->second;

    objectTreeItem->setAssetName(showAssetNameUnderObjects ? tree_interface.getObjectAssetName(object) : nullptr);

    // The match result of ObjectTreeItem depends on its child (see ObjectTreeItem::setMatchesSearchText), so we call
    // updateSearchMatch on ObjectTreeItem instead of on ObjectTreeItem::objectAssetNameTreeItem although only the
    // latter's label could have changed.
    objectTreeItem->updateSearchMatch(tree_interface);
  }

  void onObjectEditLayerChanged(IOutliner &tree_interface, RenderableEditableObject &object)
  {
    auto it = objectToTreeItemMap.find(&object);
    G_ASSERT(it != objectToTreeItemMap.end());
    ObjectTreeItem *objectTreeItem = it->second;

    int objectType;
    int objectPerTypeLayerIndex;
    if (!tree_interface.getObjectTypeAndPerTypeLayerIndex(object, objectType, objectPerTypeLayerIndex))
      return;

    ObjectTypeTreeItem *objectTypeTreeItem = objectTypes[objectType];
    for (int layerIndex = 0; layerIndex < objectTypeTreeItem->layers.size(); ++layerIndex)
    {
      LayerTreeItem *layerTreeItem = objectTypeTreeItem->layers[layerIndex];
      auto it = layerTreeItem->sortedObjects.find(objectTreeItem);
      if (it == layerTreeItem->sortedObjects.end())
        continue;

      if (layerIndex != objectPerTypeLayerIndex)
      {
        layerTreeItem->unlinkObjectFromLayer(*objectTreeItem);
        objectTypeTreeItem->layers[objectPerTypeLayerIndex]->linkObjectToLayer(*objectTreeItem);
      }
      return;
    }

    G_ASSERT(false);
  }

  void onFilterChanged(IOutliner &tree_interface)
  {
    for (ObjectTypeTreeItem *objectTypeTreeItem : objectTypes)
      objectTypeTreeItem->updateSearchMatch(tree_interface);
  }

  void addNewLayer(IOutliner &tree_interface, int type, const char *name)
  {
    const int newLayerPerTypeLayerIndex = tree_interface.addNewLayer(type, name);
    if (newLayerPerTypeLayerIndex < 0)
      return;

    ObjectTypeTreeItem *objectTypeTreeItem = objectTypes[type];
    G_ASSERT(newLayerPerTypeLayerIndex == objectTypeTreeItem->layers.size());

    LayerTreeItem *layerTreeItem = new LayerTreeItem(*this);
    layerTreeItem->setExpanded(true);
    layerTreeItem->setSelected(true);
    layerTreeItem->layerName = tree_interface.getLayerName(type, newLayerPerTypeLayerIndex);
    objectTypeTreeItem->layers.push_back(layerTreeItem);
    objectTypeTreeItem->setSelected(false);

    tree_interface.setLayerActive(type, newLayerPerTypeLayerIndex);
  }

  // The deleted_tree_item is no longer valid. Only use its address!
  void onOutlinerTreeItemDeleted(OutlinerTreeItem &deleted_tree_item)
  {
    if (&deleted_tree_item == focusedTreeItem)
      focusedTreeItem = nullptr;
  }

  void onTreeItemSelectionChanged(OutlinerTreeItem &tree_item)
  {
    int &count = selectionCountPerItemType[(int)tree_item.getType()];

    if (tree_item.isSelected())
      ++count;
    else
      --count;

    G_ASSERT(count >= 0);
  }

  bool isAnyTypeSelected() const { return selectionCountPerItemType[(int)OutlinerTreeItem::ItemType::ObjectType] > 0; }
  bool isAnyLayerSelected() const { return selectionCountPerItemType[(int)OutlinerTreeItem::ItemType::Layer] > 0; }

  // Returns with the number of selected item types if only that item type is selected and nothing else.
  int getExclusiveSelectionCount(OutlinerTreeItem::ItemType item_type) const
  {
    for (int i = 0; i < (int)OutlinerTreeItem::ItemType::Count; ++i)
      if (i != (int)item_type && selectionCountPerItemType[i] > 0)
        return -1;

    return selectionCountPerItemType[(int)item_type];
  }

  // Returns with the number of selected objects if only objects or asset names are selected and nothing else.
  int getExclusiveObjectsSelectionCountIgnoringAssetNames() const
  {
    G_STATIC_ASSERT((int)OutlinerTreeItem::ItemType::Count == 4);

    if (selectionCountPerItemType[(int)OutlinerTreeItem::ItemType::Object] > 0 &&
        selectionCountPerItemType[(int)OutlinerTreeItem::ItemType::ObjectType] == 0 &&
        selectionCountPerItemType[(int)OutlinerTreeItem::ItemType::Layer] == 0)
    {
      return selectionCountPerItemType[(int)OutlinerTreeItem::ItemType::Object];
    }

    return 0;
  }

  // Returns with the number of selected objects if only objects or asset names are selected and nothing else.
  // Also collects the exclusive selection type, the number of layers containing selected objects and the index of the first layer
  // containing them.
  int getExclusiveObjectsSelectionDetailsIgnoringAssetNames(int &selected_type, int &affected_layer_count,
    int &first_affected_layer_index) const
  {
    selected_type = -1;
    affected_layer_count = 0;
    first_affected_layer_index = -1;
    int count = getExclusiveObjectsSelectionCountIgnoringAssetNames();
    if (count == 0)
      return 0;

    const int typeCount = objectTypes.size();
    for (int typeIndex = 0; typeIndex < typeCount; ++typeIndex)
    {
      ObjectTypeTreeItem *objectTypeTreeItem = objectTypes[typeIndex];
      int objectsOfTypeSelected = 0;
      const int layerCount = objectTypeTreeItem->layers.size();
      for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
      {
        LayerTreeItem *layerTreeItem = objectTypeTreeItem->layers[layerIndex];
        if (layerTreeItem->selectedObjectCount > 0)
        {
          objectsOfTypeSelected += layerTreeItem->selectedObjectCount;
          affected_layer_count++;
          if (first_affected_layer_index < 0)
            first_affected_layer_index = layerIndex;
        }
      }
      if (objectsOfTypeSelected > 0 && objectsOfTypeSelected == count)
        selected_type = typeIndex;
    }
    return count;
  }

  // Return with true if only a single object is selected. Does not allow selection of object asset names either.
  bool isOnlyASingleObjectIsSelected() const { return getExclusiveSelectionCount(OutlinerTreeItem::ItemType::Object) == 1; }

  // Gets the selected object type index if only one object type is selected and nothing else.
  int getExclusiveSelectedType(IOutliner &tree_interface, const dag::Vector<bool> &showTypes) const
  {
    // All but one object type are disabled via visibility filters
    int typeIndexOnlyShown = -1;
    int shownTypesCount = 0;
    for (int typeIndex = 0; typeIndex < objectTypes.size(); ++typeIndex)
    {
      const bool shown = typeIndex >= showTypes.size() || showTypes[typeIndex];
      if (shown)
      {
        typeIndexOnlyShown = typeIndex;
        shownTypesCount++;
      }
    }
    if (shownTypesCount == 1)
      return typeIndexOnlyShown;

    // No more than one top level type is selected
    if (selectionCountPerItemType[(int)OutlinerTreeItem::ItemType::ObjectType] > 1)
      return -1;

    // No more than one type has layers/objects/assets selected in its sub-tree
    int hasSelectionTypeCount = 0;
    int hasSelectionTypeIndex = -1;
    for (int typeIndex = 0; typeIndex < objectTypes.size(); ++typeIndex)
    {
      const bool shown = typeIndex >= showTypes.size() || showTypes[typeIndex];
      if (!shown)
        continue;

      ObjectTypeTreeItem *objectTypeTreeItem = objectTypes[typeIndex];
      auto selectionState = objectTypeTreeItem->getFullSelectionState();
      if (selectionState != Outliner::OutlinerSelectionState::None)
      {
        hasSelectionTypeCount++;
        hasSelectionTypeIndex = typeIndex;
      }
    }
    if (hasSelectionTypeCount == 1)
      return hasSelectionTypeIndex;

    // Assets of different types are selected at the same time
    return -1;
  }

  // Returns true if only a single layer is selected.
  // selected_type: type of the selected layer
  // selected_per_type_layer_index: the index of the selected layer
  bool getExclusiveSelectedLayer(int &selected_type, int &selected_per_type_layer_index) const
  {
    if (getExclusiveSelectionCount(OutlinerTreeItem::ItemType::Layer) != 1)
      return false;

    selected_type = -1;
    selected_per_type_layer_index = -1;

    const int typeCount = objectTypes.size();
    for (int typeIndex = 0; typeIndex < typeCount; ++typeIndex)
    {
      ObjectTypeTreeItem *objectTypeTreeItem = objectTypes[typeIndex];
      G_ASSERT(!objectTypeTreeItem->isSelected());

      const int layerCount = objectTypeTreeItem->layers.size();
      for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
      {
        LayerTreeItem *layerTreeItem = objectTypeTreeItem->layers[layerIndex];

        if (layerTreeItem->isSelected())
        {
          selected_type = typeIndex;
          selected_per_type_layer_index = layerIndex;
          return true;
        }
      }
    }

    return false;
  }

  // Returns false if any type or layer or asset name is selected.
  // Returns true if all the selected objects are of a single type.
  bool getExclusiveTypeAndLayerInfoForSelectedObjects(int &selected_type, int &layers_affected_count) const
  {
    if (getExclusiveObjectsSelectionCountIgnoringAssetNames() <= 0)
      return false;

    selected_type = -1;
    layers_affected_count = 0;

    const int typeCount = objectTypes.size();
    for (int typeIndex = 0; typeIndex < typeCount; ++typeIndex)
    {
      ObjectTypeTreeItem *objectTypeTreeItem = objectTypes[typeIndex];
      G_ASSERT(!objectTypeTreeItem->isSelected());

      const int layerCount = objectTypeTreeItem->layers.size();
      for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
      {
        LayerTreeItem *layerTreeItem = objectTypeTreeItem->layers[layerIndex];
        G_ASSERT(!layerTreeItem->isSelected());

        if (layerTreeItem->selectedObjectCount == 0)
          continue;

        if (selected_type >= 0 && selected_type != typeIndex)
          return false;

        selected_type = typeIndex;
        layers_affected_count++;
      }
    }

    return selected_type >= 0;
  }

  // Returns false if no object or any object with a different type than the target layer is selected.
  // Returns true if all the selected objects are of a single type and at least one of them is not on the target layer.
  bool isLayerTargetableBySelection(int type, int per_type_layer_index) const
  {
    if (type < 0 || objectTypes.size() <= type)
      return false;

    // At least one object should be selected!
    const int objectSelectionCount = selectionCountPerItemType[(int)OutlinerTreeItem::ItemType::Object];
    if (objectSelectionCount == 0)
      return false;

    ObjectTypeTreeItem *objectTypeTreeItem = objectTypes[type];
    G_ASSERT(!objectTypeTreeItem->isSelected());

    const int layerCount = objectTypeTreeItem->layers.size();
    if (per_type_layer_index < 0 || layerCount <= per_type_layer_index)
      return false;

    // Target cannot contain all the selected objects!
    if (objectTypeTreeItem->layers[per_type_layer_index]->selectedObjectCount == objectSelectionCount)
      return false;

    // All the selected objects should be of the same object type!
    int perTypeObjectSelectionCount = 0;
    for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
      LayerTreeItem *layerTreeItem = objectTypeTreeItem->layers[layerIndex];
      perTypeObjectSelectionCount += layerTreeItem->selectedObjectCount;
    }
    return perTypeObjectSelectionCount == objectSelectionCount;
  }

  bool getShowAssetNameUnderObjects() const { return showAssetNameUnderObjects; }

  void setShowAssetNameUnderObjects(IOutliner &tree_interface, bool show)
  {
    if (show == showAssetNameUnderObjects)
      return;

    showAssetNameUnderObjects = show;

    for (auto it = objectToTreeItemMap.begin(); it != objectToTreeItemMap.end(); ++it)
      onObjectAssetNameChanged(tree_interface, *it->first);
  }

  dag::Vector<RenderableEditableObject *> getSelectedObjects() const
  {
    dag::Vector<RenderableEditableObject *> objects;

    for (auto it = objectToTreeItemMap.begin(); it != objectToTreeItemMap.end(); ++it)
    {
      ObjectTreeItem *treeItem = it->second;
      if (treeItem->isSelected())
        objects.push_back(it->first);
    }

    return objects;
  }

  OutlinerTreeItem *getSelectionHead() const { return (focusedTreeItem && focusedTreeItem->isSelected()) ? focusedTreeItem : nullptr; }

  void setSelectionHead(OutlinerTreeItem *tree_item) { focusedTreeItem = tree_item; }

  dag::Vector<ObjectTypeTreeItem *> objectTypes;
  String textToSearch;

private:
  ska::flat_hash_map<RenderableEditableObject *, ObjectTreeItem *> objectToTreeItemMap;
  OutlinerTreeItem *focusedTreeItem = nullptr; // Use getSelectionHead() instead!
  int selectionCountPerItemType[(int)OutlinerTreeItem::ItemType::Count];
  bool showAssetNameUnderObjects = true;
};

} // namespace Outliner