//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class StaticGeometryMaterial;
class IObjEntity;

class IStaticGeometryMaterialEditor
{
public:
  static constexpr unsigned HUID = 0x6AF09614u; // IStaticGeometryMaterialEditor

  struct MaterialEntry
  {
    const StaticGeometryMaterial *material;
    int node_idx;
    int mat_idx;
    unsigned int color;
  };

  virtual void setMaterialColorForEntity(IObjEntity *entity, int node_idx, int material_idx, const E3DCOLOR &color) = 0;
  virtual void getMaterialsForDecalEntity(IObjEntity *entity, Tab<MaterialEntry> &out_materials) = 0;
  virtual void saveStaticGeometryAssetByEntity(IObjEntity *entity) = 0;
};