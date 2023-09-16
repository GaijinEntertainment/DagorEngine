//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <libTools/util/strUtil.h>
#include <libTools/util/filePathname.h>
#include <libTools/dtx/filename.h>

class ILogWriter;

//================================================================================
//  dagored texture pathname-suit
//================================================================================
class DETextureName : public FilePathName
{
public:
  inline DETextureName() {}
  inline DETextureName(const char *str) : FilePathName(str) {}
  inline DETextureName(const String &s) : FilePathName(s) {}
  inline DETextureName(const FilePathName &str) : FilePathName(str) {}
  inline DETextureName(const char *path, const char *name, const char *ext = NULL) : FilePathName(path, name, ext) {}

  inline DETextureName &operator=(const char *s)
  {
    FilePathName::operator=(s);
    return *this;
  }
  inline DETextureName &operator=(const String &s)
  {
    FilePathName::operator=(s);
    return *this;
  }
  inline DETextureName &operator=(const FilePathName &s)
  {
    FilePathName::operator=(s);
    return *this;
  }

  inline DETextureName &setExt(const char *new_ext)
  {
    FilePathName::setExt(new_ext);
    return *this;
  }

  // find texture (if base_path==NULL in locations list)
  virtual bool findExisting(const char *base_path = NULL);

  // cut base path (if NULL, use locations list)
  bool makeRelativePath(const char *base_path = NULL);

  static void setLogWriter(ILogWriter *log_writer);
};
