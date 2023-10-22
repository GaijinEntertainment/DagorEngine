#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <util/dag_globDef.h>
#include <string.h>

extern "C" void dd_simplify_fname_c(char *s)
{
  if (!s)
    return;
  int i, len = i_strlen(s);

  // check for URL format to prevent removal of ://
  if (char *semi = (char *)memchr(s, ':', len > 8 ? 8 : len))
    if (len > (semi + 3 - s) && semi[1] == '/' && semi[2] == '/')
    {
      len -= int(semi + 3 - s);
      s = semi + 3;
    }
  if (*s == '%') // skip named mount prefix if any
  {
    if (char *p = strchr(s + 1, '/'))
    {
      len -= int(p + 1 - s);
      s = p + 1;
    }
    else
      return;
  }

  for (;;)
  {
    bool ok = false;
    // remove leading spaces
    for (i = 0; i < len; ++i)
      if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
        break;
    if (i > 0)
    {
      len -= i;
      memmove(s, s + i, len + 1);
      ok = true;
    }
    // remove trailing spaces
    for (i = len - 1; i >= 0; --i)
      if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
        break;
    if (i < len - 1)
    {
      s[len = i + 1] = 0;
      ok = true;
    }
    // remove quotes
    for (i = 0; i < len; ++i)
      if (s[i] == '"')
        break;
    if (i < len)
    {
      int qs = i;
      for (++i; i < len; ++i)
        if (s[i] == '"')
          break;
      if (i < len)
      {
        memmove(s + qs, s + qs + 1, len - qs);
        --len;
        --i;
        memmove(s + i, s + i + 1, len - i);
        --len;
        ok = true;
      }
    }
    if (!ok)
      break;
  }

  // replace all back slashes with normal ones
  for (i = 1; i < len; ++i)
    if (s[i] == PATH_DELIM_BACK)
      s[i] = PATH_DELIM;

  // remove extra slashes
  for (i = 1; i < len; ++i)
    if (s[i] == PATH_DELIM)
      if (s[i - 1] == PATH_DELIM)
      {
        memmove(s + i, s + i + 1, len - i);
        --len;
        --i;
      }

  // remove extra cur-dirs
  while (len > 1)
    if (s[0] == '.' && s[1] == PATH_DELIM)
    {
      len -= 2;
      memmove(s, s + 2, len + 1);
    }
    else
      break;

  for (i = 1; i < len; ++i)
    if (s[i] == '.' && (s[i - 1] == PATH_DELIM || s[i - 1] == ':'))
      if (s[i + 1] == PATH_DELIM || s[i + 1] == 0)
      {
        int e = 1;
        if (s[i + 1] == PATH_DELIM)
          ++e;
        len -= e;
        memmove(s + i, s + i + e, len + 1 - i);
        --i;
      }

  // remove extra up-dirs
  for (i = 2; i < len; ++i)
    if (s[i - 2] == PATH_DELIM && s[i - 1] == '.' && s[i] == '.')
      if (s[i + 1] == PATH_DELIM || s[i + 1] == 0)
      {
        int j;
        for (j = i - 3; j >= 0; --j)
          if (s[j] == PATH_DELIM || s[j] == ':')
            break;

        if (++j < i - 2)
        {
          int e = i + 1 - j;

          if (e != 5 || s[j] != '.' || s[j + 1] != '.')
          {
            if (s[i + 1] == PATH_DELIM)
              ++e;

            len -= e;
            memmove(s + j, s + j + e, len + 1 - j);
            i = max(j, 1);
          }
        }
      }
}

extern "C" bool dd_fname_equal(const char *fn1, const char *fn2)
{
  if (!fn1 && !fn2)
    return true;
  if (!fn1 || !fn2)
    return false;

  char s1[DAGOR_MAX_PATH], s2[DAGOR_MAX_PATH];
  strncpy(s1, fn1, DAGOR_MAX_PATH - 1);
  s1[DAGOR_MAX_PATH - 1] = '\0';
  strncpy(s2, fn2, DAGOR_MAX_PATH - 1);
  s2[DAGOR_MAX_PATH - 1] = '\0';

  dd_simplify_fname_c(s1);
  dd_simplify_fname_c(s2);

  return dd_stricmp(s1, s2) == 0;
}


extern "C" void dd_append_slash_c(char *fn)
{
  if (!fn)
    return;
  int l = i_strlen(fn);
  if (l > 0)
    if (fn[l - 1] != PATH_DELIM_BACK && fn[l - 1] != PATH_DELIM)
    {
      fn[l] = PATH_DELIM;
      fn[l + 1] = 0;
    }
}

#define EXPORT_PULL dll_pull_osapiwrappers_simplifyFname
#include <supp/exportPull.h>
