//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityManager.h>
#include <3d/dag_resPtr.h>

// supports init component *name*_res:t="res_name"
ECS_DECLARE_RELOCATABLE_TYPE(SharedTex);

// supports init components *name*_res:t="res_name" *name*_var:t="var_name"
ECS_DECLARE_RELOCATABLE_TYPE(SharedTexHolder);

// no any init components
ECS_DECLARE_RELOCATABLE_TYPE(UniqueTex);
ECS_DECLARE_RELOCATABLE_TYPE(UniqueTexHolder);
ECS_DECLARE_RELOCATABLE_TYPE(UniqueBufHolder);
