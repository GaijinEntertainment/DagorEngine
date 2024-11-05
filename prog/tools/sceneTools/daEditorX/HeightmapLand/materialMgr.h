// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>

class MaterialDataList;


class ObjLibMaterialListMgr
{
public:
  ObjLibMaterialListMgr() : mat(midmem) {}

  void reset() { mat.clear(); }

  int getMaterialId(const char *mat_asset_name);

  MaterialDataList *buildMaterialDataList();

  static void setDefaultMat0(MaterialDataList &mat);
  static void complementDefaultMat0(MaterialDataList &mat);
  static void reclearDefaultMat0(MaterialDataList &mat);

protected:
  Tab<SimpleString> mat;
};
