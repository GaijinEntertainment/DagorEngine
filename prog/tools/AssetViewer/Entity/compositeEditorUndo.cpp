// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorUndo.h"
#include "../av_appwnd.h"
#include <ioSys/dag_memIo.h>

void CompositeEditorUndoParams::restore(bool save_redo)
{
  const CompositeEditor &compositeEditor = get_app().getCompositeEditor();

  if (save_redo)
  {
    DataBlock redoDataBlock;
    compositeEditor.saveForUndo(redoDataBlock);

    unsigned redoSelection = IDataBlockIdHolder::invalid_id;
    dag::Vector<unsigned> redoMultiSelections;
    if (containsSavedSelection())
      compositeEditor.getSelectedTreeNodeDataBlockIds(redoMultiSelections, redoSelection);

    loadUndo();

    saveUndoFromDataBlock(redoDataBlock);
    selectedTreeNodeDataBlockId = redoSelection;
    selectedTreeNodeDataBlockIds.assign(redoMultiSelections.begin(), redoMultiSelections.end());
  }
  else
    loadUndo();
}

void CompositeEditorUndoParams::redo() { restore(/*save_redo = */ true); }

void CompositeEditorUndoParams::loadUndo() const
{
  CompositeEditor &compositeEditor = get_app().getCompositeEditor();

  InPlaceMemLoadCB memoryLoad(buffer.get(), bufferSize);
  DataBlock dataBlock;
  dataBlock.loadFromStream(memoryLoad);

  unsigned newSelection = IDataBlockIdHolder::invalid_id;
  dag::Vector<unsigned> newMultiSelections;
  if (containsSavedSelection())
    compositeEditor.getSelectedTreeNodeDataBlockIds(newMultiSelections, newSelection);

  compositeEditor.loadFromUndo(dataBlock, newSelection, newMultiSelections);
}

void CompositeEditorUndoParams::saveUndo(bool save_selection)
{
  const CompositeEditor &compositeEditor = get_app().getCompositeEditor();

  DataBlock dataBlock;
  compositeEditor.saveForUndo(dataBlock);
  saveUndoFromDataBlock(dataBlock);

  if (save_selection)
  {
    compositeEditor.getSelectedTreeNodeDataBlockIds(selectedTreeNodeDataBlockIds, selectedTreeNodeDataBlockId);
  }
  else
  {
    selectedTreeNodeDataBlockId = IDataBlockIdHolder::invalid_id;
    selectedTreeNodeDataBlockIds.clear();
  }
}

void CompositeEditorUndoParams::saveUndoFromDataBlock(const DataBlock &dataBlock)
{
  // The stream is roughly half the size of the DataBlock.
  DynamicMemGeneralSaveCB memorySave(tmpmem);
  dataBlock.saveToStream(memorySave);

  // And we save some more memory by storing only the actually used bytes in a raw array instead of storing
  // DynamicMemGeneralSaveCB. (DynamicMemGeneralSaveCB grows in fixed sized steps.)
  bufferSize = memorySave.size();
  buffer.reset(new uint8_t[bufferSize]);
  memcpy(buffer.get(), memorySave.data(), bufferSize);
}

bool CompositeEditorUndoParams::containsSavedSelection() const
{
  return selectedTreeNodeDataBlockId != IDataBlockIdHolder::invalid_id || selectedTreeNodeDataBlockIds.size() > 0;
}
