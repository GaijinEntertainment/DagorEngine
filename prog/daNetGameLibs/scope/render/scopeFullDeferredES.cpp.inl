// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/scopeAimRender/scopeAimRender.h>
#include "scopeFullDeferredNodes.h"

#include <daECS/core/entitySystem.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>

#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/rendererFeatures.h>
#include <render/world/frameGraphNodes/frameGraphNodes.h>
#include <render/world/wrDispatcher.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>

static void fill_scope_nodes(dag::Vector<dafg::NodeHandle> &nodes)
{
  nodes.push_back(makeScopeOpaqueNode());
  nodes.push_back(makeScopeTransNode());
  nodes.push_back(makeScopePrepassNode());
  nodes.push_back(makeScopeLensMaskNode());
  nodes.push_back(makeScopeVrsMaskNode());
  nodes.push_back(makeScopeCutDepthNode());
  nodes.push_back(makeRenderOpticsPrepassNode());
  nodes.push_back(makeRenderLensFrameNode());
  nodes.push_back(makeRenderLensOpticsNode());

  const bool alignGunInScope = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("scopeAlignGunInScope", false);

  nodes.push_back(makeScopeLensViewProjNode(alignGunInScope));
  nodes.push_back(makeRenderCrosshairNode());
  nodes.push_back(makeRenderCrosshairGUINode());

  nodes.push_back(makeRenderReflectionsNode());

  if (renderer_has_feature(FeatureRenderFlags::CAMERA_IN_CAMERA))
  {
    if (!alignGunInScope)
      LOGERR_ONCE("camera in camera feature is enabled but graphics/scopeAlignGunInScope is not.");
    makeScopeGunOpaqueNodes(nodes);

    if (WRDispatcher::isReadyToUse() && !WRDispatcher::hasHighResFx())
    {
      nodes.push_back(makeScopeDownsampleStencilNode("scope_downsample_stencil_for_fx", get_half_res_depth_with_water_producer_name(),
        get_half_res_depth_with_water_name()));
    }

    nodes.push_back(makeScopeDownsampleStencilNode("scope_downsample_stencil", "downsampled_depth_no_stencil", "downsampled_depth"));

    nodes.push_back(makeScopeHZBMask());
  }

  nodes.push_back(makeAimDofPrepareNode());
  nodes.push_back(makeAimDofRestoreNode());
  nodes.push_back(makeSetupScopeAimRenderingDataNode());
  nodes.push_back(makeGenAimRenderingDataNode());
  nodes.push_back(makeScopeLensEllipseNode());
}


ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady, ChangeRenderFeatures, SetFxQuality)
static void full_deferred_scope_render_features_changed_es_event_handler(const ecs::Event &evt,
  dag::Vector<dafg::NodeHandle> &scope__full_deferred_fg_nodes)
{
  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
  {
    if (!(changedFeatures->isFeatureChanged(CAMERA_IN_CAMERA) || changedFeatures->isFeatureChanged(UPSCALE_SAMPLING_TEX)))
      return;
  }
  scope__full_deferred_fg_nodes.clear();
  fill_scope_nodes(scope__full_deferred_fg_nodes);
}
