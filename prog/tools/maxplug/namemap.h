// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <string>
#include <unordered_map>

#include "ci.h"

class NameMap
{
public:
  NameMap() {}
  ~NameMap() {}

  /// Clear name list.
  void clear();

  /// Number of names in list.
  /// You can get them all by iterating from 0 to nameCount()-1.
  int nameCount() const { return int(s2i.size()); }

  /// Returns NULL when name_id is invalid.
  const char *getName(int i) const;

  ///  Returns -1 if not found.
  int getNameId(const char *name) const;

  /// Returns -1 if NULL. Adds name to the list if not found.
  int addNameId(const char *name);

private:
  std::unordered_map<std::string, int, CaseInsensitiveHash, CaseInsensitiveEqual> s2i;
  std::unordered_map<int, std::string> i2s;
};


/// @}

/// @}

// #include "undef_.h"
