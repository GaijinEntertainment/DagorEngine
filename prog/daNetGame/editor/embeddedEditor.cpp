// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "camera/sceneCam.h"

#include <ecs/rendInst/riExtra.h>
#include <daECS/core/entityManager.h>

static rendinst::riex_handle_t extract_ri_extra_from_eid(ecs::EntityId eid)
{
  if (const RiExtraComponent *ri_extra = g_entity_mgr->getNullable<RiExtraComponent>(eid, ECS_HASH("ri_extra")))
    return ri_extra->handle;
  return rendinst::RIEX_HANDLE_NULL;
}

#include <daEditorE/editorCommon/inGameEditor.inc.cpp>
