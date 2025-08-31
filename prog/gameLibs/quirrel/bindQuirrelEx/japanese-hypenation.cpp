// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hypenation.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_unicode.h>

static const unsigned short should_break_after[] = {
  0x3000,
  0x3001,
  0x3002,
  0x3009,
  0x300B,
  0x300D,
  0x300F,
  0x3011,
  0x3015,
  0x3017,
  0x3019,
  0x301b,
  0xFF01,
  0xFF09,
  0xFF1A,
};

static const unsigned short cannot_be_at_start[] = {0x0029, 0x005D, 0xFF5D, 0x3015, 0x3009, 0x300B, 0x300D, 0x300F, 0x3011, 0x3019,
  0x3017, 0x301F, 0x2019, 0x201D, 0xFF60, 0x00BB, 0x30FD, 0x30FE, 0x30FC, 0x30A1, 0x30A3, 0x30A5, 0x30A7, 0x30A9, 0x30C3, 0x30E3,
  0x30E5, 0x30E7, 0x30EE, 0x30F5, 0x30F6, 0x3041, 0x3043, 0x3045, 0x3047, 0x3049, 0x3063, 0x3083, 0x3085, 0x3087, 0x308E, 0x3095,
  0x3096, 0x31F0, 0x31F1, 0x31F2, 0x31F3, 0x31F4, 0x31F5, 0x31F6, 0x31F7, 0x31F8, 0x31F9, 0x31FA, 0x31FB, 0x31FC, 0x31FD, 0x31FE,
  0x31FF, 0x3005, 0x303B, 0x2010, 0x30A0, 0x2013, 0x301C, 0x003F, 0x0021, 0x203C, 0x2047, 0x2048, 0x2049, 0x30FB, 0x3001, 0x003A,
  0x003B, 0x002C, 0x3002, 0x002E};

static const unsigned short cannot_be_at_end[] = {
  0x0028, 0x005B, 0xFF5B, 0x3014, 0x3008, 0x300A, 0x300C, 0x300E, 0x3010, 0x3018, 0x3016, 0x301D, 0x2018, 0x201C, 0xFF5F, 0x00AB};

#define IS_HIRAGANA(x)  (((x) >= 0x3040) && ((x) <= 0x309F))
#define IS_KATAKANA(x)  (((x) >= 0x30A0) && ((x) <= 0x30FF))
#define IS_KANJI(x)     (((x) >= 0x4E00) && ((x) <= 0x9FAF))
#define IS_NUMBER_1(x)  (((x) >= '0') && ((x) <= '9'))
#define IS_NUMBER_2(x)  (((x) >= 0xFF10) && ((x) <= 0xFF19))
#define IS_WESTERN_1(x) (((x) >= 'A') && ((x) <= 'Z'))
#define IS_WESTERN_2(x) (((x) >= 0xFF21) && ((x) <= 0xFF3A))
#define IS_WESTERN_3(x) (((x) >= 'a') && ((x) <= 'z'))
#define IS_WESTERN_4(x) (((x) >= 0xFF41) && ((x) <= 0xFF5A))
#define IS_NUMBER(x)    (IS_NUMBER_1(x) || IS_NUMBER_2(x))
#define IS_WESTERN(x)   (IS_WESTERN_1(x) || IS_WESTERN_2(x) || IS_WESTERN_3(x) || IS_WESTERN_4(x))


static bool can_break(unsigned int first, unsigned int second)
{
  if (first == '\t' || first == L'\u200B')
    return false;
  if (second == '\t' || second == L'\u200B')
    return false;
  for (int i = 0; i < sizeof(cannot_be_at_end) / sizeof(cannot_be_at_end[0]); i++)
    if (first == cannot_be_at_end[i])
      return false;
  for (int i = 0; i < sizeof(cannot_be_at_start) / sizeof(cannot_be_at_start[0]); i++)
    if (second == cannot_be_at_start[i])
      return false;
  for (int i = 0; i < sizeof(should_break_after) / sizeof(should_break_after[0]); i++)
    if (first == should_break_after[i])
      return true;

  if ((IS_NUMBER(first) || IS_WESTERN(first)) && (IS_NUMBER(second) || IS_WESTERN(second)))
    return false;

  if (IS_NUMBER(first) && (IS_HIRAGANA(second) || IS_KATAKANA(second) || IS_WESTERN(second)))
    return true;
  if (IS_HIRAGANA(first) && (IS_KANJI(second) || IS_KATAKANA(second) || IS_WESTERN(second) || IS_NUMBER(second)))
    return true;
  if (IS_KATAKANA(first) && (IS_KANJI(second) || IS_WESTERN(second) || IS_NUMBER(second)))
    return true;
  if (IS_KANJI(first) && (IS_KATAKANA(second) || IS_WESTERN(second) || IS_NUMBER(second)))
    return true;
  if (IS_WESTERN(first) && (IS_KATAKANA(second) || IS_KANJI(second) || IS_NUMBER(second)))
    return true;

  return false;
}

unsigned int read_utf8(const dag_char8_t *&ptr)
{
  unsigned int ret = 0;
  unsigned char c = *ptr++;

  if ((c & 0x80) == 0)
    ret = c;
  else if ((c & 0xE0) == 0xC0)
  {
    unsigned char c2 = *ptr++;
    ret = ((c & 0x1F) << 6) + (c2 & 0x3F);
  }
  else if ((c & 0xF0) == 0xE0)
  {
    unsigned char c2 = *ptr++;
    unsigned char c3 = *ptr++;
    ret = ((c & 0x0F) << 12) + ((c2 & 0x3F) << 6) + (c3 & 0x3F);
  }
  else
  {
    unsigned char c2 = *ptr++;
    unsigned char c3 = *ptr++;
    unsigned char c4 = *ptr++;
    ret = ((c & 0x07) << 18) + ((c2 & 0x3F) << 12) + ((c3 & 0x3F) << 6) + (c4 & 0x3F);
  }
  return ret;
}

void write_utf8(unsigned int val, char *&ptr)
{
  if (val < 0x0080)
  {
    *ptr++ = (char)val;
  }
  else if (val < 0x0800)
  {
    *ptr++ = 0xC0 + (val >> 6);
    *ptr++ = 0x80 + (val & 0x3F);
  }
  else if (val < 0x10000)
  {
    *ptr++ = 0xE0 + ((val >> 12) & 0x0F);
    *ptr++ = 0x80 + ((val >> 6) & 0x3F);
    *ptr++ = 0x80 + (val & 0x3F);
  }
  else
  {
    *ptr++ = 0xF0 + ((val >> 18) & 0x07);
    *ptr++ = 0x80 + ((val >> 12) & 0x3F);
    *ptr++ = 0x80 + ((val >> 6) & 0x3F);
    *ptr++ = 0x80 + (val & 0x3F);
  }
}

SimpleString process_japanese_string(const char *str, wchar_t sep_char)
{
  if (!str || !*str)
    return {};

  SimpleString out;

  int len = (int)strlen(str);
  out.allocBuffer(len * 6);

  //--- PROCESS HERE ---------
  int wlen = utf8_strlen(str);
  if (wlen < 2)
  {
    strcpy(out, str);
    return out;
  }
  auto src = (const dag_char8_t *)str;
  char *dst = out.str();
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
