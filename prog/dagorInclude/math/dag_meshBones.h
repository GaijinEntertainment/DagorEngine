//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_TMatrix.h>
#include <util/dag_simpleString.h>

class MeshBones
{
public:
  DAG_DECLARE_NEW(tmpmem)

  Tab<SimpleString> boneNames;
  Tab<TMatrix> orgtm;
  Tab<TMatrix> bonetm;
  Tab<real> boneinf; // boneinf[numb][numv]

  MeshBones() : boneNames(tmpmem), orgtm(tmpmem), bonetm(tmpmem), boneinf(tmpmem) {}
  int setnumbones(int n)
  {
    boneNames.resize(n);
    orgtm.resize(n);
    bonetm.resize(n);
    return 1;
  }
};
