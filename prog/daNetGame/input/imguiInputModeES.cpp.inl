// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <imgui/imguiInput.h>
#include <daEditorE/editorCommon/inGameEditor.h>


ECS_NO_ORDER
ECS_TAG(dev, render)
static void update_hybrid_imgui_input_es(const ecs::UpdateStageInfoAct &) { imgui_use_hybrid_input_mode(get_da_editor4().isActive()); }
