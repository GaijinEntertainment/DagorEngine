// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include "consolePrivate.h"
using namespace console_private;

namespace console
{
// init console manager
void init()
{
  registerBaseConProc();

  clear_and_shrink(commandHistory);
  clear_and_shrink(pinnedCommands);

  historyPtr = -1;

#if _TARGET_PC | _TARGET_XBOX
  if (!::dd_file_exist("console_cmd.blk"))
    return;

  DataBlock blk;
  blk.load("console_cmd.blk");
  int nid = blk.getNameId("cmd");
  int pinnedNameId = blk.getNameId("pinned");

  for (int i = 0; i < blk.paramCount(); i++)
  {
    if (blk.getParamNameId(i) == nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
    {
      commandHistory.push_back(SimpleString(blk.getStr(i)));
      if (commandHistory.size() >= MAX_HISTORY_COMMANDS)
        break;
    }
    if (blk.getParamNameId(i) == pinnedNameId && blk.getParamType(i) == DataBlock::TYPE_STRING)
    {
      pinnedCommands.push_back(SimpleString(blk.getStr(i)));
      if (pinnedCommands.size() >= MAX_PINNED_COMMANDS)
        break;
    }
  }
  conHeight = blk.getReal("cur_height", 0.33);
#endif
}


// close console manager
void shutdown()
{
  saveConsoleCommands();
  set_visual_driver(NULL, NULL);
  clear_and_shrink(getProcList());

  unregisterBaseConProc();
}
} // namespace console
