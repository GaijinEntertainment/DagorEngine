// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ecsEditableObject.h"
#include "ecsObjectEditor.h"

#include <libTools/util/undo.h>

class ECSHierarchicalUndoGroup : public UndoRedoObject
{
public:
  void clear()
  {
    directlyChangedObjects.clear();
    indirectlyChangedObjects.clear();
  }

  // objects directly changed by the operation (for example they were selected and the gizmo moved them)
  void addDirectlyChangedObject(ECSEditableObject &object)
  {
    UndoRedoData &data = directlyChangedObjects.push_back();
    data.object = &object;
    data.oldData.setFromEntity(object);
    data.redoData = data.oldData;
  }

  // objects indirectly changed by the operation (e.g.: parent moves children)
  void addIndirectlyChangedObject(ECSEditableObject &object) { indirectlyChangedObjects.push_back(&object); }

  void saveTransformComponentOfAllObjects()
  {
    // We need hierarchy_attached_entity_transform_es() to run to be able to get the final transform values.
    g_entity_mgr->tick(true);
    g_entity_mgr->update(ecs::UpdateStageInfoAct(0.0f, 0.0f));

    for (UndoRedoData &undoRedoObject : directlyChangedObjects)
      ECSObjectEditor::saveComponent(undoRedoObject.object->getEid(), "transform");

    for (Ptr<ECSEditableObject> &object : indirectlyChangedObjects)
      ECSObjectEditor::saveComponent(object->getEid(), "transform");
  }

  void undo()
  {
    for (UndoRedoData &undoRedoObject : directlyChangedObjects)
      undoRedoObject.oldData.applyToEntity(*undoRedoObject.object);
  }

private:
  struct Data
  {
    void setFromEntity(const ECSEditableObject &o)
    {
      wtm = o.getWtm();

      const TMatrix *ht = g_entity_mgr->getNullable<TMatrix>(o.getEid(), ECS_HASH("hierarchy_transform"));
      hierarchyTransform = ht ? *ht : TMatrix::IDENT;

      const TMatrix *hplt = g_entity_mgr->getNullable<TMatrix>(o.getEid(), ECS_HASH("hierarchy_parent_last_transform"));
      hierarchyParentLastTransform = hplt ? *hplt : TMatrix::IDENT;
    }

    void applyToEntity(ECSEditableObject &o)
    {
      o.setWtm(wtm);

      TMatrix *t = g_entity_mgr->getNullableRW<TMatrix>(o.getEid(), ECS_HASH("transform"));
      if (t)
        *t = wtm; // Needed because setWtm() only sets the transform in ECS if the object is selected.

      TMatrix *ht = g_entity_mgr->getNullableRW<TMatrix>(o.getEid(), ECS_HASH("hierarchy_transform"));
      if (ht)
        *ht = hierarchyTransform;

      TMatrix *hplt = g_entity_mgr->getNullableRW<TMatrix>(o.getEid(), ECS_HASH("hierarchy_parent_last_transform"));
      if (hplt)
        *hplt = hierarchyParentLastTransform;
    }

    TMatrix wtm;
    TMatrix hierarchyTransform;
    TMatrix hierarchyParentLastTransform;
  };

  struct UndoRedoData
  {
    Ptr<ECSEditableObject> object;
    Data oldData, redoData;
  };

  void restore(bool save_redo) override
  {
    if (save_redo)
      for (UndoRedoData &undoRedoObject : directlyChangedObjects)
        undoRedoObject.redoData.setFromEntity(*undoRedoObject.object);

    for (UndoRedoData &undoRedoObject : directlyChangedObjects)
      undoRedoObject.oldData.applyToEntity(*undoRedoObject.object);

    saveTransformComponentOfAllObjects();
  }

  void redo() override
  {
    for (UndoRedoData &undoRedoObject : directlyChangedObjects)
      undoRedoObject.redoData.applyToEntity(*undoRedoObject.object);

    saveTransformComponentOfAllObjects();
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "HierarchicalUndoEnd"; }

  dag::Vector<UndoRedoData> directlyChangedObjects;
  PtrTab<ECSEditableObject> indirectlyChangedObjects;
};
