// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "dag_DynMap.h"

#include <util/dag_string.h>


class StrCompare
{
public:
  static inline int compare(const String &a, const String &b) { return strcmp((const char *)a, (const char *)b); }
};


class StriCompare
{
public:
  static inline int compare(const String &a, const String &b) { return stricmp((const char *)a, (const char *)b); }
};


template <class TVal>
class StrMap : public DynMap<String, TVal, StrCompare>
{
public:
  StrMap(IMemAlloc *mem) : DynMap<String, TVal, StrCompare>(mem) {}
  StrMap(const StrMap &from) : DynMap<String, TVal, StrCompare>(from) {}
};


template <class TVal>
class StriMap : public DynMap<String, TVal, StriCompare>
{
public:
  StriMap(IMemAlloc *mem) : DynMap<String, TVal, StriCompare>(mem) {}
  StriMap(const StriMap &from) : DynMap<String, TVal, StriCompare>(from) {}
};
