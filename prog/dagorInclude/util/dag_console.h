//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/functional.h>

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <util/dag_globDef.h>
#include <util/dag_pull_console_proc.h>

//************************************************************************
//* game console
//************************************************************************
namespace console
{
enum LineType
{
  CONSOLE_ERROR = 0,
  CONSOLE_WARNING,
  CONSOLE_INFO,
  CONSOLE_DEBUG,
  CONSOLE_TRACE,

  CONSOLE_DEFAULT,
};

extern unsigned main_background_color;
extern unsigned tips_background_color;
extern int max_tips;
extern bool print_logerr_to_console;


extern bool to_bool(const char *s);
extern int to_int(const char *s);
extern int to_int_from_hex(const char *s);
extern real to_real(const char *s);
class CommandStruct
{
public:
  SimpleString command, description, argsDescription, varValue;
  int minArgs;
  int maxArgs;

  CommandStruct()
  {
    minArgs = 0;
    maxArgs = 0;
    description = "";
    argsDescription = "";
    varValue = "";
  }

  CommandStruct(const char *c, int minArgs_, int maxArgs_, const char *description_ = "", const char *argsDescription_ = "",
    const char *varValue_ = "")
  {
    command = c;
    minArgs = minArgs_;
    maxArgs = maxArgs_;
    description = description_;
    argsDescription = argsDescription_;
    varValue = varValue_;
  }

  const char *str() const { return command.str(); }
};

typedef Tab<CommandStruct> CommandList;
using ConsoleOutputCallback = eastl::function<void(const char *)>;

//************************************************************************
//* command processor with argument dispatch
//************************************************************************
class ICommandProcessor
{
public:
  ICommandProcessor(int init_priority = 100) : priority(init_priority) {}
  virtual ~ICommandProcessor() {}
  virtual void destroy() = 0; // delete this

  // process command
  virtual bool processCommand(const char *argv[], int argc) = 0;

  // helper routines
  int getPriority() { return priority; }
  static bool to_bool(const char *s) { return console::to_bool(s); }
  static int to_int(const char *s) { return console::to_int(s); }
  static real to_real(const char *s) { return console::to_real(s); }

  static CommandList *cmdCollector;
  static bool cmdSearchMode; // find single command without building list of all commands

protected:
  int priority;
};

void init();
void shutdown();

void register_console_listener(const ConsoleOutputCallback &listener_func);

void con_vprintf(LineType type, bool dup_to_log, const char *f, const DagorSafeArg *arg, int anum);
#define DSA_OVERLOADS_PARAM_DECL

// console::print(fmt...) outputs CONSOLE_DEBUG string to console only
#define DSA_OVERLOADS_PARAM_PASS CONSOLE_DEBUG, false,
DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void print, con_vprintf);
#undef DSA_OVERLOADS_PARAM_PASS

// console::print_d(fmt...) outputs CONSOLE_DEBUG string to console and debug
#define DSA_OVERLOADS_PARAM_PASS CONSOLE_DEBUG, true,
DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void print_d, con_vprintf);
#undef DSA_OVERLOADS_PARAM_PASS

// console::error(fmt...) outputs CONSOLE_ERROR string to console and debug/logerr
#define DSA_OVERLOADS_PARAM_PASS CONSOLE_ERROR, true,
DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void error, con_vprintf);
#undef DSA_OVERLOADS_PARAM_PASS

// console::warning(fmt...) outputs CONSOLE_WARNING string to console and debug/logwarn
#define DSA_OVERLOADS_PARAM_PASS CONSOLE_WARNING, true,
DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void warning, con_vprintf);
#undef DSA_OVERLOADS_PARAM_PASS

// console::trace(fmt...) outputs CONSOLE_TRACE string to console only
#define DSA_OVERLOADS_PARAM_PASS CONSOLE_TRACE, false,
DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void trace, con_vprintf);
#undef DSA_OVERLOADS_PARAM_PASS

// console::trace_d(fmt...) outputs CONSOLE_TRACE string to console only and debug
#define DSA_OVERLOADS_PARAM_PASS CONSOLE_TRACE, true,
DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void trace_d, con_vprintf);
#undef DSA_OVERLOADS_PARAM_PASS

#undef DSA_OVERLOADS_PARAM_DECL

bool command(const char *cmd);
bool process_file(const char *filename);

void show();
void hide();
bool is_visible();

// call reset_important_lines() before script reloading
void reset_important_lines();

// return current console height
real get_cur_height();

// get list of all commands
void get_command_list(console::CommandList &out_list);

// get command without hook prefix
const char *get_command_hookless(const char *command, int &out_hook_index);
inline const char *get_command_hookless(const char *command)
{
  int unused;
  return get_command_hookless(command, unused);
}

// get list of commands filtered by prefix
void get_filtered_command_list(console::CommandList &commands, const String &prefix);

bool find_console_command(const char *cmd, CommandStruct &info);

real get_con_speed();

void add_history_command(const char *cmd);
const char *top_history_command();
void pop_front_history_command();

void register_command_as_hook(const char *cmd);

const char *get_prev_command();
const char *get_next_command();
const Tab<SimpleString> &get_command_history();

//! returns 1 on match, 0 on no-match-continue, -1 on arg count mismatch
int collector_cmp(const char *arg, int ac, const char *cmd, int min_ac, int max_ac, const char *description = "",
  const char *argsDescription = "");

class FuncCommandProcessor;
typedef bool (*console_function_cb)(const char *argv[], int argc);
struct FuncLinkedList
{
  FuncLinkedList(console_function_cb cb) : fun(cb)
  {
    next = tail;
    tail = this;
  }

protected:
  FuncLinkedList *next = NULL; // slist
  console_function_cb fun;
  static FuncLinkedList *tail;
  friend class FuncCommandProcessor;
};
} // namespace console

#define CONSOLE_CHECK_NAME(domain, name, min_args, max_args)                                               \
  if (found > 0)                                                                                           \
    return true;                                                                                           \
  found = console::collector_cmp(argv[0], argc, domain[0] ? (domain "." name) : name, min_args, max_args); \
  if (found == -1)                                                                                         \
    return false;                                                                                          \
  if (found == 1)

#define CONSOLE_CHECK_NAME_EX(domain, name, min_args, max_args, description, argsDescription)                                      \
  if (found > 0)                                                                                                                   \
    return true;                                                                                                                   \
  found =                                                                                                                          \
    console::collector_cmp(argv[0], argc, domain[0] ? (domain "." name) : name, min_args, max_args, description, argsDescription); \
  if (found == -1)                                                                                                                 \
    return false;                                                                                                                  \
  if (found == 1)

//************************************************************************
//* console processors
//************************************************************************
// add new console proc
void add_con_proc(console::ICommandProcessor *proc);

// remove console proc (don't destroy)
bool remove_con_proc(console::ICommandProcessor *proc);

// delete console proc (destroy)
bool del_con_proc(console::ICommandProcessor *proc);

// return true, if found
bool find_con_proc(console::ICommandProcessor *proc);

#define REGISTER_CONSOLE_PULL_VAR_NAME(func) \
  namespace console                          \
  {                                          \
  int DAG_CONSOLE_PULL_VAR_NAME(func) = 1;   \
  }

#define REGISTER_CONSOLE_HANDLER(func) \
  REGISTER_CONSOLE_PULL_VAR_NAME(func) \
  static console::FuncLinkedList DAG_CONSOLE_CC1(console_handler, __LINE__)(func)
