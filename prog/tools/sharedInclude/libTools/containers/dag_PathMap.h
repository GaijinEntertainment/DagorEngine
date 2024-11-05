// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/containers/dag_DynMap.h>
#include <libTools/util/fileUtils.h>

#include <util/dag_string.h>


class PathCompare
{
public:
  static int compare(const String &a, const String &b) { return ::dag_path_compare(a, b); }
};


template <class TVal>
class PathMap : public DynMap<String, TVal, PathCompare>
{
public:
  PathMap(IMemAlloc *mem) : DynMap<String, TVal, PathCompare>(mem) {}
  PathMap(const PathMap &from) : DynMap<String, TVal, PathCompare>(from) {}
};
