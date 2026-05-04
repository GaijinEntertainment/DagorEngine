// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "entityMatEditorUndo.h"

#include "entityMatEditor.h"

MaterialEditorUndoObject::MaterialEditorUndoObject(EntityMaterialEditor &material_editor, int lod, int material_index) :
  materialEditor(material_editor), undoLod(lod), undoMaterialIndex(material_index)
{
  materialEditor.saveForUndo(lod, material_index, undoData);
}

void MaterialEditorUndoObject::restore(bool save_redo)
{
  if (save_redo)
  {
    EntityMatProperties newUndoData;
    materialEditor.saveForUndo(undoLod, undoMaterialIndex, newUndoData);
    materialEditor.loadFromUndo(undoLod, undoMaterialIndex, undoData);
    undoData = newUndoData;
  }
  else
  {
    materialEditor.loadFromUndo(undoLod, undoMaterialIndex, undoData);
  }
}

void MaterialEditorUndoObject::redo() { restore(/*save_redo = */ true); }
