#pragma once

#include <startup/dag_globalSettings.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>

static Tab<String> fatalMessages(tmpmem);
static Tab<bool (*)(const char *, const char *, const char *, int)> prevHandlers(tmpmem);

static bool fatalHandler(const char *msg, const char *stack_msg, const char * /*file*/, int /*line*/)
{
  String message(256, "%s\n%s\n", msg, stack_msg);

  for (int i = 0; i < fatalMessages.size(); ++i)
    if (!stricmp(message, fatalMessages[i]))
      return false;

  fatalMessages.push_back(message);

  DAGOR_THROW(777);
  return false;
}

static void install_fatal_handler()
{
  fatalMessages.clear();
  prevHandlers.push_back(::dgs_fatal_handler);
  ::dgs_fatal_handler = &fatalHandler;
}
static void remove_fatal_handler()
{
  ::dgs_fatal_handler = prevHandlers.back();
  prevHandlers.pop_back();
}
static void mark_there_were_fatals()
{
  if (!fatalMessages.size())
    fatalMessages.push_back();
}
static bool were_there_fatals() { return fatalMessages.size(); }
