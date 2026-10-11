// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "levelCollision.h"
#include <rendInst/riexHandle.h>

static const LevelCollision *lru_level = nullptr;

void set_lru_collision_level(const LevelCollision *level) { lru_level = level; }

const CollisionResource *lru_collision_get_collres(uint32_t type) { return lru_level->resource(type); }

uint32_t lru_collision_get_type(rendinst::riex_handle_t h)
{
  return h == rendinst::RIEX_HANDLE_NULL ? ~0u : lru_level->handleResource(h);
}

mat43f lru_collision_get_transform(rendinst::riex_handle_t h) { return lru_level->transform(lru_level->handleInstance(h)); }
