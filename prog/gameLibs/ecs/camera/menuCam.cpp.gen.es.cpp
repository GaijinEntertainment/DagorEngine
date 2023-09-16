#include "menuCam.cpp.inl"
ECS_DEF_PULL_VAR(menuCam);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc menu_cam_init_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("menu_cam__initialAngles"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("menu_cam__dirInited"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("menu_cam__angles"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("menu_cam__angles_smooth_internal"), ecs::ComponentTypeInfo<Point2>()},
//start of 2 ro components at [4]
  {ECS_HASH("menu_cam__initialDir"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("menu_cam__shouldRotateTarget"), ecs::ComponentTypeInfo<bool>()}
};
static void menu_cam_init_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(!ECS_RW_COMP(menu_cam_init_es_comps, "menu_cam__dirInited", bool) && !ECS_RO_COMP(menu_cam_init_es_comps, "menu_cam__shouldRotateTarget", bool)) )
      continue;
    menu_cam_init_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(menu_cam_init_es_comps, "menu_cam__initialDir", Point3)
    , ECS_RW_COMP(menu_cam_init_es_comps, "menu_cam__initialAngles", Point2)
    , ECS_RW_COMP(menu_cam_init_es_comps, "menu_cam__dirInited", bool)
    , ECS_RW_COMP(menu_cam_init_es_comps, "menu_cam__angles", Point2)
    , ECS_RW_COMP(menu_cam_init_es_comps, "menu_cam__angles_smooth_internal", Point2)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc menu_cam_init_es_es_desc
(
  "menu_cam_init_es",
  "prog/gameLibs/ecs/camera/menuCam.cpp.inl",
  ecs::EntitySystemOps(menu_cam_init_es_all),
  make_span(menu_cam_init_es_comps+0, 4)/*rw*/,
  make_span(menu_cam_init_es_comps+4, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"before_camera_sync","camera_set_sync");
static constexpr ecs::ComponentDesc menu_cam_es_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("menu_cam__reference_target_pos_internal"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("menu_cam__angles_smooth_internal"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("menu_cam__offset_smooth_internal"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("menu_cam__angles"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 6 ro components at [5]
  {ECS_HASH("menu_cam__target"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("menu_cam__offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("menu_cam__offsetMult"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("menu_cam__initialAngles"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("menu_cam__target_pos_threshold"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("menu_cam__shouldRotateTarget"), ecs::ComponentTypeInfo<bool>()}
};
static void menu_cam_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(!ECS_RO_COMP(menu_cam_es_comps, "menu_cam__shouldRotateTarget", bool)) )
      continue;
    menu_cam_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(menu_cam_es_comps, "menu_cam__target", ecs::EntityId)
    , ECS_RO_COMP(menu_cam_es_comps, "menu_cam__offset", Point3)
    , ECS_RO_COMP(menu_cam_es_comps, "menu_cam__offsetMult", Point3)
    , ECS_RO_COMP(menu_cam_es_comps, "menu_cam__initialAngles", Point2)
    , ECS_RO_COMP(menu_cam_es_comps, "menu_cam__target_pos_threshold", float)
    , ECS_RW_COMP(menu_cam_es_comps, "menu_cam__reference_target_pos_internal", Point3)
    , ECS_RW_COMP(menu_cam_es_comps, "menu_cam__angles_smooth_internal", Point2)
    , ECS_RW_COMP(menu_cam_es_comps, "menu_cam__offset_smooth_internal", Point3)
    , ECS_RW_COMP(menu_cam_es_comps, "menu_cam__angles", Point2)
    , ECS_RW_COMP(menu_cam_es_comps, "transform", TMatrix)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc menu_cam_es_es_desc
(
  "menu_cam_es",
  "prog/gameLibs/ecs/camera/menuCam.cpp.inl",
  ecs::EntitySystemOps(menu_cam_es_all),
  make_span(menu_cam_es_comps+0, 5)/*rw*/,
  make_span(menu_cam_es_comps+5, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"before_camera_sync","camera_set_sync");
static constexpr ecs::ComponentDesc menu_rotate_target_cam_init_es_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("menu_cam__lastTarget"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("menu_cam__initialTransform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("menu_cam__angles"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("menu_cam__angles_smooth_internal"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("menu_cam__dirInited"), ecs::ComponentTypeInfo<bool>()},
//start of 2 ro components at [5]
  {ECS_HASH("menu_cam__target"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("menu_cam__shouldRotateTarget"), ecs::ComponentTypeInfo<bool>()}
};
static void menu_rotate_target_cam_init_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(!ECS_RW_COMP(menu_rotate_target_cam_init_es_comps, "menu_cam__dirInited", bool) && ECS_RO_COMP(menu_rotate_target_cam_init_es_comps, "menu_cam__shouldRotateTarget", bool)) )
      continue;
    menu_rotate_target_cam_init_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(menu_rotate_target_cam_init_es_comps, "menu_cam__target", ecs::EntityId)
    , ECS_RW_COMP(menu_rotate_target_cam_init_es_comps, "menu_cam__lastTarget", ecs::EntityId)
    , ECS_RW_COMP(menu_rotate_target_cam_init_es_comps, "menu_cam__initialTransform", TMatrix)
    , ECS_RW_COMP(menu_rotate_target_cam_init_es_comps, "menu_cam__angles", Point2)
    , ECS_RW_COMP(menu_rotate_target_cam_init_es_comps, "menu_cam__angles_smooth_internal", Point2)
    , ECS_RW_COMP(menu_rotate_target_cam_init_es_comps, "menu_cam__dirInited", bool)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc menu_rotate_target_cam_init_es_es_desc
(
  "menu_rotate_target_cam_init_es",
  "prog/gameLibs/ecs/camera/menuCam.cpp.inl",
  ecs::EntitySystemOps(menu_rotate_target_cam_init_es_all),
  make_span(menu_rotate_target_cam_init_es_comps+0, 5)/*rw*/,
  make_span(menu_rotate_target_cam_init_es_comps+5, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"before_camera_sync,menu_rotate_target_cam_es","camera_set_sync,menu_cam_es");
static constexpr ecs::ComponentDesc menu_rotate_target_cam_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("menu_cam__angles"), ecs::ComponentTypeInfo<Point2>()},
//start of 5 ro components at [1]
  {ECS_HASH("menu_cam__target"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("menu_cam__initialTransform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("menu_cam__dirInited"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("menu_cam__shouldRotateTarget"), ecs::ComponentTypeInfo<bool>()}
};
static void menu_rotate_target_cam_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(ECS_RO_COMP(menu_rotate_target_cam_es_comps, "menu_cam__dirInited", bool) && ECS_RO_COMP(menu_rotate_target_cam_es_comps, "menu_cam__shouldRotateTarget", bool)) )
      continue;
    menu_rotate_target_cam_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(menu_rotate_target_cam_es_comps, "menu_cam__target", ecs::EntityId)
    , ECS_RO_COMP(menu_rotate_target_cam_es_comps, "menu_cam__initialTransform", TMatrix)
    , ECS_RW_COMP(menu_rotate_target_cam_es_comps, "menu_cam__angles", Point2)
    , ECS_RO_COMP(menu_rotate_target_cam_es_comps, "transform", TMatrix)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc menu_rotate_target_cam_es_es_desc
(
  "menu_rotate_target_cam_es",
  "prog/gameLibs/ecs/camera/menuCam.cpp.inl",
  ecs::EntitySystemOps(menu_rotate_target_cam_es_all),
  make_span(menu_rotate_target_cam_es_comps+0, 1)/*rw*/,
  make_span(menu_rotate_target_cam_es_comps+1, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"before_camera_sync","camera_set_sync");
static constexpr ecs::ComponentDesc menu_cam_target_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc menu_cam_target_ecs_query_desc
(
  "menu_cam_target_ecs_query",
  empty_span(),
  make_span(menu_cam_target_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void menu_cam_target_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, menu_cam_target_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(menu_cam_target_ecs_query_comps, "transform", TMatrix)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc menu_rotate_target_cam_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc menu_rotate_target_cam_ecs_query_desc
(
  "menu_rotate_target_cam_ecs_query",
  make_span(menu_rotate_target_cam_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void menu_rotate_target_cam_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, menu_rotate_target_cam_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(menu_rotate_target_cam_ecs_query_comps, "transform", TMatrix)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc menu_rotate_target_cam_with_offset_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 3 ro components at [1]
  {ECS_HASH("menu_item__boundingBoxCenter"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("menu_item__rotationOffset"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("menu_item__cameraCenterOffsetProportion"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc menu_rotate_target_cam_with_offset_ecs_query_desc
(
  "menu_rotate_target_cam_with_offset_ecs_query",
  make_span(menu_rotate_target_cam_with_offset_ecs_query_comps+0, 1)/*rw*/,
  make_span(menu_rotate_target_cam_with_offset_ecs_query_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void menu_rotate_target_cam_with_offset_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, menu_rotate_target_cam_with_offset_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(menu_rotate_target_cam_with_offset_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(menu_rotate_target_cam_with_offset_ecs_query_comps, "menu_item__boundingBoxCenter", Point3)
            , ECS_RO_COMP_OR(menu_rotate_target_cam_with_offset_ecs_query_comps, "menu_item__rotationOffset", Point3(Point3(0.f, 0.f, 0.f)))
            , ECS_RO_COMP_OR(menu_rotate_target_cam_with_offset_ecs_query_comps, "menu_item__cameraCenterOffsetProportion", float(1.0f))
            );

        }
    }
  );
}
