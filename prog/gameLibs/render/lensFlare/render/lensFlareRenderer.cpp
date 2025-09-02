// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lensFlareRenderer.h"

#include <drv/3d/dag_vertexIndexBuffer.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_lockSbuffer.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/string.h>
#include <shaders/dag_computeShaders.h>
#include <math/dag_hlsl_floatx.h>
#include <drv/3d/dag_draw.h>
#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>
#include <generic/dag_enumerate.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_shaderConstants.h>

#include <shaders/lens_flare_info.hlsli>

CONSOLE_BOOL_VAL("lens_flares", disable_separate_occlusion_pass, false);
CONSOLE_INT_VAL("lens_flares", max_history_size, 4, 0, 4,
  "How many previous frames flare can use to average out jitter to reduce flickering.");
CONSOLE_FLOAT_VAL("lens_flares", history_invalidation_distance, 0.01,
  "Visibility history is fully discarded if the camera moves more than this in a frame");


#define VAR(a) ShaderVariableInfo a##VarId(#a, true);
LENS_FLARE_VARS_LIST
#undef VAR

#pragma pack(push)
#pragma pack(1)
struct LensFlareVertex
{
  Point2 pos;
  Point2 tc;
  Point2 flareId__vertexId;
};
#pragma pack(pop)

LensFlareRenderer::LensFlareData::LensFlareData(const eastl::string config_name, const Params &_params) :
  configName(eastl::move(config_name)), params(_params)
{}

struct RenderBlockKey
{
  eastl::string texture;
  int roundingType;

  bool operator==(const RenderBlockKey &other) const { return roundingType == other.roundingType && texture == other.texture; }
};

template <>
struct eastl::hash<RenderBlockKey>
{
  size_t operator()(const RenderBlockKey &k) const { return hash<string_view>()(k.texture) ^ hash<int>()(k.roundingType); }
};

template <typename K, typename V>
using HashMapWithFrameMem = ska::flat_hash_map<K, V, eastl::hash<K>, eastl::equal_to<K>, framemem_allocator>;

LensFlareRenderer::RenderBlock::RenderBlock(SharedTex texture, RoundingType rounding_type) :
  texture(eastl::move(texture)), roundingType(rounding_type)
{}

LensFlareRenderer::LensFlareData LensFlareRenderer::LensFlareData::parse_lens_flare_config(const LensFlareConfig &config,
  eastl::vector<uint16_t> &ib, eastl::vector<LensFlareVertex> &vb, eastl::vector<LensFlareInfo> &flares, eastl::vector<Point2> &vpos)
{
  FRAMEMEM_REGION;

  LensFlareData result = LensFlareData(config.name, {config.smoothScreenFadeoutDistance, config.useOcclusion, config.depthBias,
                                                      -config.exposureReduction, config.spotlightConeAngleCos});
  HashMapWithFrameMem<RenderBlockKey, int> renderBlockIds;
  eastl::vector<eastl::vector<int, framemem_allocator>, framemem_allocator> elementsPerRenderBlock;
  HashMapWithFrameMem<int, int> sideCountToVPosOffset;
  HashMapWithFrameMem<int, int> elementIdToVPosOffset;

  for (auto [elementId, element] : enumerate(config.elements))
  {
    if (element.sideCount <= 2)
      continue;

    const bool canShareVPosBuf = fabsf(element.preRotationOffset) < 0.000001f;
    if (!canShareVPosBuf || sideCountToVPosOffset.count(element.sideCount) == 0)
    {
      if (canShareVPosBuf)
        sideCountToVPosOffset[element.sideCount] = vpos.size();
      else
        elementIdToVPosOffset[elementId] = vpos.size();
      for (int i = 0; i <= element.sideCount; ++i)
      {
        // modulo is used to avoid different vertex positions due to floating point precision issues
        double angle = (double(i % element.sideCount) + 0.5) / element.sideCount * M_PI * 2 + element.preRotationOffset;
        vpos.push_back(Point2(sin(angle), cos(angle)));
      }
    }

    RoundingType roundingType = RoundingType::ROUNDED;
    if (element.roundness < 0.0000001f)
      roundingType = RoundingType::SHARP;
    else if (element.roundness > 0.9999999f)
      roundingType = RoundingType::CIRCLE;
    RenderBlockKey key = {eastl::string{element.texture.c_str()}, static_cast<int>(roundingType)};
    auto renderBlockItr = renderBlockIds.find(key);
    int renderBlockId = -1;
    if (renderBlockItr != renderBlockIds.end())
      renderBlockId = renderBlockItr->second;
    else
    {
      renderBlockId = result.renderBlocks.size();
      SharedTex tex;
      if (!element.texture.empty())
      {
        tex = dag::get_tex_gameres(element.texture.c_str());
        G_ASSERTF(tex.getBaseTex() != nullptr, "Couldn't find texture: <%s>", element.texture);
      }

      result.renderBlocks.emplace_back(eastl::move(tex), roundingType);
      elementsPerRenderBlock.emplace_back();
      renderBlockIds.insert_or_assign(key, renderBlockId);
    }
    elementsPerRenderBlock[renderBlockId].push_back(elementId);
  }

  for (auto [renderBlockId, renderBlock] : enumerate(result.renderBlocks))
  {
    renderBlock.indexOffset = ib.size();
    for (const auto &elementId : elementsPerRenderBlock[renderBlockId])
    {
      const auto &element = config.elements[elementId];
      if (element.sideCount <= 2)
        continue;

      // Precompute rounding parameters
      float triangleAngle = M_PI * 2 / element.sideCount;
      float sideLength = sin(triangleAngle / 2);
      float roundedSideLength = sideLength * element.roundness;
      float vertexToCircleCenterDist = roundedSideLength / sin(triangleAngle / 2);
      float circleRadius = vertexToCircleCenterDist * cos(triangleAngle / 2);
      float circleOffset = 1 - vertexToCircleCenterDist;
      float roundnessClippingConeCos = cos(triangleAngle / 2);

      // Rounding would otherwise shrink the shape
      float roundnessScaling = 1.0f / (circleOffset + circleRadius);

      float invMaxDist = 1.0f / cos(M_PI / element.sideCount);

      int vertexPosBufferOffset =
        elementIdToVPosOffset.count(elementId) > 0 ? elementIdToVPosOffset[elementId] : sideCountToVPosOffset[element.sideCount];

      uint16_t flags = 0;
      if (element.autoRotation)
        flags |= LENS_FLARE_DATA_FLAGS__AUTO_ROTATION;
      if (element.useLightColor)
        flags |= LENS_FLARE_DATA_FLAGS__USE_LIGHT_COLOR;
      if (element.gradient.inverted)
        flags |= LENS_FLARE_DATA_FLAGS__INVERTED_GRADIENT;
      if (element.radialDistortion.enabled)
        flags |= LENS_FLARE_DATA_FLAGS__RADIAL_DISTORTION;
      if (element.radialDistortion.enabled && element.radialDistortion.relativeToCenter)
        flags |= LENS_FLARE_DATA_FLAGS__RADIAL_DISTORTION_REL_TO_CENTER;
      LensFlareInfo flareData;
      flareData.tintRGB_invMaxDist = Point4::xyzV(element.tint, invMaxDist);
      flareData.offset = element.offset;
      flareData.scale = mul(element.scale, config.scale) * roundnessScaling;
      flareData.invGradient = safediv(1.f, element.gradient.gradient);
      flareData.invFalloff = safediv(1.f, element.gradient.falloff);
      flareData.intensity = element.intensity * config.intensity;
      flareData.axisOffset2 = element.axisOffset * 2;
      flareData.flags = flags;
      flareData.rotationOffsetSinCos = Point2(sinf(element.rotationOffset), cosf(element.rotationOffset));
      flareData.distortionScale = mul(element.radialDistortion.radialEdgeSize, config.scale) * roundnessScaling;
      flareData.roundness = element.roundness;
      flareData.roundingCircleRadius = circleRadius;
      flareData.roundingCircleOffset = circleOffset;
      flareData.roundingCircleCos = roundnessClippingConeCos;
      flareData.distortionPow = element.radialDistortion.distortionCurvePow;
      flareData.exposurePowParam = -config.exposureReduction;
      flareData.vertexPosBufOffset = vertexPosBufferOffset;
      int globalFlareId = flares.size();
      flares.push_back(flareData);

      renderBlock.numTriangles += element.sideCount;
      int startingVertexId = vb.size();

      const Point2 *positions = &vpos[vertexPosBufferOffset];
      BBox2 shapeBbox;
      for (int i = 0; i <= element.sideCount; ++i)
        shapeBbox += positions[i];
      // Tc: normalized pos in bounding box
      const Point2 tcTmMul = Point2(1.0f / shapeBbox.size().x, 1.0f / shapeBbox.size().y);
      const Point2 tcTmAdd = Point2(-shapeBbox.getMin().x / shapeBbox.size().x, -shapeBbox.getMin().y / shapeBbox.size().y);

      vb.push_back({Point2(0, 0), tcTmAdd, Point2(globalFlareId, -1)});
      // the first/last vertex appears two times on purpose: pixel shader needs to know the closest vertex's id
      for (int i = 0; i <= element.sideCount; ++i)
      {
        const Point2 pos = positions[i];
        const Point2 tc = mul(pos, tcTmMul) + tcTmAdd;
        vb.push_back({pos, tc, Point2(globalFlareId, i)});

        if (i < element.sideCount)
        {
          ib.push_back(startingVertexId); // center
          ib.push_back(startingVertexId + i + 1);
          ib.push_back(startingVertexId + i + 2);
        }
      }
    }
  }
  return result;
}

void LensFlareRenderer::LensFlareData::setRenderConfigIndices(const eastl::vector_set<RenderConfig> &global_render_configs)
{
  globalToLocalRenderBlockId.resize(global_render_configs.size());
  eastl::fill(globalToLocalRenderBlockId.begin(), globalToLocalRenderBlockId.end(), -1);
  for (auto [renderBlockIndex, renderBlock] : enumerate(renderBlocks))
  {
    renderBlock.globalRenderConfigId =
      eastl::distance(global_render_configs.begin(), global_render_configs.find(renderBlock.getRenderConfig()));
    globalToLocalRenderBlockId[renderBlock.globalRenderConfigId] = renderBlockIndex;
  }
}


const eastl::vector<LensFlareRenderer::RenderBlock> &LensFlareRenderer::LensFlareData::getRenderBlocks() const { return renderBlocks; }

LensFlareRenderer::LensFlareRenderer() {}

void LensFlareRenderer::init()
{
  prepareFlaresShader = new_compute_shader("prepare_lens_flare");
  prepareFlaresOcclusionShader = new_compute_shader("lens_flare_occlusion");

  lensShaderMaterial = new_shader_material_by_name("lens_flare");
  if (lensShaderMaterial.get())
  {
    static CompiledShaderChannelId channels[] = {
      {SCTYPE_FLOAT2, SCUSAGE_POS, 0, 0},
      {SCTYPE_FLOAT2, SCUSAGE_TC, 0, 0},
      {SCTYPE_FLOAT2, SCUSAGE_TC, 1, 0},
    };
    G_ASSERT(lensShaderMaterial->checkChannels(channels, countof(channels)));

    lensShaderElement.shElem = lensShaderMaterial->make_elem();
    if (lensShaderElement.shElem != nullptr)
    {
      lensShaderElement.vDecl = dynrender::addShaderVdecl(channels, countof(channels));
      G_ASSERT(lensShaderElement.vDecl != BAD_VDECL);
      lensShaderElement.stride = dynrender::getStride(channels, countof(channels));
    }
  }
  G_ASSERT_RETURN(lensShaderElement.shElem != nullptr, );
  G_ASSERT_RETURN(sizeof(LensFlareVertex) == lensShaderElement.stride, );
}

void LensFlareRenderer::markConfigsDirty() { isDirty = true; }

void LensFlareRenderer::prepareConfigBuffers(const eastl::vector<LensFlareConfig> &configs)
{
  lensFlareBuf.close();
  vertexPositionsBuf.close();
  manualLightDataBuf.close();
  indirectDrawBuf.close();
  indirectDrawStagingBuf.close();
  drawCallIndicesBufferBuf.close();
  indirectDrawInstanceOffsetsBuf.close();
  flareIB.close();
  flareVB.close();
  lensFlares.clear();
  globalRenderConfigs.clear();
  usedFlareConfigIds.clear();
  usedFlareConfigs.clear();
  indirectDrawArguments.clear();
  manualFlareDrawCallIndicesPerLensFlareId.clear();
  dynamicLightDrawIndices.clear();
  multiDrawCountPerRenderConfig.clear();
  indirectDispatchBuf.close();
  indirectDispatchInitialBuf.close();
  preCulledInstanceIndicesBuf.close();
  currentConfigCacheId++;

  manualPreparedLightsPerFlareId.resize(configs.size());
  usedFlareConfigIds.resize(configs.size());
  usedFlareConfigs.resize(configs.size());
  flareIdToInstanceRange.resize(configs.size());
  manualFlareDrawCallIndicesPerLensFlareId.resize(configs.size());

  if (configs.empty())
    return;

  // A single texel at this mip level should cover the area in gbuffer depth, that's used by the occlusion test
  const int depthMipLevel = get_bigger_log2(LENS_FLARE_OCCLUSION_DEPTH_TEXELS);
  const int farDepthMipLevel = depthMipLevel - 1;
  ShaderGlobal::set_int(lens_flare_prepare_far_depth_mipVarId, farDepthMipLevel);

  eastl::vector<uint16_t> indexBufferContent;
  eastl::vector<LensFlareVertex> vertexBufferContent;
  eastl::vector<LensFlareInfo> flaresBufferContent;
  eastl::vector<Point2> vertexPositionsBufferContent;
  lensFlares.reserve(configs.size());
  int totalNumRenderBlocks = 0;
  int maxNumRenderBlocks = 1;
  for (const auto [configId, config] : enumerate(configs))
  {
    G_ASSERT(config.elements.size() > 0);
    const auto &data = lensFlares.emplace_back(LensFlareData::parse_lens_flare_config(config, indexBufferContent, vertexBufferContent,
      flaresBufferContent, vertexPositionsBufferContent));
    manualFlareDrawCallIndicesPerLensFlareId[configId].reserve(data.getRenderBlocks().size());
    totalNumRenderBlocks += data.getRenderBlocks().size();
    maxNumRenderBlocks = max(maxNumRenderBlocks, int(data.getRenderBlocks().size()));
    for (const auto &renderBlock : data.getRenderBlocks())
    {
      const auto renderConfig = renderBlock.getRenderConfig();
      if (globalRenderConfigs.count(renderConfig) == 0)
        globalRenderConfigs.insert(renderConfig);
    }
  }
  for (auto &lensFlareData : lensFlares)
    lensFlareData.setRenderConfigIndices(globalRenderConfigs);
  usedRenderConfigs.resize(globalRenderConfigs.size());

  int maxNumIndirectDrawCalls = totalNumRenderBlocks + // for manual flares, like sun
                                maxNumRenderBlocks;    // for dynamic flares (only one config is supported at a time)

  indirectDrawArguments.reserve(maxNumIndirectDrawCalls);
  // buffer:
  //  [dynamic light id's entry; 0..configs.size()-1;]: index of first entry;
  //  {<indices of draw calls>..., terminating -1}*(configs.size())
  int drawCallIndicesBufSize = maxNumIndirectDrawCalls + (configs.size() + 1) * 2;
  dynamicLightDrawIndices.reserve(maxNumRenderBlocks);
  multiDrawCountPerRenderConfig.resize(globalRenderConfigs.size());

  ShaderGlobal::set_int(lens_flare_visibility_history_buf_sizeVarId, VISIBILITY_HISTORY_SIZE);
  ShaderGlobal::set_int(lens_flare_prepare_max_num_instanceVarId, MAX_NUM_INSTANCES);

  lensFlareBuf =
    dag::buffers::create_persistent_sr_structured(sizeof(flaresBufferContent[0]), flaresBufferContent.size(), "lens_flare_info_buf");
  manualLightDataBuf = dag::buffers::create_one_frame_sr_structured(sizeof(ManualLightFlareData), MAX_NUM_MANUAL_PREPARED_LIGHTS,
    "lens_flare_prepare_manual_lights_buf");
  lensFlareInstancesBuf =
    dag::buffers::create_ua_sr_structured(sizeof(LensFlareInstanceData), MAX_NUM_INSTANCES, "lens_flare_instances_buf");
  lensFlareVisibilityHistoryBuf = dag::buffers::create_ua_sr_structured(sizeof(LensFlareVisibilityHistoryData),
    VISIBILITY_HISTORY_SIZE, "lens_flare_visibility_history_buf");
  vertexPositionsBuf = dag::buffers::create_persistent_sr_structured(sizeof(Point2), vertexPositionsBufferContent.size(),
    "lens_flare_vertex_positions_buf");
  indirectDrawStagingBuf = dag::buffers::create_one_frame_sr_byte_address(
    (sizeof(DrawIndexedIndirectArgs) * max(maxNumIndirectDrawCalls, 1) + 3) / 4, "lens_flare_prepare_indirect_draw_staging_buf");
  indirectDrawBuf = dag::buffers::create_ua_indirect(dag::buffers::Indirect::DrawIndexed, max(maxNumIndirectDrawCalls, 1),
    "lens_flare_prepare_indirect_draw_buf");
  indirectDrawInstanceOffsetsBuf = dag::buffers::create_one_frame_sr_structured(sizeof(uint32_t), max(maxNumIndirectDrawCalls, 1),
    "lens_flare_prepare_instance_offsets_buf");
  indirectDispatchInitialBuf = dag::buffers::create_persistent_cb(1, "lens_flare_prepare_indirect_dispatch_initial_buf");
  indirectDispatchBuf =
    dag::buffers::create_ua_indirect(dag::buffers::Indirect::Dispatch, 1, "lens_flare_prepare_indirect_dispatch_buf");
  preCulledInstanceIndicesBuf =
    dag::buffers::create_ua_sr_structured(sizeof(uint32_t), MAX_NUM_INSTANCES, "lens_flare_prepare_pre_culled_instance_indices_buf");
  drawCallIndicesBufferBuf =
    dag::buffers::create_one_frame_sr_structured(sizeof(uint32_t), drawCallIndicesBufSize, "lens_flare_prepare_draw_indices_buf");
  flareVB = dag::create_vb(data_size(vertexBufferContent), SBCF_BIND_VERTEX, "LensFlareVertices");
  flareIB = dag::create_ib(lensShaderElement.stride * indexBufferContent.size(), SBCF_BIND_INDEX, "LensFlareIndices");
  lensFlareBuf->updateData(0, flaresBufferContent.size() * sizeof(flaresBufferContent[0]), flaresBufferContent.data(),
    VBLOCK_WRITEONLY);
  vertexPositionsBuf->updateData(0, sizeof(float) * 2 * vertexPositionsBufferContent.size(), vertexPositionsBufferContent.data(),
    VBLOCK_WRITEONLY);
  flareIB->updateData(0, data_size(indexBufferContent), indexBufferContent.data(), VBLOCK_WRITEONLY);
  flareVB->updateData(0, data_size(vertexBufferContent), vertexBufferContent.data(), VBLOCK_WRITEONLY);
  if (auto bufferData = lock_sbuffer<LensFlareVisibilityHistoryData>(lensFlareVisibilityHistoryBuf.getBuf(), 0,
        VISIBILITY_HISTORY_SIZE, VBLOCK_WRITEONLY))
  {
    for (int i = 0; i < VISIBILITY_HISTORY_SIZE; ++i)
    {
      bufferData[i].previousFrameId = -1;
      bufferData[i].positionHash = 0;
      bufferData[i].framesVisible = 0;
      bufferData[i].padding = 0;
      bufferData[i].history = Point4(0, 0, 0, 0);
    }
  }
  if (auto data = lock_sbuffer<uint32_t>(indirectDispatchInitialBuf.getBuf(), 0, 3, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
  {
    data[0] = 0; // filled in compute shader
    data[1] = 1;
    data[2] = 1;
  }
  d3d::resource_barrier({indirectDispatchInitialBuf.getBuf(), RB_RO_COPY_SOURCE});
  d3d::resource_barrier({lensFlareVisibilityHistoryBuf.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});
  ShaderGlobal::set_buffer(lens_flare_prepare_indirect_draw_bufVarId, indirectDrawBuf);
  ShaderGlobal::set_buffer(lens_flare_prepare_indirect_dispatch_bufVarId, indirectDispatchBuf);
  ShaderGlobal::set_buffer(lens_flare_prepare_pre_culled_instance_indices_bufVarId, preCulledInstanceIndicesBuf);
  ShaderGlobal::set_buffer(lens_flare_prepare_instance_offsets_bufVarId, indirectDrawInstanceOffsetsBuf);
}

LensFlareRenderer::~LensFlareRenderer() {}

bool LensFlareRenderer::isCachedFlareIdValid(const CachedFlareId &id) const
{
  return id.isValid() && id.cacheId == currentConfigCacheId;
}

LensFlareRenderer::CachedFlareId LensFlareRenderer::cacheFlareId(const char *flare_config_name) const
{
  for (auto [i, lensFlare] : enumerate(lensFlares))
    if (lensFlare.getConfigName() == flare_config_name)
      return {currentConfigCacheId, int(i)};
  return {};
}

void LensFlareRenderer::startPreparingLights()
{
  if (isDirty)
  {
    isDirty = false;
    updateConfigsFromECS();
  }
  numPreparedManualInstances = 0;
  for (auto &instances : manualPreparedLightsPerFlareId)
    instances.clear();
  for (auto &drawCallIndices : manualFlareDrawCallIndicesPerLensFlareId)
    drawCallIndices.clear();
  eastl::fill(usedRenderConfigs.begin(), usedRenderConfigs.end(), false);
  usedRenderConfigIds.clear();
  eastl::fill(usedFlareConfigs.begin(), usedFlareConfigs.end(), false);
  eastl::fill(flareIdToInstanceRange.begin(), flareIdToInstanceRange.end(), IPoint2(0, 0));
  usedFlareConfigIds.clear();
  dynamicLightsFlareId = -1;
  hadDynamicLights = false;
  indirectDrawArguments.clear();
  dynamicLightDrawIndices.clear();
}

bool LensFlareRenderer::prepareUseLensFlareConfig(const CachedFlareId &id)
{
  G_ASSERT_RETURN(id.cacheId == currentConfigCacheId, false);
  G_ASSERT_RETURN(id.flareConfigId < lensFlares.size(), false);

  const auto &flareData = lensFlares[id.flareConfigId];

  for (const auto &renderBlock : flareData.getRenderBlocks())
  {
    if (usedRenderConfigs[renderBlock.globalRenderConfigId])
      continue;
    usedRenderConfigs[renderBlock.globalRenderConfigId] = true;
    usedRenderConfigIds.push_back(renderBlock.globalRenderConfigId);
  }
  if (!usedFlareConfigs[id.flareConfigId])
  {
    usedFlareConfigs[id.flareConfigId] = true;
    usedFlareConfigIds.push_back(id.flareConfigId);
  }

  return true;
}

void LensFlareRenderer::prepareDynamicLightFlares(const CachedFlareId &cached_flare_config_id)
{
  G_ASSERTF_RETURN(dynamicLightsFlareId == -1, , "Multiple active flare configs for dynamic lights are currently not supported");

  if (!prepareUseLensFlareConfig(cached_flare_config_id))
    return;

  dynamicLightsFlareId = cached_flare_config_id.flareConfigId;
}

void LensFlareRenderer::prepareManualFlare(const CachedFlareId &cached_flare_config_id, const Point4 &light_pos, const Point3 &color,
  bool is_sun)
{
  if (!prepareUseLensFlareConfig(cached_flare_config_id))
    return;

  if (numPreparedManualInstances >= MAX_NUM_MANUAL_PREPARED_LIGHTS)
  {
    LOGERR_ONCE("There are too many manual lens flares visible at once. Increase the limit to allow this. Current limit: %d",
      MAX_NUM_MANUAL_PREPARED_LIGHTS);
    return;
  }

  const auto &flareData = lensFlares[cached_flare_config_id.flareConfigId];
  auto &flareConfigInstances = manualPreparedLightsPerFlareId[cached_flare_config_id.flareConfigId];

  if (flareConfigInstances.size() >= MAX_NUM_INSTANCES)
  {
    LOGERR_ONCE("There are too many lens flares visible at once. Increase the limit to allow this. Current limit: %d",
      MAX_NUM_INSTANCES);
    return;
  }

  ManualLightFlareData data;
  data.color = color;
  data.fadeoutDistance = flareData.getParams().smoothScreenFadeoutDistance;
  data.exposurePowParam = flareData.getParams().exposurePowParam;
  data.flareConfigId = cached_flare_config_id.flareConfigId;
  data.lightPos = light_pos;
  data.depthBias = flareData.getParams().depthBias;
  // Spotlight cone angle is not uploaded here, since it only applies to spotlights
  data.flags = 0;
  if (is_sun)
    data.flags |= MANUAL_LIGHT_FLARE_DATA_FLAGS__IS_SUN;
  if (flareData.getParams().useOcclusion)
    data.flags |= MANUAL_LIGHT_FLARE_DATA_FLAGS__USE_OCCLUSION;
  flareConfigInstances.push_back(eastl::move(data));
  numPreparedManualInstances++;
}

bool LensFlareRenderer::endPreparingLights(const Point3 &camera_pos, const Point3 &camera_dir, int omni_light_count,
  int spot_light_count, int shadow_frames_count)
{
  if (usedRenderConfigIds.empty() || usedFlareConfigIds.empty())
    return false;
  if (dynamicLightsFlareId >= 0 && (omni_light_count > 0 || spot_light_count > 0))
    hadDynamicLights = true;
  if (numPreparedManualInstances == 0 && !hadDynamicLights)
    return false;

  int numManualFlareDrawCalls = 0;
  if (numPreparedManualInstances > 0)
  {
    int startInstanceIndex = 0;
    if (auto bufferData = lock_sbuffer<ManualLightFlareData>(manualLightDataBuf.getBuf(), 0, numPreparedManualInstances,
          VBLOCK_DISCARD | VBLOCK_WRITEONLY))
    {
      for (const auto flareConfigId : usedFlareConfigIds)
      {
        const auto &flareData = lensFlares[flareConfigId];
        const auto &flareConfigInstances = manualPreparedLightsPerFlareId[flareConfigId];
        if (flareConfigInstances.empty())
          continue;

        memcpy(&bufferData[startInstanceIndex], flareConfigInstances.data(), data_size(flareConfigInstances));

        flareIdToInstanceRange[flareConfigId] = IPoint2(startInstanceIndex, flareConfigInstances.size());
        startInstanceIndex += flareConfigInstances.size();
        numManualFlareDrawCalls += flareData.getRenderBlocks().size();
      }
    }
    G_ASSERT(startInstanceIndex == numPreparedManualInstances);
    d3d::resource_barrier({manualLightDataBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
  }
  const LensFlareData *dynamicLightsLensFlare = hadDynamicLights ? &lensFlares[dynamicLightsFlareId] : nullptr;

  int numTotalDrawCalls =
    numManualFlareDrawCalls + (dynamicLightsLensFlare != nullptr ? dynamicLightsLensFlare->getRenderBlocks().size() : 0);
  indirectDrawArguments.resize(numTotalDrawCalls);

  // Preparing indirect draw arguments
  {
    int indirectDrawIndex = 0;
    for (const auto renderConfigId : usedRenderConfigIds)
    {
      int renderConfigStartDrawIndex = indirectDrawIndex;
      for (const auto flareConfigId : usedFlareConfigIds)
      {
        const auto &flareData = lensFlares[flareConfigId];
        if (flareIdToInstanceRange[flareConfigId].y > 0)
        {
          int localRenderBlockId = flareData.getLocalRenderBlockId(renderConfigId);
          if (localRenderBlockId >= 0)
          {
            const auto &renderBlock = flareData.getRenderBlocks()[localRenderBlockId];
            DrawArguments manualLensFlareArgs;
            manualLensFlareArgs.indexCountPerInstance = renderBlock.numTriangles * 3;
            manualLensFlareArgs.startIndexLocation = renderBlock.indexOffset;
            manualLensFlareArgs.instanceIdOffset = flareIdToInstanceRange[flareConfigId].x;
            manualFlareDrawCallIndicesPerLensFlareId[flareConfigId].push_back(indirectDrawIndex);
            indirectDrawArguments[indirectDrawIndex++] = manualLensFlareArgs;
          }
        }
      }
      if (dynamicLightsLensFlare != nullptr)
      {
        int localRenderBlockId = dynamicLightsLensFlare->getLocalRenderBlockId(renderConfigId);
        if (localRenderBlockId >= 0)
        {
          const auto &renderBlock = dynamicLightsLensFlare->getRenderBlocks()[localRenderBlockId];
          DrawArguments dynamicLightLensFlareArgs;
          dynamicLightLensFlareArgs.indexCountPerInstance = renderBlock.numTriangles * 3;
          dynamicLightLensFlareArgs.startIndexLocation = renderBlock.indexOffset;
          // dynamic instances start after manually placed ones
          dynamicLightLensFlareArgs.instanceIdOffset = numPreparedManualInstances;
          dynamicLightDrawIndices.push_back(indirectDrawIndex);
          indirectDrawArguments[indirectDrawIndex++] = dynamicLightLensFlareArgs;
        }
      }
      multiDrawCountPerRenderConfig[renderConfigId] = indirectDrawIndex - renderConfigStartDrawIndex;
    }
    if (auto indirectDrawBufferData = lock_sbuffer<DrawIndexedIndirectArgs>(indirectDrawStagingBuf.getBuf(), 0,
          indirectDrawArguments.size(), VBLOCK_WRITEONLY | VBLOCK_DISCARD))
    {
      for (auto [index, drawCallInfo] : enumerate(indirectDrawArguments))
        indirectDrawBufferData[index] = drawCallInfo;
    }
    d3d::resource_barrier({indirectDrawStagingBuf.getBuf(), RB_RO_COPY_SOURCE});
    indirectDrawStagingBuf->copyTo(indirectDrawBuf.getBuf(), 0, 0, indirectDrawArguments.size() * sizeof(DrawIndexedIndirectArgs));
    d3d::resource_barrier({indirectDrawBuf.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});
    if (auto instanceOffsetsData = lock_sbuffer<uint32_t>(indirectDrawInstanceOffsetsBuf.getBuf(), 0, indirectDrawArguments.size(),
          VBLOCK_DISCARD | VBLOCK_WRITEONLY))
    {
      for (auto [index, drawCallInfo] : enumerate(indirectDrawArguments))
        instanceOffsetsData[index] = drawCallInfo.instanceIdOffset;
    }
    d3d::resource_barrier({indirectDrawInstanceOffsetsBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
    indirectDispatchInitialBuf->copyTo(indirectDispatchBuf.getBuf(), 0, 0, sizeof(uint32_t) * 3);
    d3d::resource_barrier({indirectDispatchBuf.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});
  }
  int drawIndicesDataSize = (lensFlares.size() + 1) * 2 + numTotalDrawCalls;
  int currentDataIndex = lensFlares.size() + 1;
  if (auto drawIndicesData =
        lock_sbuffer<uint32_t>(drawCallIndicesBufferBuf.getBuf(), 0, drawIndicesDataSize, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
  {
    if (dynamicLightsLensFlare != nullptr)
    {
      drawIndicesData[0] = currentDataIndex;
      for (const auto drawCallIndex : dynamicLightDrawIndices)
        drawIndicesData[currentDataIndex++] = drawCallIndex;
      drawIndicesData[currentDataIndex++] = ~0u;
    }
    for (const auto flareConfigId : usedFlareConfigIds)
    {
      drawIndicesData[flareConfigId + 1] = currentDataIndex;
      for (const auto drawCallIndex : manualFlareDrawCallIndicesPerLensFlareId[flareConfigId])
        drawIndicesData[currentDataIndex++] = drawCallIndex;
      drawIndicesData[currentDataIndex++] = ~0u;
    }
  }
  G_ASSERT(currentDataIndex <= drawIndicesDataSize);
  d3d::resource_barrier({drawCallIndicesBufferBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});

  ShaderGlobal::set_color4(lens_flare_prepare_camera_posVarId, camera_pos, 0);
  ShaderGlobal::set_color4(lens_flare_prepare_camera_dirVarId, camera_dir, 0);
  ShaderGlobal::set_int(lens_flare_prepare_num_manual_flaresVarId, numPreparedManualInstances);
  ShaderGlobal::set_int(lens_flare_prepare_num_omni_light_flaresVarId, omni_light_count);
  ShaderGlobal::set_int(lens_flare_prepare_num_spot_light_flaresVarId, spot_light_count);
  if (hadDynamicLights)
  {
    const auto &flareData = lensFlares[dynamicLightsFlareId];
    ShaderGlobal::set_real(lens_flare_prepare_dynamic_lights_fadeout_distanceVarId, flareData.getParams().smoothScreenFadeoutDistance);
    ShaderGlobal::set_int(lens_flare_prepare_dynamic_lights_use_occlusionVarId, flareData.getParams().useOcclusion ? 1 : 0);
    ShaderGlobal::set_real(lens_flare_prepare_dynamic_lights_depth_biasVarId, flareData.getParams().depthBias);
    ShaderGlobal::set_real(lens_flare_prepare_dynamic_lights_exposure_pow_paramVarId, flareData.getParams().exposurePowParam);
    ShaderGlobal::set_real(lens_flare_prepare_spot_lights_cone_angle_cosVarId, flareData.getParams().spotlightConeAngleCos);
  }

  if (numPreparedManualInstances > 0)
  {
    ShaderGlobal::set_int(lens_flare_prepare_flare_typeVarId, 0);
    prepareFlaresShader->dispatchThreads(numPreparedManualInstances, 1, 1);
  }

  if (hadDynamicLights)
  {
    if (omni_light_count > 0)
    {
      ShaderGlobal::set_int(lens_flare_prepare_flare_typeVarId, 1);
      prepareFlaresShader->dispatchThreads(omni_light_count, 1, 1);
    }
    if (spot_light_count > 0)
    {
      ShaderGlobal::set_int(lens_flare_prepare_flare_typeVarId, 2);
      prepareFlaresShader->dispatchThreads(spot_light_count, 1, 1);
    }
  }

  d3d::resource_barrier({preCulledInstanceIndicesBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
  d3d::resource_barrier({indirectDispatchBuf.getBuf(), RB_RO_INDIRECT_BUFFER | RB_STAGE_COMPUTE});
  d3d::resource_barrier({lensFlareInstancesBuf.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});
  d3d::resource_barrier({indirectDrawBuf.getBuf(), RB_RO_INDIRECT_BUFFER | RB_STAGE_ALL_GRAPHICS});

  if (!disable_separate_occlusion_pass)
  {
    const auto cameraMoved = camera_pos - lastCameraPos;
    lastCameraPos = camera_pos;
    int maxHistory = ::min(max_history_size.get(), shadow_frames_count - 1);
    if (cameraMoved.lengthSq() > history_invalidation_distance.get() * history_invalidation_distance.get())
      maxHistory = 0; // This will invalidate the history
    ShaderGlobal::set_int(lens_flare_visibility_history_frame_idVarId, nextHistoryFrameId++);
    ShaderGlobal::set_int(lens_flare_max_visibility_history_sizeVarId, maxHistory);
    prepareFlaresOcclusionShader->dispatch_indirect(indirectDispatchBuf.getBuf(), 0);
  }

  d3d::resource_barrier({lensFlareInstancesBuf.getBuf(), RB_RO_SRV | RB_STAGE_VERTEX});
  return true;
}

void LensFlareRenderer::render(const Point2 &resolution, float zoom) const
{
  if (usedRenderConfigIds.empty() || usedFlareConfigIds.empty() || (numPreparedManualInstances == 0 && !hadDynamicLights))
    return;
  const auto vb = flareVB.getBuf();
  const auto ib = flareIB.getBuf();

  if (vb == nullptr || ib == nullptr || lensShaderElement.shElem == nullptr)
    return;

  const Point2 globalScale =
    resolution.x >= resolution.y ? Point2(float(resolution.y) / resolution.x, 1) : Point2(1, float(resolution.x) / resolution.y);
  ShaderGlobal::set_color4(lens_flare_resolutionVarId, resolution);
  ShaderGlobal::set_color4(lens_flare_global_scaleVarId, globalScale * zoom);

  d3d::setvsrc(0, vb, lensShaderElement.stride);
  d3d::setind(ib);

  int indirectDrawIndex = 0;
  for (const auto renderConfigId : usedRenderConfigIds)
  {
    if (multiDrawCountPerRenderConfig[renderConfigId] == 0)
      continue;
    const auto &renderConfig = globalRenderConfigs[renderConfigId];
    ShaderGlobal::set_texture(lens_flare_textureVarId, renderConfig.textureId);
    ShaderGlobal::set_int(lens_flare_rounding_typeVarId, static_cast<int>(renderConfig.roundingType));
    lensShaderElement.shElem->setStates();

    for (int i = 0; i < multiDrawCountPerRenderConfig[renderConfigId]; ++i)
    {
      constexpr int immediateConstCount = 1;
      const uint32_t immediateConsts[immediateConstCount] = {indirectDrawArguments[indirectDrawIndex].instanceIdOffset};

      d3d::set_immediate_const(STAGE_VS, immediateConsts, immediateConstCount);
      d3d::draw_indexed_indirect(PRIM_TRILIST, indirectDrawBuf.getBuf(), indirectDrawIndex * sizeof(DrawIndexedIndirectArgs));
      indirectDrawIndex++;
    }
  }
  d3d::set_immediate_const(STAGE_VS, nullptr, 0);
}
LensFlareRenderer::DrawArguments::operator DrawIndexedIndirectArgs() const
{
  DrawIndexedIndirectArgs result = {};
  result.indexCountPerInstance = indexCountPerInstance;
  result.startIndexLocation = startIndexLocation;
  result.baseVertexLocation = 0;
  result.startInstanceLocation = 0; // instanceIdOffset;
  result.instanceCount = 0;         // filled from compute shader
  return result;
}
