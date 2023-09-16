#include <util/dag_string.h>
#include <utf8.h>
#include "sqUtf8.h"
#include <debug/dag_debug.h>

// UTF8 for Squirrel.


namespace bindquirrel
{

Utf8::Utf8() {}


Utf8::Utf8(const char *text)
{
  G_ASSERT(text != NULL);
  if (text == NULL)
    return;

  // store only valid UTF8 string
  const char *end = utf8::find_invalid(text, text + strlen(text));
  utf8.setStr(text, end - text);
}


// Return UTF8 string.
const char *Utf8::str() const { return utf8.str(); }


// Return count of characters in UTF8 string.
int Utf8::charCount() const { return utf8::distance(utf8.str(), utf8.str() + utf8.length()); }


// Translate characters or replace UTF8 substrings and return new string.
const char *Utf8::strtr(const char *from, const char *to)
{
  // check
  G_ASSERT(from != NULL);
  G_ASSERT(to != NULL);
  if (from == NULL || to == NULL)
  {
    // on error return original string
    return utf8.str();
  }

  const int utf8_length = utf8.length();
  const utf8::iterator<const char *> utf8_end(utf8.str() + utf8_length);
  const utf8::iterator<const char *> from_end(from + strlen(from));

  clear_and_shrink(result);
  result.reserve(utf8_length);

  // foreach original string
  utf8::iterator<const char *> i(utf8.str());
  for (; i != utf8_end; i++)
  {
    const uint32_t ch = *i;
    uint32_t ch_out = ch;

    // foreach 'from' string
    utf8::iterator<const char *> k(from);
    utf8::iterator<const char *> j(to);
    for (; k != from_end; k++, j++)
      if (ch == *k)
      {
        ch_out = *j;
        break;
      }

    // put char in 'result' string
    char add[5] = {0, 0, 0, 0, 0}; // max one UTF8 char size is 4 and plus '\0'
    utf8::append(ch_out, add);
    result += add;
  }

  return result.str();
}


// Extract a section of UTF8 string and return new string.
const char *Utf8::slice(HSQUIRRELVM v)
{
  clear_and_shrink(result);
  int nArgs = sq_gettop(v);
  if (nArgs < 2)
  {
    G_ASSERTF(0, "utf8.slice: expected one or 2 params");
    logerr("utf8.slice: expected one or 2 params");
    return result.str();
  }

  int from = 0;
  int to = 0;

  if (sq_gettype(v, 2) != OT_INTEGER)
  {
    G_ASSERTF(0, "utf8.slice: expected integer as first param");
    logerr("utf8.slice: expected integer as first param");
    return result.str();
  }

  SQInteger temp;
  sq_getinteger(v, 2, &temp);
  from = (int)temp;

  bool isToAtEnd = false;
  if (nArgs > 2)
  {
    if (sq_gettype(v, 3) != OT_INTEGER)
    {
      G_ASSERTF(0, "utf8.slice: expected integer as second param");
      logerr("utf8.slice: expected integer as second param");
      return result.str();
    }
    sq_getinteger(v, 3, &temp);
    to = (int)temp;
    isToAtEnd = to < 0;
  }
  else
    isToAtEnd = true;

  const int utf8_length = utf8.length();
  const utf8::iterator<const char *> utf8_end(utf8.str() + utf8_length);

  if (isToAtEnd || from < 0)
  {
    const int cnt = charCount();
    if (from < 0)
      from += cnt;
    if (isToAtEnd)
      to += cnt;
  }

  result.reserve(min(2 * (to - from), utf8_length)); // size of ordinary UTF8 char is 2

  // foreach original string
  utf8::iterator<const char *> it(utf8.str());
  int charIdx = 0;
  for (; it != utf8_end && charIdx < to; it++, charIdx++)
  {
    if (charIdx < from)
      continue;

    char add[5] = {0, 0, 0, 0, 0}; // max one UTF8 char size is 4 and plus '\0'
    utf8::append(*it, add);
    result += add;
  }

  return result.str();
}


// Return a unicode char index of a substring in UTF8 string.
int Utf8::indexof(HSQUIRRELVM vm)
{
  const int nArgs = sq_gettop(vm);
  if (nArgs < 2 || sq_gettype(vm, 2) != OT_STRING || (nArgs > 2 && sq_gettype(vm, 3) != OT_INTEGER))
  {
    const char *err = "utf8.indexof expected params: (substring [, startIndex])";
    G_ASSERTF(0, err);
    logerr(err);
    return -1;
  }

  int startIdx = 0;
  if (nArgs > 2)
  {
    SQInteger temp;
    sq_getinteger(vm, 3, &temp);
    startIdx = (int)temp;
    if (startIdx < 0)
      return -1;
  }

  const char *sub = nullptr;
  sq_getstring(vm, 2, &sub);
  if (*sub == 0)
    return -1;

  int index = 0;
  const char *str = utf8.str();
  const int subLen = strlen(sub);

  while (*str && index < startIdx)
  {
    str++;
    if ((*str & 0xC0) != 0x80)
      index++;
  }

  while (*str)
  {
    if (*str == sub[0])
      if (!strncmp(str, sub, subLen))
      {
        str += subLen;
        if ((*str & 0xC0) != 0x80)
          return index;
        str -= subLen;
      }

    str++;
    while ((*str & 0xC0) == 0x80)
      str++;
    index++;
  }

  return -1;
}

} // namespace bindquirrel
