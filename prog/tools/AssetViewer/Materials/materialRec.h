// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_materialData.h>
#include <generic/dag_tab.h>
#include <generic/dag_DObject.h>
#include <util/dag_string.h>

class DataBlock;
class DagorAsset;


class MaterialRec
{
public:
  Tab<DagorAsset *> textures;
  String className;
  String script;
  D3dMaterial mat;
  int flags, defMatCol;

  MaterialRec(DagorAsset &_asset);

  bool operator==(const MaterialRec &from) const;
  bool operator!=(const MaterialRec &from) const { return !operator==(from); }

  Ptr<MaterialData> getMaterial() const;
  const char *getTexRef(int tex_idx) const;

  void saveChangesToAsset();

protected:
  DagorAsset *asset;
};
