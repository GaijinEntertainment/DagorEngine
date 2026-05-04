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

    const unsigned redoSelection =
      containsSavedSelection() ? compositeEditor.getSelectedTreeNodeDataBlockId() : IDataBlockIdHolder::invalid_id;

    loadUndo();

    saveUndoFromDataBlock(redoDataBlock);
    selectedTreeNodeDataBlockId = redoSelection;
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

  const unsigned newSelection =
    containsSavedSelection() ? selectedTreeNodeDataBlockId : compositeEditor.getSelectedTreeNodeDataBlockId();

  compositeEditor.loadFromUndo(dataBlock, newSelection);
}

void CompositeEditorUndoParams::saveUndo(bool save_selection)
{
  const CompositeEditor &compositeEditor = get_app().getCompositeEditor();

  DataBlock dataBlock;
  compositeEditor.saveForUndo(dataBlock);
  saveUndoFromDataBlock(dataBlock);

  selectedTreeNodeDataBlockId = save_selection ? compositeEditor.getSelectedTreeNodeDataBlockId() : IDataBlockIdHolder::invalid_id;
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