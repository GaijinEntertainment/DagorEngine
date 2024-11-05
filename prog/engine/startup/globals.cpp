// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_tab.h>
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_globDef.h>
#include <util/dag_simpleString.h>
#include <util/dag_bitArray.h>

#include <string.h>
#include <stdio.h>

static Bitarray arg_used;

static const char *arg_value_delimiters = ":=";

static const char *_arg_value(const char *arg)
{
  for (const char *s = arg; *s; s++)
    if (strchr(arg_value_delimiters, *s) != nullptr)
      return s + 1;

  return nullptr;
}


bool dgs_is_arg_used(int arg_index)
{
  return (arg_index > 0 && arg_index < arg_used.size()) ? arg_used[arg_index] : !arg_index; // argv[0] is always 'used'
}


void dgs_set_arg_used(int arg_index, bool used)
{
  if (arg_index > 0 && arg_index < arg_used.size())
    arg_used.set(arg_index, used ? 1 : 0);
}


static const char *_arg_name(const char *arg)
{
  const char *origin = arg; // We must advance only if it's a parameter name

  if (arg[0] == '/' && *++arg)
  { // Ensure this is not a Unix path
    const char *value = _arg_value(arg);
    const char *slash = strchr(arg, '/');
    return ((value && slash && value <= slash) || !slash) ? arg : NULL;
  }

  if (arg[0] == '-' && *++arg)   // Not an stdin shortcut
    if (arg[0] == '-' && *++arg) // Not an argument list terminator
      return arg;

  return (arg != origin && *arg) ? arg : NULL;
}


const char *dgs_get_argv(const char *name, int &it, const char *default_value)
{
#if _TARGET_PC_WIN
#define STRINGS_MATCH !strnicmp
#else
#define STRINGS_MATCH !strncmp
#endif

  for (; it < ::dgs_argc; ++it)
  {
    const char *arg = _arg_name(::dgs_argv[it]);
    size_t len = strlen(name);
    if (arg && STRINGS_MATCH(arg, name, len) && (!arg[len] || strchr(arg_value_delimiters, arg[len]) != nullptr))
    {
      dgs_set_arg_used(it);
      const char *value = _arg_value(arg);
      if (!value)
      {
        ++it; // Advance to check if next argument is a value
        value = (it < ::dgs_argc && !_arg_name(::dgs_argv[it])) ? ::dgs_argv[it] : "";
      }

      ++it; // the iterator should always point after last checked element
      return value;
    }
  }

  return default_value;
#undef STRINGS_MATCH
}


#if _TARGET_PC_LINUX || _TARGET_APPLE
static char *ps_title = NULL;
static size_t ps_title_len = 0;
static size_t ps_title_size = 0;

// Hopefully implementation of these containers will not change:
// we rely on the fact that &SimpleString == &SimpleString.data to
// provide backwards-compatible ::dgs_argv.
static Tab<SimpleString> argv_storage(strmem);
static Tab<SimpleString> env_storage(strmem);

extern char **environ;

void dgs_init_argv(int argc, char **argv)
{
  argv_storage.resize(argc + 1); // argv[argc] should be NULL, hence +1
  char *eob = NULL;
  for (int i = 0; i < argc; ++i)
  {
    argv_storage[i] = argv[i];

    if (i == 0 || eob + 1 == argv[i])
      eob = argv[i] + strlen(argv[i]);
  }
  G_ASSERT(eob);

  ::dgs_argc = argc;
  ::dgs_argv = (char **)argv_storage.data();

  size_t env_count = 0;
  for (int i = 0; environ && environ[i]; ++i, ++env_count)
    if (eob + 1 == environ[i])
      eob = environ[i] + strlen(environ[i]);

  env_storage.resize(env_count + 1); // environ should be NULL-terminated, hence +1
  for (int i = 0; environ && environ[i]; ++i)
    env_storage[i] = environ[i];
  environ = (char **)env_storage.data();

  ps_title = argv[0];
  ps_title_size = eob - ps_title;
  ps_title_len = ps_title_size; // To zero all out later when setting title
  G_UNUSED(ps_title_len);

  for (int i = 1; i < argc; ++i)
    argv[i] = ps_title + ps_title_size;

  arg_used.resize(argc);
}

#else
void dgs_init_argv(int argc, char **argv)
{
  ::dgs_argc = argc;
  ::dgs_argv = argv;
  arg_used.resize(argc);
}
#endif


#if _TARGET_PC_LINUX || _TARGET_PC_MACOSX

void dgs_setproctitle(const char *title)
{
  G_ASSERT(ps_title && ps_title_size);

  size_t len = snprintf(ps_title, ps_title_size, "%s", title);
  if (ps_title_len > len)
    memset(ps_title + len, '\0', ps_title_len - len);
  ps_title_len = strlen(ps_title);
}
const char *dgs_getproctitle() { return ps_title; }

#elif _TARGET_PC_WIN

void dgs_setproctitle(const char *title) { win32_set_window_title(title); }

#endif
