// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <stdio.h>
#include "loadta.h"

TexAnimFile::~TexAnimFile() { clear(); }

void TexAnimFile::clear()
{
  frm.ZeroCount();
  for (int i = 0; i < tex.Count(); ++i)
    if (tex[i])
      free(tex[i]);
  tex.ZeroCount();
}

void _simplify_filename(char *s)
{
  if (!s)
    return;
  int i, len = (int)strlen(s);
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
  // remove extra slashes
  for (i = 1; i < len; ++i)
    if (s[i] == '\\' || s[i] == '/')
      if (s[i - 1] == '\\' || s[i - 1] == '/')
      {
        memmove(s + i, s + i + 1, len - i);
        --len;
        --i;
      }
  // make all slashes '\'
  for (i = 0; i < len; ++i)
    if (s[i] == '/')
      s[i] = '\\';
  // remove extra cur-dirs
  if (len > 0)
  {
    if (s[0] == '.')
    {
      int e = 1;
      if (s[1] == '\\')
        ++e;
      len -= e;
      memmove(s, s + e, len + 1);
    }
  }
  for (i = 1; i < len; ++i)
    if (s[i] == '.' && (s[i - 1] == '\\' || s[i - 1] == ':'))
      if (s[i + 1] == '\\' || s[i + 1] == 0)
      {
        int e = 1;
        if (s[i + 1] == '\\')
          ++e;
        len -= e;
        memmove(s + i, s + i + e, len + 1 - i);
        --i;
      }
  // remove extra up-dirs
  for (i = 2; i < len; ++i)
    if (s[i - 2] == '\\' && s[i - 1] == '.' && s[i] == '.')
      if (s[i + 1] == '\\' || s[i + 1] == 0)
      {
        int j;
        for (j = i - 3; j >= 0; --j)
          if (s[j] == '\\' || s[j] == ':')
            break;
        if (++j < i - 2)
        {
          int e = i + 1 - j;
          if (s[i + 1] == '\\')
            ++e;
          len -= e;
          memmove(s + j, s + j + e, len + 1 - j);
          i = j;
        }
      }
}

void _append_slash(char *fn)
{
  if (!fn)
    return;
  int l = (int)strlen(fn);
  if (l > 0)
    if (fn[l - 1] != '\\' && fn[l - 1] != '/')
    {
      fn[l] = '\\';
      fn[l + 1] = 0;
    }
}

const char *get_extension(const char *fn)
{
  if (!fn)
    return fn;
  for (int i = (int)strlen(fn) - 1; i >= 0; --i)
    if (fn[i] == '.')
      return fn + i;
    else if (fn[i] == '\\' || fn[i] == '/' || fn[i] == ':')
      break;
  return NULL;
}

static char basepath[256], basefile[256];

static void setbasefile(const char *fn)
{
  int len = (int)strlen(fn);
  int i;
  for (i = len - 1; i >= 0; --i)
    if (fn[i] == '\\' || fn[i] == '/' || fn[i] == ':')
      break;
  ++i;
  memcpy(basepath, fn, i);
  basepath[i] = 0;
  _append_slash(basepath);
  int fl = len - i;
  const char *p = get_extension(fn + i);
  if (p)
    fl = (p - fn) - i;
  memcpy(basefile, fn + i, fl);
  basefile[fl] = 0;
}

void TexAnimFile::add_frame(char *fname)
{
  char fn[256];
  strcpy(fn, basepath);
  strcat(fn, fname);
  _simplify_filename(fn);
  int i;
  for (i = 0; i < tex.Count(); ++i)
    if (stricmp(tex[i], fn) == 0)
      break;
  if (i >= tex.Count())
  {
    char *s = strdup(fn);
    tex.Append(1, &s);
  }
  frm.Append(1, &i);
}

struct StopParser
{};

static char *errmsg;
static int errln, errcol;

char *TexAnimFile::getlasterr()
{
  static char buf[256];
  sprintf(buf, "(line %d, col %d): %s", errln, errcol, errmsg);
  return buf;
}

static void error(char *msg)
{
  errmsg = msg;
  throw StopParser();
}

static void skipcomm(char *&text, bool skipnl)
{
  for (; *text;)
  {
    if (text[0] == '/' && text[1] == '/')
    {
      // single-line comment
      for (text += 2; *text; ++text)
        if (*text == '\r' || *text == '\n')
          break;
      if (text[0] == '\r' && text[1] == '\n')
        text += 2;
      else if (*text)
        ++text;
      continue;
    }
    if (text[0] == '/' && text[1] == '*')
    {
      // comment
      for (text += 2; *text; ++text)
        if (text[0] == '*' && text[1] == '/')
        {
          text += 2;
          break;
        }
      continue;
    }
    if (*text == ' ' || *text == '\t')
    {
      ++text;
      continue;
    }
    if (skipnl)
    {
      if (*text == '\n')
      {
        ++text;
        continue;
      }
      if (*text == '\r')
      {
        if (text[1] == '\n')
          ++text;
        ++text;
        continue;
      }
    }
    break;
  }
}

/*
static bool skipident(char* &text) {
  if((*text>='a' && *text<='z') || (*text>='A' && *text<='Z') || *text=='_') {
    ++text;
    while(*text>='a' && *text<='z') || (*text>='A' && *text<='Z')
    || (*text>='0' && *text<='9') || *text=='_') ++text;
    return true;
  }else return false;
}
*/

static bool parse_int(char *&text, int *val)
{
  if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
  {
    text += 2;
    char *p = text;
    int v = strtoul(p, &text, 16);
    if (p == text)
    {
      text -= 2;
      return false;
    }
    if (val)
      *val = v;
    return true;
  }
  else if (*text >= '0' && *text <= '9')
  {
    char *p = text;
    int v = strtoul(p, &text, 10);
    if (p == text)
      return false;
    if (val)
      *val = v;
    return true;
  }
  return false;
}

bool TexAnimFile::parse(char *text)
{
  char *tstart = text;
  try
  {
    for (; *text;)
    {
      skipcomm(text, true);
      if (!*text)
        break;
      if (*text == '+')
      {
        ++text;
        for (; *text;)
        {
          skipcomm(text, false);
          if (!*text)
            break;
          if (*text == '\r' || *text == '\n')
            break;
          ++text;
        }
      }
      else if (*text == '@')
      {
        ++text;
        skipcomm(text, true);
        if (*text != '"')
          error("expected \"");
        char templ[256];
        char *p = templ;
        for (++text; *text; ++text)
        {
          if (*text == '"')
            break;
          if (*text == '\r' || *text == '\n')
            break;
          if (*text == '~')
          {
            ++text;
            if (*text == 'r')
              *p++ = '\r';
            else if (*text == 'n')
              *p++ = '\n';
            else if (*text == 't')
              *p++ = '\t';
            else
              *p++ = *text;
            continue;
          }
          *p++ = *text;
        }
        *p = 0;
        if (*text != '"')
          error("unclosed string");
        ++text;
        for (;;)
        {
          skipcomm(text, true);
          if (!*text)
            break;
          if (*text == ';')
          {
            ++text;
            break;
          }
          int f1, f2, fs = 1;
          if (!parse_int(text, &f1))
            error("expected integer number");
          f2 = f1;
          skipcomm(text, true);
          if (*text == '-')
          {
            ++text;
            skipcomm(text, true);
            if (!parse_int(text, &f2))
              error("expected integer number");
            if (f2 < f1)
              fs = -1;
          }
          else if (*text == ',')
          {
            ++text;
            skipcomm(text, true);
            if (!parse_int(text, &fs))
              error("expected integer number");
            fs = fs - f1;
            if (fs == 0)
              error("frame step can't be zero");
            skipcomm(text, true);
            if (*text != '-')
              error("expected -");
            ++text;
            skipcomm(text, true);
            if (!parse_int(text, &f2))
              error("expected integer number");
            if ((f2 - f1) * fs < 0)
              error("invalid frame step");
          }
          else if (*text == '*')
          {
            ++text;
            skipcomm(text, true);
            int rep;
            if (!parse_int(text, &rep))
              error("expected integer number");
            char fn[256];
            sprintf(fn, templ, f1);
            for (; rep > 0; --rep)
              add_frame(fn);
            continue;
          }
          else {}
          for (int f = f1;; f += fs)
          {
            if (fs > 0)
            {
              if (f > f2)
                break;
            }
            else
            {
              if (f < f2)
                break;
            }
            char fn[256];
            sprintf(fn, templ, f);
            add_frame(fn);
          }
        }
      }
      else
      {
        char fn[256];
        char *p = text;
        if (*p == '!')
        {
          ++p;
          strcpy(fn, basefile);
        }
        else
        {
          fn[0] = 0;
        }
        for (; *text; ++text)
          if (*text == '\n' || *text == '\r')
            break;
        if (text != p || fn[0])
        {
          if (text != p)
            strncat(fn, p, text - p);
          add_frame(fn);
        }
      }
    }
  }
  catch (StopParser)
  {
    int pos = text - tstart;
    char *p = tstart;
    int ln = 1, col = 1;
    for (; *p && p - tstart < pos;)
    {
      if (p[0] == '\r' && p[1] == '\n')
      {
        p += 2;
        ++ln;
        col = 1;
        continue;
      }
      if (*p == '\r' || *p == '\n')
      {
        ++p;
        ++ln;
        col = 1;
        continue;
      }
      if (*p == '\t')
      {
        ++p;
        col = ((col - 1) / 8 + 1) * 8 + 1;
        continue;
      }
      ++p;
      ++col;
    }
    errln = ln;
    errcol = col;
    return false;
  }
  return true;
}

bool TexAnimFile::load(const char *fn)
{
  errln = 0;
  errcol = 0;
  errmsg = "Error reading file";
  FILE *h = fopen(fn, "rb");
  if (!h)
    return false;
  fseek(h, 0, SEEK_END);
  int l = ftell(h);
  if (l < 0)
  {
    fclose(h);
    return false;
  }
  fseek(h, 0, SEEK_SET);
  char *text = (char *)malloc(l + 1);
  if (!text)
  {
    fclose(h);
    return false;
  }
  if (fread(text, l, 1, h) != 1)
  {
    free(text);
    fclose(h);
    return false;
  }
  for (int i = 0; i < l; ++i)
    if (!text[i])
      text[i] = ' ';
  text[l] = 0;
  fclose(h);
  setbasefile(fn);
  bool res = parse(text);
  free(text);
  return res;
}
