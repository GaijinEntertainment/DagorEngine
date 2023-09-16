//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dag/dag_vector.h>
#include <util/dag_globDef.h>

class WindowBase;
class DataBlock;
class String;

class WindowControlEventHandler
{
public:
  virtual ~WindowControlEventHandler() {}

  virtual long onWcChanging(WindowBase *source)
  {
    G_UNUSED(source);
    return 0;
  }
  virtual void onWcChange(WindowBase *source) { G_UNUSED(source); }
  virtual void onWcClick(WindowBase *source) { G_UNUSED(source); }
  virtual void onWcRightClick(WindowBase *source) { G_UNUSED(source); }
  virtual void onWcCommand(WindowBase *source, unsigned notify_code, unsigned elem_id)
  {
    G_UNUSED(source);
    G_UNUSED(notify_code);
    G_UNUSED(elem_id);
  }
  virtual void onWcDoubleClick(WindowBase *source) { G_UNUSED(source); }
  virtual long onWcKeyDown(WindowBase *source, unsigned v_key)
  {
    G_UNUSED(source);
    G_UNUSED(v_key);
    return 0;
  }
  virtual void onWcResize(WindowBase *source) { G_UNUSED(source); }
  virtual bool onWcClose(WindowBase *source)
  {
    G_UNUSED(source);
    return true;
  }
  virtual void onWcRefresh(WindowBase *source) { G_UNUSED(source); }
  virtual void onWcPostEvent(unsigned id) { G_UNUSED(id); }
  virtual long onWcClipboardCopy(WindowBase *source, DataBlock &blk)
  {
    G_UNUSED(source);
    G_UNUSED(blk);
    return 0;
  }
  virtual long onWcClipboardPaste(WindowBase *source, const DataBlock &blk)
  {
    G_UNUSED(source);
    G_UNUSED(blk);
    return 0;
  }
  virtual long onDropFiles(WindowBase *source, const dag::Vector<String> &paths)
  {
    G_UNUSED(source);
    G_UNUSED(paths);
    return 0;
  }
};
