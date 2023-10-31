// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/sort.h>

#include <util/dag_console.h>
#include <gui/dag_visConsole.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_dataBlock.h>
#include "consolePrivate.h"
#include <stdio.h>
#include <memory/dag_framemem.h>


using namespace console_private;

// save commands
void console_private::saveConsoleCommands()
{
#if _TARGET_PC | _TARGET_XBOX
  if (commandHistory.size())
  {
    DataBlock blk;

    for (int i = 0; i < commandHistory.size(); i++)
      blk.addStr("cmd", commandHistory[i]);
    blk.addReal("cur_height", conHeight);
    blk.saveToTextFile("console_cmd.blk");
  }
#endif

  historyPtr = -1;
}

namespace console
{
FuncLinkedList *FuncLinkedList::tail = nullptr;
static class FuncCommandProcessor final : public console::ICommandProcessor
{
public:
  FuncCommandProcessor() { add_con_proc(this); }
  bool find_first = true;
  bool processCommand(const char *argv[], int argc)
  {
    for (FuncLinkedList *s = FuncLinkedList::tail; s; s = s->next)
      if (s->fun(argv, argc) && find_first)
        return true;
    return false;
  }
  void destroy() {}
} func_command_processor;

//************************************************************************
//* common console commands (must be used after set_console_driver)
//************************************************************************

static eastl::vector<ConsoleOutputCallback> console_listeners;
static String cached_prefix;
static console::CommandList cached_commands;

void register_console_listener(const ConsoleOutputCallback &listener_func) { console_listeners.push_back(listener_func); }


void reset_important_lines()
{
  if (conDriver)
    conDriver->reset_important_lines();
}

// output string to console (printf form)
void con_vprintf(LineType type, bool dup_to_log, const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!conDriver && !console_listeners.size() && !dup_to_log)
    return;

  String buffer(framemem_ptr());
  if (anum)
  {
    buffer.vprintf(128, fmt, arg, anum);
    fmt = buffer;
  }
  if (conDriver)
    conDriver->puts(fmt, type);
  for (ConsoleOutputCallback &func : console_listeners)
    func(fmt);
  if (dup_to_log)
    switch (type)
    {
      case console::CONSOLE_ERROR: logerr("CON: %s", fmt); break;
      case console::CONSOLE_WARNING: logwarn("CON: %s", fmt); break;
      case console::CONSOLE_TRACE: debug("CON:trace: %s", fmt); break;
      default: debug("CON: %s", fmt); break;
    }
}

// fing command args
static inline void findArgs(char *cmd, Tab<char *> &arg)
{
  static const char spc[] = " \t\r\n";

  char *s = cmd;
  bool insideString = false;
  int len = 0;
  while (*s)
  {
    if (*s == '\"')
      insideString = !insideString;
    if (insideString && *s == ' ')
      *s = 0x01;
    s++;
    len++;
  }

#ifdef __GNUC__
  for (char *brkt, *p = strtok_r(cmd, spc, &brkt); p; p = strtok_r(NULL, spc, &brkt))
    arg.push_back(p);
#else
  for (char *p = strtok(cmd, spc); p; p = strtok(NULL, spc))
    arg.push_back(p);
#endif

  for (s = cmd; s < cmd + len; s++)
    if (*s == 0x01)
      *s = ' ';

  arg.push_back(NULL);
}

// get prev history command
const char *get_prev_command()
{
  historyPtr++;
  if (historyPtr >= commandHistory.size())
  {
    historyPtr = -1;
    return "";
  }
  return commandHistory[historyPtr];
}

// get next history command
const char *get_next_command()
{
  historyPtr--;
  if (historyPtr < -1)
    historyPtr = commandHistory.size() - 1;

  if (historyPtr < 0)
    return "";

  return commandHistory[historyPtr];
}

const Tab<SimpleString> &get_command_history() { return commandHistory; }

// add command to history
void add_history_command(const char *cmd)
{
  if (!cmd || !*cmd)
    return;

  // remove simular commands
  for (int i = commandHistory.size() - 1; i >= 0; i--)
  {
    if (strcmp(commandHistory[i], cmd) == 0)
    {
      erase_items(commandHistory, i, 1);
    }
  }

  historyPtr = -1;
  insert_items(commandHistory, 0, 1);
  commandHistory[0] = cmd;
  while (commandHistory.size() > MAX_HISTORY_COMMANDS)
  {
    commandHistory.pop_back();
  }
}

const char *top_history_command() { return !commandHistory.empty() ? commandHistory[0].c_str() : nullptr; }

void pop_front_history_command()
{
  if (!commandHistory.empty())
    erase_items(commandHistory, 0, 1);
}

void register_command_as_hook(const char *cmd)
{
  if (!cmd || !*cmd)
    return;

  String value(cmd);
  if (find_value_idx(console_private::commandHooks, value) != -1)
    return;

  console_private::commandHooks.push_back(value);
}

// execute console command
bool command(const char *cmd)
{
  if (!cmd || !*cmd)
    return false;

  auto &procList = console_private::getProcList();
  IMemAlloc *tmpa = framemem_ptr();
  char *s = str_dup(cmd, tmpa);
  Tab<char *> arg(tmpa);
  findArgs(s, arg);

  if (arg.size() < 2) // 2 is because of trailing NULL
    return false;

  debug("CON: exec command '%s'", cmd);
  bool res = false;
  for (int i = 0; i < procList.size(); i++)
  {
    res = procList[i]->processCommand((const char **)&arg[0], arg.size() - 1);
    if (res)
      break;
  }

  if (!res)
    console::warning("unknown command: '%s'", arg[0]);

  cached_prefix.clear();
  cached_commands.clear();

  memfree(s, tmpa);
  return res;
}


// process file
bool process_file(const char *filename)
{
  console::trace_d("Processing commands from %s", filename);
  file_ptr_t h = df_open(filename, DF_READ);
  if (!h)
  {
    console::error("Can't open file");
    return false;
  }
  int l = df_length(h);
  if (l < 0)
  {
    df_close(h);
    console::error("Error reading file");
    return false;
  }

  char *s = (char *)memalloc((l + 1) * sizeof(char), tmpmem);

  if (!s)
  {
    df_close(h);
    console::error("Not enough memory to load file (%dKb)", (l + 1) >> 10);
    return false;
  }
  if (df_read(h, s, l) != l)
  {
    memfree(s, tmpmem);
    df_close(h);
    console::error("Error reading file");
    return false;
  }
  df_close(h);
  for (int i = 0; i < l;)
  {
    for (; i < l; ++i)
      if (s[i] != '\n' && s[i] != '\r' && s[i] != 0x1A)
        break;
    if (i >= l)
      break;
    int j;
    for (j = i; j < l; ++j)
      if (s[j] == '\n' || s[j] == '\r' || s[j] == 0x1A)
        break;
    s[j] = 0;
    command(s + i);
    i = j + 1;
  }
  memfree(s, tmpmem);

  return true;
}


// open console on screen
void show()
{
  clear_and_shrink(cached_prefix);
  conDriver->show();
}


// close console on screen
void hide()
{
  saveConsoleCommands();
  conDriver->hide();
}


// return true, if console is present and visible
bool is_visible() { return conDriver && conDriver->is_visible(); }


// return current console height
real get_cur_height() { return conDriver ? conDriver->get_cur_height() : 0; }

// get list of all commands
void get_command_list(console::CommandList &out_list, const char *arg)
{
  clear_and_shrink(out_list);

  auto &procList = console_private::getProcList();
  ICommandProcessor::cmdCollector = &out_list;
  func_command_processor.find_first = false;
  for (int i = 0; i < procList.size(); i++)
    procList[i]->processCommand(&arg, 1);
  func_command_processor.find_first = true;
  ICommandProcessor::cmdCollector = NULL;
}

void get_command_list(console::CommandList &out_list) { get_command_list(out_list, ""); }

// get command without hook prefix
const char *get_command_hookless(const char *command, int &out_hook_index)
{
  if (!command)
    return nullptr;

  int commandLen = strlen(command);

  for (int i = 0, n = console_private::commandHooks.size(); i < n; ++i)
  {
    const String &commandHook = console_private::commandHooks[i];
    if (commandLen > commandHook.size() && strncmp(command, commandHook.c_str(), commandHook.size() - 1) == 0 &&
        command[commandHook.size() - 1] == ' ')
    {
      out_hook_index = i;
      return command + commandHook.size();
    }
  }

  out_hook_index = -1;
  return command;
}

// get list of commands filtered by prefix
void get_filtered_command_list(console::CommandList &out_commands, const String &prefix)
{
  if (cached_prefix.length() > 0 && cached_prefix == prefix)
  {
    out_commands = cached_commands;
    return;
  }
  cached_prefix = prefix;
  clear_and_shrink(cached_commands);

  int commandHookIndex = -1;
  const char *prefixWithoutCommandHook = get_command_hookless(prefix.c_str(), commandHookIndex);

  console::CommandList list;
  get_command_list(list, prefixWithoutCommandHook);

  eastl::string search(prefixWithoutCommandHook);
  search.make_lower();
  search.erase(eastl::remove(search.begin(), search.end(), '_'), search.end());

  eastl::string exactCmdSearch(search.substr(0, search.find_first_of(" ")));

  int numGroups = 0;
  int prefixMatchInsertPos = 0;
  int anyMatchPos = 0;
  for (int i = 0; i < list.size(); i++)
  {
    if (list[i].command.length() < exactCmdSearch.length())
      continue;

    eastl::string cmd(list[i].command.c_str());
    cmd.make_lower();
    eastl_size_t dotPosWithUnderscore = cmd.find('.');
    cmd.erase(eastl::remove(cmd.begin(), cmd.end(), '_'), cmd.end());
    eastl_size_t dotPos = cmd.find('.');
    eastl_size_t findPos = cmd.find(search);
    eastl_size_t exactCmdPos = cmd.find(exactCmdSearch);

    if (findPos == 0 && dotPosWithUnderscore != eastl::string::npos && search.find('.') == eastl::string::npos)
    {
      // insert a dummy command entry "<group>." so we can autocomplete only on the group
      eastl::string group(list[i].command.c_str(), dotPosWithUnderscore + 1);
      auto it = eastl::find_if(cached_commands.begin(), cached_commands.begin() + numGroups,
        [&group](CommandStruct c) { return group == c.command.c_str(); });
      if (it >= cached_commands.begin() + numGroups)
        insert_item_at(cached_commands, numGroups++, CommandStruct(group.c_str(), 0, 0));
    }
    if (findPos == eastl::string::npos && exactCmdPos == 0)
      cached_commands.push_back(list[i]); // The least suitable commands should be at the end of the list
                                          // (important for custom processors like das console_processor)
    if (findPos == 0 || findPos == dotPos + 1)
      insert_item_at(cached_commands, numGroups + prefixMatchInsertPos++, list[i]);
    else if (findPos != eastl::string::npos)
      insert_item_at(cached_commands, numGroups + prefixMatchInsertPos + anyMatchPos++, list[i]);
    else if (list[i].description.length())
    {
      eastl::string cmdinfo(list[i].description.c_str());
      cmdinfo.make_lower();
      findPos = cmdinfo.find(search);
      if (findPos != eastl::string::npos)
      {
        cached_commands.push_back(list[i]);
      }
    }
  }

  if (commandHookIndex != -1)
  {
    const String &commandHook = console_private::commandHooks[commandHookIndex];
    for (CommandStruct &command : cached_commands)
      command.command = String(0, "%s %s", commandHook.c_str(), command.command);
  }

  const int afterPrefixed = numGroups + prefixMatchInsertPos;
  const int afterAnyMatch = afterPrefixed + anyMatchPos;
  const int numCachedCommands = (int)cached_commands.size();
  eastl::vector<int> order;
  order.resize(numCachedCommands);
  for (int i = 0; i < numCachedCommands; ++i)
    order[i] = i;

  eastl::sort(order.begin(), order.end(), [&](int pos1, int pos2) {
    const int rank1 = (pos1 < numGroups) ? 0 : (pos1 < afterPrefixed) ? 1 : (pos1 < afterAnyMatch) ? 2 : 3;
    const int rank2 = (pos2 < numGroups) ? 0 : (pos2 < afterPrefixed) ? 1 : (pos2 < afterAnyMatch) ? 2 : 3;
    if (rank1 < rank2)
      return true;
    if (rank1 > rank2)
      return false;
    const char *cmd1 = cached_commands[pos1].str();
    const char *cmd2 = cached_commands[pos2].str();
    return strcmp(cmd1, cmd2) < 0;
  });

  clear_and_shrink(out_commands);
  out_commands.resize(numCachedCommands);
  for (int i = 0; i < numCachedCommands; ++i)
    out_commands[i] = cached_commands[order[i]];
  cached_commands = out_commands; // cache sorted
}

bool find_console_command(const char *cmd, CommandStruct &info)
{
  Tab<console::ICommandProcessor *> &procList = console_private::getProcList();
  const char *args[]{cmd, nullptr};

  console::CommandList list;
  clear_and_shrink(list);

  for (int i = 0; i < procList.size(); i++)
  {
    procList[i]->cmdCollector = &list;
    procList[i]->cmdSearchMode = true;
    procList[i]->processCommand(args, 1);
    procList[i]->cmdCollector = nullptr;
    procList[i]->cmdSearchMode = false;
  }

  if (list.size() >= 1)
  {
    info = list.at(0);
    return true;
  }

  return false;
}

// return console speed
real get_con_speed() { return consoleSpeed; }

} // namespace console

//************************************************************************
//* console processors
//************************************************************************
// add new console proc
void add_con_proc(console::ICommandProcessor *proc)
{
  if (!proc)
    return;
  remove_con_proc(proc);

  auto &procList = console_private::getProcList();
  for (int i = 0; i < procList.size(); i++)
  {
    if (procList[i]->getPriority() < proc->getPriority())
    {
      insert_items(procList, i, 1, &proc);
      return;
    }
  }

  procList.push_back(proc);
}

// remove console proc (don't destroy)
bool remove_con_proc(console::ICommandProcessor *proc)
{
  if (!proc)
    return false;

  auto &procList = console_private::getProcList();
  for (int i = 0; i < procList.size(); i++)
  {
    if (proc == procList[i])
    {
      erase_items(procList, i, 1);
      return true;
    }
  }

  return false;
}


// delete console proc (destroy)
bool del_con_proc(console::ICommandProcessor *proc)
{
  if (!remove_con_proc(proc))
    return false;

  proc->destroy();
  return true;
}


// return true, if found
bool find_con_proc(console::ICommandProcessor *proc)
{
  auto &procList = console_private::getProcList();
  for (int i = procList.size() - 1; i >= 0; i--)
    if (procList[i] == proc)
      return true;
  return false;
}

bool run_con_batch(const char *pathname)
{
  if (!pathname || !*pathname)
  {
    logwarn("run_con_batch: not specified");
    return false;
  }

  file_ptr_t f = df_open(pathname, DF_READ | DF_IGNORE_MISSING);
  if (!f)
  {
    logwarn("run_con_batch: not found - %s", pathname);
    return false;
  }
  char buf[512];
  while (df_gets(buf, sizeof(buf), f))
  {
    for (int i = (int)strlen(buf); --i >= 0;)
      if (buf[i] == '\n' || buf[i] == '\r')
        buf[i] = '\0';
    if (strlen(buf) > 1 && buf[0] == '/' && buf[1] == '/')
      continue;
    debug("run_con_batch: [%s]", buf);
    console::command(buf);
  }
  df_close(f);
  return true;
}
