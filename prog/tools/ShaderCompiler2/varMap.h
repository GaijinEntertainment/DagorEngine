// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "nameMap.h"

/************************************************************************
  variable names map for shaders
/************************************************************************/
class VarNameMap
{
  SCFastNameMap map;

public:
  static constexpr int BAD_ID = -1;
  VarNameMap() = default;
  ~VarNameMap() { map.reset(); }
  int getVarId(const char *var_name) const { return map.getNameId(var_name); }
  int addVarId(const char *var_name) { return map.addNameId(var_name); }
  int getIdCount() const { return map.nameCount(); }
  const char *getName(int id) const { return map.getName(id); }
};