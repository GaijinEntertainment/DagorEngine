#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>
#include <memory/dag_framemem.h>
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_memIo.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <perfMon/dag_cpuFreq.h>

#include <quirrel/sqModules/sqModules.h>
#include <squirrel.h>
#include <squirrel/sqpcheader.h>
#include <squirrel/sqvm.h>

#if _TARGET_PC_WIN
#include <Windows.h>

static String path_to_static_analyzer;
static String path_to_sqconfig;
static String drey_args;
static String sa_output;
static bool is_auto_drey_inited = false;
static int drey_call_count = 0;

static Tab<int> depth;
static SQDEBUGHOOK hook_before_module_reload = nullptr;
static FILE *locals_file = nullptr;
static String locals_file_name;
static Tab<SQDEBUGHOOK> prev_hook;

static void open_locals_list_for_write()
{
  G_ASSERT(locals_file == nullptr);
  locals_file_name.setStr(tmpnam(NULL));
  locals_file = fopen(locals_file_name, "wt");
}

static void close_locals_list()
{
  if (locals_file)
  {
    fclose(locals_file);
    locals_file = nullptr;
  }
}

static void remove_locals_list()
{
  G_ASSERT(locals_file == nullptr);
  remove(locals_file_name.str());
}


static String read_file_to_string(const char *file_name)
{
  file_ptr_t h = df_open(file_name, DF_READ | DF_IGNORE_MISSING);
  if (!h)
    return String("");

  int len = df_length(h);

  String res;
  res.resize(len + 1);
  df_read(h, &res[0], len);
  res.back() = '\0';

  df_close(h);
  return res;
}


static void write_sqconfig_to_locals_list()
{
  G_ASSERT(locals_file);

  if (path_to_sqconfig.empty())
    return;

  fprintf(locals_file, "###[SQCONFIG]%s\n", path_to_sqconfig.str());
}


static void system_no_window(const char *cmd)
{
#if _TARGET_PC_WIN
  PROCESS_INFORMATION p_info;
  STARTUPINFO s_info;
  LPSTR programpath;

  memset(&s_info, 0, sizeof(s_info));
  memset(&p_info, 0, sizeof(p_info));
  s_info.cb = sizeof(s_info);

  programpath = strdup(cmd);

  if (CreateProcessA(NULL, programpath, NULL, NULL, 0, CREATE_NO_WINDOW, NULL, NULL, &s_info, &p_info))
  {
    WaitForSingleObject(p_info.hProcess, INFINITE);
    CloseHandle(p_info.hProcess);
    CloseHandle(p_info.hThread);
  }

  free(programpath);
#else
  system(cmd);
#endif
}

static void run_sq3_static_analyzer()
{
  String outputFn = locals_file_name + ".output";
  String command =
    path_to_static_analyzer + " --output-mode:1-line " + drey_args + " --stream:" + locals_file_name + " --output:" + outputFn;

  command.replaceAll("/", "\\");
  command = String("cmd /c \"") + command + "\"";

  system_no_window(command);

  if (!dd_file_exists(outputFn))
    sa_output.printf(0, "ERROR: failed to execute '%s'", command.str());
  else
    sa_output = read_file_to_string(outputFn);
  remove(outputFn);
  drey_call_count++;
}


static void print_sq_table(HSQUIRRELVM v, const String &prefix, int max_depth)
{
  HSQOBJECT container;
  sq_getstackobj(v, -1, &container);
  sq_pushobject(v, container);
  sq_pushnull(v);
  while (SQ_SUCCEEDED(sq_next(v, -2)))
  {
    const SQChar *key = nullptr;
    SQInteger sz = 0;
    sq_getstringandsize(v, -2, &key, &sz);
    HSQOBJECT value;
    sq_getstackobj(v, -1, &value);
    sq_pop(v, 2);
    fprintf(locals_file, "%s%s\n", prefix.str(), key);
    if (max_depth > 0)
    {
      if (value._type == OT_TABLE || value._type == OT_CLASS)
      {
        sq_pushobject(v, value);
        String pre(32, "%s.", key);
        print_sq_table(v, pre, max_depth - 1);
        sq_pop(v, 1);
      }
    }
  }
  sq_pop(v, 2);
}

static void sq3_sa_debug_hook(HSQUIRRELVM v, SQInteger type, const SQChar *sourcename, SQInteger line, const SQChar *funcname)
{
  G_UNUSED(sourcename);
  G_UNUSED(line);
  G_UNUSED(funcname);
  G_UNUSED(v);

  switch (type)
  {
    case 'c':
    {
      int &d = depth.back();
      d++;
    }
    break;

    case 'r':
    {
      int &d = depth.back();
      if (d == 1 && locals_file)
      {
        fprintf(locals_file, "###[FILE_NAME]%s\n", sourcename);
        fprintf(locals_file, "###[LOCALS]\n");
        d--;

        const SQChar *name = nullptr;
        int seq = 0;
        for (;;)
        {
          name = sq_getlocal(v, 0, seq);
          if (!name)
            break;
          fprintf(locals_file, "%s\n", name);

          int objType = sq_gettype(v, -1);
          if (objType == OT_TABLE || objType == OT_CLASS)
          {
            HSQOBJECT container;
            sq_getstackobj(v, -1, &container);
            sq_pushobject(v, container);
            sq_pushnull(v);
            while (SQ_SUCCEEDED(sq_next(v, -2)))
            {
              const SQChar *key = nullptr;
              SQInteger sz = 0;
              sq_getstringandsize(v, -2, &key, &sz);
              sq_pop(v, 2);
              fprintf(locals_file, "%s.%s\n", name, key);
            }
            sq_pop(v, 2);
          }

          seq++;
          sq_pop(v, 1);
        }

        fprintf(locals_file, "###[GLOBALS]\n");
        sq_pushroottable(v);
        print_sq_table(v, String(""), 1);
        sq_pop(v, 1);

        fprintf(locals_file, "###[CONSTANTS]\n");
        sq_pushconsttable(v);
        print_sq_table(v, String(""), 1);
        sq_pop(v, 1);

        fprintf(locals_file, "###[CODE]\n");
        file_ptr_t f = df_open(sourcename, DF_READ);
        if (f)
        {
          Tab<char> buf;
          int len = df_length(f);
          buf.resize(len + 1);
          G_VERIFY(df_read(f, (void *)buf.data(), len) == len);
          buf[len] = 0;
          df_close(f);
          fprintf(locals_file, "%s\n", buf.data());
        }
      }
    }
    break;

    default: break;
  }

  if (hook_before_module_reload)
    hook_before_module_reload(v, type, sourcename, line, funcname);
}


static void auto_drey_before_execute_module(HSQUIRRELVM vm, const char *, const char *)
{
  prev_hook.push_back(vm->_debughook_native);
  if (vm->_debughook_native != sq3_sa_debug_hook)
    hook_before_module_reload = vm->_debughook_native;
  sq_setnativedebughook(vm, sq3_sa_debug_hook);
  depth.push_back(0);
}

static void auto_drey_after_execute_module(HSQUIRRELVM vm, const char *, const char *)
{
  depth.pop_back();
  sq_setnativedebughook(vm, prev_hook.back());
  prev_hook.pop_back();
}


////////////////////////////////////////////////////////////////////////////

namespace sqdrey
{

void init(const char *path_to_static_analyzer_, const char *drey_args_, const char *path_to_sqconfig_)
{
  debug("sqdrey init: path to static analyzer = '%s'", path_to_static_analyzer_);
  G_ASSERT(path_to_static_analyzer_ && path_to_static_analyzer_[0]);
  path_to_static_analyzer.setStr(path_to_static_analyzer_);
  drey_args.setStr(drey_args_ ? drey_args_ : "");
  path_to_sqconfig.setStr(path_to_sqconfig_ ? path_to_sqconfig_ : "");
  is_auto_drey_inited = true;
}

void before_reload_scripts()
{
  if (!is_auto_drey_inited)
    return;

  debug("sqdrey::before_reload_scripts()");
  SqModules::setModuleExecutionCallbacks(auto_drey_before_execute_module, auto_drey_after_execute_module);

  open_locals_list_for_write();
  write_sqconfig_to_locals_list();
}

void after_reload_scripts(bool output_to_logerr)
{
  if (!is_auto_drey_inited)
    return;

  debug("sqdrey::after_reload_scripts()");
  SqModules::setModuleExecutionCallbacks(nullptr, nullptr);

  close_locals_list();
  run_sq3_static_analyzer();
  remove_locals_list();


  if (output_to_logerr && sa_output.length() > 4)
  {
    logerr("Drey output:");
    const char *ptr = sa_output.str();
    do
    {
      const char *nextPtr = strchr(ptr, '\n');
      int crlf = (nextPtr > sa_output.str() && *(nextPtr - 1) == '\r') ? 1 : 0;
      SimpleString line = nextPtr ? SimpleString(ptr, nextPtr - ptr - crlf) : SimpleString(ptr);
      ptr = nextPtr;
      if (ptr)
        ptr++;
      if ((ptr && ptr[0]) || line.str()[0])
        logerr("%s", line.str());
    } while (ptr);
  }
}

String get_drey_result() { return sa_output; }

int get_drey_call_count() { return drey_call_count; }
} // namespace sqdrey

#else

namespace sqdrey
{
void init(const char *, const char *, const char *) { debug("WARNING: sqdrey is disabled for this platform."); }

void before_reload_scripts() {}
void after_reload_scripts(bool) {}

String get_drey_result() { return String(""); }

int get_drey_call_count() { return 0; }

} // namespace sqdrey

#endif
