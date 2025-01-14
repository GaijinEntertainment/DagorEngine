// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ver_obj_dxc.h"

#include "util/dag_baseDef.h"

// Increase this number if changes in the compiler invalidate .obj for dx12
extern const int VER_OBJ_DXC_VAL = _MAKE4C('12.4');

#if _CROSS_TARGET_DX12
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>

//
// Assertions made to make sure cache version is up to date
//

G_STATIC_ASSERT(dxil::MAX_B_REGISTERS == 14);
G_STATIC_ASSERT(dxil::MAX_T_REGISTERS == 32);
G_STATIC_ASSERT(dxil::MAX_U_REGISTERS == 13);
G_STATIC_ASSERT(dxil::ROOT_CONSTANT_BUFFER_REGISTER_INDEX == 8);
G_STATIC_ASSERT(dxil::ROOT_CONSTANT_BUFFER_REGISTER_SPACE_OFFSET == 1);
G_STATIC_ASSERT(dxil::SPECIAL_CONSTANTS_REGISTER_INDEX == 7);
G_STATIC_ASSERT(dxil::DRAW_ID_REGISTER_SPACE == 1);
G_STATIC_ASSERT(dxil::MAX_UNBOUNDED_REGISTER_SPACES == 8);

G_STATIC_ASSERT(dxil::REGULAR_RESOURCES_SPACE_INDEX == 0);
G_STATIC_ASSERT(dxil::BINDLESS_REGISTER_INDEX == 0);
G_STATIC_ASSERT(dxil::BINDLESS_SAMPLERS_SPACE_COUNT == 2);
G_STATIC_ASSERT(dxil::BINDLESS_SAMPLERS_SPACE_OFFSET == 1);

G_STATIC_ASSERT(dxil::BINDLESS_RESOURCES_SPACE_COUNT == 30);
G_STATIC_ASSERT(dxil::BINDLESS_RESOURCES_SPACE_OFFSET == 1);

G_STATIC_ASSERT(dxil::BINDLESS_SAMPLERS_SPACE_BITS_SHIFT == 0);

#endif