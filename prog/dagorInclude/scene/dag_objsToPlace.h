//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_roDataBlock.h>
#include <math/dag_TMatrix.h>


//! single dump with objects placements and optional parameters
struct ObjectsToPlace
{
  struct ObjRec
  {
    //! proto-object name
    PatchablePtr<const char> resName;
    //! array of inctance matrices
    PatchableTab<TMatrix> tm;
    //! data block with addtional data for instances: addData.blockCount()==tm.size()
    RoDataBlock addData;
  };

  //! single type 4CC ('PhO', 'FX', etc.) for all objects in container
  unsigned typeFourCc;
  //! array of different object sequences (one proto-obj + multiple places)
  PatchableTab<ObjRec> objs;

public:
  //! creates object from stream (allocates memory from tmpmem)
  static ObjectsToPlace *make(IGenLoad &crd, int size);
  //! destroys object (frees memory form tmpmem)
  void destroy() { memfree(this, tmpmem); }
};
