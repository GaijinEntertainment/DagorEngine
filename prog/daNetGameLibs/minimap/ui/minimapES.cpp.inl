// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/ecsQuery.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>
#include "render/renderEvent.h"

#include "minimapContext.h"

ECS_TAG(render, gameClient)
ECS_REQUIRE(ecs::Tag &minimap_context)
static inline void hud_minimap_es(const RenderEventUI &evt) { minimap_on_render_ui(evt); }
