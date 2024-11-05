// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_outlinerInterface.h>
#include "hmlLayers.h"

class HmapLandObjectEditor;
class RenderableEditableObject;

class HeightmapLandOutlinerInterface : public Outliner::IOutliner
{
public:
  explicit HeightmapLandOutlinerInterface(HmapLandObjectEditor &object_editor);

  virtual int getTypeCount() override;
  virtual const char *getTypeName(int type, bool plural = false) override;
  virtual bool isTypeVisible(int type) override;
  virtual bool isTypeLocked(int type) override;
  virtual bool canAddNewLayerWithName(int type, const char *name, String &error_message) override;

  virtual void selectAllTypeObjects(int type, bool select) override;
  virtual void toggleTypeVisibility(int type) override;
  virtual void toggleTypeLock(int type) override;
  virtual int addNewLayer(int type, const char *name) override;

  virtual int getLayerCount(int type) override;
  virtual const char *getLayerName(int type, int per_type_layer_index) override;
  virtual bool isLayerActive(int type, int per_type_layer_index) override;
  virtual bool isLayerVisible(int type, int per_type_layer_index) override;
  virtual bool isLayerLocked(int type, int per_type_layer_index) override;
  virtual bool isLayerAppliedToMask(int type, int per_type_layer_index) override;
  virtual bool isLayerExported(int type, int per_type_layer_index) override;
  virtual bool canChangeLayerVisibility(int type, int per_type_layer_index) override;
  virtual bool canChangeLayerLock(int type, int per_type_layer_index) override;
  virtual bool isLayerRenameable(int type, int per_type_layer_index) override;
  virtual bool canRenameLayerTo(int type, int per_type_layer_index, const char *name, String &error_message) override;

  virtual void setLayerActive(int type, int per_type_layer_index) override;
  virtual void selectAllLayerObjects(int type, int per_type_layer_index, bool select) override;
  virtual void toggleLayerVisibility(int type, int per_type_layer_index) override;
  virtual void toggleLayerLock(int type, int per_type_layer_index) override;
  virtual void toggleLayerApplyToMask(int type, int per_type_layer_index) override;
  virtual void toggleLayerExport(int type, int per_type_layer_index) override;
  virtual void renameLayer(int type, int per_type_layer_index, const char *name) override;

  virtual bool isObjectSelected(RenderableEditableObject &object) override;
  virtual bool canSelectObject(RenderableEditableObject &object) override;
  virtual bool canRenameObject(RenderableEditableObject &object, const char *name, String &error_message) override;
  virtual bool getObjectTypeAndPerTypeLayerIndex(RenderableEditableObject &object, int &type, int &per_type_layer_index) override;
  virtual const char *getObjectAssetName(RenderableEditableObject &object) override;
  virtual int getObjectAssetType(RenderableEditableObject &object, const char *&asset_type_name) override;
  virtual bool isSampleObject(RenderableEditableObject &object) override;

  virtual void startObjectSelection() override;
  virtual void setObjectSelected(RenderableEditableObject &object, bool selected) override;
  virtual void endObjectSelection() override;
  virtual void unselectAllObjects() override;
  virtual void moveObjectsToLayer(dag::Span<RenderableEditableObject *> objects, int type,
    int per_type_destination_layer_index) override;
  virtual void renameObject(RenderableEditableObject &object, const char *name) override;
  virtual void changeObjectAsset(dag::Span<RenderableEditableObject *> objects) override;
  virtual void deleteObjects(dag::Span<RenderableEditableObject *> objects) override;
  virtual void zoomAndCenterObject(RenderableEditableObject &object) override;

private:
  struct ObjectTypeState
  {
    bool visible = true;
    bool locked = false;
  };

  int getLayerPropsIndex(int type, int per_type_layer_index) const;
  int getPerTypeLayerIndex(int type, int layer_props_index) const;
  EditLayerProps *getEditLayerProps(int type, int per_type_layer_index) const;

  static bool getObjectTypeAndLayerPropsIndex(RenderableEditableObject &object, int &type, int &layer_props_index);
  static const char *getAssetName(RenderableEditableObject &object);
  static bool isValidLayerName(const char *name, String &error_message);

  HmapLandObjectEditor &objectEditor;
  ObjectTypeState objectTypeStates[EditLayerProps::TYPENUM];
};
