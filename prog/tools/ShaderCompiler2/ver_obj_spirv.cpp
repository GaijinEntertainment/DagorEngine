// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ver_obj_spirv.h"

#include "util/dag_baseDef.h"

extern const int VER_OBJ_SPIRV_VAL = _MAKE4C('12.9');

#if _CROSS_TARGET_SPIRV
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>

//
// Assertions made to make sure cache version is up to date
//

G_STATIC_ASSERT(spirv::REGISTER_ENTRIES == 53);

G_STATIC_ASSERT(spirv::B_REGISTER_INDEX_MAX == 8);
G_STATIC_ASSERT(spirv::T_REGISTER_INDEX_MAX == 32);
G_STATIC_ASSERT(spirv::U_REGISTER_INDEX_MAX == 13);

G_STATIC_ASSERT(spirv::WORK_GROUP_SIZE_X_CONSTANT_ID == 1);
G_STATIC_ASSERT(spirv::WORK_GROUP_SIZE_Y_CONSTANT_ID == 2);
G_STATIC_ASSERT(spirv::WORK_GROUP_SIZE_Z_CONSTANT_ID == 3);

G_STATIC_ASSERT(spirv::B_CONST_BUFFER_OFFSET == 0);

#endif