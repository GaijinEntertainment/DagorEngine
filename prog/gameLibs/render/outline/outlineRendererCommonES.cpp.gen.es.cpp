// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "outlineRendererCommonES.cpp.inl"
ECS_DEF_PULL_VAR(outlineRendererCommon);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc outline_render_always_visible_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
//start of 9 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_always_visible_ecs_query_desc
(
  "outline_render_always_visible_ecs_query",
  make_span(outline_render_always_visible_ecs_query_comps+0, 2)/*rw*/,
  make_span(outline_render_always_visible_ecs_query_comps+2, 9)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_always_visible_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_always_visible_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP_OR(outline_render_always_visible_ecs_query_comps, "animchar_render__enabled", bool( true)) && ECS_RO_COMP(outline_render_always_visible_ecs_query_comps, "outline__always_visible", bool) && ECS_RO_COMP(outline_render_always_visible_ecs_query_comps, "outline__enabled", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_always_visible_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(outline_render_always_visible_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RW_COMP(outline_render_always_visible_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            , ECS_RO_COMP(outline_render_always_visible_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RO_COMP_PTR(outline_render_always_visible_ecs_query_comps, "attaches_list", ecs::EidList)
            , ECS_RO_COMP_PTR(outline_render_always_visible_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP_OR(outline_render_always_visible_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_always_visible_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_always_visible_ri_ecs_query_comps[] =
{
//start of 8 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_always_visible_ri_ecs_query_desc
(
  "outline_render_always_visible_ri_ecs_query",
  empty_span(),
  make_span(outline_render_always_visible_ri_ecs_query_comps+0, 8)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_always_visible_ri_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_always_visible_ri_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "outline__always_visible", bool) && ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "outline__enabled", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "ri_extra", RiExtraComponent)
            , ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_always_visible_ri_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_always_visible_ri_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_always_visible_ri_handle_ecs_query_comps[] =
{
//start of 8 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra__handle"), ecs::ComponentTypeInfo<rendinst::riex_handle_t>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
//start of 1 no components at [8]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()}
};
static ecs::CompileTimeQueryDesc outline_render_always_visible_ri_handle_ecs_query_desc
(
  "outline_render_always_visible_ri_handle_ecs_query",
  empty_span(),
  make_span(outline_render_always_visible_ri_handle_ecs_query_comps+0, 8)/*ro*/,
  empty_span(),
  make_span(outline_render_always_visible_ri_handle_ecs_query_comps+8, 1)/*no*/);
template<typename Callable>
inline void outline_render_always_visible_ri_handle_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_always_visible_ri_handle_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "outline__always_visible", bool) && ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "outline__enabled", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "ri_extra__handle", rendinst::riex_handle_t)
            , ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_always_visible_ri_handle_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_always_visible_ri_handle_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_pass_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
//start of 12 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__isOccluded"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_pass_ecs_query_desc
(
  "outline_render_z_pass_ecs_query",
  make_span(outline_render_z_pass_ecs_query_comps+0, 1)/*rw*/,
  make_span(outline_render_z_pass_ecs_query_comps+1, 12)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_z_pass_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_pass_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_pass_ecs_query_comps, "outline__always_visible", bool( false)) && !ECS_RO_COMP_OR(outline_render_z_pass_ecs_query_comps, "outline__z_fail", bool( false)) && ECS_RO_COMP_OR(outline_render_z_pass_ecs_query_comps, "animchar_render__enabled", bool( true)) && ECS_RO_COMP(outline_render_z_pass_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_pass_ecs_query_comps, "outline__isOccluded", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_pass_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(outline_render_z_pass_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(outline_render_z_pass_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RW_COMP(outline_render_z_pass_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            , ECS_RO_COMP_PTR(outline_render_z_pass_ecs_query_comps, "attaches_list", ecs::EidList)
            , ECS_RO_COMP_PTR(outline_render_z_pass_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP_OR(outline_render_z_pass_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_pass_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_pass_ri_ecs_query_comps[] =
{
//start of 10 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__isOccluded"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_pass_ri_ecs_query_desc
(
  "outline_render_z_pass_ri_ecs_query",
  empty_span(),
  make_span(outline_render_z_pass_ri_ecs_query_comps+0, 10)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_z_pass_ri_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_pass_ri_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_pass_ri_ecs_query_comps, "outline__always_visible", bool( false)) && !ECS_RO_COMP_OR(outline_render_z_pass_ri_ecs_query_comps, "outline__z_fail", bool( false)) && ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "outline__isOccluded", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "ri_extra", RiExtraComponent)
            , ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_z_pass_ri_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_pass_ri_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_pass_ri_handle_ecs_query_comps[] =
{
//start of 10 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra__handle"), ecs::ComponentTypeInfo<rendinst::riex_handle_t>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__isOccluded"), ecs::ComponentTypeInfo<bool>()},
//start of 1 no components at [10]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_pass_ri_handle_ecs_query_desc
(
  "outline_render_z_pass_ri_handle_ecs_query",
  empty_span(),
  make_span(outline_render_z_pass_ri_handle_ecs_query_comps+0, 10)/*ro*/,
  empty_span(),
  make_span(outline_render_z_pass_ri_handle_ecs_query_comps+10, 1)/*no*/);
template<typename Callable>
inline void outline_render_z_pass_ri_handle_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_pass_ri_handle_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__always_visible", bool( false)) && !ECS_RO_COMP_OR(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__z_fail", bool( false)) && ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__isOccluded", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "ri_extra__handle", rendinst::riex_handle_t)
            , ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_fail_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
  {ECS_HASH("outline__frames_visible"), ecs::ComponentTypeInfo<int>()},
//start of 12 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__frames_history"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_fail_ecs_query_desc
(
  "outline_render_z_fail_ecs_query",
  make_span(outline_render_z_fail_ecs_query_comps+0, 2)/*rw*/,
  make_span(outline_render_z_fail_ecs_query_comps+2, 12)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_z_fail_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_fail_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_fail_ecs_query_comps, "outline__always_visible", bool( false)) && ECS_RO_COMP_OR(outline_render_z_fail_ecs_query_comps, "animchar_render__enabled", bool( true)) && ECS_RO_COMP(outline_render_z_fail_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_fail_ecs_query_comps, "outline__z_fail", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_fail_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(outline_render_z_fail_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(outline_render_z_fail_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RW_COMP(outline_render_z_fail_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            , ECS_RO_COMP_PTR(outline_render_z_fail_ecs_query_comps, "attaches_list", ecs::EidList)
            , ECS_RO_COMP_PTR(outline_render_z_fail_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RW_COMP(outline_render_z_fail_ecs_query_comps, "outline__frames_visible", int)
            , ECS_RO_COMP_OR(outline_render_z_fail_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_fail_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            , ECS_RO_COMP_OR(outline_render_z_fail_ecs_query_comps, "outline__frames_history", int(20))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_fail_ri_ecs_query_comps[] =
{
//start of 9 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_fail_ri_ecs_query_desc
(
  "outline_render_z_fail_ri_ecs_query",
  empty_span(),
  make_span(outline_render_z_fail_ri_ecs_query_comps+0, 9)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_z_fail_ri_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_fail_ri_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_fail_ri_ecs_query_comps, "outline__always_visible", bool( false)) && ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "outline__z_fail", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "ri_extra", RiExtraComponent)
            , ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_z_fail_ri_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_fail_ri_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_fail_ri_handle_ecs_query_comps[] =
{
//start of 9 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra__handle"), ecs::ComponentTypeInfo<rendinst::riex_handle_t>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>()},
//start of 1 no components at [9]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_fail_ri_handle_ecs_query_desc
(
  "outline_render_z_fail_ri_handle_ecs_query",
  empty_span(),
  make_span(outline_render_z_fail_ri_handle_ecs_query_comps+0, 9)/*ro*/,
  empty_span(),
  make_span(outline_render_z_fail_ri_handle_ecs_query_comps+9, 1)/*no*/);
template<typename Callable>
inline void outline_render_z_fail_ri_handle_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_fail_ri_handle_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_fail_ri_handle_ecs_query_comps, "outline__always_visible", bool( false)) && ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "outline__z_fail", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "ri_extra__handle", rendinst::riex_handle_t)
            , ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_z_fail_ri_handle_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_fail_ri_handle_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc attach_ecs_query_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("animchar_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc attach_ecs_query_desc
(
  "attach_ecs_query",
  empty_span(),
  make_span(attach_ecs_query_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void attach_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, attach_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          if ( !(ECS_RO_COMP_OR(attach_ecs_query_comps, "animchar_render__enabled", bool( true))) )
            return;
          function(
              ECS_RO_COMP(attach_ecs_query_comps, "animchar_attach__attachedTo", ecs::EntityId)
            , ECS_RO_COMP(attach_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(attach_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RO_COMP(attach_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            , ECS_RO_COMP_PTR(attach_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP_PTR(attach_ecs_query_comps, "attaches_list", ecs::EidList)
            );

        }
      }
  );
}
