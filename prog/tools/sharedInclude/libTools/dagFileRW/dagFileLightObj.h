// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/dagFileRW/dagFileNode.h>
#include <drv/3d/dag_driver.h>


class LightObject : public Object
{
public:
  DAG_DECLARE_NEW(nodemem)

  D3dLight lt;
  int flags;

  LightObject(D3dLight *l = NULL);
  ~LightObject() override;
  int classid() override;
  bool isSubOf(unsigned) override;
};
