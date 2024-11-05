// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scopeAimRender.h"
#include "scopeMobileNodes.h"

#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_driver.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/rendererFeatures.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/gbufferConsts.h>
#include <render/world/mobileDeferredResources.h>
#include <shaders/subpass_registers.hlsli>

struct ScopeMobileDeferredManager
{
  d3d::RenderPass *opaqueWithScope = nullptr;

  ScopeMobileDeferredManager() = default;
  ScopeMobileDeferredManager(ScopeMobileDeferredManager &&) = default;
  ~ScopeMobileDeferredManager();
  void destroy();
};

void ScopeMobileDeferredManager::destroy()
{
  if (opaqueWithScope)
  {
    d3d::delete_render_pass(opaqueWithScope);
    opaqueWithScope = nullptr;
  }
}

ScopeMobileDeferredManager::~ScopeMobileDeferredManager() { destroy(); }

ECS_DECLARE_RELOCATABLE_TYPE(ScopeMobileDeferredManager)
ECS_REGISTER_RELOCATABLE_TYPE(ScopeMobileDeferredManager, nullptr)

enum
{
  SCOPE_DEPTH_SP,
  SCOPE_MESH_SP,
  SCOPE_LENS_MASK_SP,
  SCOPE_DEPTH_CUT_SP,
  SCOPE_SP_COUNT
};

static void reset_mobile_deferred_scope_rendering(ScopeMobileDeferredManager &scopeMobileDeferredManager,
  dabfg::NodeHandle &scopeMobileDeferredBeginRpNode,
  dabfg::NodeHandle &scopeMobileDeferredPrepassNode,
  dabfg::NodeHandle &scopeMobileDeferredNode,
  dabfg::NodeHandle &scopeMobileDeferredLensMaskNode,
  dabfg::NodeHandle &scopeMobileDeferredDepthCutNode,
  dabfg::NodeHandle &scopeLensMobileNode,
  dabfg::NodeHandle &setupScopeAimRenderingDataNode)
{
  const uint32_t rtFmt = get_frame_render_target_format() | TEXCF_RTARGET;
  const uint32_t depthFmt = get_gbuffer_depth_format() | TEXCF_RTARGET;
  const uint32_t scopeMaskFmt = get_scope_mask_format() | TEXCF_RTARGET;
  const bool simplified = renderer_has_feature(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS);

  using MDR = MobileDeferredResources;
  auto targets = MDR::getTargetDescriptions(rtFmt, depthFmt, simplified);
  targets.push_back(RenderPassTargetDesc{nullptr, scopeMaskFmt, false});

  const int32_t opaqueBeginSp = SCOPE_SP_COUNT;
  const int32_t opaqueSp = opaqueBeginSp + MDR::OPAQUE_SP;
  const int32_t decalsSp = opaqueBeginSp + MDR::DECALS_SP;
  const int32_t resolveSp = opaqueBeginSp + MDR::RESOLVE_SP;
  const int32_t panoramaApplySp = opaqueBeginSp + MDR::PANORAMA_APPLY_SP;

  eastl::vector<RenderPassBind> binds = MDR::constructOpaqueBinds(simplified, opaqueSp, decalsSp, resolveSp, panoramaApplySp);
  if (simplified)
  {
    const int32_t iLensMask = MDR::SIMPLE_MAT_MAIN_RT_COUNT;
    binds.insert(binds.end(),
      {
        // depth prepass
        {MDR::SIMPLE_MAT_IDEPTH, SCOPE_DEPTH_SP, MDR::iDS, RP_TA_LOAD_CLEAR | RP_TA_SUBPASS_WRITE, MDR::rwDepth},
        // mesh
        {MDR::SIMPLE_MAT_IGBUF0, SCOPE_MESH_SP, SP_MAIN_PASS_MRT_GBUF0, RP_TA_LOAD_CLEAR | RP_TA_SUBPASS_WRITE, MDR::rwMrt},
        {MDR::SIMPLE_MAT_IGBUF1, SCOPE_MESH_SP, SP_MAIN_PASS_MRT_GBUF1, RP_TA_LOAD_CLEAR | RP_TA_SUBPASS_WRITE, MDR::rwMrt},
        {MDR::SIMPLE_MAT_IDEPTH, SCOPE_MESH_SP, MDR::iDS, RP_TA_SUBPASS_WRITE, MDR::rwDepth},
        // lens
        {iLensMask, SCOPE_LENS_MASK_SP, SP_LENS_MASK_MRT_LENS_MASK, RP_TA_LOAD_CLEAR | RP_TA_SUBPASS_WRITE, MDR::rwMrt},
        {MDR::SIMPLE_MAT_IDEPTH, SCOPE_LENS_MASK_SP, MDR::iDS, RP_TA_SUBPASS_READ, MDR::roDepth},
        {MDR::SIMPLE_MAT_IGBUF0, SCOPE_LENS_MASK_SP, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {MDR::SIMPLE_MAT_IGBUF1, SCOPE_LENS_MASK_SP, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        // depth cut
        {iLensMask, SCOPE_DEPTH_CUT_SP, SP_LENS_DEPTH_CUT_IA_LENS_MASK, RP_TA_SUBPASS_READ, MDR::inputAtt},
        {MDR::SIMPLE_MAT_IDEPTH, SCOPE_DEPTH_CUT_SP, MDR::iDS, RP_TA_SUBPASS_WRITE, MDR::rwDepth},
        {MDR::SIMPLE_MAT_IGBUF0, SCOPE_DEPTH_CUT_SP, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {MDR::SIMPLE_MAT_IGBUF1, SCOPE_DEPTH_CUT_SP, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        // keep lens mask until the end of the rp
        {iLensMask, opaqueSp, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {iLensMask, decalsSp, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {iLensMask, resolveSp, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {iLensMask, panoramaApplySp, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        // end
        {iLensMask, MDR::iEX, 0, RP_TA_STORE_WRITE, MDR::inputAtt},
      });
  }
  else
  {
    const int32_t iLensMask = MDR::FULL_MAT_MAIN_RT_COUNT;
    binds.insert(binds.end(),
      {
        // depth prepass
        {MDR::FULL_MAT_IDEPTH, SCOPE_DEPTH_SP, MDR::iDS, RP_TA_LOAD_CLEAR | RP_TA_SUBPASS_WRITE, MDR::rwDepth},
        // mesh
        {MDR::FULL_MAT_IGBUF0, SCOPE_MESH_SP, SP_MAIN_PASS_MRT_GBUF0, RP_TA_LOAD_CLEAR | RP_TA_SUBPASS_WRITE, MDR::rwMrt},
        {MDR::FULL_MAT_IGBUF1, SCOPE_MESH_SP, SP_MAIN_PASS_MRT_GBUF1, RP_TA_LOAD_CLEAR | RP_TA_SUBPASS_WRITE, MDR::rwMrt},
        {MDR::FULL_MAT_IGBUF2, SCOPE_MESH_SP, SP_MAIN_PASS_MRT_GBUF2, RP_TA_LOAD_CLEAR | RP_TA_SUBPASS_WRITE, MDR::rwMrt},
        {MDR::FULL_MAT_IDEPTH, SCOPE_MESH_SP, MDR::iDS, RP_TA_SUBPASS_WRITE, MDR::rwDepth},
        // lens
        {iLensMask, SCOPE_LENS_MASK_SP, SP_LENS_MASK_MRT_LENS_MASK, RP_TA_LOAD_CLEAR | RP_TA_SUBPASS_WRITE, MDR::rwMrt},
        {MDR::FULL_MAT_IDEPTH, SCOPE_LENS_MASK_SP, MDR::iDS, RP_TA_SUBPASS_READ, MDR::roDepth},
        {MDR::FULL_MAT_IGBUF0, SCOPE_LENS_MASK_SP, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {MDR::FULL_MAT_IGBUF1, SCOPE_LENS_MASK_SP, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {MDR::FULL_MAT_IGBUF2, SCOPE_LENS_MASK_SP, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        // depth cut
        {iLensMask, SCOPE_DEPTH_CUT_SP, SP_LENS_DEPTH_CUT_IA_LENS_MASK, RP_TA_SUBPASS_READ, MDR::inputAtt},
        {MDR::FULL_MAT_IDEPTH, SCOPE_DEPTH_CUT_SP, MDR::iDS, RP_TA_SUBPASS_WRITE, MDR::rwDepth},
        {MDR::FULL_MAT_IGBUF0, SCOPE_DEPTH_CUT_SP, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {MDR::FULL_MAT_IGBUF1, SCOPE_DEPTH_CUT_SP, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {MDR::FULL_MAT_IGBUF2, SCOPE_DEPTH_CUT_SP, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        // keep lens mask until the end of the rp
        {iLensMask, opaqueSp, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {iLensMask, decalsSp, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {iLensMask, resolveSp, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        {iLensMask, panoramaApplySp, 0, RP_TA_SUBPASS_KEEP, MDR::rwMrt},
        // end
        {iLensMask, MDR::iEX, 0, RP_TA_STORE_WRITE, MDR::inputAtt},
      });
  }

  scopeMobileDeferredManager.opaqueWithScope = d3d::create_render_pass(
    {"opaque_with_scope", (uint32_t)targets.size(), (uint32_t)binds.size(), targets.data(), binds.data(), SP_REG_BASE});

  scopeMobileDeferredBeginRpNode = mk_scope_begin_rp_mobile_node(scopeMobileDeferredManager.opaqueWithScope);
  scopeMobileDeferredPrepassNode = mk_scope_prepass_mobile_node();
  scopeMobileDeferredNode = mk_scope_mobile_node();
  scopeMobileDeferredLensMaskNode = mk_scope_lens_mask_mobile_node();
  scopeMobileDeferredDepthCutNode = mk_scope_depth_cut_mobile_node();
  scopeLensMobileNode = mk_scope_lens_mobile_node();
  setupScopeAimRenderingDataNode = makeSetupScopeAimRenderingDataNode();
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
static void create_mobile_deferred_scope_render_pass_es_event_handler(const ecs::Event &,
  ScopeMobileDeferredManager &scope__mobile_deferred__manager,
  dabfg::NodeHandle &scope__mobile_deferred__begin_rp_node,
  dabfg::NodeHandle &scope__mobile_deferred__prepass_node,
  dabfg::NodeHandle &scope__mobile_deferred__node,
  dabfg::NodeHandle &scope__mobile_deferred__lens_mask_node,
  dabfg::NodeHandle &scope__mobile_deferred__depth_cut_node,
  dabfg::NodeHandle &scope__lens_mobile_node,
  dabfg::NodeHandle &setup_scope_aim_rendering_data_node)
{
  if (!renderer_has_feature(FeatureRenderFlags::MOBILE_DEFERRED))
    return;

  reset_mobile_deferred_scope_rendering(scope__mobile_deferred__manager, scope__mobile_deferred__begin_rp_node,
    scope__mobile_deferred__prepass_node, scope__mobile_deferred__node, scope__mobile_deferred__lens_mask_node,
    scope__mobile_deferred__depth_cut_node, scope__lens_mobile_node, setup_scope_aim_rendering_data_node);
}

ECS_TAG(render)
ECS_ON_EVENT(ChangeRenderFeatures)
static void mobile_deferred_scope_render_features_changed_es_event_handler(const ecs::Event &evt,
  ScopeMobileDeferredManager &scope__mobile_deferred__manager,
  dabfg::NodeHandle &scope__mobile_deferred__begin_rp_node,
  dabfg::NodeHandle &scope__mobile_deferred__prepass_node,
  dabfg::NodeHandle &scope__mobile_deferred__node,
  dabfg::NodeHandle &scope__mobile_deferred__lens_mask_node,
  dabfg::NodeHandle &scope__mobile_deferred__depth_cut_node,
  dabfg::NodeHandle &scope__lens_mobile_node,
  dabfg::NodeHandle &setup_scope_aim_rendering_data_node)
{
  auto *changedFeatures = evt.cast<ChangeRenderFeatures>();
  if (!changedFeatures)
    return;

  if (!changedFeatures->isFeatureChanged(FeatureRenderFlags::MOBILE_DEFERRED) &&
      !changedFeatures->isFeatureChanged(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS))
    return;

  if (!renderer_has_feature(FeatureRenderFlags::MOBILE_DEFERRED))
  {
    scope__mobile_deferred__manager.destroy();
    scope__mobile_deferred__begin_rp_node = {};
    scope__mobile_deferred__prepass_node = {};
    scope__mobile_deferred__node = {};
    scope__mobile_deferred__lens_mask_node = {};
    scope__mobile_deferred__depth_cut_node = {};
    scope__lens_mobile_node = {};
    setup_scope_aim_rendering_data_node = {};
    return;
  }

  reset_mobile_deferred_scope_rendering(scope__mobile_deferred__manager, scope__mobile_deferred__begin_rp_node,
    scope__mobile_deferred__prepass_node, scope__mobile_deferred__node, scope__mobile_deferred__lens_mask_node,
    scope__mobile_deferred__depth_cut_node, scope__lens_mobile_node, setup_scope_aim_rendering_data_node);
}
