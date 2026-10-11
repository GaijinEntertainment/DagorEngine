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
    unsigned redoSelection = IDataBlockIdHolder::invalid_id;
    dag::Vector<unsigned> redoMultiSelections;
    compositeEditor.saveForUndo(redoDataBlock, redoSelection, redoMultiSelections);

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

  compositeEditor.loadFromUndo(dataBlock, selectedTreeNodeDataBlockId, selectedTreeNodeDataBlockIds);
}

void CompositeEditorUndoParams::saveUndo()
{
  const CompositeEditor &compositeEditor = get_app().getCompositeEditor();

  DataBlock dataBlock;
  compositeEditor.saveForUndo(dataBlock, selectedTreeNodeDataBlockId, selectedTreeNodeDataBlockIds);
  saveUndoFromDataBlock(dataBlock);
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
