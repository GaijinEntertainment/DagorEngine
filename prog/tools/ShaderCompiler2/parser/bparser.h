#ifndef __DAGOR_BPARSER_H
#define __DAGOR_BPARSER_H
#pragma once

#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <memory/dag_mem.h>
#include "blexpars.h"

class ParserFileInput : public ParserInputStream
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
  BaseLexicalAnalyzer *parser;
  Tab<IncFile> incfile;
  Tab<FilePos> incstk;
  int curfile;
  unsigned curpos;
  bool at_eof, _keep_all_text;

  ParserFileInput(BaseLexicalAnalyzer *p = NULL);
  virtual ~ParserFileInput();
  bool is_real_eof();
  bool eof();
  char get();
  void stream_set(BaseLexicalAnalyzer &);
  const char *get_filename(int);
  virtual int find_incfile(const char *filename);
  virtual bool include(int);
  virtual bool include(const char *text, unsigned textsize, const char *fn = NULL);
  virtual bool include_alefile(const char *filename);
  virtual int get_include_file_index(const char *fn);
  virtual void err_nomem();
  virtual void err_fileopen(const char *fn);
  virtual void err_fileread(const char *fn);
};
DAG_DECLARE_RELOCATABLE(ParserFileInput::IncFile);

enum
{
  DIAG_SYNTAX_ERROR,
  DIAG_ERROR,
  DIAG_WARNING,
  DIAG_USER,
};

#endif // __DAGOR_BPARSER_H
