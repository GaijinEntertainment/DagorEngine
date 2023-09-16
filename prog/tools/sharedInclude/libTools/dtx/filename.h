#ifndef __GAIJIN_DDS_TEXTURE_NAME_H__
#define __GAIJIN_DDS_TEXTURE_NAME_H__
#pragma once

#include <libTools/util/strUtil.h>
#include <libTools/util/filePathname.h>

//================================================================================
//  DDS pathname-suit
//================================================================================
class DDSPathName : public FilePathName
{
public:
  inline DDSPathName() {}
  inline DDSPathName(const char *str) : FilePathName(str) {}
  inline DDSPathName(const String &s) : FilePathName(s) {}
  inline DDSPathName(const FilePathName &str) : FilePathName(str) {}
  inline DDSPathName(const char *path, const char *name, const char *ext = NULL) : FilePathName(path, name, ext) {}

  inline DDSPathName &operator=(const char *s)
  {
    FilePathName::operator=(s);
    return *this;
  }
  inline DDSPathName &operator=(const String &s)
  {
    FilePathName::operator=(s);
    return *this;
  }
  inline DDSPathName &operator=(const FilePathName &s)
  {
    FilePathName::operator=(s);
    return *this;
  }

  virtual DDSPathName findExisting(const char *base_path = NULL) const;
  virtual DDSPathName &simplify();
};

#endif // __GAIJIN_DDS_TEXTURE_NAME_H__
