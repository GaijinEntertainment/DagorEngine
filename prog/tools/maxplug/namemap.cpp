// Copyright 2023 by Gaijin Games KFT, All rights reserved.
// #include "globdef.h"
// #include "saveload.h"
#include "namemap.h"
#include "debug.h"


BaseNameMap::BaseNameMap()
{
  /*
    debug_ctx ( "NameMap::NameMap=%p from:", this );

    static String s(inimem_ptr());
    s.reserve(5000);
    _getstackinfo(s);
    debug ( "%s", (char*)s );
  //*/
}


BaseNameMap::~BaseNameMap() { clear(); }


void BaseNameMap::clear() { names.Delete(0, names.Count()); }


void BaseNameMap::save(FILE *f /*GeneralSaveCB &cb*/) const
{
  int n = names.Count();
  fwrite(&n, sizeof(n), 1, f);

  for (int i = 0; i < names.Count(); ++i)
  {
    int len = (int)strlen(names[i]);
    if (len >= (1 << 16))
      debug("NameMap string is too long to save");
    unsigned short l = len;
    fwrite(&l, sizeof(l), 1, f);
    if (l)
      fwrite(names[i], l, 1, f);
  }
}


void BaseNameMap::load(FILE *f)
{
  clear();

  int n;
  fread(&n, sizeof(n), 1, f);
  names.Resize(n);

  for (int i = 0; i < names.Count(); ++i)
  {
    unsigned short l;
    fread(&l, sizeof(l), 1, f);
    char *s = (char *)malloc(l + 1);

    if (l)
      fread(s, l, 1, f);
    s[l] = 0;
    names[i] = s;
  }
}


const char *BaseNameMap::getName(int i) const
{
  if (i < 0 || i >= names.Count())
    return NULL;
  return names[i];
}


void BaseNameMap::copyFrom(const BaseNameMap &nm)
{
  clear();
  names.Resize(nm.names.Count());
  for (int i = 0; i < names.Count(); ++i)
  {
    names[i] = (char *)malloc(strlen(nm.names[i]) + 1);
    strcpy(names[i], nm.names[i]);
  }
}


int NameMap::getNameId(const char *name) const
{
  if (!name)
    return -1;

  for (int i = 0; i < names.Count(); ++i)
    if (strcmp(names[i], name) == 0)
      return i;
  return -1;
}


int NameMap::addNameId(const char *name)
{
  if (!name)
    return -1;

  int i;
  for (i = 0; i < names.Count(); ++i)
    if (strcmp(names[i], name) == 0)
      return i;

  String *str = new String(name);
  i = names.Append(1, str);
  return i;
}


int NameMapCI::getNameId(const char *name) const
{
  if (!name)
    return -1;

  for (int i = 0; i < names.Count(); ++i)
    if (stricmp(names[i], name) == 0)
      return i;
  return -1;
}


int NameMapCI::addNameId(const char *name)
{
  if (!name)
    return -1;

  int i;
  for (i = 0; i < names.Count(); ++i)
    if (stricmp(names[i], name) == 0)
      return i;

  String *str = new String(name);
  i = names.Append(1, str);

  return i;
}
