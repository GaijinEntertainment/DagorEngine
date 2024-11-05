// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// ############################################################################
// ##                                                                        ##
// ##  STABLE.H                                                              ##
// ##                                                                        ##
// ##  Misc. Support structure.  Defines table of strings, no duplicates.    ##
// ##                                                                        ##
// ##  OpenSourced 12/5/2000 by John W. Ratcliff                             ##
// ##                                                                        ##
// ##  No warranty expressed or implied.                                     ##
// ##                                                                        ##
// ##  Part of the Q3BSP project, which converts a Quake 3 BSP file into a   ##
// ##  polygon mesh.                                                         ##
// ############################################################################
// ##                                                                        ##
// ##  Contact John W. Ratcliff at jratcliff@verant.com                      ##
// ############################################################################


#include "stl.h"
#include <string.h>

class CharPtrLess
{
public:
  bool operator()(const char *v1, const char *v2) const
  {
    int v = stricmp(v1, v2);
    if (v < 0)
      return true;
    return false;
  };
};

typedef std::set<const char *, CharPtrLess> CharPtrSet;

class StringTable
{
public:
  StringTable(void){};

  ~StringTable(void)
  {
    CharPtrSet::iterator i;
    for (i = mStrings.begin(); i != mStrings.end(); i++)
    {
      char *str = (char *)(*i);
      delete str;
    }
  }

  const char *Get(const char *str)
  {
    CharPtrSet::iterator found;
    found = mStrings.find(str);
    if (found != mStrings.end())
      return (*found);
    int l = strlen(str);
    char *mem = new char[l + 1];
    strcpy(mem, str);
    mStrings.insert(mem);
    return mem;
  };

  const char *Get(const StdString &str) { return Get(str.c_str()); };

private:
  CharPtrSet mStrings;
};
