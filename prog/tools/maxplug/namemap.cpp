// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "namemap.h"


void NameMap::clear()
{
  s2i.clear();
  i2s.clear();
}


const char *NameMap::getName(int i) const
{
  auto it = i2s.find(i);
  if (it == i2s.end())
    return 0;

  return it->second.data();
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

  int id = getNameId(name);
  if (id != -1)
    return id;

  std::string nm(name);
  id = int(s2i.size());
  s2i.emplace(nm, id);
  i2s.emplace(id, nm);
  return id;
}
