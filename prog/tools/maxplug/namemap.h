// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <max.h>
#include "str.h"
// #include "dobject.h"

/// @addtogroup utility_classes
/// @{

/// @addtogroup namemap
/// @{


/// @file
/// NameMap class derived from DObject that maps string to integers and back.


/// This class maps strings to integers and back.
/// NULL is mapped to -1.
/// All other strings are mapped to non-negative numbers (>=0).
/// NOTE: this class misses some methods that are implemented in NameMap and NameMapCI.
class BaseNameMap // : public DObject
{
public:
  // DAG_DECLARE_NEW(tmpmem)

  /// Constructor. Memory allocator can be specified (defaults to #tmpmem).
  BaseNameMap();
  ~BaseNameMap();

  /// Clear name list.
  void clear();

  /// Number of names in list.
  /// You can get them all by iterating from 0 to nameCount()-1.
  int nameCount() const { return names.Count(); }

  /// Returns NULL when name_id is invalid.
  const char *getName(int name_id) const;

  /// Save this name map.
  void save(FILE *) const;

  /// Load this name map.
  void load(FILE *);

protected:
  Tab<String> names;

  void copyFrom(const BaseNameMap &nm);

private:
  BaseNameMap(const BaseNameMap &nm) { copyFrom(nm); }
  BaseNameMap &operator=(const BaseNameMap &nm)
  {
    copyFrom(nm);
    return *this;
  }
};


/// Case-sensitive version of BaseNameMap.
class NameMap : public BaseNameMap
{
public:
  // DAG_DECLARE_NEW(tmpmem)

  /// Constructor. Memory allocator can be specified (defaults to #tmpmem).
  NameMap() : BaseNameMap() {}

  ///  Returns -1 if not found.
  int getNameId(const char *name) const;

  /// Returns -1 if NULL. Adds name to the list if not found.
  int addNameId(const char *name);

  /// To be used instead of private copy constructor.
  void copyFrom(const NameMap &nm) { BaseNameMap::copyFrom(nm); }
};


/// Case-insensitive version of BaseNameMap.
class NameMapCI : public BaseNameMap
{
public:
  // DAG_DECLARE_NEW(tmpmem)

  /// Constructor. Memory allocator can be specified (defaults to #tmpmem).
  NameMapCI() : BaseNameMap() {}

  ///  Returns -1 if not found.
  int getNameId(const char *name) const;

  /// Returns -1 if NULL. Adds name to the list if not found.
  int addNameId(const char *name);

  /// To be used instead of private copy constructor.
  void copyFrom(const NameMapCI &nm) { BaseNameMap::copyFrom(nm); }
};


/// @}

/// @}

// #include "undef_.h"
