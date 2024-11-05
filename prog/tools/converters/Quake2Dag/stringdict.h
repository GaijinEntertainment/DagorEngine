// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "stl.h"
#include <string.h>

// ############################################################################
// ##                                                                        ##
// ##  STRINGDICT.H                                                          ##
// ##                                                                        ##
// ##  Global collection of all strings in application, no duplicates.       ##
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

#include "stable.h"

class StringRef
{
public:
  StringRef(void) { mString = "null"; };

  inline StringRef(const char *str);
  inline StringRef(const StdString &str);
  inline StringRef(const StringRef &str);

  operator const char *() const { return mString; };

  operator const StdString() const { return mString; };

  const char *Get(void) const { return mString; };

  void Set(const char *str) { mString = str; };

  const StringRef &operator=(const StringRef &rhs)
  {
    mString = rhs.Get();
    return *this;
  };

  bool operator==(const StringRef &rhs) const { return rhs.mString == mString; };

  bool operator<(const StringRef &rhs) const { return rhs.mString < mString; };

  bool operator!=(const StringRef &rhs) const { return rhs.mString != mString; };

  bool operator>(const StringRef &rhs) const { return rhs.mString > mString; };

  bool operator<=(const StringRef &rhs) const { return rhs.mString < mString; };

  bool operator>=(const StringRef &rhs) const { return rhs.mString >= mString; };

private:
  const char *mString; // the actual char ptr
};

class StringDict
{
public:
  StringRef Get(const char *text)
  {
    const char *foo = mStringTable.Get(text);
    StringRef ref;
    ref.Set(foo);
    return ref;
  }

  StringRef Get(const StdString &text) { return Get(text.c_str()); };


  static StringDict &gStringDict(void) // global instance
  {
    if (!gSingleton)
      gSingleton = new StringDict;
    return *gSingleton;
  }

  static void ExplicitDestroy(void)
  {
    delete gSingleton;
    gSingleton = 0;
  }

private:
  static StringDict *gSingleton;
  StringTable mStringTable;
};

typedef std::vector<StringRef> StringRefVector;
typedef std::vector<StringRefVector> StringRefVectorVector;
typedef std::set<StringRef> StringRefSet;


#define SGET(x) StringDict::gStringDict().Get(x)

inline StringRef::StringRef(const char *str)
{
  StringRef ref = SGET(str);
  mString = ref.mString;
};

inline StringRef::StringRef(const StdString &str)
{
  StringRef ref = SGET(str);
  mString = ref.mString;
};

inline StringRef::StringRef(const StringRef &str) { mString = str.Get(); };
