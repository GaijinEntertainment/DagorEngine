//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/componentType.h>
#include <terraform/terraformComponent.h>

// It's boxed since it's register pointer to itself in terraform (`registerListener()`)
ECS_DECLARE_BOXED_TYPE(TerraformComponent);

class DataBlock;
class HeightmapHandler;

namespace terraform
{
void init(const DataBlock *terraform_blk, HeightmapHandler *hmap, TerraformComponent &terraform);
}