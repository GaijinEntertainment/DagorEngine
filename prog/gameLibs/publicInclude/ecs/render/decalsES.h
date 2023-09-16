//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>
#include <ecs/rendInst/riExtra.h>
#include <daECS/core/entityId.h>
#include <projectiveDecals/projectiveDecals.h>
#include <decalMatrices/decalsMatrices.h>

namespace decals
{
extern void (*update_bullet_hole)(uint32_t matrix_id, const TMatrix &matrix);

extern uint32_t (*add_new_matrix_id_bullet_hole)();
extern void (*del_matrix_id_bullet_hole)(uint32_t matrix_id);

extern void (*entity_update)(ecs::EntityId eid, const TMatrix &tm);
extern void (*entity_delete)(ecs::EntityId eid);
}; // namespace decals

ECS_DECLARE_RELOCATABLE_TYPE(ResizableDecals)
ECS_DECLARE_RELOCATABLE_TYPE(RingBufferDecals)
ECS_DECLARE_BOXED_TYPE(DecalsMatrices)
