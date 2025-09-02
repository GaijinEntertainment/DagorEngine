#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t blood_puddle_emitter__collNodeId_get_type();
static ecs::LTComponentList blood_puddle_emitter__collNodeId_component(ECS_HASH("blood_puddle_emitter__collNodeId"), blood_puddle_emitter__collNodeId_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_puddle_emitter__human_get_type();
static ecs::LTComponentList blood_puddle_emitter__human_component(ECS_HASH("blood_puddle_emitter__human"), blood_puddle_emitter__human_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_puddle_emitter__lastPos_get_type();
static ecs::LTComponentList blood_puddle_emitter__lastPos_component(ECS_HASH("blood_puddle_emitter__lastPos"), blood_puddle_emitter__lastPos_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_puddle_emitter__offset_get_type();
static ecs::LTComponentList blood_puddle_emitter__offset_component(ECS_HASH("blood_puddle_emitter__offset"), blood_puddle_emitter__offset_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_puddles__dir_get_type();
static ecs::LTComponentList blood_puddles__dir_component(ECS_HASH("blood_puddles__dir"), blood_puddles__dir_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_puddles__pos_get_type();
static ecs::LTComponentList blood_puddles__pos_component(ECS_HASH("blood_puddles__pos"), blood_puddles__pos_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__gravity_get_type();
static ecs::LTComponentList blood_splash_emitter__gravity_component(ECS_HASH("blood_splash_emitter__gravity"), blood_splash_emitter__gravity_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__itm_get_type();
static ecs::LTComponentList blood_splash_emitter__itm_component(ECS_HASH("blood_splash_emitter__itm"), blood_splash_emitter__itm_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__matrix_id_get_type();
static ecs::LTComponentList blood_splash_emitter__matrix_id_component(ECS_HASH("blood_splash_emitter__matrix_id"), blood_splash_emitter__matrix_id_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__normal_get_type();
static ecs::LTComponentList blood_splash_emitter__normal_component(ECS_HASH("blood_splash_emitter__normal"), blood_splash_emitter__normal_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__pos_get_type();
static ecs::LTComponentList blood_splash_emitter__pos_component(ECS_HASH("blood_splash_emitter__pos"), blood_splash_emitter__pos_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__size_get_type();
static ecs::LTComponentList blood_splash_emitter__size_component(ECS_HASH("blood_splash_emitter__size"), blood_splash_emitter__size_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__targetPos_get_type();
static ecs::LTComponentList blood_splash_emitter__targetPos_component(ECS_HASH("blood_splash_emitter__targetPos"), blood_splash_emitter__targetPos_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t blood_splash_emitter__velocity_get_type();
static ecs::LTComponentList blood_splash_emitter__velocity_component(ECS_HASH("blood_splash_emitter__velocity"), blood_splash_emitter__velocity_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "bloodPuddlesES.cpp.inl"
ECS_DEF_PULL_VAR(bloodPuddles);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc is_bood_enabled_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("isBloodEnabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc is_bood_enabled_ecs_query_desc
(
  "is_bood_enabled_ecs_query",
  empty_span(),
  make_span(is_bood_enabled_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void is_bood_enabled_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, is_bood_enabled_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(is_bood_enabled_ecs_query_comps, "isBloodEnabled", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_blood_color_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("isBloodEnabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("disabledBloodColor"), ecs::ComponentTypeInfo<E3DCOLOR>()}
};
static ecs::CompileTimeQueryDesc get_blood_color_ecs_query_desc
(
  "get_blood_color_ecs_query",
  empty_span(),
  make_span(get_blood_color_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_blood_color_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_blood_color_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_blood_color_ecs_query_comps, "isBloodEnabled", bool)
            , ECS_RO_COMP(get_blood_color_ecs_query_comps, "disabledBloodColor", E3DCOLOR)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t blood_puddle_emitter__collNodeId_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t blood_puddle_emitter__human_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t blood_puddle_emitter__lastPos_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_puddle_emitter__offset_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_puddles__dir_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_puddles__pos_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_splash_emitter__gravity_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_splash_emitter__itm_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
static constexpr ecs::component_t blood_splash_emitter__matrix_id_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t blood_splash_emitter__normal_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_splash_emitter__pos_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_splash_emitter__size_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t blood_splash_emitter__targetPos_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t blood_splash_emitter__velocity_get_type(){return ecs::ComponentTypeInfo<Point3>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
