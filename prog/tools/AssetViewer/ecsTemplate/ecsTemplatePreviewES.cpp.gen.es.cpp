#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t ri_extra__bboxMax_get_type();
static ecs::LTComponentList ri_extra__bboxMax_component(ECS_HASH("ri_extra__bboxMax"), ri_extra__bboxMax_get_type(), "prog/tools/AssetViewer/ecsTemplate/ecsTemplatePreviewES.cpp.inl", "", 0);
static constexpr ecs::component_t ri_extra__bboxMin_get_type();
static ecs::LTComponentList ri_extra__bboxMin_component(ECS_HASH("ri_extra__bboxMin"), ri_extra__bboxMin_get_type(), "prog/tools/AssetViewer/ecsTemplate/ecsTemplatePreviewES.cpp.inl", "", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/tools/AssetViewer/ecsTemplate/ecsTemplatePreviewES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "ecsTemplatePreviewES.cpp.inl"
ECS_DEF_PULL_VAR(ecsTemplatePreview);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::component_t ri_extra__bboxMax_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t ri_extra__bboxMin_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
