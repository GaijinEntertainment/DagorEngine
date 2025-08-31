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

  int getTypeCount() override;
  const char *getTypeName(int type, bool plural = false) override;
  bool isTypeVisible(int type) override;
  bool isTypeLocked(int type) override;
  bool canAddNewLayerWithName(int type, const char *name, String &error_message) override;

  void selectAllTypeObjects(int type, bool select) override;
  void toggleTypeVisibility(int type) override;
  void toggleTypeLock(int type) override;
  int addNewLayer(int type, const char *name) override;

  int getLayerCount(int type) override;
  const char *getLayerName(int type, int per_type_layer_index) override;
  bool isLayerActive(int type, int per_type_layer_index) override;
  bool isLayerVisible(int type, int per_type_layer_index) override;
  bool isLayerLocked(int type, int per_type_layer_index) override;
  bool isLayerAppliedToMask(int type, int per_type_layer_index) override;
  bool isLayerExported(int type, int per_type_layer_index) override;
  bool canChangeLayerVisibility(int type, int per_type_layer_index) override;
  bool canChangeLayerLock(int type, int per_type_layer_index) override;
  bool isLayerRenameable(int type, int per_type_layer_index) override;
  bool canRenameLayerTo(int type, int per_type_layer_index, const char *name, String &error_message) override;

  void setLayerActive(int type, int per_type_layer_index) override;
  void selectAllLayerObjects(int type, int per_type_layer_index, bool select) override;
  void toggleLayerVisibility(int type, int per_type_layer_index) override;
  void toggleLayerLock(int type, int per_type_layer_index) override;
  void toggleLayerApplyToMask(int type, int per_type_layer_index) override;
  void toggleLayerExport(int type, int per_type_layer_index) override;
  void renameLayer(int type, int per_type_layer_index, const char *name) override;

  bool isObjectSelected(RenderableEditableObject &object) override;
  bool canSelectObject(RenderableEditableObject &object) override;
  bool canRenameObject(RenderableEditableObject &object, const char *name, String &error_message) override;
  bool getObjectTypeAndPerTypeLayerIndex(RenderableEditableObject &object, int &type, int &per_type_layer_index) override;
  const char *getObjectAssetName(RenderableEditableObject &object) override;
  int getObjectAssetType(RenderableEditableObject &object, const char *&asset_type_name) override;
  bool isSampleObject(RenderableEditableObject &object) override;

  void startObjectSelection() override;
  void setObjectSelected(RenderableEditableObject &object, bool selected) override;
  void endObjectSelection() override;
  void unselectAllObjects() override;
  void moveObjectsToLayer(dag::Span<RenderableEditableObject *> objects, int type, int per_type_destination_layer_index) override;
  void renameObject(RenderableEditableObject &object, const char *name) override;
  void changeObjectAsset(dag::Span<RenderableEditableObject *> objects) override;
  void deleteObjects(dag::Span<RenderableEditableObject *> objects) override;
  void zoomAndCenterObject(RenderableEditableObject &object) override;

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
