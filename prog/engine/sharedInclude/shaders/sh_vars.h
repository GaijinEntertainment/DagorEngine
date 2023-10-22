// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __SH_VARS_H
#define __SH_VARS_H

#pragma once

namespace shglobvars
{
// other vars
extern int globalTransRGvId;
extern int worldViewPosGvId;
extern int localWorldXGvId, localWorldYGvId, localWorldZGvId;
extern int shaderLodGvId;
extern int dynamic_pos_unpack_reg;

extern void init_varids_loaded();
} // namespace shglobvars

#endif
