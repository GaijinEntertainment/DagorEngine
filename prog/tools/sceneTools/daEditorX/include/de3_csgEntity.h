//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>
class IObjEntity;
class Point3;


class ICsgEntity
{
public:
  static constexpr unsigned HUID = 0x2BFE9D9Cu; // ICsgEntity

  virtual void setFoundationPath(dag::Span<Point3> p, bool closed) = 0;
};
