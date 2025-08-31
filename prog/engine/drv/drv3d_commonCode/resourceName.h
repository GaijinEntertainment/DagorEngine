// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>

template <class T>
class D3dResourceNameImpl : public T
{
public:
  const char *getName() const final { return statName.c_str(); }
  void setName(const char *name) final { statName = name ? name : ""; }
  void setName(const char *name, int size) final { statName.assign(name, name + size); }

private:
  eastl::string statName;
};
