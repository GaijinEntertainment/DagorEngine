//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resMgr.h>

class TextureIdSet;

extern void init_dxp_factory_service();
extern void term_dxp_factory_service();

extern void dxp_factory_force_discard(bool all);
extern void dxp_factory_force_discard(const TextureIdSet &tid);
extern void dxp_factory_reload_tex(TEXTUREID tid, TexQL ql);

extern void dxp_factory_after_reset();
