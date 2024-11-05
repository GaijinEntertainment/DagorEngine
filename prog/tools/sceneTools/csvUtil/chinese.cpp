// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_unicode.h>

extern unsigned short read_utf8(const char *&ptr);
extern void write_utf8(unsigned short val, char *&ptr);

const char *cant_break_before =
  u8"\r\n\t !%),.:;>?]}¢¨°·ˇˉ―‖’”„‟†‡›℃∶、。〃〆〈《「『〕〗〞︵︹︽︿﹃﹘﹚﹜！＂％＇），．：；？］｀｜｝～";
const char *cant_break_after = u8"\r\n\t $(*,£¥·‘“〈《「『【〔〖〝﹗﹙﹛＄（．［｛￡￥";

#define IS_LOWER(x)   ((x) < 0x100)
#define IS_CHINESE(x) (((x) >= 0x4E00) && ((x) <= 9FFF))

static bool can_break(unsigned short first, unsigned short second)
{
  if (first == '\t' || first == L'\u200B')
    return false;
  if (second == '\t' || second == L'\u200B')
    return false;

  if (IS_LOWER(first) && IS_LOWER(second))
    return false;

  const char *ptr = cant_break_after;
  for (int i = 0; i < utf8_strlen(cant_break_after); i++)
  {
    if (first == read_utf8(ptr))
      return false;
  }

  ptr = cant_break_before;
  for (int i = 0; i < utf8_strlen(cant_break_before); i++)
  {
    if (second == read_utf8(ptr))
      return false;
  }


  return true;
}

const char *process_chinese_string(const char *str, wchar_t sep_char)
{
  if (!str || !*str)
    return "";

  static char *out = NULL;
  static int out_len = 0;

  int len = (int)strlen(str);
  if (out_len < len * 6)
  {
    if (out)
      delete[] out;
    out_len = len * 6;
    out = new char[out_len];
  }

  //--- PROCESS HERE ---------
  int wlen = utf8_strlen(str);
  if (wlen < 2)
  {
    strcpy(out, str);
    return out;
  }
  const char *src = str;
  char *dst = out;
  unsigned first = read_utf8(src);
  unsigned second = 0;
  for (int i = 0; i < wlen - 1; i++)
  {
    second = read_utf8(src);
    if ((first == '\t' || first == L'\u200B') && (second == '\t' || second == L'\u200B'))
      continue;
    write_utf8(first, dst);
    if (can_break(first, second))
      write_utf8(sep_char, dst);
    first = second;
  }
  write_utf8(second, dst);
  *dst = 0;
  //--- PROCESS HERE ---------

  return out;
}
