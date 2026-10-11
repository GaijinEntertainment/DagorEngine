// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ver_obj_metal.h"

#include "util/dag_baseDef.h"
#include "util/dag_globDef.h"

// Increase this number if changes in the compiler invalidate .obj for metal
extern const int VER_OBJ_METAL_VAL = _MAKE4C('13.8');

#if _CROSS_TARGET_METAL
#include <drv/shadersMetaData/metal/register_limits.h>
#include "buffBindPoints.h"

//
// Assertions made to make sure cache version is up to date
//

G_STATIC_ASSERT(metal::MAX_B_REGISTERS == 12);
G_STATIC_ASSERT(metal::MAX_T_REGISTERS == 32);
G_STATIC_ASSERT(metal::MAX_S_REGISTERS == 16);
G_STATIC_ASSERT(metal::MAX_U_REGISTERS == 10);
G_STATIC_ASSERT(metal::IMMEDIATE_CB_REGISTER == 13);
G_STATIC_ASSERT(metal::UAVS_CONTEND_WITH_RTVS == false);

G_STATIC_ASSERT(drv3d_metal::GEOM_BUFFER_COUNT == 2);
G_STATIC_ASSERT(drv3d_metal::IMMEDIATE_BIND_SLOT == 4);
G_STATIC_ASSERT(drv3d_metal::BINDLESS_TEXTURE_ID_BUFFER_COUNT == 5);
G_STATIC_ASSERT(drv3d_metal::BINDLESS_SAMPLER_ID_BUFFER_COUNT == 3);
G_STATIC_ASSERT(drv3d_metal::BINDLESS_BUFFER_ID_BUFFER_COUNT == 3);

#endif
