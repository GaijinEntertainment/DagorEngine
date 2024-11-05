// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <scene/dag_objsToPlace.h>
#include <ioSys/dag_genIo.h>


ObjectsToPlace *ObjectsToPlace::make(IGenLoad &crd, int size)
{
#if _TARGET_64BIT //== since we forgot 64-bit alignment in structure layout...
  ObjectsToPlace *o = (ObjectsToPlace *)memalloc(size + 4, tmpmem);
  void *bo = ((char *)o) + 4;
  crd.read(&o->typeFourCc, 4);
  crd.read(&o->objs, size - 4);
#else
  ObjectsToPlace *o = (ObjectsToPlace *)memalloc(size, tmpmem);
  void *bo = o;
  crd.read(o, size);
#endif
  o->objs.patch(bo);
  for (int i = o->objs.size() - 1; i >= 0; i--)
  {
    o->objs[i].resName.patch(bo);
    o->objs[i].tm.patch(bo);
    o->objs[i].addData.patchData(bo);
    o->objs[i].addData.patchNameMap(bo);
  }
  return o;
}
