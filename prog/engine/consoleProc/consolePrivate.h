#ifndef _DAGOR_CONSOLE_PRIVATE_H__
#define _DAGOR_CONSOLE_PRIVATE_H__
#pragma once
#include <util/dag_console.h>
#include <gui/dag_visConsole.h>
#include <generic/dag_tab.h>

namespace console_private
{

extern real consoleSpeed; // pixes/sec
extern real conHeight;

static constexpr int MAX_HISTORY_COMMANDS = 64;

extern console::IVisualConsoleDriver *conDriver;
extern Tab<SimpleString> commandHistory;
extern int historyPtr;
extern Tab<String> commandHooks; // List of special subcommands that take regular commands as arguments
                                 // f.e. this is used to correctly generate hints for these command arguments

Tab<console::ICommandProcessor *> &getProcList();
void saveConsoleCommands();

void registerBaseConProc();
void unregisterBaseConProc();
} // namespace console_private

#endif
