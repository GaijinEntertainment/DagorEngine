// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define USE_JOLT_PHYSICS 1

#define PHYSICS_SKIP_INIT_TERM is_dng_based_render_used() // DNG already creates and owns Jolt physworld
static bool is_dng_based_render_used();
#include "phys.inc.cpp"
IMPLEMENT_PHYS_API(jolt)

#include <de3_dynRenderService.h>
static bool is_dng_based_render_used()
{
  if (IDynRenderService *rs = EDITORCORE->queryEditorInterface<IDynRenderService>())
    return rs->getRenderType() == rs->RTYPE_DNG_BASED;
  return false;
}
