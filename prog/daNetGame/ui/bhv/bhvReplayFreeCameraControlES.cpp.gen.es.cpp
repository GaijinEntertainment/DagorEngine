// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "bhvReplayFreeCameraControlES.cpp.inl"
ECS_DEF_PULL_VAR(bhvReplayFreeCameraControl);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc move_replay_camera_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("free_cam_input__moveUI"), ecs::ComponentTypeInfo<Point2>()}
};
static ecs::CompileTimeQueryDesc move_replay_camera_ecs_query_desc
(
  "move_replay_camera_ecs_query",
  make_span(move_replay_camera_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void move_replay_camera_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, move_replay_camera_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(move_replay_camera_ecs_query_comps, "free_cam_input__moveUI", Point2)
            );

        }while (++comp != compE);
    }
  );
}
