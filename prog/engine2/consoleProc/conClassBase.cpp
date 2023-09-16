#include <util/dag_console.h>
#include <osApiWrappers/dag_localConv.h>
#include <string.h>
#include <stdlib.h>

namespace console
{
//************************************************************************
//* command processor with argument dispatch
//************************************************************************

CommandList *ICommandProcessor::cmdCollector = NULL;
bool ICommandProcessor::cmdSearchMode = false;

int collector_cmp(const char *arg, int ac, const char *cmd, int min_args, int max_args, const char *description,
  const char *argsDescription)
{
  if (!ICommandProcessor::cmdCollector)
  {
    bool match = dd_stricmp(arg, cmd) == 0;
    if (!match)
      return 0;
    if (ac >= min_args && ac <= max_args)
      return 1;
    return -1;
  }

  G_ASSERT(cmd);
  G_ASSERT(strlen(cmd));

  if (!ICommandProcessor::cmdSearchMode || dd_stricmp(arg, cmd) == 0)
  {
    CommandStruct command(cmd, min_args, max_args, description, argsDescription);
    ICommandProcessor::cmdCollector->push_back(command);
    // continue if collecting all commands, stop if trying to find command and command found
    return !ICommandProcessor::cmdSearchMode ? 0 : -1;
  }
  return 0;
}

// helper routines
bool to_bool(const char *s)
{
  if (dd_stricmp(s, "on") == 0 || dd_stricmp(s, "yes") == 0 || dd_stricmp(s, "true") == 0)
    return true;
  else if (dd_stricmp(s, "off") == 0 || dd_stricmp(s, "no") == 0 || dd_stricmp(s, "false") == 0)
    return false;
  return strtol(s, NULL, 0) ? true : false;
}

int to_int_from_hex(const char *s)
{
  if (!s || !*s)
    return 0;
  if (!strncmp(s, "0x", 2))
    s += 2;
  return int(strtoll(s, NULL, 16));
}

int to_int(const char *s) { return strtol(s, NULL, 0); }
real to_real(const char *s) { return strtod(s, NULL); }

unsigned main_background_color = 0xD0102030;
unsigned tips_background_color = 0xC0202020;
int max_tips = 9999;
bool print_logerr_to_console = true;
} // namespace console
