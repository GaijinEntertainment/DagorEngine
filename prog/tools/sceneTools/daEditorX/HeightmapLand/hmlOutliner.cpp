// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlOutliner.h"
#include "hmlEntity.h"
#include "hmlLight.h"
#include "hmlObjectsEditor.h"
#include "hmlPlugin.h"
#include "hmlSnow.h"
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include <assets/asset.h>
#include <de3_interface.h>
#include <EditorCore/ec_rendEdObject.h>

HeightmapLandOutlinerInterface::HeightmapLandOutlinerInterface(HmapLandObjectEditor &object_editor) : objectEditor(object_editor) {}

int HeightmapLandOutlinerInterface::getTypeCount() { return EditLayerProps::TYPENUM; }

const char *HeightmapLandOutlinerInterface::getTypeName(int type, bool plural)
{
  // Assert needed because of OutlinerWindow::Icons::getForType.
  G_STATIC_ASSERT(EditLayerProps::ENT == 0);
  G_STATIC_ASSERT(EditLayerProps::SPL == 1);
  G_STATIC_ASSERT(EditLayerProps::PLG == 2);

  if (type == EditLayerProps::ENT)
    return plural ? "Entities" : "Entity";
  else if (type == EditLayerProps::SPL)
    return plural ? "Splines" : "Spline";
  else if (type == EditLayerProps::PLG)
    return plural ? "Polygons" : "Polygon";

  G_ASSERT(false);
  return nullptr;
}

bool HeightmapLandOutlinerInterface::isTypeVisible(int type) { return objectTypeStates[type].visible; }

bool HeightmapLandOutlinerInterface::isTypeLocked(int type) { return objectTypeStates[type].locked; }

bool HeightmapLandOutlinerInterface::canAddNewLayerWithName(int type, const char *name, String &error_message)
{
  if (EditLayerProps::layerProps.size() >= EditLayerProps::MAX_LAYERS)
  {
    error_message = "Reached the maximum number of layers, cannot add more!";
    return false;
  }

  if (!isValidLayerName(name, error_message))
    return false;

  if (EditLayerProps::findLayer(type, EditLayerProps::layerNames.getNameId(name)) >= 0)
  {
    error_message = "A layer already exists with this name in this type.";
    return false;
  }

  return true;
}

void HeightmapLandOutlinerInterface::selectAllTypeObjects(int type, bool select)
{
  objectEditor.getUndoSystem()->begin();

  for (int i = 0; i < EditLayerProps::layerProps.size(); ++i)
    if (EditLayerProps::layerProps[i].type == type && !EditLayerProps::layerProps[i].lock)
      HmapLandPlugin::self->selectLayerObjects(i, select);

  objectEditor.getUndoSystem()->accept("Type selection in Outliner");
}

void HeightmapLandOutlinerInterface::toggleTypeVisibility(int type)
{
  const bool newVisible = !objectTypeStates[type].visible;
  objectTypeStates[type].visible = newVisible;

  for (int i = 0; i < EditLayerProps::layerProps.size(); ++i)
  {
    EditLayerProps &layerProp = EditLayerProps::layerProps[i];

    if (layerProp.type == type)
    {
      layerProp.hide = !newVisible;
      if (layerProp.hide)
        DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() | (1ull << i));
      else
        DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() & ~(1ull << i));
    }
  }
}

void HeightmapLandOutlinerInterface::toggleTypeLock(int type)
{
  const bool newLock = !objectTypeStates[type].locked;
  objectTypeStates[type].locked = newLock;

  for (int i = 0; i < EditLayerProps::layerProps.size(); ++i)
  {
    EditLayerProps &layerProp = EditLayerProps::layerProps[i];

    if (layerProp.type == type)
    {
      layerProp.lock = newLock;
      if (layerProp.lock)
        HmapLandPlugin::self->selectLayerObjects(i, false);
    }
  }
}

int HeightmapLandOutlinerInterface::addNewLayer(int type, const char *name)
{
  String errorMessage;
  if (!canAddNewLayerWithName(type, name, errorMessage))
    return -1;

  const int oldLayerCount = EditLayerProps::layerProps.size();
  const int layerPropsIndex = EditLayerProps::findLayerOrCreate(type, EditLayerProps::layerNames.addNameId(name));
  if (EditLayerProps::layerProps.size() == oldLayerCount)
    return -1;

  EditLayerProps::layerProps[layerPropsIndex].renameable = true;

  const int perTypeLayerIndex = getPerTypeLayerIndex(type, layerPropsIndex);
  return perTypeLayerIndex;
}

int HeightmapLandOutlinerInterface::getLayerCount(int type)
{
  int count = 0;
  for (int i = 0; i < EditLayerProps::layerProps.size(); ++i)
    if (EditLayerProps::layerProps[i].type == type)
      ++count;

  return count;
}

const char *HeightmapLandOutlinerInterface::getLayerName(int type, int per_type_layer_index)
{
  EditLayerProps *props = getEditLayerProps(type, per_type_layer_index);
  if (props)
    return props->name();

  G_ASSERT(false);
  return nullptr;
}

bool HeightmapLandOutlinerInterface::isLayerActive(int type, int per_type_layer_index)
{
  const int activeLayerPropsIndex = EditLayerProps::activeLayerIdx[type];
  return activeLayerPropsIndex >= 0 && getLayerPropsIndex(type, per_type_layer_index) == activeLayerPropsIndex;
}

bool HeightmapLandOutlinerInterface::isLayerVisible(int type, int per_type_layer_index)
{
  EditLayerProps *props = getEditLayerProps(type, per_type_layer_index);
  if (props)
    return !props->hide;

  G_ASSERT(false);
  return true;
}

bool HeightmapLandOutlinerInterface::isLayerLocked(int type, int per_type_layer_index)
{
  EditLayerProps *props = getEditLayerProps(type, per_type_layer_index);
  if (props)
    return props->lock;

  G_ASSERT(false);
  return false;
}

bool HeightmapLandOutlinerInterface::isLayerAppliedToMask(int type, int per_type_layer_index)
{
  EditLayerProps *props = getEditLayerProps(type, per_type_layer_index);
  if (props)
    return props->renderToMask;

  G_ASSERT(false);
  return false;
}

bool HeightmapLandOutlinerInterface::isLayerExported(int type, int per_type_layer_index)
{
  EditLayerProps *props = getEditLayerProps(type, per_type_layer_index);
  if (props)
    return props->exp;

  G_ASSERT(false);
  return false;
}

bool HeightmapLandOutlinerInterface::canChangeLayerVisibility(int type, int per_type_layer_index) { return isTypeVisible(type); }

bool HeightmapLandOutlinerInterface::canChangeLayerLock(int type, int per_type_layer_index) { return !isTypeLocked(type); }

bool HeightmapLandOutlinerInterface::isLayerRenameable(int type, int per_type_layer_index)
{
  EditLayerProps *props = getEditLayerProps(type, per_type_layer_index);
  if (props)
    return props->renameable;

  G_ASSERT(false);
  return false;
}

bool HeightmapLandOutlinerInterface::canRenameLayerTo(int type, int per_type_layer_index, const char *name, String &error_message)
{
  if (!isValidLayerName(name, error_message))
    return false;

  // Do not show error if the new name is the same is the old one. It is annoying to see an error right after starting
  // renaming because the name is the same. It will be a no operation in renameLayer anyway.
  if (strcmp(getLayerName(type, per_type_layer_index), name) == 0)
    return true;

  if (EditLayerProps::findLayer(type, EditLayerProps::layerNames.getNameId(name)) >= 0)
  {
    error_message = "A layer already exists with this name in this type.";
    return false;
  }

  return true;
}

void HeightmapLandOutlinerInterface::setLayerActive(int type, int per_type_layer_index)
{
  const int layerPropsIndex = getLayerPropsIndex(type, per_type_layer_index);
  EditLayerProps::activeLayerIdx[type] = layerPropsIndex;
}

void HeightmapLandOutlinerInterface::selectAllLayerObjects(int type, int per_type_layer_index, bool select)
{
  const int layerPropsIndex = getLayerPropsIndex(type, per_type_layer_index);
  if (EditLayerProps::layerProps[layerPropsIndex].lock)
    return;

  HmapLandPlugin::self->selectLayerObjects(layerPropsIndex, select);
}

void HeightmapLandOutlinerInterface::toggleLayerVisibility(int type, int per_type_layer_index)
{
  if (!canChangeLayerVisibility(type, per_type_layer_index))
    return;

  const int layerPropsIndex = getLayerPropsIndex(type, per_type_layer_index);
  EditLayerProps &layerProp = EditLayerProps::layerProps[layerPropsIndex];

  layerProp.hide = !layerProp.hide;
  if (layerProp.hide)
    DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() | (1ull << layerPropsIndex));
  else
    DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() & ~(1ull << layerPropsIndex));
}

void HeightmapLandOutlinerInterface::toggleLayerLock(int type, int per_type_layer_index)
{
  if (!canChangeLayerLock(type, per_type_layer_index))
    return;

  const int layerPropsIndex = getLayerPropsIndex(type, per_type_layer_index);
  EditLayerProps &layerProp = EditLayerProps::layerProps[layerPropsIndex];

  layerProp.lock = !layerProp.lock;
  if (layerProp.lock)
    HmapLandPlugin::self->selectLayerObjects(layerPropsIndex, false);
}

void HeightmapLandOutlinerInterface::toggleLayerApplyToMask(int type, int per_type_layer_index)
{
  const int layerPropsIndex = getLayerPropsIndex(type, per_type_layer_index);
  EditLayerProps &layerProp = EditLayerProps::layerProps[layerPropsIndex];

  layerProp.renderToMask = !layerProp.renderToMask;
}

void HeightmapLandOutlinerInterface::toggleLayerExport(int type, int per_type_layer_index)
{
  const int layerPropsIndex = getLayerPropsIndex(type, per_type_layer_index);
  EditLayerProps &layerProp = EditLayerProps::layerProps[layerPropsIndex];

  layerProp.exp = !layerProp.exp;
}

void HeightmapLandOutlinerInterface::renameLayer(int type, int per_type_layer_index, const char *name)
{
  String errorMessage;
  if (!canRenameLayerTo(type, per_type_layer_index, name, errorMessage))
    return;

  if (strcmp(getLayerName(type, per_type_layer_index), name) == 0)
    return;

  const int layerPropsIndex = getLayerPropsIndex(type, per_type_layer_index);
  if (layerPropsIndex < 0)
    return;

  EditLayerProps::renameLayer(layerPropsIndex, name);
}

bool HeightmapLandOutlinerInterface::isObjectSelected(RenderableEditableObject &object)
{
  if (LandscapeEntityObject *entity = RTTI_cast<LandscapeEntityObject>(&object))
    return entity->isSelected();

  if (SplineObject *spline = RTTI_cast<SplineObject>(&object))
  {
    if (spline->isSelected())
      return true;

    for (SplinePointObject *splinePoint : spline->points)
      if (splinePoint->isSelected())
        return true;

    return false;
  }

  int type;
  int layerPropsIndex;
  G_ASSERT(!getObjectTypeAndLayerPropsIndex(object, type, layerPropsIndex));
  return false;
}

bool HeightmapLandOutlinerInterface::canSelectObject(RenderableEditableObject &object) { return objectEditor.canSelectObj(&object); }

bool HeightmapLandOutlinerInterface::canRenameObject(RenderableEditableObject &object, const char *name, String &error_message)
{
  if (!object.mayRename())
  {
    error_message = "This object cannot be renamed.";
    return false;
  }

  if (name == nullptr || name[0] == 0)
  {
    error_message = "The name cannot be empty.";
    return false;
  }

  // Do not show error if the new name is the same is the old one. It is annoying to see an error right after starting
  // renaming because the name is the same. It will be a no operation in ObjectEditor.renameObject anyway.
  if (strcmp(object.getName(), name) == 0)
    return true;

  if (objectEditor.getObjectByName(name))
  {
    error_message = "The name must be unique. There is already an object with this name.";
    return false;
  }

  return true;
}

bool HeightmapLandOutlinerInterface::getObjectTypeAndPerTypeLayerIndex(RenderableEditableObject &object, int &type,
  int &per_type_layer_index)
{
  int layerPropsIndex;
  if (!getObjectTypeAndLayerPropsIndex(object, type, layerPropsIndex))
    return false;

  per_type_layer_index = getPerTypeLayerIndex(type, layerPropsIndex);
  return true;
}

const char *HeightmapLandOutlinerInterface::getObjectAssetName(RenderableEditableObject &object) { return getAssetName(object); }

int HeightmapLandOutlinerInterface::getObjectAssetType(RenderableEditableObject &object, const char *&asset_type_name)
{
  const char *assetName = getAssetName(object);
  const DagorAsset *asset = DAEDITOR3.getAssetByName(assetName);
  if (!asset)
  {
    asset_type_name = nullptr;
    return -1;
  }

  asset_type_name = DAEDITOR3.getAssetTypeName(asset->getType());
  return asset->getType();
}

bool HeightmapLandOutlinerInterface::isSampleObject(RenderableEditableObject &object) { return objectEditor.isSampleObject(object); }

void HeightmapLandOutlinerInterface::startObjectSelection() { objectEditor.getUndoSystem()->begin(); }

void HeightmapLandOutlinerInterface::setObjectSelected(RenderableEditableObject &object, bool selected)
{
  // Ensure that startObjectSelection has been called.
  G_ASSERT(objectEditor.getUndoSystem()->is_holding());

  if (!objectEditor.canSelectObj(&object))
    return;

  object.selectObject(selected);

  // We must unselect all the sub-objects to be consistent with isObjectSelected.
  if (!selected)
    if (SplineObject *spline = RTTI_cast<SplineObject>(&object))
      for (SplinePointObject *splinePoint : spline->points)
        splinePoint->selectObject(false);
}

void HeightmapLandOutlinerInterface::endObjectSelection()
{
  objectEditor.updateGizmo();
  objectEditor.getUndoSystem()->accept("Selection in Outliner");
}

void HeightmapLandOutlinerInterface::unselectAllObjects()
{
  objectEditor.getUndoSystem()->begin();
  objectEditor.unselectAll();
  objectEditor.getUndoSystem()->accept("Unselect all in Outliner");
}

void HeightmapLandOutlinerInterface::moveObjectsToLayer(dag::Span<RenderableEditableObject *> objects, int type,
  int per_type_destination_layer_index)
{
  dag::Vector<RenderableEditableObject *> objectsToMove;
  objectsToMove.set_capacity(objects.size());

  for (RenderableEditableObject *object : objects)
  {
    G_ASSERT(object);
    if (!object)
      continue;

    int objectType;
    int layerPropsIndex;
    if (getObjectTypeAndLayerPropsIndex(*object, objectType, layerPropsIndex) && objectType == type)
      objectsToMove.push_back(object);
  }

  if (objectsToMove.empty())
    return;

  const int dstLayerPropsIndex = getLayerPropsIndex(type, per_type_destination_layer_index);
  HmapLandPlugin::self->moveObjectsToLayer(dstLayerPropsIndex, make_span(objectsToMove));
}

void HeightmapLandOutlinerInterface::renameObject(RenderableEditableObject &object, const char *name)
{
  String errorMessage;
  if (!canRenameObject(object, name, errorMessage))
    return;

  objectEditor.renameObject(&object, name);
}

void HeightmapLandOutlinerInterface::changeObjectAsset(dag::Span<RenderableEditableObject *> objects)
{
  LandscapeEntityObject *firstEntityObject = nullptr;
  SplineObject *firstSplineObject = nullptr;

  dag::Vector<RenderableEditableObject *> objectsToChange;
  objectsToChange.set_capacity(objects.size());

  for (RenderableEditableObject *object : objects)
  {
    if (LandscapeEntityObject *currentEntityObject = RTTI_cast<LandscapeEntityObject>(object))
    {
      if (!firstEntityObject)
        firstEntityObject = currentEntityObject;
    }
    else if (SplineObject *currentSplineObject = RTTI_cast<SplineObject>(object))
    {
      if (!firstSplineObject)
        firstSplineObject = currentSplineObject;
    }
    else
    {
      continue;
    }

    if (firstEntityObject && firstSplineObject)
      return;

    objectsToChange.push_back(object);
  }

  if (firstEntityObject)
    LandscapeEntityObject::changeAssset(objectEditor, objectsToChange, firstEntityObject->getProps().entityName);
  else if (firstSplineObject)
    SplineObject::changeAsset(objectEditor, objectsToChange, firstSplineObject->getProps().blkGenName, firstSplineObject->isPoly());
}

void HeightmapLandOutlinerInterface::deleteObjects(dag::Span<RenderableEditableObject *> objects)
{
  dag::Vector<RenderableEditableObject *> objectsToDelete;
  objectsToDelete.set_capacity(objects.size());

  for (RenderableEditableObject *object : objects)
    if (object->mayDelete())
      objectsToDelete.push_back(object);

  if (objectsToDelete.empty())
    return;

  objectEditor.getUndoSystem()->begin();
  objectEditor.removeObjects(&objectsToDelete[0], objectsToDelete.size());
  objectEditor.getUndoSystem()->accept("Delete");
}

void HeightmapLandOutlinerInterface::zoomAndCenterObject(RenderableEditableObject &object)
{
  BBox3 bbox;
  if (!object.getWorldBox(bbox))
    return;

  IGenViewportWnd *viewport = EDITORCORE->getCurrentViewport();
  if (viewport)
    viewport->zoomAndCenter(bbox);
}

int HeightmapLandOutlinerInterface::getLayerPropsIndex(int type, int per_type_layer_index) const
{
  int count = 0;
  for (int i = 0; i < EditLayerProps::layerProps.size(); ++i)
    if (EditLayerProps::layerProps[i].type == type)
    {
      if (count == per_type_layer_index)
        return i;

      ++count;
    }

  G_ASSERT(false);
  return -1;
}

int HeightmapLandOutlinerInterface::getPerTypeLayerIndex(int type, int layer_props_index) const
{
  int perTypeLayerIndex = 0;
  for (int i = 0; i < EditLayerProps::layerProps.size(); ++i)
    if (EditLayerProps::layerProps[i].type == type)
    {
      if (i == layer_props_index)
        return perTypeLayerIndex;

      ++perTypeLayerIndex;
    }

  G_ASSERT(false);
  return -1;
}

EditLayerProps *HeightmapLandOutlinerInterface::getEditLayerProps(int type, int per_type_layer_index) const
{
  const int layerPropsIndex = getLayerPropsIndex(type, per_type_layer_index);
  if (layerPropsIndex >= 0)
    return &EditLayerProps::layerProps[layerPropsIndex];

  G_ASSERT(false);
  return nullptr;
}

bool HeightmapLandOutlinerInterface::getObjectTypeAndLayerPropsIndex(RenderableEditableObject &object, int &type,
  int &layer_props_index)
{
  LandscapeEntityObject *entity = RTTI_cast<LandscapeEntityObject>(&object);
  if (entity)
  {
    type = EditLayerProps::ENT;
    layer_props_index = entity->getEditLayerIdx();
    return true;
  }

  SplineObject *spline = RTTI_cast<SplineObject>(&object);
  if (spline)
  {
    type = spline->isPoly() ? EditLayerProps::PLG : EditLayerProps::SPL;
    layer_props_index = spline->getEditLayerIdx();
    return true;
  }

  G_ASSERT(RTTI_cast<SplinePointObject>(&object) || RTTI_cast<SphereLightObject>(&object) || RTTI_cast<SnowSourceObject>(&object));
  return false;
}

const char *HeightmapLandOutlinerInterface::getAssetName(RenderableEditableObject &object)
{
  if (const LandscapeEntityObject *entityObject = RTTI_cast<LandscapeEntityObject>(&object))
    return entityObject->getProps().entityName.c_str();
  if (const SplineObject *splineObject = RTTI_cast<SplineObject>(&object))
    return splineObject->getProps().blkGenName;
  return nullptr;
}

bool HeightmapLandOutlinerInterface::isValidLayerName(const char *name, String &error_message)
{
  if (name == nullptr || name[0] == 0)
  {
    error_message = "The layer name cannot be empty!";
    return false;
  }

  for (const char *p = name; *p != 0; ++p)
  {
    if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_')
      continue;

    error_message = "Only latin letters (a-z, A-Z), digits (0-9) and the underscore (_) character are allowed in the name.";
    return false;
  }

  return true;
}
