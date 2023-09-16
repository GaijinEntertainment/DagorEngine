#include <libTools/util/strUtil.h>
#include <libTools/util/filePathname.h>
#include <libTools/util/de_TextureName.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

//==============================================================================
String make_path_relative(const char *path, const char *base)
{
  if (!base || !*base || !path || !*path)
    return String(path);

  char relPath[1024];
  String pathStr(path);
  String baseStr(base);

  ::make_ms_slashes(pathStr);
  ::make_ms_slashes(baseStr);

  if (::PathRelativePathToA(relPath, baseStr, GetFileAttributes(baseStr), pathStr, GetFileAttributes(pathStr)))
    return ::make_good_path((*relPath == '\\') ? &relPath[1] : relPath);

  return ::make_good_path(path);
}


//================================================================================
bool FilePathName::makeRelativePath(const char *base)
{
  if (!base || !*base || *this == "")
    return false;

  char relPath[1024];
  String pathStr(*this);
  String baseStr(base);

  // this block need because dump MS's function can't work with '/' slashes
  for (int i = 0; i < pathStr.length(); ++i)
    if (pathStr[i] == '/')
      pathStr[i] = '\\';

  for (int i = 0; i < baseStr.length(); ++i)
    if (baseStr[i] == '/')
      baseStr[i] = '\\';

  if (PathRelativePathToA(relPath, baseStr, GetFileAttributes(baseStr), pathStr, GetFileAttributes(pathStr)))
  {
    *this = ::make_good_path((*relPath == '\\') ? &relPath[1] : relPath);
    return true;
  }

  return false;
}
