// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_oaHashNameMap.h>
#include <stdio.h>

extern OAHashNameMap<true> preproc_defines;

static char *find(char *begin, char *end, const char *sub)
{
  size_t sublen = strlen(sub);
  for (char *ptr = begin; ptr <= end - sublen; ptr++)
  {
    int i = 0;
    for (i = 0; i < sublen; i++)
      if (ptr[i] != sub[i])
        break;
    if (i >= sublen)
      return ptr;
  }
  return NULL;
}
static char *findchar(char *begin, char *end, const char *set, int &out_n)
{
  for (; begin < end; begin++)
  {
    const char *p = strchr(set, *begin);
    if (p)
    {
      out_n = p - set;
      return begin;
    }
  }
  return NULL;
}

static void erase(char *begin, char *&end, int cnt, bool keep = false)
{
  if (keep)
  {
    int lc = 0;
    for (char *ptr = begin; (ptr != end) && (ptr < begin + cnt); ptr++)
      if (*ptr == '\n')
        lc++;
    if (lc)
    {
      memset(begin, '\n', lc);
      begin += lc;
      cnt -= lc;
    }
  }
  memmove(begin, begin + cnt, (end - begin) - cnt);
  end -= cnt;
}


static bool find_else_endif(char *begin, char *end, char *&out_else, char *&out_endif)
{
  out_else = NULL;
  out_endif = NULL;
  for (;;)
  {
    char *pif = find(begin, end, "#if_");
    char *pelse = find(begin, end, "#else");
    char *pendif = find(begin, end, "#endif");
    if (!pendif)
      return false;

    if (pif && strncmp(pif, "#if_target(", 11) != 0 && strncmp(pif, "#if_def(", 8) != 0)
    {
      begin = pif + 4;
      continue;
    }

    if ((pelse && pif > pelse) || pif > pendif)
      pif = NULL;

    if (pif)
    {
      char *pife = find(pif, end, ")");
      if (!pife)
        return false;
      if (!find_else_endif(pife + 1, end, out_else, out_endif))
        return false;
      begin = out_endif + 6;
      continue;
    }
    if (!pelse || pendif < pelse)
    {
      out_else = NULL;
      out_endif = pendif;
      return true;
    }
    out_else = pelse;
    begin = pelse + 5;
    break;
  }

  for (;;)
  {
    char *pif = find(begin, end, "#if_");
    char *pendif = find(begin, end, "#endif");
    if (!pendif)
      return false;

    if (pif && strncmp(pif, "#if_target(", 11) != 0 && strncmp(pif, "#if_def(", 8) != 0)
    {
      begin = pif + 4;
      continue;
    }

    if (pif > pendif)
      pif = NULL;

    if (pif)
    {
      char *pife = find(pif, end, ")");
      char *pelse;
      if (!pife)
        return false;
      if (!find_else_endif(pife + 1, end, pelse, out_endif))
        return false;
      begin = out_endif + 6;
      continue;
    }
    out_endif = pendif;
    return true;
  }
}

static void preprocess(char *buf, int &len, const char *target, bool keepLines)
{
  char *pif = buf, *pife;
  char *end = buf + len;

  for (pif = find(pif, end, "#if_"); pif; pif = find(pif, end, "#if_"))
  {
    bool cond_true = false;
    if (strncmp(pif, "#if_target(", 11) == 0)
    {
      erase(pif, end, 11); // strlen("#if_target(")
      pife = find(pif, end, ")");
      if (!pife)
        DAGOR_THROW(IGenSave::SaveException("file bad - missing ) after #if_target", 0));
      *pife = '\0';

      char *pifnext = find(pif, pife, "|");
      if (pifnext)
        *pifnext = '\0';

      cond_true = (strcmp(pif, target) == 0);
      while (pifnext)
      {
        char *p = pifnext + 1;
        pifnext = find(p, pife, "|");
        if (pifnext)
          *pifnext = '\0';
        if (strcmp(p, target) == 0)
          cond_true = true;
      }
    }
    else if (strncmp(pif, "#if_def(", 8) == 0)
    {
      erase(pif, end, 8); // strlen("#if_def(")
      pife = find(pif, end, ")");
      if (!pife)
        DAGOR_THROW(IGenSave::SaveException("file bad - missing ) after #if_def", 0));
      *pife = '\0';

      int c0 = 0, op = 0;
      char *pifnext = findchar(pif, pife, "|&", op);
      if (pifnext)
        *pifnext = '\0';

      int c1 = (preproc_defines.getNameId(pif) >= 0);
      while (pifnext)
      {
        char *p = pifnext + 1;
        int op2 = 0;
        pifnext = findchar(p, pife, "|&", op2);
        if (pifnext)
          *pifnext = '\0';
        if (op == 0)
        {
          // |, or
          c0 += c1;
          c1 = (preproc_defines.getNameId(p) >= 0);
        }
        else
        {
          // &, and
          c1 *= (preproc_defines.getNameId(p) >= 0);
        }
        op = op2;
      }
      cond_true = (c0 + c1) != 0;
    }
    else
    {
      pif += 4;
      continue;
    }

    erase(pif, end, (pife - pif) + 1);

    char *pendif, *pelse;
    if (!find_else_endif(pif, end, pelse, pendif))
      DAGOR_THROW(IGenSave::SaveException("file bad - missing #endif", 0));
    erase(pendif, end, 6); // strlen("#endif")

    if (!pelse)
    {
      if (!cond_true)
        erase(pif, end, pendif - pif, keepLines);
    }
    else
    {
      if (cond_true)
        erase(pelse, end, pendif - pelse, keepLines);
      else
        erase(pif, end, pelse + 5 - pif, keepLines);
    }
  }
  len = end - buf;
}

static int getBlockEnd(const char *str, const char block_chr)
{
  if (block_chr == '\0')
    return i_strlen(str);

  int i = 0;
  while (str[i] != '\0')
  {
    if (str[i] == block_chr)
      return i;
    i++;
  }
  return -1;
}
static int getBlockEnd(const char *str, const char *block_str, int block_str_len)
{
  int i = 0;
  while (str[i] != '\0')
  {
    if (strncmp(&str[i], block_str, block_str_len) == 0)
      return i + block_str_len;
    i++;
  }
  return -1;
}
static const char *delBlock(const char *str, int at, int length, int str_len)
{
  char *buf = new (uimem) char[str_len - length + 1];
  _snprintf(buf, str_len - length + 1, "%.*s%s", at, str, &str[at + length]);
  return buf;
}

static void process_and_copy_file_to_stream(file_ptr_t fp, IGenSave &cwr, int size, const char *targetString, bool keepLines,
  bool strip_css_comments)
{
  int len = size;
  char *buf = new char[size + 1];
  df_read(fp, buf, len);
  buf[len] = 0;

  if (strip_css_comments)
  {
    // debug("strip css commments: src_len=%d", len);
    bool strCreated = false;
    const char *lastStr = NULL;
    const char *css_text = buf;
    int i = 0;
    int cached_strlen = i_strlen(css_text);
    while (css_text[i] != '\0')
    {
      if (css_text[i] == '/')
      {
        if (css_text[i + 1] == '/')
        {
          lastStr = css_text;
          int delCnt = getBlockEnd(&css_text[i + 2], '\n') + 2 + 1;
          int mlc = getBlockEnd(&css_text[i + 2], "/*", 2);
          if ((mlc > 0) && (mlc < delCnt))
          {
            mlc += getBlockEnd(&css_text[i + 2 + mlc], "*/", 2) + 2;
            if (mlc > delCnt)
              delCnt = mlc;
          }
          css_text = delBlock(css_text, i, delCnt, cached_strlen);
          cached_strlen -= delCnt;
          if (strCreated)
            delete (lastStr);
          strCreated = true;
          continue;
        }
        else if (css_text[i + 1] == '*')
        {
          lastStr = css_text;
          int delCnt = getBlockEnd(&css_text[i + 2], "*/", 2) + 2;
          css_text = delBlock(css_text, i, delCnt, cached_strlen);
          cached_strlen -= delCnt;
          if (strCreated)
            delete (lastStr);
          strCreated = true;
          continue;
        }
      }
      i++;
    }
    if (cached_strlen < size)
    {
      size = len = cached_strlen;
      memcpy(buf, css_text, len);
    }
    // debug("  -> stripped_len=%d", len);
  }
  preprocess(buf, len, targetString, keepLines);

  cwr.write(buf, len);
  delete[] buf;
}


void process_and_copy_file_to_stream(const char *fname, IGenSave &cwr, const char *targetString, bool keepLines)
{
  file_ptr_t fp = df_open(fname, DF_READ);
  if (!fp)
    DAGOR_THROW(IGenSave::SaveException("file not found", 0));
  process_and_copy_file_to_stream(fp, cwr, df_length(fp), targetString, keepLines, stricmp(dd_get_fname_ext(fname), ".css") == 0);
  df_close(fp);
}
