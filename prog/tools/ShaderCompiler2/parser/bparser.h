// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <memory/dag_mem.h>
#include "blexpars.h"

class InputFile : public InputStream
{
public:
  struct IncFile
  {
    const char *fn = nullptr;
    char *text = nullptr;
    unsigned size = 0;
    IncFile() = default;
    IncFile(IncFile &&a) : fn(a.fn), text(a.text), size(a.size)
    {
      a.fn = nullptr;
      a.text = nullptr;
      a.size = 0;
    }
    IncFile(const IncFile &a) : fn(str_dup(a.fn, strmem)), text(str_dup(a.text, strmem)), size(a.size) {}
    ~IncFile();
    IncFile &operator=(IncFile &&a)
    {
      eastl::swap(fn, a.fn);
      eastl::swap(text, a.text);
      eastl::swap(size, a.size);
      return *this;
    }
    void free_text();
  };
  struct FilePos
  {
    int file, line, col, pos;
  };
  BaseLexicalAnalyzer *lexer;
  Tab<IncFile> incfile;
  Tab<FilePos> incstk;
  int curfile;
  unsigned curpos;
  bool at_eof, _keep_all_text;

  InputFile(BaseLexicalAnalyzer *l = nullptr);
  bool is_real_eof() override;
  bool eof() override;
  char get() override;
  void stream_set(BaseLexicalAnalyzer &) override;
  const char *get_filename(int) override;
  int find_incfile(const char *filename);
  bool include(int);
  bool include(const char *text, unsigned textsize, const char *fn = NULL);
  bool include_alefile(const char *filename);
  int get_include_file_index(const char *fn);
  void err_nomem();
  void err_fileopen(const char *fn);
  void err_fileread(const char *fn);
};
DAG_DECLARE_RELOCATABLE(InputFile::IncFile);

enum
{
  DIAG_SYNTAX_ERROR,
  DIAG_ERROR,
  DIAG_WARNING,
  DIAG_USER,
  DIAG_INFO,
};
