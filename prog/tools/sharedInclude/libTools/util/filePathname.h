//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_string.h>

//================================================================================
//  filepathname-suit
//================================================================================
class FilePathName : public String
{
public:
  FilePathName() {}
  FilePathName(const char *str);
  FilePathName(const String &s);
  FilePathName(const FilePathName &str) : String(str) {}
  FilePathName(const char *path, const char *name, const char *ext = NULL);

  FilePathName &operator=(const char *s);
  FilePathName &operator=(const String &s);
  inline FilePathName &operator=(const FilePathName &s)
  {
    String::operator=(s);
    return *this;
  }

  FilePathName getPath() const;
  FilePathName getName(bool with_ext = true) const;
  FilePathName getExt() const;

  FilePathName &setPath(const char *new_path);
  FilePathName &setName(const char *new_name, bool with_ext = true);
  FilePathName &setExt(const char *new_ext);

  bool makeRelativePath(const char *base_path); // cut base path
  bool makeFullPath(const char *base_path);     // add base path
  FilePathName &makeIdent(bool name_and_ext_only = true, char mark = '_');

  //------------------------------------------------------------------------
  inline bool operator==(const char *s) const { return stricmp((const char *)*this, s) == 0; }
  inline bool operator!=(const char *s) const { return stricmp((const char *)*this, s) != 0; }
  inline bool operator==(const String &s) const { return stricmp((const char *)*this, (const char *)s) == 0; }
  inline bool operator!=(const String &s) const { return stricmp((const char *)*this, (const char *)s) != 0; }
  inline bool operator==(const FilePathName &s) const { return stricmp((const char *)*this, (const char *)s) == 0; }
  inline bool operator!=(const FilePathName &s) const { return stricmp((const char *)*this, (const char *)s) != 0; }
};
