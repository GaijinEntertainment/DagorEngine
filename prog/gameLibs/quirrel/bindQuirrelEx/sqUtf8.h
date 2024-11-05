// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <squirrel.h>
#include <util/dag_string.h>

namespace bindquirrel
{
class Utf8
{
public:
  Utf8();
  Utf8(const char *text);

  // Return UTF8 string.
  const char *str() const;

  // Return count of characters in UTF8 string.
  int charCount() const;

  // Translate characters or replace UTF8 substrings and return new string.
  const char *strtr(const char *from, const char *to);

  // Extract a section of UTF8 string and return new string.
  const char *slice(HSQUIRRELVM v);

  // Return a unicode char index of a substring in UTF8 string.
  int indexof(HSQUIRRELVM v);

private:
  String utf8;   // UTF8 string
  String result; // storage for return string (byte array)
};
} // namespace bindquirrel
