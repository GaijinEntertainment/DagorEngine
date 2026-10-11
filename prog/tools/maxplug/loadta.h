// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <string>
#include <vector>

struct TexAnimFile
{
  std::vector<std::wstring> tex;
  std::vector<int> frm;

  bool load(const wchar_t *fn);
  static std::wstring getlasterr();

private:
  std::string basepath, basefile;

  void setbasefile(const wchar_t *fn);
  void add_frame(const std::string &fname);
  bool parse(char *);
};
