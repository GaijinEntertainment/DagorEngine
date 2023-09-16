//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityComponent.h>
#include <gameRes/dag_collisionResource.h>
#include <gamePhys/collision/collisionObject.h>

ECS_DECLARE_BOXED_TYPE(CollisionResource);
ECS_DECLARE_RELOCATABLE_TYPE(CollisionObject);


Point2 get_collres_slice_mean_and_dispersion(const CollisionResource &collres, float min_height, float max_height);
