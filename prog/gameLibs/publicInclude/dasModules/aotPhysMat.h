//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>

#include <scene/dag_physMat.h>
#include <gamePhys/props/physMatDamageModelProps.h>
#include <gamePhys/props/deformMatProps.h>
#include <dasModules/aotProps.h>

MAKE_TYPE_FACTORY(PhysMat::MaterialData, PhysMat::MaterialData);
MAKE_TYPE_FACTORY(PhysMatDamageModelProps, PhysMatDamageModelProps);
MAKE_TYPE_FACTORY(DeformMatProps, physmat::DeformMatProps);

namespace bind_dascript
{
inline const char *get_material_name(const PhysMat::MaterialData &mat_data) { return mat_data.name.c_str(); }
} // namespace bind_dascript
