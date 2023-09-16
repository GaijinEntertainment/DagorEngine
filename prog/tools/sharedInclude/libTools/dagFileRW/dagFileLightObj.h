
#ifndef __DAGOR_NODE_LIGHTOBJ_H
#define __DAGOR_NODE_LIGHTOBJ_H
#pragma once

#include <libTools/dagFileRW/dagFileNode.h>
#include <3d/dag_drv3d.h>


class LightObject : public Object
{
public:
  DAG_DECLARE_NEW(nodemem)

  D3dLight lt;
  int flags;

  LightObject(D3dLight *l = NULL);
  ~LightObject();
  int classid();
  bool isSubOf(unsigned);
};

#endif
