// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/physMat.h"
#include <gamePhys/props/physMatDamageModelProps.h>
#include <gamePhys/props/softMaterialProps.h>
#include <gamePhys/props/physDestructibleProps.h>
#include <gamePhys/props/physContactProps.h>
#include <gamePhys/props/shakeMatProps.h>
#include <gamePhys/props/deformMatProps.h>
#include <gamePhys/props/arcadeBoostProps.h>
#include <billboardDecals/matProps.h>
#include <scene/dag_physMat.h>
#include <ioSys/dag_dataBlock.h>
#include <propsRegistry/propsRegistry.h>
#include <osApiWrappers/dag_direct.h>
#include <startup/dag_globalSettings.h>
#include <gamePhys/collision/collisionLib.h>

static int register_physmat_props(const char *name, const DataBlock *blk, void *data)
{
  return propsreg::register_props(name, blk, (int)(uintptr_t)data);
}

eastl::string physmat::get_config_filename()
{
  static const char *DEFAULT_PHYSMAT_BLK = "config/physmat.blk";
  const char *physmatBlk = ::dgs_get_game_params()->getStr("physmatBlk", DEFAULT_PHYSMAT_BLK);
  return dd_file_exists(physmatBlk) ? physmatBlk : DEFAULT_PHYSMAT_BLK;
}

void physmat::init()
{
  if (propsreg::get_prop_class("physmat") >= 0) // props is registed, unregister it
    propsreg::unreg_props("physmat");

  BillboardDecalsProps::register_props_class();
  BillboardDecalTexProps::register_props_class();
  PhysMatDamageModelProps::register_props_class();
  physmat::SoftMaterialProps::register_props_class();
  physmat::ShakeMatProps::register_props_class();
  physmat::DeformMatProps::register_props_class();
  physmat::ArcadeBoostProps::register_props_class();
  physmat::PhysDestructibleProps::register_props_class();
  physmat::PhysContactProps::register_props_class();

  static const char *bbdecals_blk_fn = "config/billboardDecals.blk";
  DataBlock billboardDecalsBlk;
  if (dd_file_exists(bbdecals_blk_fn))
    billboardDecalsBlk.load(bbdecals_blk_fn);
  else
    LOGWARN_CTX("skip missing %s", bbdecals_blk_fn);

  load_billboard_decals(&billboardDecalsBlk);

  int physMatPropsId = propsreg::get_prop_class("physmat");

  const eastl::string physmatFilename = physmat::get_config_filename();

  DataBlock physMatFileBlk;
  if (dd_file_exists(physmatFilename.c_str()))
    physMatFileBlk.load(physmatFilename.c_str());
  else
  {
    LOGWARN_CTX("skip missing %s", physmatFilename.c_str());
    physMatFileBlk.addBlock("PhysMats")->addBlock("__DefaultParams")->setReal("iak", 1.0);
    physMatFileBlk.addBlock("InteractPropsList");
  }

  PhysMat::init("", &physMatFileBlk, &register_physmat_props, (void *)(uintptr_t)physMatPropsId);
  dacoll::init_phys_materials();
  compute_used_billboard_decals();
  PhysMat::on_physmat_ready();
}
