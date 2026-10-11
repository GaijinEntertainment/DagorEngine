// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ver_obj_pcdx.h"

#include "util/dag_baseDef.h"
#include "util/dag_globDef.h"

// Increase this number if changes in the compiler invalidate .obj for dx11
extern const int VER_OBJ_PCDX_VAL = _MAKE4C('10.8');

#if _CROSS_TARGET_DX11
#include <drv/shadersMetaData/dx11/register_limits.h>

//
// Assertions made to make sure cache version is up to date
//

G_STATIC_ASSERT(dx11::MAX_B_REGISTERS == 12);
G_STATIC_ASSERT(dx11::MAX_T_REGISTERS == 32);
G_STATIC_ASSERT(dx11::MAX_S_REGISTERS == 16);
G_STATIC_ASSERT(dx11::MAX_U_REGISTERS == 8);
G_STATIC_ASSERT(dx11::IMMEDIATE_CB_REGISTER == 8);
G_STATIC_ASSERT(dx11::UAVS_CONTEND_WITH_RTVS == true);

#elif _CROSS_TARGET_EMPTY
#include "const3d.h"

// The stub target has no driver header to share, so its limits are pinned against this same version.

G_STATIC_ASSERT(MAX_B_REGISTERS == 12);
G_STATIC_ASSERT(MAX_T_REGISTERS == 32);
G_STATIC_ASSERT(MAX_S_REGISTERS == 16);
G_STATIC_ASSERT(MAX_U_REGISTERS == 13);
G_STATIC_ASSERT(IMMEDIATE_CB_REGISTER == 8);
G_STATIC_ASSERT(UAVS_CONTEND_WITH_RTVS == false);

#endif
