// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scopeAimRender.h"
#include "scopeFullDeferredNodes.h"

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/rendererFeatures.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/world/frameGraphNodes/frameGraphNodes.h>

static void make_full_deferred_scope_nodes(dabfg::NodeHandle &scopeFullDeferredOpaqueNode,
  dabfg::NodeHandle &scopeFullDeferredPrepassNode,
  dabfg::NodeHandle &scopeFullDeferredLensMaskNode,
  dabfg::NodeHandle &scopeFullDeferredVrsMaskNode,
  dabfg::NodeHandle &scopeFullDeferredCutDepthNode,
  dabfg::NodeHandle &scopeFullDeferredRenderLensFrameNode,
  dabfg::NodeHandle &scopeFullDeferredRenderLensOpticsNode,
  dabfg::NodeHandle &scopeFullDeferredRenderCrossharNode,
  dabfg::NodeHandle &aimDofPrepareNode,
  dabfg::NodeHandle &aimDofRestoreNode,
  dabfg::NodeHandle &setupScopeAimRenderingDataNode,
  dabfg::NodeHandle &setupAimRenderingDataNode)
{
  scopeFullDeferredOpaqueNode = makeScopeOpaqueNode();
  scopeFullDeferredPrepassNode = makeScopePrepassNode();
  scopeFullDeferredLensMaskNode = makeScopeLensMaskNode();
  scopeFullDeferredVrsMaskNode = makeScopeVrsMaskNode();
  scopeFullDeferredCutDepthNode = makeScopeCutDepthNode();
  scopeFullDeferredRenderLensFrameNode = makeRenderLensFrameNode();
  scopeFullDeferredRenderLensOpticsNode = makeRenderLensOpticsNode();
  scopeFullDeferredRenderCrossharNode = makeRenderCrosshairNode();
  aimDofPrepareNode = makeAimDofPrepareNode();
  aimDofRestoreNode = makeAimDofRestoreNode();
  setupScopeAimRenderingDataNode = makeSetupScopeAimRenderingDataNode();
  setupAimRenderingDataNode = makeGenAimRenderingDataNode();
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
static void init_full_deferred_scope_rendering_es_event_handler(const ecs::Event &,
  dabfg::NodeHandle &scope__full_deferred__opaque_node,
  dabfg::NodeHandle &scope__full_deferred__prepass_node,
  dabfg::NodeHandle &scope__full_deferred__lens_mask_node,
  dabfg::NodeHandle &scope__full_deferred__vrs_mask_node,
  dabfg::NodeHandle &scope__full_deferred__cut_depth_node,
  dabfg::NodeHandle &scope__full_deferred__crosshair_node,
  dabfg::NodeHandle &scope__full_deferred__render_lens_frame_node,
  dabfg::NodeHandle &scope__full_deferred__render_lens_optics_node,
  dabfg::NodeHandle &aim_dof_prepare_node,
  dabfg::NodeHandle &aim_dof_restore_node,
  dabfg::NodeHandle &setup_scope_aim_rendering_data_node,
  dabfg::NodeHandle &setup_aim_rendering_data_node)
{
  if (renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING) && renderer_has_feature(FeatureRenderFlags::MOBILE_DEFERRED))
    return;

  make_full_deferred_scope_nodes(scope__full_deferred__opaque_node, scope__full_deferred__prepass_node,
    scope__full_deferred__lens_mask_node, scope__full_deferred__vrs_mask_node, scope__full_deferred__cut_depth_node,
    scope__full_deferred__crosshair_node, scope__full_deferred__render_lens_frame_node, scope__full_deferred__render_lens_optics_node,
    aim_dof_prepare_node, aim_dof_restore_node, setup_scope_aim_rendering_data_node, setup_aim_rendering_data_node);
}

ECS_TAG(render)
ECS_ON_EVENT(ChangeRenderFeatures)
static void full_deferred_scope_render_features_changed_es_event_handler(const ecs::Event &evt,
  dabfg::NodeHandle &scope__full_deferred__opaque_node,
  dabfg::NodeHandle &scope__full_deferred__prepass_node,
  dabfg::NodeHandle &scope__full_deferred__lens_mask_node,
  dabfg::NodeHandle &scope__full_deferred__vrs_mask_node,
  dabfg::NodeHandle &scope__full_deferred__cut_depth_node,
  dabfg::NodeHandle &scope__full_deferred__render_lens_frame_node,
  dabfg::NodeHandle &scope__full_deferred__render_lens_optics_node,
  dabfg::NodeHandle &scope__full_deferred__crosshair_node,
  dabfg::NodeHandle &aim_dof_prepare_node,
  dabfg::NodeHandle &aim_dof_restore_node,
  dabfg::NodeHandle &setup_scope_aim_rendering_data_node,
  dabfg::NodeHandle &setup_aim_rendering_data_node)
{
  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
    if (!(changedFeatures->isFeatureChanged(FeatureRenderFlags::FULL_DEFERRED)))
      return;

  if (renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING) && renderer_has_feature(FeatureRenderFlags::MOBILE_DEFERRED))
  {
    scope__full_deferred__opaque_node = {};
    scope__full_deferred__prepass_node = {};
    scope__full_deferred__lens_mask_node = {};
    scope__full_deferred__vrs_mask_node = {};
    scope__full_deferred__cut_depth_node = {};
    scope__full_deferred__render_lens_frame_node = {};
    scope__full_deferred__render_lens_optics_node = {};
    scope__full_deferred__crosshair_node = {};
    aim_dof_prepare_node = {};
    aim_dof_restore_node = {};
    setup_scope_aim_rendering_data_node = {};
    setup_aim_rendering_data_node = {};
    return;
  }

  make_full_deferred_scope_nodes(scope__full_deferred__opaque_node, scope__full_deferred__prepass_node,
    scope__full_deferred__lens_mask_node, scope__full_deferred__vrs_mask_node, scope__full_deferred__cut_depth_node,
    scope__full_deferred__render_lens_frame_node, scope__full_deferred__render_lens_optics_node, scope__full_deferred__crosshair_node,
    aim_dof_prepare_node, aim_dof_restore_node, setup_scope_aim_rendering_data_node, setup_aim_rendering_data_node);
}
