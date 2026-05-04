// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <render/renderEvent.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>

#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"

#include <render/world/partitionSphere.h>
ECS_REGISTER_TYPE(PartitionSphere, nullptr);

dafg::NodeHandle makeAcesFxTransparentPartitionNode();
dafg::NodeHandle makeAcesFxEarlyTransparentNode();
eastl::fixed_vector<dafg::NodeHandle, 2, false> makeAcesFxEarlyLowresTransparentNodes();

dafg::NodeHandle makeRendinstTransparentEarlyNode();

ECS_TAG(render)
ECS_ON_EVENT(on_appear, BeforeLoadLevel, SetFxQuality)
static void init_early_particles_node_es_event_handler(const ecs::Event &,
  dafg::NodeHandle &early_rendinst_node,
  dafg::NodeHandle &particles_partition_node,
  dafg::NodeHandle &early_particles_node,
  dafg::NodeHandle &early_particles_lowres_prepare_node,
  dafg::NodeHandle &early_particles_lowres_render_node,
  bool &are_early_particle_nodes_created)
{
  early_rendinst_node = dafg::NodeHandle();

  particles_partition_node = dafg::NodeHandle();
  early_particles_node = dafg::NodeHandle();
  early_particles_lowres_prepare_node = dafg::NodeHandle();
  early_particles_lowres_render_node = dafg::NodeHandle();

  auto wrPtr = static_cast<WorldRenderer *>(get_world_renderer());
  if (!wrPtr)
    return;

  early_rendinst_node = makeRendinstTransparentEarlyNode();

  const bool shouldHaveLowres = wrPtr->getFxRtOverride() != FX_RT_OVERRIDE_HIGHRES;
  particles_partition_node = makeAcesFxTransparentPartitionNode();
  early_particles_node = makeAcesFxEarlyTransparentNode();
  are_early_particle_nodes_created = true;
  if (shouldHaveLowres)
  {
    eastl::fixed_vector<dafg::NodeHandle, 2, false> nodes = makeAcesFxEarlyLowresTransparentNodes();
    early_particles_lowres_prepare_node = eastl::move(nodes[0]);
    early_particles_lowres_render_node = eastl::move(nodes[1]);
  }
}
