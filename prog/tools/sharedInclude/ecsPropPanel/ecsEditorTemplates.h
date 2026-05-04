// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <daECS/core/utility/ecsRecreate.h>

inline constexpr const char *ECS_EDITOR_SELECTED_TEMPLATE = "daeditor_selected";
inline constexpr const char *ECS_EDITOR_PREVIEW_TEMPLATE = "daeditor_preview_entity";
inline constexpr const char *ECS_EDITOR_HIERARCHIAL_FREE_TRANSFORM_TEMPLATE = "hierarchial_free_transform";

inline void removeSelectedTemplateName(eastl::string &templ_name)
{
  templ_name = remove_sub_template_name(templ_name.c_str(), ECS_EDITOR_SELECTED_TEMPLATE);
}

inline void removeEditorTemplateNames(eastl::string &templ_name)
{
  templ_name = remove_sub_template_name(templ_name.c_str(), ECS_EDITOR_SELECTED_TEMPLATE);
  templ_name = remove_sub_template_name(templ_name.c_str(), ECS_EDITOR_PREVIEW_TEMPLATE);
  templ_name = remove_sub_template_name(templ_name.c_str(), ECS_EDITOR_HIERARCHIAL_FREE_TRANSFORM_TEMPLATE);
}
