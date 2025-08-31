// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/util/undo.h>
#include <EASTL/unique_ptr.h>

class DataBlock;

class CompositeEditorUndoParams : public UndoRedoObject
{
public:
  void saveUndo();

private:
  void restore(bool save_redo) override;
  void redo() override;
  size_t size() override { return bufferSize; }
  void accepted() override {}
  void get_description(String &s) override { s = "CompositeEditorUndoParams"; }

  void loadUndo() const;
  void saveUndoFromDataBlock(const DataBlock &dataBlock);

  eastl::unique_ptr<uint8_t[]> buffer;
  size_t bufferSize = 0;
};