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
  void add_frame(char *);
  bool parse(char *);
};
