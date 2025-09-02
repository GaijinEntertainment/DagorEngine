// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_string.h>
#include <stdio.h>

extern OAHashNameMap<true> preproc_defines;
static String exc_text;

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

static void preprocess(char *buf, int &len, const char *target, bool keepLines, const char *fname)
{
  char *pif = buf, *pife;
  char *end = buf + len;

  if (char *p = strstr(pif, "#ifdef"))
  {
    bool violates = false;
    while (pif)
    {
      pif = strstr(p + 6, "#ifdef");
      for (p--; p >= buf; p--)
        if (*p == '\"')
          break;
        else if (*p == '\n')
        {
          violates = true;
          break;
        }
        else if (*p == ' ' || *p == '\t' || *p == '/')
          continue;
        else
          break;
      if (violates)
      {
        exc_text.printf(0, "%s: incorrect directive #ifdef, use #if_def() instead", fname);
        DAGOR_THROW(IGenSave::SaveException(exc_text, 0));
      }
      p = pif;
    }
    pif = buf;
  }

  for (pif = find(pif, end, "#if_"); pif; pif = find(pif, end, "#if_"))
  {
    bool cond_true = false;
    if (strncmp(pif, "#if_target(", 11) == 0)
    {
      erase(pif, end, 11); // strlen("#if_target(")
      pife = find(pif, end, ")");
      if (!pife)
      {
        exc_text.printf(0, "%s: missing ) after #if_target", fname);
        DAGOR_THROW(IGenSave::SaveException(exc_text, 0));
      }
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
      {
        exc_text.printf(0, "%s: missing ) after #if_def", fname);
        DAGOR_THROW(IGenSave::SaveException(exc_text, 0));
      }
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
    {
      exc_text.printf(0, "%s: missing #endif", fname);
      DAGOR_THROW(IGenSave::SaveException(exc_text, 0));
    }
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
static const char *delBlock(const char *str, int at, int length, int &str_len, bool keepLines)
{
  int new_lines = 0;
  if (keepLines)
    for (const char *p = str + at, *pe = p + length; p < pe; p++)
      if (*p == '\n')
        new_lines++;
  char *buf = new (uimem) char[new_lines + str_len - length + 1];
  if (at > 0)
    memcpy(buf, str, at);
  memset(buf + at, '\n', new_lines);
  if (str_len > at + length)
    memcpy(buf + at + new_lines, str + at + length, str_len - length - at);
  buf[str_len - length + new_lines] = '\0';
  str_len -= length - new_lines;
  return buf;
}

static void process_and_copy_file_to_stream(file_ptr_t fp, IGenSave &cwr, int size, const char *targetString, bool keepLines,
  bool strip_comments, const char *fname)
{
  int len = size;
  char *buf = new char[size + 1];
  df_read(fp, buf, len);
  buf[len] = 0;

  preprocess(buf, len, targetString, keepLines, fname);
  size = len;
  if (strip_comments)
  {
    // debug("strip css commments: src_len=%d", len);
    bool strCreated = false;
    const char *lastStr = NULL;
    const char *text = buf;
    int i = 0;
    int cached_strlen = size;
    int quote_open = 0;
    while (text[i] != '\0')
    {
      if (text[i] == '\\' && strchr("\\\"\'", text[i + 1]))
      {
        i += 2; // skip escaped symbol
        continue;
      }
      if (text[i] == '"' && (!quote_open || quote_open == 1))
        quote_open = quote_open ? 0 : 1;
      else if (text[i] == '\'' && (!quote_open || quote_open == 2))
        quote_open = quote_open ? 0 : 2;
      if (quote_open)
      {
        // skip // and /* inside strings
      }
      else if (text[i] == '/')
      {
        if (text[i + 1] == '/')
        {
          lastStr = text;
          int delCnt = getBlockEnd(&text[i + 2], '\n');
          if (delCnt >= 0)
            delCnt += 2 + 1; // include // and \n
          else
            delCnt = cached_strlen - i; // remove all trailing text (as no \n found)
          int mlc = getBlockEnd(&text[i + 2], "/*", 2);
          if ((mlc > 0) && (mlc < delCnt))
          {
            mlc += getBlockEnd(&text[i + 2 + mlc], "*/", 2) + 2;
            if (mlc > delCnt)
              delCnt = mlc;
          }
          text = delBlock(text, i, delCnt, cached_strlen, keepLines);
          if (strCreated)
            delete (lastStr);
          strCreated = true;
          continue;
        }
        else if (text[i + 1] == '*')
        {
          lastStr = text;
          int delCnt = getBlockEnd(&text[i + 2], "*/", 2);
          if (delCnt >= 0)
            delCnt += 2; // include /*
          else
            delCnt = cached_strlen - i; // remove all trailing text (as no */ found)
          text = delBlock(text, i, delCnt, cached_strlen, keepLines);
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
      memcpy(buf, text, len);
    }
    // debug("  -> stripped_len=%d", len);
  }

  cwr.write(buf, len);
  delete[] buf;
}


void process_and_copy_file_to_stream(const char *fname, IGenSave &cwr, const char *targetString, bool keepLines, bool strip_comments)
{
  file_ptr_t fp = df_open(fname, DF_READ);
  if (!fp)
  {
    exc_text.printf(0, "%s: file not found", fname);
    DAGOR_THROW(IGenSave::SaveException(exc_text, 0));
  }
  if (const char *ext = dd_get_fname_ext(fname)) // always strip comments in *.nut, *.das, *.js and *.css
  {
    if (stricmp(ext, ".nut") == 0 || stricmp(ext, ".das") == 0)
      strip_comments = keepLines = true;
    else if (stricmp(ext, ".css") == 0 || stricmp(ext, ".js") == 0)
      strip_comments = true;
  }
  process_and_copy_file_to_stream(fp, cwr, df_length(fp), targetString, keepLines, strip_comments, fname);
  df_close(fp);
}
