// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "entityMatProperties.h"

#include <libTools/util/undo.h>

class EntityMaterialEditor;

class MaterialEditorUndoObject : public UndoRedoObject
{
public:
  MaterialEditorUndoObject(EntityMaterialEditor &material_editor, int lod, int material_index);

private:
  void restore(bool save_redo) override;
  void redo() override;
  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "MaterialEditorUndoObject"; }

  EntityMaterialEditor &materialEditor;
  const int undoLod;
  const int undoMaterialIndex;
  EntityMatProperties undoData;
};
