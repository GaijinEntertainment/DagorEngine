//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_simpleString.h>
#include <scene/dag_physMat.h>

class DataBlock;

struct BillboardDecalsProps
{
  int billboardDecalId;
  float scale;

  void load(const DataBlock *blk);
  static const BillboardDecalsProps *get_props(int prop_id);
  static void register_props_class();
  static bool can_load(const DataBlock *);
};


struct BillboardDecalTexProps
{
  SimpleString diffuseTexName;
  SimpleString normalTexName;
  SimpleString shaderType;

  void load(const DataBlock *blk);
  static const BillboardDecalTexProps *get_props(int prop_id);
  static void register_props_class();
  static bool can_load(const DataBlock *);
};


dag::ConstSpan<const BillboardDecalTexProps *> get_used_billboard_decals();
void compute_used_billboard_decals();
int get_remaped_decal_tex(PhysMat::MatID mat_id);
void load_billboard_decals(const DataBlock *blk);
