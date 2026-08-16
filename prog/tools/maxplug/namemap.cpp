// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "namemap.h"


void NameMap::clear()
{
  s2i.clear();
  names.clear();
}


const char *NameMap::getName(int i) const
{
  if (i < 0 || i >= int(names.size()))
    return 0;

  return names[i];
}


int NameMap::getNameId(const char *name) const
{
  if (!name)
    return -1;

  auto it = s2i.find(name);
  if (it == s2i.end())
    return -1;

  return it->second;
}


int NameMap::addNameId(const char *name)
{
  if (!name)
    return -1;

  auto it = s2i.find(name);
  if (it != s2i.end())
    return it->second;

  it = s2i.emplace(name, int(names.size())).first;
  names.push_back(it->first.c_str());
  return it->second;
}
