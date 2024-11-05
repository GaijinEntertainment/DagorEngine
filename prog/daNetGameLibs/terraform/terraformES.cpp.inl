// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <ecs/terraform/terraform.h>

#include <landMesh/lmeshManager.h>
#include <startup/dag_globalSettings.h>

#include "main/level.h"

static inline void terraform_init_es_event_handler(const EventLevelLoaded &, TerraformComponent &terraform)
{
  const LandMeshManager *lmeshMgr = get_landmesh_manager();
  if (!lmeshMgr)
  {
    debug("[Terraform]: The level does not have a Landmesh. Terraforming is not possible.");
    return;
  }

  const HeightmapHandler *hmap = lmeshMgr->getHmapHandler();
  if (!hmap)
  {
    debug("[Terraform]: The level does not have a Heightmap. Terraforming is not possible.");
    return;
  }

  const DataBlock *terraformBlk = ::dgs_get_game_params()->getBlockByNameEx("terraform");
  terraform::init(terraformBlk, lmeshMgr->getHmapHandler(), terraform);
}