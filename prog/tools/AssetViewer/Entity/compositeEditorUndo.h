// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <de3_dataBlockIdHolder.h>
#include <libTools/util/undo.h>
#include <dag/dag_vector.h>
#include <EASTL/unique_ptr.h>

class DataBlock;

class CompositeEditorUndoParams : public UndoRedoObject
{
public:
  CompositeEditorUndoParams() { saveUndo(); }

  UNDO_MERGE_SNAPSHOT_BY_TARGET(0x3E647F18u, nullptr) // CompositeEditorUndoParams

private:
  void restore(bool save_redo) override;
  void redo() override;
  size_t size() override { return bufferSize; }
  void accepted() override {}
  void get_description(String &s) override { s = "CompositeEditorUndoParams"; }

  void loadUndo() const;
  void saveUndo();
  void saveUndoFromDataBlock(const DataBlock &dataBlock);

  eastl::unique_ptr<uint8_t[]> buffer;
  size_t bufferSize = 0;
  unsigned selectedTreeNodeDataBlockId = IDataBlockIdHolder::invalid_id;
  dag::Vector<unsigned> selectedTreeNodeDataBlockIds;
};