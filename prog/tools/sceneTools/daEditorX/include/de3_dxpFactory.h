//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class TextureIdSet;

extern void init_dxp_factory_service();
extern void term_dxp_factory_service();

extern void dxp_factory_force_discard(bool all);
extern void dxp_factory_force_discard(const TextureIdSet &tid);

extern void dxp_factory_after_reset();
