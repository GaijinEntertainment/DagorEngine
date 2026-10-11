// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "neuralRenderingES.cpp.inl"
ECS_DEF_PULL_VAR(neuralRendering);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc neural_rendering_nodes_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("render_settings__neuralRendering"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__antialiasing_mode"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void neural_rendering_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    neural_rendering_nodes_es(evt
        , components.manager()
    , ECS_RO_COMP(neural_rendering_nodes_es_comps, "render_settings__neuralRendering", bool)
    , ECS_RO_COMP(neural_rendering_nodes_es_comps, "render_settings__antialiasing_mode", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc neural_rendering_nodes_es_es_desc
(
  "neural_rendering_nodes_es",
  "prog/daNetGame/render/neuralRenderingES.cpp.inl",
  ecs::EntitySystemOps(nullptr, neural_rendering_nodes_es_all_events),
  empty_span(),
  make_span(neural_rendering_nodes_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeaturesEarly,
                       OnRenderSettingsReady>::build(),
  0
,"render","render_settings__antialiasing_mode,render_settings__neuralRendering");
//static constexpr ecs::ComponentDesc neural_rendering_ensure_passthrough_es_comps[] ={};
static void neural_rendering_ensure_passthrough_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  neural_rendering_ensure_passthrough_es(evt
        );
}
static ecs::EntitySystemDesc neural_rendering_ensure_passthrough_es_es_desc
(
  "neural_rendering_ensure_passthrough_es",
  "prog/daNetGame/render/neuralRenderingES.cpp.inl",
  ecs::EntitySystemOps(nullptr, neural_rendering_ensure_passthrough_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc neural_rendering_control_mask_node_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("neural_rendering__control_mask_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
//start of 1 ro components at [1]
  {ECS_HASH("neural_rendering__autoMask"), ecs::ComponentTypeInfo<bool>()}
};
static void neural_rendering_control_mask_node_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    neural_rendering_control_mask_node_es(evt
        , ECS_RO_COMP(neural_rendering_control_mask_node_es_comps, "neural_rendering__autoMask", bool)
    , ECS_RW_COMP(neural_rendering_control_mask_node_es_comps, "neural_rendering__control_mask_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc neural_rendering_control_mask_node_es_es_desc
(
  "neural_rendering_control_mask_node_es",
  "prog/daNetGame/render/neuralRenderingES.cpp.inl",
  ecs::EntitySystemOps(nullptr, neural_rendering_control_mask_node_es_all_events),
  make_span(neural_rendering_control_mask_node_es_comps+0, 1)/*rw*/,
  make_span(neural_rendering_control_mask_node_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","neural_rendering__autoMask");
static constexpr ecs::ComponentDesc neural_rendering_params_es_comps[] =
{
//start of 10 ro components at [0]
  {ECS_HASH("neural_rendering__style"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("neural_rendering__preset"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("neural_rendering__intensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("neural_rendering__localToneStrength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("neural_rendering__localStructureStrength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("neural_rendering__autoMask"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("neural_rendering__skinStructureStrength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("neural_rendering__controlMaskDefault"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("neural_rendering__controlMaskSkin"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("neural_rendering__controlMaskFoliage"), ecs::ComponentTypeInfo<Point4>()}
};
static void neural_rendering_params_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    neural_rendering_params_es(evt
        , ECS_RO_COMP(neural_rendering_params_es_comps, "neural_rendering__style", int)
    , ECS_RO_COMP(neural_rendering_params_es_comps, "neural_rendering__preset", ecs::string)
    , ECS_RO_COMP(neural_rendering_params_es_comps, "neural_rendering__intensity", float)
    , ECS_RO_COMP(neural_rendering_params_es_comps, "neural_rendering__localToneStrength", float)
    , ECS_RO_COMP(neural_rendering_params_es_comps, "neural_rendering__localStructureStrength", float)
    , ECS_RO_COMP(neural_rendering_params_es_comps, "neural_rendering__autoMask", bool)
    , ECS_RO_COMP(neural_rendering_params_es_comps, "neural_rendering__skinStructureStrength", float)
    , ECS_RO_COMP(neural_rendering_params_es_comps, "neural_rendering__controlMaskDefault", Point4)
    , ECS_RO_COMP(neural_rendering_params_es_comps, "neural_rendering__controlMaskSkin", Point4)
    , ECS_RO_COMP(neural_rendering_params_es_comps, "neural_rendering__controlMaskFoliage", Point4)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc neural_rendering_params_es_es_desc
(
  "neural_rendering_params_es",
  "prog/daNetGame/render/neuralRenderingES.cpp.inl",
  ecs::EntitySystemOps(nullptr, neural_rendering_params_es_all_events),
  empty_span(),
  make_span(neural_rendering_params_es_comps+0, 10)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","neural_rendering__autoMask,neural_rendering__controlMaskDefault,neural_rendering__controlMaskFoliage,neural_rendering__controlMaskSkin,neural_rendering__intensity,neural_rendering__localStructureStrength,neural_rendering__localToneStrength,neural_rendering__preset,neural_rendering__skinStructureStrength,neural_rendering__style");
static constexpr ecs::ComponentDesc recreate_neural_rendering_nodes_ecs_query_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("neural_rendering__tonemap_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("neural_rendering__dlss_nr_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("neural_rendering__inverse_tonemap_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("neural_rendering__control_mask_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
//start of 1 ro components at [4]
  {ECS_HASH("neural_rendering__autoMask"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc recreate_neural_rendering_nodes_ecs_query_desc
(
  "recreate_neural_rendering_nodes_ecs_query",
  make_span(recreate_neural_rendering_nodes_ecs_query_comps+0, 4)/*rw*/,
  make_span(recreate_neural_rendering_nodes_ecs_query_comps+4, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void recreate_neural_rendering_nodes_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, recreate_neural_rendering_nodes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(recreate_neural_rendering_nodes_ecs_query_comps, "neural_rendering__tonemap_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_neural_rendering_nodes_ecs_query_comps, "neural_rendering__dlss_nr_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_neural_rendering_nodes_ecs_query_comps, "neural_rendering__inverse_tonemap_node", dafg::NodeHandle)
            , ECS_RW_COMP(recreate_neural_rendering_nodes_ecs_query_comps, "neural_rendering__control_mask_node", dafg::NodeHandle)
            , ECS_RO_COMP(recreate_neural_rendering_nodes_ecs_query_comps, "neural_rendering__autoMask", bool)
            );

        }while (++comp != compE);
    }
  );
}
