// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_rendEdObject.h>
#include <EditorCore/ec_outlinerInterface.h>
#include <ioSys/dag_dataBlock.h>
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
    Root = -1,

    ObjectType,
    Layer,
    Object,
    ObjectAssetName,

    Count,
  };

  OutlinerTreeItem(OutlinerModel &outliner_model, OutlinerTreeItem *in_parent) : outlinerModel(outliner_model), parent(in_parent) {}
  virtual ~OutlinerTreeItem();

  virtual ItemType getType() const = 0;
  virtual const char *getLabel() const = 0;
  virtual int getUnfilteredChildCount() const = 0;
  virtual OutlinerTreeItem *getUnfilteredChild(int index) const = 0;
  virtual int getFilteredChildCount() const = 0;
  virtual OutlinerTreeItem *getFilteredChild(int index) const = 0;
  virtual bool doesItemMatchSearch(IOutliner &tree_interface) const = 0;

  virtual void addToFilteredChildren(OutlinerTreeItem &tree_item)
  {
    G_ASSERT(!tree_item.registeredToFilteredTree);
    tree_item.registeredToFilteredTree = true;
  }

  virtual void removeFromFilteredChildren(OutlinerTreeItem &tree_item)
  {
    G_ASSERT(tree_item.registeredToFilteredTree);
    tree_item.registeredToFilteredTree = false;
  }

  OutlinerTreeItem *getParent() const { return parent; }
  void setParent(OutlinerTreeItem *in_parent) { parent = in_parent; }
  bool isExpanded() const { return expanded; }
  void setExpanded(bool in_expanded) { expanded = in_expanded; }
  void setExpandedRecursive(bool in_expanded);
  bool isSelected() const { return selected; }
  void setSelected(bool in_selected);
  bool isRegisteredToFilteredTree() const { return registeredToFilteredTree; }

protected:
  OutlinerModel &outlinerModel;

private:
  OutlinerTreeItem *parent;
  bool expanded = false;
  bool selected = false;
  bool registeredToFilteredTree = false;
};

class ObjectAssetNameTreeItem : public OutlinerTreeItem
{
public:
  using OutlinerTreeItem::OutlinerTreeItem;

  ~ObjectAssetNameTreeItem() override
  {
    // To make the per type selection counter correct. Uses a virtual call cannot be in ~OutlinerTreeItem().
    setSelected(false);
  }

  ItemType getType() const override { return ItemType::ObjectAssetName; }
  const char *getLabel() const override { return assetName; }
  int getUnfilteredChildCount() const override { return 0; }
  OutlinerTreeItem *getUnfilteredChild(int index) const override { return nullptr; }
  int getFilteredChildCount() const override { return 0; }
  OutlinerTreeItem *getFilteredChild(int index) const override { return nullptr; }
  bool doesItemMatchSearch(IOutliner &tree_interface) const override;

  String assetName;
};

class ObjectTreeItem : public OutlinerTreeItem
{
public:
  using OutlinerTreeItem::OutlinerTreeItem;

  ~ObjectTreeItem() override;

  ItemType getType() const override { return ItemType::Object; }
  const char *getLabel() const override { return renderableEditableObject->getName(); }
  int getUnfilteredChildCount() const override { return objectAssetNameTreeItem.get() ? 1 : 0; }
  OutlinerTreeItem *getUnfilteredChild(int index) const override { return objectAssetNameTreeItem.get(); }
  int getFilteredChildCount() const override { return objectAssetNameTreeItem.get() ? 1 : 0; }
  OutlinerTreeItem *getFilteredChild(int index) const override { return objectAssetNameTreeItem.get(); }
  bool doesItemMatchSearch(IOutliner &tree_interface) const override;

  void setAssetName(const char *object_asset_name)
  {
    if (object_asset_name && *object_asset_name)
    {
      if (!objectAssetNameTreeItem)
        objectAssetNameTreeItem.reset(new ObjectAssetNameTreeItem(outlinerModel, this));

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
  LayerTreeItem(OutlinerModel &outliner_model, OutlinerTreeItem *in_parent, int per_type_layer_index) :
    OutlinerTreeItem(outliner_model, in_parent), perTypeLayerIndex(per_type_layer_index)
  {}

  ~LayerTreeItem() override
  {
    while (!sortedObjects.empty())
    {
      // Deleting the ObjectTreeItem will also unlink it from the layer.
      // sortedObjects is a vector_set, so erasing from the back should be faster.
      delete sortedObjects.back();
    }

    // To make the per type selection counter correct. Uses a virtual call cannot be in ~OutlinerTreeItem().
    setSelected(false);
  }

  ItemType getType() const override { return ItemType::Layer; }
  const char *getLabel() const override { return layerName; }
  int getUnfilteredChildCount() const override { return sortedObjects.size(); }
  OutlinerTreeItem *getUnfilteredChild(int index) const override { return sortedObjects[index]; }
  int getFilteredChildCount() const override { return filteredSortedObjects.size(); }
  OutlinerTreeItem *getFilteredChild(int index) const override { return filteredSortedObjects[index]; }
  bool doesItemMatchSearch(IOutliner &tree_interface) const override;
  void addToFilteredChildren(OutlinerTreeItem &tree_item) override;
  void removeFromFilteredChildren(OutlinerTreeItem &tree_item) override;

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

    object_tree_item.layerTreeItem = this;
    object_tree_item.setParent(this);
  }

  void unlinkObjectFromLayer(ObjectTreeItem &object_tree_item)
  {
    object_tree_item.layerTreeItem = nullptr;
    object_tree_item.setParent(nullptr);

    if (object_tree_item.isSelected())
    {
      --selectedObjectCount;
      G_ASSERT(selectedObjectCount >= 0);
    }

    G_VERIFY(sortedObjects.erase(&object_tree_item) == 1);
    G_ASSERT(selectedObjectCount <= sortedObjects.size());
  }

  const int perTypeLayerIndex;
  String layerName;
  eastl::vector_set<ObjectTreeItem *, ObjectTreeItemLess> sortedObjects;
  eastl::vector_set<ObjectTreeItem *, ObjectTreeItemLess> filteredSortedObjects;
  int selectedObjectCount = 0;
};

class ObjectTypeTreeItem : public OutlinerTreeItem
{
public:
  ObjectTypeTreeItem(OutlinerModel &outliner_model, OutlinerTreeItem *in_parent, int object_type_index) :
    OutlinerTreeItem(outliner_model, in_parent), objectTypeIndex(object_type_index)
  {}

  ~ObjectTypeTreeItem() override
  {
    clear_all_ptr_items(layers);

    // To make the per type selection counter correct. Uses a virtual call cannot be in ~OutlinerTreeItem().
    setSelected(false);
  }

  ItemType getType() const override { return ItemType::ObjectType; }
  const char *getLabel() const override { return objectTypeName; }
  int getUnfilteredChildCount() const override { return layers.size(); }
  OutlinerTreeItem *getUnfilteredChild(int index) const override { return layers[index]; }
  int getFilteredChildCount() const override { return filteredLayers.size(); }
  OutlinerTreeItem *getFilteredChild(int index) const override { return filteredLayers[index]; }
  bool doesItemMatchSearch(IOutliner &tree_interface) const override;
  void addToFilteredChildren(OutlinerTreeItem &tree_item) override;
  void removeFromFilteredChildren(OutlinerTreeItem &tree_item) override;

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

  const int objectTypeIndex;
  String objectTypeName;
  dag::Vector<LayerTreeItem *> layers;
  dag::Vector<LayerTreeItem *> filteredLayers;
  bool objectTypeVisible = true;
};

class RootTreeItem : public OutlinerTreeItem
{
public:
  RootTreeItem(OutlinerModel &outliner_model, dag::Vector<ObjectTypeTreeItem *> &object_types,
    dag::Vector<ObjectTypeTreeItem *> &filtered_objectTypes) :
    OutlinerTreeItem(outliner_model, nullptr), objectTypes(object_types), filteredObjectTypes(filtered_objectTypes)
  {}

  ItemType getType() const override { return ItemType::Root; }
  const char *getLabel() const override { return nullptr; }
  int getUnfilteredChildCount() const override { return objectTypes.size(); }
  OutlinerTreeItem *getUnfilteredChild(int index) const override { return objectTypes[index]; }
  int getFilteredChildCount() const override { return filteredObjectTypes.size(); }
  OutlinerTreeItem *getFilteredChild(int index) const override { return filteredObjectTypes[index]; }
  bool doesItemMatchSearch(IOutliner &tree_interface) const override;
  void addToFilteredChildren(OutlinerTreeItem &tree_item) override;
  void removeFromFilteredChildren(OutlinerTreeItem &tree_item) override;

private:
  dag::Vector<ObjectTypeTreeItem *> &objectTypes;
  dag::Vector<ObjectTypeTreeItem *> &filteredObjectTypes;
};

class OutlinerModel
{
public:
  OutlinerModel() : rootTreeItem(*this, objectTypes, filteredObjectTypes)
  {
    for (int i = 0; i < (int)OutlinerTreeItem::ItemType::Count; ++i)
      selectionCountPerItemType[i] = 0;
  }

  ~OutlinerModel() { clear_all_ptr_items(objectTypes); }

  void expandAll(bool expand)
  {
    for (ObjectTypeTreeItem *objectTypeTreeItem : filteredObjectTypes)
      objectTypeTreeItem->setExpandedRecursive(expand);
  }

  void loadOutlinerState(IOutliner &tree_interface, const DataBlock &state)
  {
    if (const DataBlock *expansionState = state.getBlockByName("expansion"))
      loadTreeExpansionState(*expansionState, rootTreeItem);

    if (const char *savedTextToSearch = state.getStr("textToSearch"))
    {
      textToSearch = savedTextToSearch;
      onFilterChanged(tree_interface);
    }
  }

  void saveOutlinerState(DataBlock &state) const
  {
    DataBlock *expansionState = state.addNewBlock("expansion");
    saveTreeExpansionState(*expansionState, rootTreeItem);

    if (!textToSearch.empty())
      state.addStr("textToSearch", textToSearch);
  }

  void fillTypesAndLayers(IOutliner &tree_interface)
  {
    G_ASSERT(objectTypes.empty());
    G_ASSERT(filteredObjectTypes.empty());

    for (int typeIndex = 0; typeIndex < tree_interface.getTypeCount(); ++typeIndex)
    {
      ObjectTypeTreeItem *objectTypeTreeItem = new ObjectTypeTreeItem(*this, &rootTreeItem, typeIndex);
      objectTypeTreeItem->setExpanded(true);
      objectTypeTreeItem->objectTypeName = tree_interface.getTypeName(typeIndex);
      objectTypes.push_back(objectTypeTreeItem);
      rootTreeItem.addToFilteredChildren(*objectTypeTreeItem);

      for (int layerIndex = 0; layerIndex < tree_interface.getLayerCount(typeIndex); ++layerIndex)
      {
        LayerTreeItem *layerTreeItem = new LayerTreeItem(*this, objectTypeTreeItem, layerIndex);
        layerTreeItem->setExpanded(true);
        layerTreeItem->layerName = tree_interface.getLayerName(typeIndex, layerIndex);
        objectTypeTreeItem->layers.push_back(layerTreeItem);
        objectTypeTreeItem->addToFilteredChildren(*layerTreeItem);
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
        updateSearchMatch(tree_interface, *layerTreeItem);
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

    LayerTreeItem *layerTreeItem = objectTypes[type]->layers[perTypeLayerIndex];

    ObjectTreeItem *objectTreeItem = new ObjectTreeItem(*this, layerTreeItem);
    objectTreeItem->renderableEditableObject = &object;
    objectTreeItem->setSelected(tree_interface.isObjectSelected(object));
    objectTreeItem->setAssetName(showAssetNameUnderObjects ? tree_interface.getObjectAssetName(object) : nullptr);

    layerTreeItem->linkObjectToLayer(*objectTreeItem);

    updateSearchMatch(tree_interface, *objectTreeItem);

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

    if (objectTreeItem->isRegisteredToFilteredTree())
      removeFilteredTreeItem(tree_interface, *objectTreeItem);

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

    // It has been already renamed, so we have to find it by pointer.
    G_ASSERT(objectTreeItem->layerTreeItem);
    eastl::vector_set<ObjectTreeItem *, ObjectTreeItemLess> &sortedObjects = objectTreeItem->layerTreeItem->sortedObjects;
    auto itSortedObjects = eastl::find(sortedObjects.begin(), sortedObjects.end(), objectTreeItem);
    G_ASSERT(itSortedObjects != sortedObjects.end());
    sortedObjects.erase(itSortedObjects);

    sortedObjects.insert(objectTreeItem);

    // Same for the filtered objects.
    eastl::vector_set<ObjectTreeItem *, ObjectTreeItemLess> &filteredObjects = objectTreeItem->layerTreeItem->filteredSortedObjects;
    auto itFilteredObjects = eastl::find(filteredObjects.begin(), filteredObjects.end(), objectTreeItem);
    if (itFilteredObjects != filteredObjects.end())
    {
      filteredObjects.erase(itFilteredObjects);
      filteredObjects.insert(objectTreeItem);
    }

    updateSearchMatch(tree_interface, *objectTreeItem);
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

    // The match result of ObjectTreeItem depends on its child (see ObjectTreeItem::doesItemMatchSearch), so we call
    // updateSearchMatch on ObjectTreeItem instead of on ObjectTreeItem::objectAssetNameTreeItem although only the
    // latter's label could have changed.
    updateSearchMatch(tree_interface, *objectTreeItem);
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
        if (objectTreeItem->isRegisteredToFilteredTree())
          removeFilteredTreeItem(tree_interface, *objectTreeItem);

        layerTreeItem->unlinkObjectFromLayer(*objectTreeItem);
        objectTypeTreeItem->layers[objectPerTypeLayerIndex]->linkObjectToLayer(*objectTreeItem);

        updateSearchMatch(tree_interface, *objectTreeItem);
      }
      return;
    }

    G_ASSERT(false);
  }

  void onFilterChanged(IOutliner &tree_interface)
  {
    for (ObjectTypeTreeItem *objectTypeTreeItem : objectTypes)
      updateSearchMatch(tree_interface, *objectTypeTreeItem);
  }

  void addNewLayer(IOutliner &tree_interface, int type, const char *name)
  {
    const int newLayerPerTypeLayerIndex = tree_interface.addNewLayer(type, name);
    if (newLayerPerTypeLayerIndex < 0)
      return;

    ObjectTypeTreeItem *objectTypeTreeItem = objectTypes[type];
    G_ASSERT(newLayerPerTypeLayerIndex == objectTypeTreeItem->layers.size());

    LayerTreeItem *layerTreeItem = new LayerTreeItem(*this, objectTypeTreeItem, newLayerPerTypeLayerIndex);
    layerTreeItem->setExpanded(true);
    layerTreeItem->setSelected(true);
    layerTreeItem->layerName = tree_interface.getLayerName(type, newLayerPerTypeLayerIndex);
    objectTypeTreeItem->layers.push_back(layerTreeItem);
    objectTypeTreeItem->setSelected(false);

    updateSearchMatch(tree_interface, *layerTreeItem);

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
  int getExclusiveSelectedType(IOutliner &tree_interface) const
  {
    // All but one object type are disabled via visibility filters
    int typeIndexOnlyShown = -1;
    int shownTypesCount = 0;
    for (int typeIndex = 0; typeIndex < objectTypes.size(); ++typeIndex)
    {
      if (objectTypes[typeIndex]->objectTypeVisible)
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
      if (!objectTypes[typeIndex]->objectTypeVisible)
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
  dag::Vector<ObjectTypeTreeItem *> filteredObjectTypes;
  String textToSearch;

private:
  void addTreeItemToFilteredItemsTillRootIfNeeded(OutlinerTreeItem &tree_item)
  {
    OutlinerTreeItem *parentTreeItem = tree_item.getParent();
    G_ASSERT(parentTreeItem);
    parentTreeItem->addToFilteredChildren(tree_item);

    if (!parentTreeItem->isRegisteredToFilteredTree() && parentTreeItem != &rootTreeItem)
      addTreeItemToFilteredItemsTillRootIfNeeded(*parentTreeItem);
  }

  void removeFilteredTreeItem(IOutliner &tree_interface, OutlinerTreeItem &tree_item)
  {
    OutlinerTreeItem *parentTreeItem = tree_item.getParent();
    G_ASSERT(parentTreeItem);
    parentTreeItem->removeFromFilteredChildren(tree_item);

    if (parentTreeItem != &rootTreeItem && parentTreeItem->isRegisteredToFilteredTree() &&
        parentTreeItem->getFilteredChildCount() == 0 && !parentTreeItem->doesItemMatchSearch(tree_interface))
      removeFilteredTreeItem(tree_interface, *parentTreeItem);
  }

  bool updateSearchMatch(IOutliner &tree_interface, OutlinerTreeItem &tree_item)
  {
    bool foundMatch = false;

    const int childCount = tree_item.getUnfilteredChildCount();
    for (int i = 0; i < childCount; ++i)
      if (updateSearchMatch(tree_interface, *tree_item.getUnfilteredChild(i)))
        foundMatch = true;

    if (foundMatch || tree_item.doesItemMatchSearch(tree_interface))
    {
      if (!tree_item.isRegisteredToFilteredTree())
        addTreeItemToFilteredItemsTillRootIfNeeded(tree_item);

      return true;
    }
    else
    {
      if (tree_item.isRegisteredToFilteredTree())
        removeFilteredTreeItem(tree_interface, tree_item);

      return false;
    }
  }

  static void loadTreeExpansionState(const DataBlock &state, OutlinerTreeItem &tree_item)
  {
    const int childCount = tree_item.getUnfilteredChildCount();
    for (int childIndex = 0; childIndex < childCount; ++childIndex)
    {
      OutlinerTreeItem *childTreeItem = tree_item.getUnfilteredChild(childIndex);
      if (childTreeItem->getUnfilteredChildCount() > 0)
      {
        const DataBlock *childState = state.getBlockByName(childTreeItem->getLabel());
        if (childState)
        {
          childTreeItem->setExpanded(childState->getBool("expanded", childTreeItem->isExpanded()));
          loadTreeExpansionState(*childState, *childTreeItem);
        }
      }
    }
  }

  static void saveTreeExpansionState(DataBlock &state, const OutlinerTreeItem &tree_item)
  {
    const int childCount = tree_item.getUnfilteredChildCount();
    for (int childIndex = 0; childIndex < childCount; ++childIndex)
    {
      const OutlinerTreeItem *childTreeItem = tree_item.getUnfilteredChild(childIndex);
      if (childTreeItem->getUnfilteredChildCount() > 0)
      {
        DataBlock *childState = state.addBlock(childTreeItem->getLabel());
        childState->addBool("expanded", childTreeItem->isExpanded());

        saveTreeExpansionState(*childState, *childTreeItem);
      }
    }
  }

  ska::flat_hash_map<RenderableEditableObject *, ObjectTreeItem *> objectToTreeItemMap;
  RootTreeItem rootTreeItem;
  OutlinerTreeItem *focusedTreeItem = nullptr; // Use getSelectionHead() instead!
  int selectionCountPerItemType[(int)OutlinerTreeItem::ItemType::Count];
  bool showAssetNameUnderObjects = true;
};

} // namespace Outliner