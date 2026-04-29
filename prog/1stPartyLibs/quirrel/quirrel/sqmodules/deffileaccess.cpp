#include "sqmodules.h"
#include "path.h"
#include "helpers.h"

#ifdef _WIN32
#  include <io.h>
#else
#  include <unistd.h>
#  include <limits.h>
#  define _access access
#endif


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

void DefSqModulesFileAccess::destroy()
{
    if (needToDeleteSelf)
        delete this;
}


SqModules::string DefSqModulesFileAccess::makeRelativeFilename(const char *cur_file, const char *requested_fn)
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

void DefSqModulesFileAccess::resolveFileName(const char *requested_fn, const char *running_script, string &res)
{
  res.clear();

  // try relative path first
  if (running_script)
  {
    string scriptPath = makeRelativeFilename(running_script, requested_fn);
    bool exists = _access(scriptPath.c_str(), 0) == 0;
    if (exists)
      res = std::move(scriptPath);
  }

  if (res.empty())
  {
    scriptPathBuf.resize(strlen(requested_fn) + 1);
    strcpy(&scriptPathBuf[0], requested_fn);
    dd_simplify_fname_c(&scriptPathBuf[0]);
    res.insert(res.end(), scriptPathBuf.begin(), scriptPathBuf.begin() + strlen(&scriptPathBuf[0]));
    scriptPathBuf.clear();
  }

  if (useAbsolutePath)
  {
    char buffer[MAX_PATH_LENGTH] = {0};
    const char *real_path = computeAbsolutePath(res.c_str(), buffer, sizeof buffer);
    if (real_path)
      res = real_path;
  }
}

bool DefSqModulesFileAccess::readFile(const string &resolved_fn, const char *requested_fn, vector<char> &buf,
    string &out_err_msg)
{
  FILE* f = fopen(resolved_fn.c_str(), "rb");
  if (!f)
  {
    out_err_msg = sqm::format_string("Script file %s (%s) not found", requested_fn, resolved_fn.c_str());
    return false;
  }

#ifdef _WIN32
  long len = _filelength(_fileno(f));
#else
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  if (len < 0)
  {
    out_err_msg = sqm::format_string("Failed to read script file %s", resolved_fn.c_str());
    fclose(f);
    return false;
  }

  fseek(f, 0, SEEK_SET);
#endif

  buf.resize(len + 1);
  if (fread((void *)buf.data(), 1, len, f) != len)
  {
    out_err_msg = sqm::format_string("Failed to read script file %s (%s)", requested_fn, resolved_fn.c_str());
    fclose(f);
    return false;
  }

  buf[len] = 0;
  fclose(f);

  return true;
}
