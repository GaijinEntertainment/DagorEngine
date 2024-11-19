// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lensFlareRenderer.h"

#include <drv/3d/dag_vertexIndexBuffer.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_lockSbuffer.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/string.h>
#include <shaders/dag_computeShaders.h>
#include <drv/3d/dag_shaderConstants.h>
#include <math/dag_hlsl_floatx.h>

#include <lens_flare/shaders/lens_flare_info.hlsli>
#include <generic/dag_enumerate.h>


#define VAR(a) ShaderVariableInfo a##VarId(#a);
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

LensFlareRenderer::LensFlareData LensFlareRenderer::LensFlareData::parse_lens_flare_config(const LensFlareConfig &config,
  eastl::vector<uint16_t> &ib,
  eastl::vector<LensFlareVertex> &vb,
  eastl::vector<LensFlareInfo> &flares,
  eastl::vector<Point2> &vpos)
{
  FRAMEMEM_REGION;

  LensFlareData result = LensFlareData(config.name, {config.smoothScreenFadeoutDistance, config.useOcclusion});
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

      result.renderBlocks.push_back({0, 0, eastl::move(tex), roundingType});
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


const eastl::vector<LensFlareRenderer::LensFlareRenderBlock> &LensFlareRenderer::LensFlareData::getRenderBlocks() const
{
  return renderBlocks;
}

LensFlareRenderer::LensFlareRenderer() {}

void LensFlareRenderer::init()
{
  prepareFlaresShader = new_compute_shader("prepare_lens_flare");

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
  flareIB.close();
  flareVB.close();
  lensFlares.clear();
  currentConfigCacheId++;

  if (configs.empty())
    return;

  eastl::vector<uint16_t> indexBufferContent;
  eastl::vector<LensFlareVertex> vertexBufferContent;
  eastl::vector<LensFlareInfo> flaresBufferContent;
  eastl::vector<Point2> vertexPositionsBufferContent;
  lensFlares.reserve(configs.size());
  for (const auto &config : configs)
    lensFlares.emplace_back(LensFlareData::parse_lens_flare_config(config, indexBufferContent, vertexBufferContent,
      flaresBufferContent, vertexPositionsBufferContent));

  numTotalVertices = vertexBufferContent.size();

  lensFlareBuf =
    dag::buffers::create_persistent_sr_structured(sizeof(flaresBufferContent[0]), flaresBufferContent.size(), "lens_flare_info_buf");
  preparedLightsBuf = dag::buffers::create_ua_sr_structured(sizeof(LensFLarePreparedLightSource), maxNumPreparedLights,
    "lens_flare_prepared_lights_buf");
  vertexPositionsBuf = dag::buffers::create_persistent_sr_structured(sizeof(Point2), vertexPositionsBufferContent.size(),
    "lens_flare_vertex_positions_buf");
  flareVB = dag::create_vb(data_size(vertexBufferContent), SBCF_BIND_VERTEX, "LensFlareVertices");
  flareIB = dag::create_ib(lensShaderElement.stride * indexBufferContent.size(), SBCF_BIND_INDEX, "LensFlareIndices");
  lensFlareBuf->updateData(0, flaresBufferContent.size() * sizeof(flaresBufferContent[0]), flaresBufferContent.data(),
    VBLOCK_WRITEONLY);
  vertexPositionsBuf->updateData(0, sizeof(float) * 2 * vertexPositionsBufferContent.size(), vertexPositionsBufferContent.data(),
    VBLOCK_WRITEONLY);
  flareIB->updateData(0, data_size(indexBufferContent), indexBufferContent.data(), VBLOCK_WRITEONLY);
  flareVB->updateData(0, data_size(vertexBufferContent), vertexBufferContent.data(), VBLOCK_WRITEONLY);
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
  preparedLights.clear();
}

void LensFlareRenderer::prepareFlare(const TMatrix4 &view_proj,
  const Point3 &camera_pos,
  const Point3 &camera_dir,
  const CachedFlareId &cached_flare_config_id,
  const Point4 &light_pos,
  const Point3 &color,
  bool is_sun)
{
  G_ASSERT_RETURN(cached_flare_config_id.cacheId == currentConfigCacheId, );
  G_ASSERT_RETURN(cached_flare_config_id.flareConfigId < lensFlares.size(), );

  if (preparedLights.size() >= maxNumPreparedLights)
  {
    LOGERR_ONCE("There are too many lens flares visible at once. Increase the limit to allow this. Current limit: %d",
      maxNumPreparedLights);
    return;
  }

  // TODO this logic should be moved to the compute shader later when multiple light sources are used; use with indirect rendering
  const Point4 cameraToLight = light_pos - Point4::xyz1(camera_pos) * light_pos.w;
  if (dot(camera_dir, Point3::xyz(cameraToLight)) <= 0)
    return;
  Point4 projectedLightPos = light_pos * view_proj;
  if (fabsf(projectedLightPos.w) < 0.0000001f)
    return;
  projectedLightPos /= projectedLightPos.w;
  float screenEdgeSignedDistance = 1.f - max(fabsf(projectedLightPos.x), fabsf(projectedLightPos.y));
  if (screenEdgeSignedDistance <= 0)
    return;

  preparedLights.push_back(
    PreparedLight{color, Point2(projectedLightPos.x, projectedLightPos.y), cached_flare_config_id.flareConfigId, is_sun});
}

bool LensFlareRenderer::endPreparingLights(const Point3 &camera_pos)
{
  if (preparedLights.empty())
    return false;

  // TODO:
  //  Improve/refactor this when flares are added to multiple light sources.
  //  For now only the sun can have flares. This for loop is fine for now.
  for (auto [i, preparedLight] : enumerate(preparedLights))
  {
    const auto &flareData = lensFlares[preparedLight.flareConfigId];

    ShaderGlobal::set_int(lens_flare_prepare_flares_offsetVarId, i);
    ShaderGlobal::set_int(lens_flare_prepare_num_flaresVarId, 1);
    ShaderGlobal::set_int(lens_flare_prepare_is_sunVarId, preparedLight.isSun ? 1 : 0);
    ShaderGlobal::set_int(lens_flare_prepare_use_occlusionVarId, flareData.getParams().useOcclusion ? 1 : 0);
    ShaderGlobal::set_color4(lens_flare_prepare_sun_colorVarId, preparedLight.lightColor, 0);
    ShaderGlobal::set_real(lens_flare_prepare_fadeout_distanceVarId, flareData.getParams().smoothScreenFadeoutDistance);
    ShaderGlobal::set_color4(lens_flare_prepare_sun_screen_tcVarId, preparedLight.lightPos);
    ShaderGlobal::set_color4(lens_flare_prepare_camera_posVarId, camera_pos, 0);
    prepareFlaresShader->dispatch(1, 1, 1);
  }
  d3d::resource_barrier({preparedLightsBuf.getBuf(), RB_RO_SRV | RB_STAGE_VERTEX});

  return true;
}

void LensFlareRenderer::render(const Point2 &resolution) const
{
  if (preparedLights.empty())
    return;
  const auto vb = flareVB.getBuf();
  const auto ib = flareIB.getBuf();

  if (vb == nullptr || ib == nullptr || lensShaderElement.shElem == nullptr)
    return;

  const Point2 globalScale =
    resolution.x >= resolution.y ? Point2(float(resolution.y) / resolution.x, 1) : Point2(1, float(resolution.x) / resolution.y);
  ShaderGlobal::set_color4(lens_flare_resolutionVarId, resolution);
  ShaderGlobal::set_color4(lens_flare_global_scaleVarId, globalScale);

  d3d::setvsrc(0, vb, lensShaderElement.stride);
  d3d::setind(ib);

  for (auto [preparedLightId, preparedLight] : enumerate(preparedLights))
  {
    const auto &flareData = lensFlares[preparedLight.flareConfigId];
    const auto &renderBlocks = flareData.getRenderBlocks();
    if (renderBlocks.empty())
      continue;

    ShaderGlobal::set_int(lens_flare_light_source_idVarId, preparedLightId);

    for (const auto &renderBlock : renderBlocks)
    {
      ShaderGlobal::set_texture(lens_flare_textureVarId, renderBlock.texture.getTexId());
      ShaderGlobal::set_int(lens_flare_rounding_typeVarId, static_cast<int>(renderBlock.roundingType));
      lensShaderElement.shElem->setStates();
      lensShaderElement.shElem->render(0, numTotalVertices, renderBlock.indexOffset, renderBlock.numTriangles, 0, PRIM_TRILIST);
    }
  }
}
