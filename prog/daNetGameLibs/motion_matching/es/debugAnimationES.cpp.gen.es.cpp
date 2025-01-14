#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t motion_matching__dataBaseTemplateName_get_type();
static ecs::LTComponentList motion_matching__dataBaseTemplateName_component(ECS_HASH("motion_matching__dataBaseTemplateName"), motion_matching__dataBaseTemplateName_get_type(), "prog/daNetGameLibs/motion_matching/es/debugAnimationES.cpp.inl", "enable_mm_on_player_ecs_query", 0);
#include "debugAnimationES.cpp.inl"
ECS_DEF_PULL_VAR(debugAnimation);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc debug_motion_matching_skeleton_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()},
  {ECS_HASH("motion_matching__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static void debug_motion_matching_skeleton_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(ECS_RO_COMP(debug_motion_matching_skeleton_es_comps, "motion_matching__enabled", bool)) )
      continue;
    debug_motion_matching_skeleton_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(debug_motion_matching_skeleton_es_comps, "motion_matching__controller", MotionMatchingController)
    , ECS_RW_COMP(debug_motion_matching_skeleton_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc debug_motion_matching_skeleton_es_es_desc
(
  "debug_motion_matching_skeleton_es",
  "prog/daNetGameLibs/motion_matching/es/debugAnimationES.cpp.inl",
  ecs::EntitySystemOps(debug_motion_matching_skeleton_es_all),
  make_span(debug_motion_matching_skeleton_es_comps+0, 1)/*rw*/,
  make_span(debug_motion_matching_skeleton_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc debug_motion_matching_es_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()},
  {ECS_HASH("mm_trajectory__featurePositions"), ecs::ComponentTypeInfo<ecs::Point3List>()},
  {ECS_HASH("mm_trajectory__featureDirections"), ecs::ComponentTypeInfo<ecs::Point3List>()},
  {ECS_HASH("motion_matching__goalFeature"), ecs::ComponentTypeInfo<FrameFeatures>()},
  {ECS_HASH("motion_matching__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static void debug_motion_matching_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(ECS_RO_COMP(debug_motion_matching_es_comps, "motion_matching__enabled", bool)) )
      continue;
    debug_motion_matching_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(debug_motion_matching_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(debug_motion_matching_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(debug_motion_matching_es_comps, "motion_matching__controller", MotionMatchingController)
    , ECS_RO_COMP(debug_motion_matching_es_comps, "mm_trajectory__featurePositions", ecs::Point3List)
    , ECS_RO_COMP(debug_motion_matching_es_comps, "mm_trajectory__featureDirections", ecs::Point3List)
    , ECS_RO_COMP(debug_motion_matching_es_comps, "motion_matching__goalFeature", FrameFeatures)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc debug_motion_matching_es_es_desc
(
  "debug_motion_matching_es",
  "prog/daNetGameLibs/motion_matching/es/debugAnimationES.cpp.inl",
  ecs::EntitySystemOps(debug_motion_matching_es_all),
  empty_span(),
  make_span(debug_motion_matching_es_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc imgui_visualization_skeleton_ecs_query_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("mm_visualization_show_skeleton"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("mm_visualization_show_skeleton_original"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("mm_visualization_show_skeleton_node_labels"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("mm_visualization__selectedClipIdx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("mm_visualization__selectedFrameIdx"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc imgui_visualization_skeleton_ecs_query_desc
(
  "imgui_visualization_skeleton_ecs_query",
  empty_span(),
  make_span(imgui_visualization_skeleton_ecs_query_comps+0, 5)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void imgui_visualization_skeleton_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, imgui_visualization_skeleton_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(imgui_visualization_skeleton_ecs_query_comps, "mm_visualization_show_skeleton", bool)
            , ECS_RO_COMP(imgui_visualization_skeleton_ecs_query_comps, "mm_visualization_show_skeleton_original", bool)
            , ECS_RO_COMP(imgui_visualization_skeleton_ecs_query_comps, "mm_visualization_show_skeleton_node_labels", bool)
            , ECS_RO_COMP(imgui_visualization_skeleton_ecs_query_comps, "mm_visualization__selectedClipIdx", int)
            , ECS_RO_COMP(imgui_visualization_skeleton_ecs_query_comps, "mm_visualization__selectedFrameIdx", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc imgui_visualization_debug_features_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("mm_visualization_show_feature_nodes"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("mm_visualization_show_trajectory"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc imgui_visualization_debug_features_ecs_query_desc
(
  "imgui_visualization_debug_features_ecs_query",
  empty_span(),
  make_span(imgui_visualization_debug_features_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void imgui_visualization_debug_features_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, imgui_visualization_debug_features_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(imgui_visualization_debug_features_ecs_query_comps, "mm_visualization_show_feature_nodes", bool)
            , ECS_RO_COMP(imgui_visualization_debug_features_ecs_query_comps, "mm_visualization_show_trajectory", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_clip_for_debug_trajectory_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("mm_imguiAnimcharEid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("mm_visualization__selectedClipIdx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("mm_visualization__selectedFrameIdx"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc get_clip_for_debug_trajectory_ecs_query_desc
(
  "get_clip_for_debug_trajectory_ecs_query",
  empty_span(),
  make_span(get_clip_for_debug_trajectory_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_clip_for_debug_trajectory_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_clip_for_debug_trajectory_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_clip_for_debug_trajectory_ecs_query_comps, "mm_imguiAnimcharEid", ecs::EntityId)
            , ECS_RO_COMP(get_clip_for_debug_trajectory_ecs_query_comps, "mm_visualization__selectedClipIdx", int)
            , ECS_RO_COMP(get_clip_for_debug_trajectory_ecs_query_comps, "mm_visualization__selectedFrameIdx", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc disable_mm_on_player_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 2 rq components at [1]
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()},
  {ECS_HASH("watchedByPlr"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc disable_mm_on_player_ecs_query_desc
(
  "disable_mm_on_player_ecs_query",
  empty_span(),
  make_span(disable_mm_on_player_ecs_query_comps+0, 1)/*ro*/,
  make_span(disable_mm_on_player_ecs_query_comps+1, 2)/*rq*/,
  empty_span());
template<typename Callable>
inline void disable_mm_on_player_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, disable_mm_on_player_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(disable_mm_on_player_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc enable_mm_on_player_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("watchedByPlr"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc enable_mm_on_player_ecs_query_desc
(
  "enable_mm_on_player_ecs_query",
  empty_span(),
  make_span(enable_mm_on_player_ecs_query_comps+0, 1)/*ro*/,
  make_span(enable_mm_on_player_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void enable_mm_on_player_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, enable_mm_on_player_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(enable_mm_on_player_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_current_imgui_character_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("mm_imguiAnimcharEid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc get_current_imgui_character_ecs_query_desc
(
  "get_current_imgui_character_ecs_query",
  empty_span(),
  make_span(get_current_imgui_character_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_current_imgui_character_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_current_imgui_character_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_current_imgui_character_ecs_query_comps, "mm_imguiAnimcharEid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc process_current_character_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()},
//start of 3 ro components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("motion_matching__goalFeature"), ecs::ComponentTypeInfo<FrameFeatures>()},
  {ECS_HASH("motion_matching__presetIdx"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc process_current_character_ecs_query_desc
(
  "process_current_character_ecs_query",
  make_span(process_current_character_ecs_query_comps+0, 1)/*rw*/,
  make_span(process_current_character_ecs_query_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void process_current_character_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, process_current_character_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(process_current_character_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            , ECS_RW_COMP(process_current_character_ecs_query_comps, "motion_matching__controller", MotionMatchingController)
            , ECS_RO_COMP(process_current_character_ecs_query_comps, "motion_matching__goalFeature", FrameFeatures)
            , ECS_RO_COMP(process_current_character_ecs_query_comps, "motion_matching__presetIdx", int)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc update_clip_for_debug_trajectory_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("mm_visualization__selectedClipIdx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("mm_visualization__selectedFrameIdx"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc update_clip_for_debug_trajectory_ecs_query_desc
(
  "update_clip_for_debug_trajectory_ecs_query",
  make_span(update_clip_for_debug_trajectory_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void update_clip_for_debug_trajectory_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, update_clip_for_debug_trajectory_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(update_clip_for_debug_trajectory_ecs_query_comps, "mm_visualization__selectedClipIdx", int)
            , ECS_RW_COMP(update_clip_for_debug_trajectory_ecs_query_comps, "mm_visualization__selectedFrameIdx", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t motion_matching__dataBaseTemplateName_get_type(){return ecs::ComponentTypeInfo<eastl::basic_string<char, eastl::allocator>>::type; }
