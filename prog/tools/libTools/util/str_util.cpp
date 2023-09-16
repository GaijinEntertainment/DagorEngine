#include <libTools/util/strUtil.h>
#include <libTools/util/filePathname.h>

#include <util/dag_string.h>
#include <osApiWrappers/dag_direct.h>

#include <debug/dag_debug.h>

#include <ctype.h>
#include <stdlib.h>


//==============================================================================
bool is_full_path(const char *path)
{
  if (!path || !*path)
    return false;

  if (isalpha(*path) && path[1] == ':')
    return true;

  if (*path == '\\' && path[1] == '\\')
    return true;

  return false;
}


//==============================================================================
void split_path(const char *path, String &location, String &filename)
{
  if (!path)
    path = "";

  int i;
  for (i = i_strlen(path) - 1; i >= 0; --i)
    if (path[i] == '/' || path[i] == '\\' || path[i] == ':')
      break;

  if (i < 0)
    i = 0;

  location.printf(0, "%.*s", i, path);

  if (path[i] == '/' || path[i] == '\\' || path[i] == ':')
    ++i;

  filename = path + i;
}

//==============================================================================
bool find_in_base(const char *path, const char *base, String &new_path)
{
  new_path = "";

  if (!path || !*path || !base || !*base)
    return false;

  const int pathLen = i_strlen(path) - 1;

  for (int i = pathLen; i >= 0; --i)
  {
    switch (path[i])
    {
      case '/':
      case '\\':
      {
        if (i == pathLen)
          break;

        const String newPath = ::make_full_path(base, path + i + 1);

        if (::dd_file_exist(newPath))
        {
          new_path = newPath;
          return true;
        }

        break;
      }

      case ':': return false;
    }
  }

  const String newPath = ::make_full_path(base, path);
  if (::dd_file_exist(newPath))
  {
    new_path = newPath;
    return true;
  }

  return false;
}


//==============================================================================
String find_in_base_smart(const char *path, const char *base, const char *def_path)
{
  if (!path || !*path)
    return String(def_path);

  if (!base || !*base)
    return String(def_path);

  const bool isFullPath = ::is_full_path(path);

  if (isFullPath && ::dd_file_exist(path))
    return String(path);

  String ret;
  if (!isFullPath)
  {
    ret = ::make_full_path(base, path);
    if (::dd_file_exist(ret))
      return ret;
  }

  if (!::find_in_base(path, base, ret))
    ret = def_path;

  return ret;
}

//==============================================================================
const char *get_file_ext(const char *path)
{
  if (::is_empty_string(path))
    return NULL;

  for (const char *p = path + strlen(path) - 1; p >= path; --p)
    switch (*p)
    {
      case '.': return p + 1;
      case '\\':
      case '/':
      case ':': return NULL;
    }

  return NULL;
}

void append_slash(String &s)
{
  int l = s.length();
  if (l <= 0)
    return;
  if (s[l - 1] != '\\' && s[l - 1] != '/')
    s.append("/");
}

//==============================================================================
String add_dot_slash(const char *path)
{
  if (!*path)
    return String();
  String ret(path);

  for (int i = 0; i < ret.length(); ++i)
    if (ret[i] == '\\' || ret[i] == '/')
      return ret;

  ret.insert(0, "./", 2);
  return ret;
}


//==============================================================================
String make_good_path(const char *path)
{
  if (!path || !*path)
    return String("");

  String ret(path);
  ::simplify_fname(ret);

  return ::add_dot_slash(ret);
}

//==============================================================================
const char *get_file_name(const char *path)
{
  if (::is_empty_string(path))
    return NULL;

  for (const char *p = path + strlen(path) - 1; p >= path; --p)
    if (*p == '\\' || *p == '/')
      return p + 1;

  return path;
}


//==============================================================================
String get_file_name_wo_ext(const char *path)
{
  if (::is_empty_string(path))
    return String("");

  const char *fname = ::get_file_name(path);
  const char *ext = ::get_file_ext(fname);
  String ret = String(fname);

  if (ext)
  {
    const int len = ext - fname;
    ret.resize(len);
    ret[len - 1] = 0;
  }

  return ret;
}


//==============================================================================
String make_full_path(const char *dir, const char *fname)
{
  if ((!fname || !*fname) && (!dir || !*dir))
    return String("");

  if (!fname || !*fname)
  {
    String ret = String(dir);
    ::append_slash(ret);
    return ret;
  }

  if (is_full_path(fname) || !dir || !*dir)
    return ::make_good_path(fname);

  const char *mask = "%s/%s";
  const char dirEnd = dir[strlen(dir) - 1];
  if (dirEnd == '\\' || dirEnd == '/')
    mask = "%s%s";

  return ::make_good_path(String(1024, mask, dir, fname));
}


//==============================================================================
void location_from_path(String &full_path)
{
  simplify_fname(full_path);
  char *p = strrchr(full_path, '/');
  if (!p)
    full_path = "";
  else
    erase_items(full_path, p - &full_path[0] + 1, full_path.length() - (p - &full_path[0]) - 1);
}


//==============================================================================
const char *remove_end_zeros(char *s)
{
  if (!s || !*s || !strchr(s, '.') || strchr(s, 'e') || strchr(s, 'E'))
    return s;

  char *ends = s + strlen(s) - 1;

  while (ends >= s && (*ends == '0' || *ends == '.'))
  {
    if (*ends != '.')
    {
      *ends = 0;
      --ends;
    }
    else
    {
      *ends = 0;
      return s;
    }
  }

  return s;
}


//==============================================================================
String bytes_to_kb(uint64_t bytes) { return String(32, "%.2f Kb", bytes / 1024.0); }


//==============================================================================
String bytes_to_mb(uint64_t bytes) { return String(32, "%.2f Mb", bytes / (1024.0 * 1024.0)); }


//==============================================================================
bool trail_strcmp(const char *str, const char *trail_str)
{
  int s_len = i_strlen(str);
  int t_len = i_strlen(trail_str);
  if (s_len < t_len)
    return false;
  return strcmp(str + (s_len - t_len), trail_str) == 0;
}


//==============================================================================
bool trail_stricmp(const char *str, const char *trail_str)
{
  int s_len = i_strlen(str);
  int t_len = i_strlen(trail_str);
  if (s_len < t_len)
    return false;
  return stricmp(str + (s_len - t_len), trail_str) == 0;
}


//==============================================================================
void remove_trailing_string(String &target, const char *trail_str)
{
  int s_len = i_strlen(target);
  int t_len = i_strlen(trail_str);
  if (s_len < t_len || stricmp(target.c_str() + (s_len - t_len), trail_str) != 0)
    return;
  erase_items(target, s_len - t_len, t_len);
}


//==============================================================================
void make_next_numbered_name(String &name, int num_digits)
{
  int i;
  for (i = name.length() - 1; i >= 0; --i)
    if (name[i] < '0' || name[i] > '9')
      break;
  ++i;

  int num = 0;

  if (i < name.length())
  {
    num = strtoul(&name[i], NULL, 10);
    erase_items(name, i, name.length() - i);
  }

  name.aprintf(32, "%0*d", num_digits, num + 1);
}


void make_ident_like_name(String &name)
{
  for (int i = name.length() - 1; i >= 0; --i)
  {
    char c = name[i];
    if (c == '_')
      continue;
    if (c >= 'A' && c <= 'Z')
      continue;
    if (c >= 'a' && c <= 'z')
      continue;
    if (c >= '0' && c <= '9')
      continue;
    name[i] = '_';
  }
}


//==============================================================================
void ensure_trailing_string(String &target, const char *suffix)
{
  if (::trail_stricmp(target, suffix))
    return;
  target += suffix;
}


//==============================================================================
void cvt_pathname_to_valid_filename(String &inout_name)
{
  for (int j = inout_name.length() - 1; j >= 0; --j)
    if (inout_name[j] == '/' || inout_name[j] == '\\' || inout_name[j] == ':')
      inout_name[j] = '_';
}


//==============================================================================
void string_to_words(const String &str, Tab<String> &list)
{
  clear_and_shrink(list);

  String s;

  for (const char *p = str; *p; p++)
  {
    char c = *p;

    if (c > ' ')
    {
      char cc[2] = {c, 0};
      s.append(cc);
    }
    else
    {
      if (s.size())
      {
        list.push_back(s);
        clear_and_shrink(s);
      }
    }
  }

  if (s.size())
    list.push_back(s);
}


//==================================================================================================
String unify_path_from_win32_dlg(const char *path, const char *def_ext)
{
  String ret(path);
  String extStr(*def_ext != '.' ? def_ext : def_ext + 1);

  Tab<String> ext(tmpmem);

  while (char *curExt = strrchr(extStr, '.')) //-V1044
  {
    ext.push_back() = curExt + 1;
    *curExt = 0;
  }

  ext.push_back(extStr);

  for (int i = 0; i < ext.size(); ++i)
  {
    char *fext = (char *)::get_file_ext(ret);

    while (fext && !stricmp(fext, ext[i]))
    {
      *(fext - 1) = 0;
      fext = (char *)::get_file_ext(ret);
    }
  }

  return String(512, (*def_ext != '.') ? "%s.%s" : "%s%s", (const char *)ret, def_ext);
}


//==================================================================================================
void trim(String &str)
{
  int strLen = str.length();

  for (; strLen && ::isspace(str[strLen - 1]); --strLen)
    ;

  str.resize(strLen + 1);
  str[strLen] = 0;

  const char *lastSpace = NULL;
  for (lastSpace = str.begin(); *lastSpace && ::isspace(*lastSpace); ++lastSpace)
    ;

  int erase = lastSpace - str.begin();
  if (erase)
    erase_items(str, 0, erase);
}


//==================================================================================================
const char *trim(char *str)
{
  if (!str || !*str)
    return str;

  for (char *c = str + strlen(str) - 1; c >= str && ::isspace(*c); --c)
    *c = '\0';

  for (; *str && ::isspace(*str); ++str)
    ;

  return str;
}
