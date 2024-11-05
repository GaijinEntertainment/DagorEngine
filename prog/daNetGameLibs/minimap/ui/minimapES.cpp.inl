// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/entitySystem.h>
#include "render/renderEvent.h"

#include "minimapContext.h"

ECS_TAG(render, gameClient)
ECS_REQUIRE(ecs::Tag &minimap_context)
static inline void hud_minimap_es(const RenderEventUI &evt) { minimap_on_render_ui(evt); }
