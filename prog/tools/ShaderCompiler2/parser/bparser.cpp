// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_stdint.h>
#include <memory/dag_mem.h>
#include "bparser.h"
// #include <debug/dag_debug.h>
#include <osApiWrappers/dag_direct.h>
#include <stdio.h>

InputFile::IncFile::~IncFile()
{
  if (fn)
    memfree((void *)fn, strmem);
  if (text)
    memfree(text, strmem);
}

void InputFile::IncFile::free_text()
{
  if (text)
  {
    memfree(text, strmem);
    text = NULL;
    size = 0;
  }
}

InputFile::InputFile(BaseLexicalAnalyzer *l) : lexer(l), incfile(tmpmem), incstk(tmpmem)
{
  curfile = -1;
  curpos = 0;
  at_eof = false;
  _keep_all_text = false;
}

void InputFile::stream_set(BaseLexicalAnalyzer &l)
{
  lexer = &l;
  if (lexer)
  {
    lexer->set_cur_file(curfile);
    lexer->set_cur_offset(curpos);
  }
}

const char *InputFile::get_filename(int f)
{
  if (f < 0 || f >= incfile.size())
    return NULL;
  return incfile[f].fn;
}

bool InputFile::is_real_eof() { return curfile < 0 || curfile >= incfile.size(); }

bool InputFile::eof()
{
  if (curfile < 0 || curfile >= incfile.size())
    return true;
  return at_eof;
}

char InputFile::get()
{
  if (curfile < 0 || curfile >= incfile.size())
    return 0;
  if (at_eof)
  {
    at_eof = false;
    int i;
    for (i = 0; i < incstk.size(); ++i)
      if (incstk[i].file == curfile)
        break;
    if (!_keep_all_text)
      if (i >= incstk.size())
        incfile[curfile].free_text();
    if (incstk.empty())
    {
      curfile = -1;
      return 0;
    }
    const FilePos &p = incstk.back();
    curfile = p.file;
    curpos = p.pos;
    if (lexer)
    {
      lexer->set_cur_file(curfile);
      lexer->set_cur_line(p.line);
      lexer->set_cur_column(p.col);
      lexer->set_cur_offset(curpos);
    }
    incstk.pop_back();
  }
  if (curpos >= incfile[curfile].size)
  {
    at_eof = true;
    return 0;
  }
  return incfile[curfile].text[curpos++];
}

int InputFile::find_incfile(const char *fn)
{
  if (!fn)
    return -1;
  for (int i = 0; i < incfile.size(); ++i)
    if (incfile[i].fn)
      if (strcmp(incfile[i].fn, fn) == 0)
        return i;
  return -1;
}

bool InputFile::include(int f)
{
  if (f < 0 || f >= incfile.size())
    return false;
  if (curfile >= 0)
  {
    FilePos fp;
    fp.file = curfile;
    if (lexer)
    {
      fp.line = lexer->get_cur_line();
      fp.col = lexer->get_cur_column();
      curpos -= lexer->buffer_size();
      lexer->clear_buffer();
    }
    else
    {
      fp.line = 1;
      fp.col = 1;
    }
    fp.pos = curpos;
    incstk.push_back(fp);
  }
  curfile = f;
  curpos = 0;
  if (lexer)
  {
    lexer->set_cur_file(curfile);
    lexer->set_cur_line(1);
    lexer->set_cur_column(1);
    lexer->set_cur_offset(curpos);
    lexer->clear_eof();
  }
  return true;
}

bool InputFile::include(const char *text, unsigned sz, const char *fn)
{
  if (!text)
    return false;
  struct AutoErase
  {
    Tab<IncFile> &tab;
    int ind;
    AutoErase(Tab<IncFile> &t) : tab(t) { ind = -1; }
    ~AutoErase()
    {
      if (ind >= 0)
        erase_items(tab, ind, 1);
    }
  } ae(incfile);
  int f = find_incfile(fn);
  if (f < 0)
  {
    ae.ind = f = append_items(incfile, 1);
    if (f < 0)
    {
      err_nomem();
      return false;
    }
  }
  else if (incfile[f].text)
    return include(f);
  incfile[f].text = (char *)memalloc(sz, strmem);
  if (!incfile[f].text)
  {
    err_nomem();
    return false;
  }
  const char *s = text;
  char *d = incfile[f].text;
  for (uint32_t l = sz; l; --l, ++s, ++d)
    *d = ((*s == 0 || *s == 0x1A) ? ' ' : *s);
  incfile[f].size = sz;
  ae.ind = -1;
  if (fn)
  {
    incfile[f].fn = str_dup(fn, strmem);
    if (!incfile[f].fn)
      err_nomem();
  }
  return include(f);
}

int InputFile::get_include_file_index(const char *fn)
{
  struct AutoErase
  {
    Tab<IncFile> &tab;
    int ind;
    AutoErase(Tab<IncFile> &t) : tab(t) { ind = -1; }
    ~AutoErase()
    {
      if (ind >= 0)
        erase_items(tab, ind, 1);
    }
  } ae(incfile);

  char includeFileName[260];
  strcpy(includeFileName, fn);
  dd_simplify_fname_c(includeFileName);
  file_ptr_t fp = df_open(includeFileName, DF_READ);
  if (!fp && curfile >= 0)
  {
    dd_get_fname_location(includeFileName, incfile[curfile].fn);
    dd_append_slash_c(includeFileName);
    strcat(includeFileName, fn);
    dd_simplify_fname_c(includeFileName);
    fp = df_open(includeFileName, DF_READ);
  }
  if (!fp)
  {
    return -1;
  }

  int f = find_incfile(includeFileName);
  if (f < 0)
  {
    ae.ind = f = append_items(incfile, 1);
    if (f < 0)
    {
      err_nomem();
      return false;
    }
  }
  else if (incfile[f].text)
  {
    df_close(fp);
    return f;
  }

  uint32_t sz = ::df_length(fp);
  if (sz == -1)
  {
    err_fileread(includeFileName);
    df_close(fp);
    return -1;
  }
  incfile[f].text = (char *)memalloc(sz, strmem);
  if (!incfile[f].text)
  {
    err_nomem();
    df_close(fp);
    return -1;
  }

  sz = (int)df_read(fp, incfile[f].text, sz);
  incfile[f].text = (char *)strmem->realloc(incfile[f].text, sz);
  df_close(fp);

  // remove /r and replace \0 and EOF with spaces
  char *p = incfile[f].text, *dp = p, *pe = p + sz;
  for (; p < pe; ++p)
    if (*p == 0 || *p == 0x1A)
    {
      *dp = ' ';
      dp++;
    }
    else if (*p != '\r')
    {
      if (p != dp)
        *dp = *p;
      dp++;
    }
  incfile[f].size = sz = dp - incfile[f].text;
  ae.ind = -1;
  incfile[f].fn = str_dup(includeFileName, strmem);
  if (!incfile[f].fn)
    err_nomem();
  return f;
}

bool InputFile::include_alefile(const char *fn)
{
  int f = get_include_file_index(fn);
  return f >= 0 ? include(f) : false;
}

void InputFile::err_nomem() { G_VERIFY(0); }

void InputFile::err_fileopen(const char *fn)
{
  if (lexer)
    lexer->set_error(String("can't open file ") + fn);
}

void InputFile::err_fileread(const char *fn)
{
  if (lexer)
    lexer->set_error(String("error reading file ") + fn);
}
