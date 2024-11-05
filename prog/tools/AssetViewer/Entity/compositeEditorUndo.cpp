// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorUndo.h"
#include "../av_appwnd.h"
#include <ioSys/dag_memIo.h>

void CompositeEditorUndoParams::restore(bool save_redo)
{
  if (save_redo)
  {
    DataBlock redoDataBlock;
    get_app().getCompositeEditor().saveForUndo(redoDataBlock);

    loadUndo();

    saveUndoFromDataBlock(redoDataBlock);
  }
  else
    loadUndo();
}

void CompositeEditorUndoParams::redo() { restore(/*save_redo = */ true); }

void CompositeEditorUndoParams::loadUndo() const
{
  InPlaceMemLoadCB memoryLoad(buffer.get(), bufferSize);
  DataBlock dataBlock;
  dataBlock.loadFromStream(memoryLoad);
  get_app().getCompositeEditor().loadFromUndo(dataBlock);
}

void CompositeEditorUndoParams::saveUndo()
{
  DataBlock dataBlock;
  get_app().getCompositeEditor().saveForUndo(dataBlock);
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