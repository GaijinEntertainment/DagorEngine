// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scopeAimRender.h"
#include "scopeMobileNodes.h"

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/rendererFeatures.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/world/frameGraphNodes/frameGraphNodes.h>

static void make_forward_scope_nodes(dabfg::NodeHandle &forwardSetupAimRenderingDataNode,
  dabfg::NodeHandle &scopeForwardPrepassNode,
  dabfg::NodeHandle &scopeForwardNode,
  dabfg::NodeHandle &scopeForwardLensMaskNode,
  dabfg::NodeHandle &scopeForwardVrsMaskNode,
  dabfg::NodeHandle &scopeForwardLensHoleNode,
  dabfg::NodeHandle &scopeLensMobileNode,
  dabfg::NodeHandle &setupScopeAimRenderingDataNode)
{
  forwardSetupAimRenderingDataNode = makeGenAimRenderingDataNode();
  scopeForwardPrepassNode = mk_scope_forward_node();
  scopeForwardNode = mk_scope_prepass_forward_node();
  scopeForwardLensMaskNode = mk_scope_lens_mask_forward_node();
  scopeForwardVrsMaskNode = mk_scope_vrs_mask_forward_node();
  scopeForwardLensHoleNode = mk_scope_lens_hole_forward_node();
  scopeLensMobileNode = mk_scope_lens_mobile_node();
  setupScopeAimRenderingDataNode = makeSetupScopeAimRenderingDataNode();
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
static void init_forward_scope_rendering_es_event_handler(const ecs::Event &,
  dabfg::NodeHandle &forward__setup_aim_rendering_data_node,
  dabfg::NodeHandle &scope__forward__prepass_node,
  dabfg::NodeHandle &scope__forward__node,
  dabfg::NodeHandle &scope__forward__lens_mask_node,
  dabfg::NodeHandle &scope__forward__vrs_mask_node,
  dabfg::NodeHandle &scope__forward__lens_hole_node,
  dabfg::NodeHandle &scope__lens_mobile_node,
  dabfg::NodeHandle &setup_scope_aim_rendering_data_node)
{
  if (!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING) || renderer_has_feature(FeatureRenderFlags::MOBILE_DEFERRED))
    return;

  make_forward_scope_nodes(forward__setup_aim_rendering_data_node, scope__forward__prepass_node, scope__forward__node,
    scope__forward__lens_mask_node, scope__forward__vrs_mask_node, scope__forward__lens_hole_node, scope__lens_mobile_node,
    setup_scope_aim_rendering_data_node);
}

ECS_TAG(render)
ECS_ON_EVENT(ChangeRenderFeatures)
static void forward_scope_render_features_changed_es_event_handler(const ecs::Event &evt,
  dabfg::NodeHandle &forward__setup_aim_rendering_data_node,
  dabfg::NodeHandle &scope__forward__prepass_node,
  dabfg::NodeHandle &scope__forward__node,
  dabfg::NodeHandle &scope__forward__lens_mask_node,
  dabfg::NodeHandle &scope__forward__vrs_mask_node,
  dabfg::NodeHandle &scope__forward__lens_hole_node,
  dabfg::NodeHandle &scope__lens_mobile_node,
  dabfg::NodeHandle &setup_scope_aim_rendering_data_node)
{
  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
    if (!(changedFeatures->isFeatureChanged(FeatureRenderFlags::FORWARD_RENDERING)))
      return;

  if (!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING) || renderer_has_feature(FeatureRenderFlags::MOBILE_DEFERRED))
  {
    forward__setup_aim_rendering_data_node = {};
    scope__forward__prepass_node = {};
    scope__forward__node = {};
    scope__forward__lens_mask_node = {};
    scope__forward__vrs_mask_node = {};
    scope__forward__lens_hole_node = {};
    scope__lens_mobile_node = {};
    return;
  }

  make_forward_scope_nodes(forward__setup_aim_rendering_data_node, scope__forward__prepass_node, scope__forward__node,
    scope__forward__lens_mask_node, scope__forward__vrs_mask_node, scope__forward__lens_hole_node, scope__lens_mobile_node,
    setup_scope_aim_rendering_data_node);
}
