// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>

class RenderableEditableObject;
class String;

namespace Outliner
{

class IOutliner
{
public:
  virtual int getTypeCount() = 0;
  virtual const char *getTypeName(int type, bool plural = false) = 0;
  virtual bool isTypeVisible(int type) = 0;
  virtual bool isTypeLocked(int type) = 0;
  virtual bool canAddNewLayerWithName(int type, const char *name, String &error_message) = 0;

  virtual void selectAllTypeObjects(int type, bool select) = 0;
  virtual void toggleTypeVisibility(int type) = 0;
  virtual void toggleTypeLock(int type) = 0;

  // It must return with the per type layer index of the new layer or -1 in case of failure.
  virtual int addNewLayer(int type, const char *name) = 0;

  virtual int getLayerCount(int type) = 0;
  virtual const char *getLayerName(int type, int per_type_layer_index) = 0;
  virtual bool isLayerActive(int type, int per_type_layer_index) = 0;
  virtual bool isLayerVisible(int type, int per_type_layer_index) = 0;
  virtual bool isLayerLocked(int type, int per_type_layer_index) = 0;
  virtual bool isLayerAppliedToMask(int type, int per_type_layer_index) = 0;
  virtual bool isLayerExported(int type, int per_type_layer_index) = 0;
  virtual bool canChangeLayerVisibility(int type, int per_type_layer_index) = 0;
  virtual bool canChangeLayerLock(int type, int per_type_layer_index) = 0;
  virtual bool isLayerRenameable(int type, int per_type_layer_index) = 0;
  virtual bool canRenameLayerTo(int type, int per_type_layer_index, const char *name, String &error_message) = 0;

  virtual void setLayerActive(int type, int per_type_layer_index) = 0;
  virtual void selectAllLayerObjects(int type, int per_type_layer_index, bool select) = 0;
  virtual void toggleLayerVisibility(int type, int per_type_layer_index) = 0;
  virtual void toggleLayerLock(int type, int per_type_layer_index) = 0;
  virtual void toggleLayerApplyToMask(int type, int per_type_layer_index) = 0;
  virtual void toggleLayerExport(int type, int per_type_layer_index) = 0;
  virtual void renameLayer(int type, int per_type_layer_index, const char *name) = 0;

  virtual bool isObjectSelected(RenderableEditableObject &object) = 0;
  virtual bool canSelectObject(RenderableEditableObject &object) = 0;
  virtual bool canRenameObject(RenderableEditableObject &object, const char *name, String &error_message) = 0;
  virtual bool getObjectTypeAndPerTypeLayerIndex(RenderableEditableObject &object, int &type, int &per_type_layer_index) = 0;
  virtual const char *getObjectAssetName(RenderableEditableObject &object) = 0;

  // Return -1 if there is no asset type.
  virtual int getObjectAssetType(RenderableEditableObject &object, const char *&asset_type_name) = 0;

  // The sample object is the object that is currently being placed.
  virtual bool isSampleObject(RenderableEditableObject &object) = 0;

  virtual void startObjectSelection() = 0;
  virtual void setObjectSelected(RenderableEditableObject &object, bool selected) = 0;
  virtual void endObjectSelection() = 0;
  virtual void unselectAllObjects() = 0;
  virtual void moveObjectsToLayer(dag::Span<RenderableEditableObject *> objects, int type, int per_type_destination_layer_index) = 0;
  virtual void renameObject(RenderableEditableObject &object, const char *name) = 0;
  virtual void changeObjectAsset(dag::Span<RenderableEditableObject *> objects) = 0;
  virtual void deleteObjects(dag::Span<RenderableEditableObject *> objects) = 0;
  virtual void zoomAndCenterObject(RenderableEditableObject &object) = 0;
};

} // namespace Outliner