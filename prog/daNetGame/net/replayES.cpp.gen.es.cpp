#include "replayES.cpp.inl"
ECS_DEF_PULL_VAR(replay);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc replay_save_key_frame_es_comps[] ={};
static void replay_save_key_frame_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  replay_save_key_frame_es(evt
        );
}
static ecs::EntitySystemDesc replay_save_key_frame_es_es_desc
(
  "replay_save_key_frame_es",
  "prog/daNetGame/net/replayES.cpp.inl",
  ecs::EntitySystemOps(nullptr, replay_save_key_frame_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RequestSaveKeyFrame>::build(),
  0
);
