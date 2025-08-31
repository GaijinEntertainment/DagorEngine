// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>
#include <EASTL/unique_ptr.h>
#include <render/ssao_common.h>
#include <render/deferredRenderer.h>
#include <render/viewDependentResource.h>
#include <render/world/gbufferConsts.h>
#include <render/renderer.h>

namespace dafg
{
class NodeHandle;
}

namespace resource_slot
{
struct NodeHandleWithSlotsAccess;
}

enum class SSRQuality;

// pImpl-like construction

dafg::NodeHandle makePrepareGbufferDepthNode(uint32_t global_flags, int depth_format);
dafg::NodeHandle makePrepareGbufferNode(
  uint32_t global_flags, uint32_t gbuf_cnt, eastl::span<uint32_t> main_gbuf_fmts, bool has_motion_vectors);

dafg::NodeHandle makeHideAnimcharNodesEcsNode();

dafg::NodeHandle makeGenAimRenderingDataNode();
dafg::NodeHandle makeTargetRenameBeforeMotionBlurNode();
dafg::NodeHandle makePrepareLightsNode();
dafg::NodeHandle makePrepareTiledLightsNode();
dafg::NodeHandle makePrepareDepthForPostFxNode(bool withHistory);
dafg::NodeHandle makePrepareDepthAfterTransparent();
dafg::NodeHandle makePrepareMotionVectorsAfterTransparent(bool withHistory);
dafg::NodeHandle makeDelayedRenderCubeNode();
dafg::NodeHandle makeDelayedRenderDepthAboveNode();

dafg::NodeHandle makeAsyncAnimcharRenderingStartNode(bool has_motion_vectors);

dafg::NodeHandle makeGrassGenerationNode();

eastl::fixed_vector<dafg::NodeHandle, 3> makeControlOpaqueDynamicsNodes(const char *prev_region_ns);
dafg::NodeHandle makeOpaqueDynamicsNode();
eastl::fixed_vector<dafg::NodeHandle, 4> makeControlOpaqueStaticsNodes(const char *prev_region_ns);
eastl::fixed_vector<dafg::NodeHandle, 8> makeOpaqueStaticNodes(bool prepassEnabled);

// Early flag forces ground to be rendered before all other statics
// used for tiled architectures
dafg::NodeHandle makeGroundNode(bool early);

eastl::fixed_vector<dafg::NodeHandle, 3> makeCreateVrsTextureNode();

dafg::NodeHandle makeDecalsOnStaticNode();

dafg::NodeHandle makeOpaqueInWorldPanelsNode();

dafg::NodeHandle makeHzbResolveNode(bool beforeDynamics);

dafg::NodeHandle makeAcesFxUpdateNode();

dafg::NodeHandle makeAcesFxOpaqueNode();

dafg::NodeHandle makeDecalsOnDynamicNode();

dafg::NodeHandle makeReactiveMaskClearNode();

struct DownsampleNodeParams
{
  uint32_t downsampledTexturesMipCount;
  bool needCheckerboard;
  bool needNormals;
  bool needMotionVectors;
  bool storeDownsampledTexturesInEsram;
};

eastl::array<dafg::NodeHandle, 3> makeDownsampleDepthNodes(const DownsampleNodeParams &params);

eastl::array<dafg::NodeHandle, 2> makeSceneShadowPassNodes(const DataBlock *level_blk);

eastl::array<dafg::NodeHandle, 9> makeVolumetricLightsNodes();

enum
{
  SSR_DENOISER_NONE = 0,
  SSR_DENOISER_SIMPLE = 1
};
eastl::fixed_vector<dafg::NodeHandle, 3> makeScreenSpaceReflectionNodes(
  int w, int h, bool is_fullres, int denoiser, uint32_t fmt, SSRQuality ssr_quality);

enum class AoAlgo
{
  SSAO,
  GTAO
};
eastl::array<dafg::NodeHandle, 3> makeAmbientOcclusionNodes(AoAlgo algo, int w, int h, uint32_t flags = SsaoCreationFlags::SSAO_NONE);


dafg::NodeHandle makeGiCalcNode();
dafg::NodeHandle makeGiFeedbackNode();
dafg::NodeHandle makeGiScreenDebugNode();
dafg::NodeHandle makeGiScreenDebugDepthNode();

eastl::array<dafg::NodeHandle, 2> makeDeferredLightNode(bool need_reprojection);
eastl::fixed_vector<dafg::NodeHandle, 2, false> makeResolveGbufferNodes(const char *resolve_pshader_name,
  const char *resolve_cshader_name,
  const char *classify_cshader_name,
  const ShadingResolver::PermutationsDesc &permutations_desc);

dafg::NodeHandle makePrepareWaterNode();

eastl::fixed_vector<dafg::NodeHandle, 4> makeEnvironmentNodes();

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
extern const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_SSR_COLOR_TOKEN;
extern const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT_WITH_RENAMES)> WATER_SSR_STRENGTH_TEX;
extern const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_NORMAL_DIR_TEX;

dafg::NodeHandle makeWaterNode(WaterRenderMode mode);
eastl::fixed_vector<dafg::NodeHandle, 2, false> makeWaterSSRNode(WaterRenderMode mode);

dafg::NodeHandle makeDownsampleDepthWithWaterNode();

dafg::NodeHandle makeReprojectedHzbImportNode();
dafg::NodeHandle makeOcclusionFinalizeNode();

dafg::NodeHandle makeRendinstTransparentNode();

dafg::NodeHandle makeTransparentEcsNode();

dafg::NodeHandle makeAcesFxTransparentNode();
eastl::fixed_vector<dafg::NodeHandle, 2, false> makeAcesFxLowresTransparentNodes();

dafg::NodeHandle makeTranslucentInWorldPanelsNode();

dafg::NodeHandle makeTransparentSceneLateNode();
dafg::NodeHandle makeMainHeroTransNode();

dafg::NodeHandle makeUnderWaterFogNode();

dafg::NodeHandle makeUnderWaterParticlesNode();

dafg::NodeHandle makeDepthWithTransparencyNode();

dafg::NodeHandle makeDownsampleDepthWithTransparencyNode();

resource_slot::NodeHandleWithSlotsAccess makePostFxNode();

dafg::NodeHandle makeUpsampleDepthForSceneDebugNode();
dafg::NodeHandle makeShowSceneDebugNode();

dafg::NodeHandle makeSSAANode();

eastl::fixed_vector<dafg::NodeHandle, 3> makeFsrNodes();

dafg::NodeHandle makeFXAANode(const char *target_name, bool external_target);

dafg::NodeHandle makeStaticUpsampleNode(const char *source_name);

resource_slot::NodeHandleWithSlotsAccess makePostFxInputSlotProviderNode();

resource_slot::NodeHandleWithSlotsAccess makePreparePostFxNode();

dafg::NodeHandle makeFrameBeforeDistortionProducerNode();

dafg::NodeHandle makeDistortionFxNode();

dafg::NodeHandle makeRenameDepthNode();

eastl::fixed_vector<dafg::NodeHandle, 5, false> makeSubsamplingNodes(bool sub_sampling, bool super_sampling);

dafg::NodeHandle makeShaderAssertNode();

dafg::NodeHandle makeFrameToPresentProducerNode();

eastl::array<dafg::NodeHandle, 2> makeExternalFinalFrameControlNodes(bool requires_multisampling);

dafg::NodeHandle makePostfxTargetProducerNode(bool requires_multisampling);

dafg::NodeHandle makePrepareForPostfxNoAANode();

dafg::NodeHandle makeAfterWorldRenderNode();

dafg::NodeHandle makeFrameDownsampleNode();
dafg::NodeHandle makeFrameDownsampleSamplerNode();

dafg::NodeHandle makeNoFxFrameNode();

eastl::fixed_vector<dafg::NodeHandle, 2> makeBeforeUIControlNodes();
dafg::NodeHandle makeUIRenderNode(bool withHistory);
dafg::NodeHandle makeUIBlendNode();

dafg::NodeHandle makeRendinstUpdateNode();

eastl::fixed_vector<dafg::NodeHandle, 2, false> makeCameraInCameraSetupNodes();

eastl::array<dafg::NodeHandle, 3> makeResolveMotionAndEnviCoverNode(bool has_motion_vecs, bool use_envi_cover_nodes, bool use_NBS);

dafg::NodeHandle makeEmptyDebugVisualizationNode();
void makeDebugVisualizationNodes(eastl::vector<dafg::NodeHandle> &fg_node_handles);
