// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "exp_tools.h"

#include <ioSys/dag_dataBlock.h>

void read_data_from_blk(Tab<GeomMeshHelperDagObject> &dagObjectsList, const DataBlock &blk)
{
  int nid = blk.getNameId("obj");
  for (unsigned int blockNo = 0; blockNo < blk.blockCount(); blockNo++)
    if (blk.getBlock(blockNo)->getBlockNameId() == nid)
    {
      const DataBlock &b = *blk.getBlock(blockNo);
      GeomMeshHelperDagObject dagObject;

      dagObject.name = blk.getBlock(blockNo)->getStr("name", "");

      dagObject.wtm.setcol(0, b.getPoint3("wtm0", Point3(1.f, 0.f, 0.f)));
      dagObject.wtm.setcol(1, b.getPoint3("wtm1", Point3(0.f, 1.f, 0.f)));
      dagObject.wtm.setcol(2, b.getPoint3("wtm2", Point3(0.f, 0.f, 1.f)));
      dagObject.wtm.setcol(3, b.getPoint3("wtm3", Point3(0.f, 0.f, 0.f)));

      dagObject.mesh.load(*b.getBlockByNameEx("mesh"));

      dagObjectsList.push_back(dagObject);
    }
}