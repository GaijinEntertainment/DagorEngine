// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "consolePrivate.h"

real console_private::consoleSpeed = 1400; // pixes/sec
real console_private::conHeight = 0.33f;
console::IVisualConsoleDriver *console_private::conDriver = NULL;

Tab<SimpleString> console_private::commandHistory(midmem_ptr());
Tab<SimpleString> console_private::pinnedCommands(midmem_ptr());
int console_private::historyPtr = -1;

Tab<String> console_private::commandHooks(midmem_ptr());

Tab<console::ICommandProcessor *> &console_private::getProcList()
{
  static Tab<console::ICommandProcessor *> procList(midmem_ptr());
  return procList;
}
