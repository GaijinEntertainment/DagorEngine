#include <libTools/util/filePathname.h>
#include <libTools/util/strUtil.h>

//================================================================================
//  file-pathname-wrapper
//================================================================================
FilePathName::FilePathName(const char *str) : String(str) { ::simplify_fname(*this); }


//================================================================================
FilePathName::FilePathName(const String &s) : String(s) { ::simplify_fname(*this); }


//================================================================================
FilePathName::FilePathName(const char *path, const char *name, const char *ext) : String(::make_full_path(path, name))
{
  if (ext)
  {
    if (*ext != '.' && *end(1) != '.')
      *this += '.';
    *this += ext;
  }
  ::simplify_fname(*this);
}


//================================================================================
FilePathName &FilePathName::operator=(const char *s)
{
  String::operator=(s);
  ::simplify_fname(*this);
  return *this;
}


//================================================================================
FilePathName &FilePathName::operator=(const String &s)
{
  String::operator=(s);
  ::simplify_fname(*this);
  return *this;
}


//================================================================================
FilePathName FilePathName::getPath() const
{
  for (const char *p = end(); p >= begin(); p--)
    if (*p == '/' || *p == '\\')
      return String::mk_sub_str(begin(), p);
  return "";
}


//================================================================================
FilePathName FilePathName::getName(bool with_ext) const
{
  if (with_ext)
    return ::get_file_name(*this);

  return ::get_file_name_wo_ext(*this);
}


//================================================================================
FilePathName FilePathName::getExt() const { return ::get_file_ext(*this); }


//================================================================================
FilePathName &FilePathName::setPath(const char *new_path) { return *this = ::make_full_path(new_path, ::get_file_name(*this)); }


//================================================================================
FilePathName &FilePathName::setName(const char *new_name, bool with_ext)
{
  String loc, name;
  ::split_path(*this, loc, name);
  if (with_ext)
    String::operator=(::make_full_path(loc, new_name));
  else
  {
    String str(260, "%s.%s", new_name, get_file_ext(name));
    String::operator=(::make_full_path(loc, str));
  }
  return *this;
}


//================================================================================
FilePathName &FilePathName::setExt(const char *new_ext)
{
  const char *p = end();
  while (*p != '.')
    if (p == begin())
    {
      p = end();
      break;
    }
    else
      p--;

  if (*new_ext == '.')
    new_ext++;
  setStr(String(int(p - begin() + strlen(new_ext) + 2), "%.*s.%s", p - begin(), begin(), new_ext));
  return *this;
}


//================================================================================
bool FilePathName::makeFullPath(const char *base_path)
{
  *this = ::make_full_path(base_path, *this);
  return true;
}


//================================================================================
FilePathName &FilePathName::makeIdent(bool name_and_ext_only, char mark)
{
  char *p = begin();
  if (name_and_ext_only)
  {
    p += length();
    while (p > begin() && *p != '/' && *p != '\\')
      p--;
  }

  for (; *p; p++)
  {
    bool skip = (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '.';
    if (!skip)
      *p = mark;
  }
  return *this;
}
