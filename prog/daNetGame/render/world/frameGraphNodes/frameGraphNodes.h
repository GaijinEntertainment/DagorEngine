// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>
#include <EASTL/unique_ptr.h>
#include <render/ssao_common.h>
#include <render/deferredRenderer.h>
#include <render/viewDependentResource.h>
#include <render/world/gbufferConsts.h>


namespace dabfg
{
class NodeHandle;
}

namespace resource_slot
{
struct NodeHandleWithSlotsAccess;
}

enum class SSRQuality;

// pImpl-like construction

dabfg::NodeHandle makePrepareGbufferNode(
  uint32_t global_flags, uint32_t gbuf_cnt, eastl::span<uint32_t> main_gbuf_fmts, int depth_format, bool has_motion_vectors);

dabfg::NodeHandle makeHideAnimcharNodesEcsNode();

dabfg::NodeHandle makeGenAimRenderingDataNode();
dabfg::NodeHandle makeTargetRenameBeforeMotionBlurNode();
dabfg::NodeHandle makePrepareLightsNode();
dabfg::NodeHandle makePrepareTiledLightsNode();
dabfg::NodeHandle makePrepareDepthForPostFxNode(bool withHistory);
dabfg::NodeHandle makePrepareDepthAfterTransparent();
dabfg::NodeHandle makePrepareMotionVectorsAfterTransparent(bool withHistory);
dabfg::NodeHandle makeDelayedRenderCubeNode();
dabfg::NodeHandle makeDelayedRenderDepthAboveNode();

dabfg::NodeHandle makeAsyncAnimcharRenderingStartNode(bool has_motion_vectors);

eastl::fixed_vector<dabfg::NodeHandle, 2> makeControlOpaqueDynamicsNodes(const char *prev_region_ns);
dabfg::NodeHandle makeOpaqueDynamicsNode();
eastl::fixed_vector<dabfg::NodeHandle, 3> makeControlOpaqueStaticsNodes(const char *prev_region_ns);
eastl::fixed_vector<dabfg::NodeHandle, 8> makeOpaqueStaticNodes();

// Early flag forces ground to be rendered before all other statics
// used for tiled architectures
dabfg::NodeHandle makeGroundNode(bool early);

dabfg::NodeHandle makeCreateVrsTextureNode();

dabfg::NodeHandle makeDecalsOnStaticNode();

dabfg::NodeHandle makeOpaqueInWorldPanelsNode();

dabfg::NodeHandle makeHzbResolveNode(bool beforeDynamics);

dabfg::NodeHandle makeAcesFxUpdateNode();

dabfg::NodeHandle makeAcesFxOpaqueNode();

dabfg::NodeHandle makeDecalsOnDynamicNode();

dabfg::NodeHandle makeResolveMotionVectorsNode();

dabfg::NodeHandle makeReactiveMaskClearNode();

struct DownsampleNodeParams
{
  uint32_t downsampledTexturesMipCount;
  bool needCheckerboard;
  bool needNormals;
  bool needMotionVectors;
  bool storeDownsampledTexturesInEsram;
};

eastl::array<dabfg::NodeHandle, 3> makeDownsampleDepthNodes(const DownsampleNodeParams &params);

eastl::array<dabfg::NodeHandle, 2> makeSceneShadowPassNodes();

eastl::array<dabfg::NodeHandle, 6> makeVolumetricLightsNodes();

enum
{
  SSR_DENOISER_NONE = 0,
  SSR_DENOISER_SIMPLE = 1,
  SSR_DENOISER_NRD = 2
};
eastl::fixed_vector<dabfg::NodeHandle, 5> makeScreenSpaceReflectionNodes(
  int w, int h, bool is_fullres, int denoiser, uint32_t fmt, SSRQuality ssr_quality);
void closeSSR();

enum class AoAlgo
{
  SSAO,
  GTAO
};
dabfg::NodeHandle makeAmbientOcclusionNode(AoAlgo algo, int w, int h, uint32_t flags = SsaoCreationFlags::SSAO_NONE);


dabfg::NodeHandle makeGiCalcNode();
dabfg::NodeHandle makeGiFeedbackNode();
dabfg::NodeHandle makeGiScreenDebugNode();
dabfg::NodeHandle makeGiScreenDebugDepthNode();

dabfg::NodeHandle makeDeferredLightNode(bool need_reprojection);
dabfg::NodeHandle makeResolveGbufferNode(const char *resolve_pshader_name,
  const char *resolve_cshader_name,
  const char *classify_cshader_name,
  const ShadingResolver::PermutationsDesc &permutations_desc);

dabfg::NodeHandle makePrepareWaterNode();

eastl::fixed_vector<dabfg::NodeHandle, 3> makeEnvironmentNodes();

enum class WaterRenderMode
{
  EARLY_BEFORE_ENVI,
  EARLY_AFTER_ENVI,
  LATE,
  COUNT,
  COUNT_WITH_RENAMES
};
extern const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT_WITH_RENAMES)> WATER_SSR_DEPTH_TEX;
extern const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT_WITH_RENAMES)> WATER_SSR_COLOR_TEX;
extern const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT_WITH_RENAMES)> WATER_SSR_STRENGTH_TEX;
extern const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_REFLECT_DIR_TEX;

dabfg::NodeHandle makeWaterNode(WaterRenderMode mode);
dabfg::NodeHandle makeWaterSSRNode(WaterRenderMode mode);

dabfg::NodeHandle makeDownsampleDepthWithWaterNode();

dabfg::NodeHandle makeOcclusionFinalizeNode();

dabfg::NodeHandle makeRendinstTransparentNode();

dabfg::NodeHandle makeTransparentEcsNode();

dabfg::NodeHandle makeTransparentParticlesNode();
dabfg::NodeHandle makeTransparentParticlesLowresPrepareNode();

dabfg::NodeHandle makeTranslucentInWorldPanelsNode();

dabfg::NodeHandle makeTransparentSceneLateNode();
dabfg::NodeHandle makeMainHeroTransNode();

dabfg::NodeHandle makeUnderWaterFogNode();

dabfg::NodeHandle makeUnderWaterParticlesNode();

dabfg::NodeHandle makeDepthWithTransparencyNode();

dabfg::NodeHandle makeDownsampleDepthWithTransparencyNode();

resource_slot::NodeHandleWithSlotsAccess makePostFxNode();

dabfg::NodeHandle makeShowSceneDebugNode();

dabfg::NodeHandle makeSSAANode();

eastl::fixed_vector<dabfg::NodeHandle, 3> makeFsrNodes();

dabfg::NodeHandle makeFXAANode(const char *target_name, bool external_target);

dabfg::NodeHandle makeStaticUpsampleNode(const char *source_name);

dabfg::NodeHandle makeAfterPostFxEcsEventNode();

resource_slot::NodeHandleWithSlotsAccess makePostFxInputSlotProviderNode();

resource_slot::NodeHandleWithSlotsAccess makePreparePostFxNode();

dabfg::NodeHandle makeFrameBeforeDistortionProducerNode();

dabfg::NodeHandle makeDistortionFxNode();

dabfg::NodeHandle makeRenameDepthNode();

eastl::fixed_vector<dabfg::NodeHandle, 5, false> makeSubsamplingNodes(bool sub_sampling, bool super_sampling);

dabfg::NodeHandle makeShaderAssertNode();

dabfg::NodeHandle makeFrameToPresentProducerNode();

eastl::array<dabfg::NodeHandle, 2> makeExternalFinalFrameControlNodes(bool requires_multisampling);

dabfg::NodeHandle makePostfxTargetProducerNode(bool requires_multisampling);

dabfg::NodeHandle makePrepareForPostfxNoAANode();

dabfg::NodeHandle makeAfterWorldRenderNode();

dabfg::NodeHandle makeFrameDownsampleNode();
dabfg::NodeHandle makeFrameDownsampleSamplerNode();

dabfg::NodeHandle makeNoFxFrameNode();

eastl::fixed_vector<dabfg::NodeHandle, 2> makeBeforeUIControlNodes();
dabfg::NodeHandle makeUIRenderNode(bool withHistory);
dabfg::NodeHandle makeUIBlendNode();

dabfg::NodeHandle makeRendinstUpdateNode();
