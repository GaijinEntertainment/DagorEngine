// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqmodules/helpers.h>
#include <quirrel/bindQuirrelEx/sqModulesDagor.h>
#include <sqStackChecker.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_basePath.h>
#include <utf8/utf8.h>
#include <memory/dag_framemem.h>
#include <util/dag_strUtil.h>

SqModulesDagorFileAccess sq_modules_dagor_file_access(/* delete_self = */ false);

#if _TARGET_PC
bool SqModulesDagorFileAccess::tryOpenFilesFromRealFS = true; // by default we allow loading from fs on PC.
#elif DAGOR_DBGLEVEL > 0
bool SqModulesDagorFileAccess::tryOpenFilesFromRealFS = false; // by default we disallow loading from fs on non-PC platforms, but allow
                                                               // to toggle
// in other configurations it's always false
#endif

static const char SCRIPT_MODULE_FILE_EXT[] = ".nut";


#ifdef _TARGET_PC
#ifdef _WIN32
#define MAX_PATH_LENGTH _MAX_PATH
static const char *computeAbsolutePath(const char *resolved_fn, char *buffer, size_t size)
{
  return _fullpath(buffer, resolved_fn, size);
}
#else // _WIN32
#define MAX_PATH_LENGTH PATH_MAX
static const char *computeAbsolutePath(const char *resolved_fn, char *buffer, size_t size)
{
  (void)size;
  return realpath(resolved_fn, buffer);
}
#endif // _WIN32
#else  // _TARGET_PC
#define MAX_PATH_LENGTH 1
static const char *computeAbsolutePath(const char *resolved_fn, char *buffer, size_t size)
{
  (void)resolved_fn;
  (void)buffer;
  (void)size;
  return nullptr;
}
#endif // _TARGET_PC


SqModulesDagorFileAccess::SqModulesDagorFileAccess(bool delete_self, SqModulesConfigBits cfg) :
  needToDeleteSelf(delete_self), configBits(cfg)
{}


void SqModulesDagorFileAccess::destroy()
{
  if (needToDeleteSelf)
    delete this;
}


SqModulesDagorFileAccess::string SqModulesDagorFileAccess::makeRelativeFilename(const char *cur_file, const char *requested_fn)
{
  scriptPathBuf.resize(strlen(cur_file) + 2);
  dd_get_fname_location(&scriptPathBuf[0], cur_file);
  dd_append_slash_c(&scriptPathBuf[0]);
  size_t locationLen = strlen(&scriptPathBuf[0]);
  size_t reqFnLen = strlen(requested_fn);
  scriptPathBuf.resize(locationLen + reqFnLen + 1);
  strcpy(&scriptPathBuf[locationLen], requested_fn);
  dd_simplify_fname_c(&scriptPathBuf[0]);
  scriptPathBuf.resize(strlen(&scriptPathBuf[0]) + 1);

  return SqModules::string(scriptPathBuf.data());
}

void SqModulesDagorFileAccess::resolveFileName(const char *requested_fn, const char *running_script, string &res)
{
  res.clear();

  bool isScriptFile = str_ends_with(requested_fn, SCRIPT_MODULE_FILE_EXT, strlen(requested_fn));
  bool modules_with_ext = (configBits & SqModulesConfigBits::PermissiveModuleNames) == SqModulesConfigBits::PermissiveModuleNames;
  bool isMount = (*requested_fn == '%'); // not searching for files from mounts in paths relative to the running script

  // try relative path first
  if (running_script && (isScriptFile || modules_with_ext) && !isMount)
  {
    string scriptPath = makeRelativeFilename(running_script, requested_fn);

    bool exists = dd_check_named_mount_in_path_valid(scriptPath.c_str());
    if (exists)
      exists = tryOpenFilesFromRealFS ? dd_file_exists(scriptPath.c_str()) : vromfs_check_file_exists(scriptPath.c_str());
    if (!isScriptFile && exists && dd_dir_exists(scriptPath.c_str()))
      exists = false;
    if (exists)
    {
      if (const char *real_fn = df_get_abs_fname(scriptPath.c_str()))
        if (real_fn != scriptPath.c_str())
        {
          scriptPath = real_fn;
          real_fn = df_get_abs_fname(scriptPath.c_str()); // do second check for case of mixed abs/rel base pathes
          if (real_fn != scriptPath.c_str())
            scriptPath = real_fn;
        }
      res = std::move(scriptPath);
    }
  }

  if (res.empty() && dd_check_named_mount_in_path_valid(requested_fn))
  {
    scriptPathBuf.resize(strlen(requested_fn) + 1);
    strcpy(&scriptPathBuf[0], requested_fn);
    dd_simplify_fname_c(&scriptPathBuf[0]);
    res.insert(res.end(), scriptPathBuf.begin(), scriptPathBuf.begin() + strlen(&scriptPathBuf[0]));
    scriptPathBuf.clear();

    if (const char *real_fn = df_get_abs_fname(res.c_str()))
      if (real_fn != res.data())
      {
        res = real_fn;
        real_fn = df_get_abs_fname(res.c_str()); // do second check for case of mixed abs/rel base pathes
        if (real_fn != res.data())
          res = real_fn;
      }
  }

  if (useAbsolutePath)
  {
    char buffer[MAX_PATH_LENGTH] = {0};
    const char *real_path = computeAbsolutePath(res.c_str(), buffer, sizeof buffer);
    if (real_path)
      res = real_path;
  }
}


bool SqModulesDagorFileAccess::readFile(const string &resolved_fn, const char *requested_fn, vector<char> &buf, string &out_err_msg)
{
  uint32_t fileFlags = DF_READ | DF_IGNORE_MISSING | (tryOpenFilesFromRealFS ? 0 : DF_VROM_ONLY);
  file_ptr_t f = df_open(resolved_fn.c_str(), fileFlags);
  if (!f)
  {
    out_err_msg = sqm::format_string("Script file %s (%s) not found", requested_fn, resolved_fn.c_str());
    return false;
  }

  int len = df_length(f);
  buf.resize(len + 1);
  if (df_read(f, (void *)buf.data(), len) != len)
  {
    logerr("Failed to read script file %s (%s)", requested_fn, resolved_fn.c_str());
    out_err_msg = sqm::format_string("Failed to read script file %s (%s)", requested_fn, resolved_fn.c_str());
    df_close(f);
    return false;
  }
  buf[len] = 0;
  df_close(f);
  return true;
}


void SqModulesDagorFileAccess::getSearchTargets(const char *fn, bool &search_native, bool &search_script)
{
  search_native = true;
  search_script = true;

  int fnLen = strlen(fn);

  if ((configBits & SqModulesConfigBits::PermissiveModuleNames) == SqModulesConfigBits::None)
  {
    search_script = str_ends_with(fn, SCRIPT_MODULE_FILE_EXT, fnLen);
    search_native = !search_script;
  }
}
