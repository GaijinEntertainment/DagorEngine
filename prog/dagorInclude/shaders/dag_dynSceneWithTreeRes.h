//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_DObject.h>
#include <math/dag_geomTree.h>


decl_dclass_and_id(DynamicSceneWithTreeResource, DObject, 0xB885220D)
public:
  DAG_DECLARE_NEW(midmem)

  Ptr<class DynamicRenderableSceneLodsResource> scene;
  GeomNodeTree nodeTree;


  static DynamicSceneWithTreeResource *loadResource(IGenLoad & crd, int srl_flags);

  DynamicSceneWithTreeResource() {}

end_dclass_decl();


#define DynamicSceneWithTreeResourceClassName "DynScene"
