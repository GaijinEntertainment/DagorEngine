#include "impostorBaker.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <debug/dag_debug.h>
#include <image/dag_tga.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <shaders/dag_rendInstRes.h>
#include <3d/dag_texIdSet.h>
#include <3d/dag_lockTexture.h>
#include <util/dag_simpleString.h>
#include <ioSys/dag_fileIo.h>
#include <assets/asset.h>
#include <assets/assetHlp.h>
#include <render/noiseTex.h>
#include <shaders/pack_impostor_texture.hlsl>
#include <shaders/dag_IRenderWrapperControl.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderBlock.h>
#include <image/dag_tiff.h>
#include <image/dag_texPixel.h>
#include <perfMon/dag_statDrv.h>

#include <de3_interface.h>
#include <rendInst/rendInstGen.h>

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/string.h>

#include <fstream>
#include <random>
#include <regex>
#include "impostorTexturePacker.h"

#include <rendInst/impostorTextureMgr.h>

#define ALBEDO_ALPHA_NAME        "aa"
#define NORMAL_TRANSLUCENCY_NAME "nt"
#define AO_SMOOTHNESS_NAME       "as"

#define FILE_REGEX "^.*_(aa|nt|as)\\.(tiff|tex\\.blk)$"

#define GLOBAL_VARS_LIST                   \
  VAR(impostor_sdf_max_distance)           \
  VAR(impostor_color_padding)              \
  VAR(texture_size)                        \
  VAR(impostor_texture_transform)          \
  VAR(impostor_packed_albedo_alpha)        \
  VAR(impostor_packed_normal_translucency) \
  VAR(impostor_packed_ao_smoothness)       \
  VAR(slice_id)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

static struct TreeCrown
{
  Point4 brightness_falloff_start_stop_dist = Point4(0, 0, 0, 0), crownCenter1 = Point4(0, 0, 0, 0), invCrownRad1 = Point4(0, 0, 0, 0),
         crownCenter2 = Point4(0, 0, 0, 0), invCrownRad2 = Point4(0, 0, 0, 0);
} treeCrownData;

ImpostorBaker::ImpostorBaker(ILogWriter *log_writer, bool display) noexcept : logWriter(log_writer), displayExportedImages(display)
{
  conlog("ImpostorBaker initialized");

  dynamic_impostor_texture_const_no = ShaderGlobal::get_int_fast(::get_shader_glob_var_id("dynamic_impostor_texture_const_no"));

  Point3 minR = Point3(-0.28, -0.23, -0.24);
  Point3 maxR = Point3(0.29, 0.27, 0.24);
  init_and_get_perlin_noise_3d(minR, maxR).setVar();
  treeCrown = dag::create_sbuffer(sizeof(TreeCrown), 1, SBCF_MAYBELOST | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0, "treeCrown");

  objectsBuffer = dag::create_sbuffer(sizeof(Point4) * 3, 1, SBCF_MAYBELOST | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0,
    "Impostor_texture_mgr_instancing");
  set_objects_buffer();
}

ImpostorBaker::~ImpostorBaker()
{
  release_perline_noise_3d();
  conlog("ImpostorBaker closed");
}

void ImpostorBaker::set_objects_buffer()
{
  Point4 *data = nullptr;
  int res = objectsBuffer->lock(0, sizeof(Point4) * 3, reinterpret_cast<void **>(&data), VBLOCK_WRITEONLY);
  data[0] = Point4(1, 0, 0, 0);
  data[1] = Point4(0, 1, 0, 0);
  data[2] = Point4(0, 0, 1, 0);
  objectsBuffer->unlock();
}

Point2 decode_position(const Point2 &p, bool clamp = true)
{
  float angle = p.x;
  float x = cos(angle) * p.y;
  float y = -sin(angle) * p.y;
  if (clamp)
  {
    x = min(1.f, max(-1.f, x));
    y = min(1.f, max(-1.f, y));
  }
  return Point2(x, y);
}

static int rotation_direction(const Point2 &origin, Point2 a, Point2 b)
{
  a -= origin;
  b -= origin;
  float dir = dot(Point2{-a.y, a.x}, b);
  if (dir < 0)
    return -1;
  if (dir > 0)
    return 1;
  return 0;
}

static float get_wrapping_quality(const eastl::vector<float> &samples, eastl::vector<Point2> &result)
{
  float area = 0;
  for (unsigned int i = 0; i < result.size(); ++i)
  {
    Point2 start = result[i];
    Point2 end = result[(i + 1) % result.size()];
    if (i + 1 < result.size())
    {
      if (start.x <= end.x)
        return -1;
    }
    else
    {
      if (start.x >= end.x || start.x <= end.x - PI * 2)
        return -1;
    }
    // samples go in the other direction -> start and end are swapped
    unsigned int sampleEnd = static_cast<unsigned int>(start.x / (2 * PI) * samples.size()) % samples.size();
    unsigned int sampleStart = static_cast<unsigned int>(end.x / (2 * PI) * samples.size()) % samples.size();
    unsigned int numSamples =
      sampleStart <= sampleEnd ? sampleEnd - sampleStart + 1 : (unsigned)samples.size() + sampleEnd - sampleStart + 1;

    Point2 startPoint = decode_position(start);
    Point2 endPoint = decode_position(end);
    if (rotation_direction(startPoint, endPoint, Point2{0, 0}) != 1)
      return -1;
    for (int j = 0; j < numSamples; ++j)
    {
      unsigned int sampleInd = static_cast<unsigned int>((j + sampleStart) % samples.size());
      const Point2 p = decode_position(Point2(static_cast<float>(sampleInd) / samples.size() * 2 * PI, samples[sampleInd]));
      G_UNUSED(p);
      // something is not inside of the polygon
      if (rotation_direction(startPoint, endPoint, p) == -1)
        return -1;
    }
    // don't use clamping for area calculation
    startPoint = decode_position(start, false);
    endPoint = decode_position(end, false);
    // Area of (startPoint; endPoint; (0, 0))
    // Area = sin(alpha) * |endPoint-(0, 0)| * |startPoint-(0, 0)| / 2 = |cross(startPoint,
    // endPoint)| / 2
    area += length(cross(Point3(endPoint.x, endPoint.y, 0), Point3(startPoint.x, startPoint.y, 0))) / 2;
  }
  return area;
}

static void get_wrapping_polygon(float starting_angle, const eastl::vector<float> &samples, eastl::vector<Point2> &result,
  const char *asset_name)
{
  std::mt19937 gen(0); //-V1057
  // (angle, length)
  eastl::vector<Point2> starting(result.size());
  for (unsigned int i = 0; i < result.size(); ++i)
  {
    float angle = starting_angle - static_cast<float>(i) / result.size() * PI * 2;
    // start at distance 3 to ensure that the starting solution is valid
    starting[i] = Point2(angle, 2.5f);
  }
  eastl::vector<Point2> best = starting;
  float bestQuality = get_wrapping_quality(samples, best);
  G_ASSERTF(bestQuality >= 0, "Starting state is invalid");

  constexpr unsigned int numTries = 100;
  constexpr unsigned int numSteps = 10000;
  std::uniform_real_distribution<float> chanceDistribution(0.f, 1.f);
  // Simulated annealing
  for (unsigned int i = 0; i < numTries; ++i)
  {
    eastl::vector<Point2> current = starting;
    float currentQuality = get_wrapping_quality(samples, current);
    for (unsigned int j = 0; j < numSteps; ++j)
    {
      const float temperature = static_cast<float>(numSteps - j) / numSteps;
      const float offsetRange = 0.01f * lerp(1.f, 0.1f, temperature);
      const float contractionRange = 0.1f * lerp(1.f, 0.1f, temperature);
      std::uniform_real_distribution<float> offsetDistribution(-offsetRange, offsetRange);
      std::uniform_real_distribution<float> contractionDistribution(1.f - contractionRange, 1.f + contractionRange / 4);
      // modify vertices in order in a cycle
      const Point2 c = current[j % current.size()];
      current[j % current.size()].x += offsetDistribution(gen) * 2 * PI;
      current[j % current.size()].y *= contractionDistribution(gen);
      const float quality = get_wrapping_quality(samples, current);
      if (quality < 0)
      {
        // invalid
        current[j % current.size()] = c; // restore
        continue;
      }
      if (quality < bestQuality)
      {
        best = current;
        bestQuality = quality;
      }

      if (quality < currentQuality)
      {
        currentQuality = quality;
      }
      else
      {
        float chance = exp(-(quality - bestQuality) / temperature);
        if (chanceDistribution(gen) < chance)
          currentQuality = quality;
        else
          current[j % current.size()] = c; // restore
      }
    }
  }
  G_ASSERTF(bestQuality >= 0, "Best state is invalid");

  for (unsigned int i = 0; i < result.size(); ++i)
  {
    result[i] = decode_position(best[i]);
    result[i].y *= -1;
  }
}

static void perform_padding(const PostFxRenderer &padder, ImpostorBaker::ImpostorTextureData &source,
  ImpostorBaker::ImpostorTextureData &destination, uint32_t mip)
{
  SCOPE_RENDER_TARGET;
  TIME_D3D_PROFILE(impostor_padding);
  d3d::set_render_target({nullptr, 0}, true,
    {{destination.albedo_alpha.getTex2D(), mip}, {destination.normal_translucency.getTex2D(), mip},
      {destination.ao_smoothness.getTex2D(), mip}});
  TextureInfo info;
  destination.albedo_alpha.getTex2D()->getinfo(info, mip);
  d3d::setview(0, 0, info.w, info.h, 0, 1);
  ShaderGlobal::set_color4(texture_sizeVarId, Color4(info.w, info.h, 0, 0));
  source.albedo_alpha.getTex2D()->texmiplevel(mip, mip);
  source.normal_translucency.getTex2D()->texmiplevel(mip, mip);
  source.ao_smoothness.getTex2D()->texmiplevel(mip, mip);
  ShaderGlobal::set_texture(impostor_packed_albedo_alphaVarId, source.albedo_alpha.getTexId());
  ShaderGlobal::set_texture(impostor_packed_normal_translucencyVarId, source.normal_translucency.getTexId());
  ShaderGlobal::set_texture(impostor_packed_ao_smoothnessVarId, source.ao_smoothness.getTexId());
  padder.render();
}

ImpostorBaker::ImpostorData ImpostorBaker::generate(DagorAsset *asset, const ImpostorTextureManager::GenerationData &gen_data,
  RenderableInstanceLodsResource *res, const String &asset_name, DataBlock *impostor_blk)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR

  G_ASSERTF(gen_data.verticalSamples == 1, "Currently impostor layout only supports slices in a single row");
  G_ASSERT(res && res->lods.size());
  G_ASSERT(get_impostor_texture_mgr());
  TIME_D3D_PROFILE(GenerateImpostor);
  if (!d3d::is_inited())
  {
    G_ASSERTF(0, "Drawing with uninitialized d3d");
    return {};
  }
  {
    TextureIdSet usedTexIds;
    res->gatherUsedTex(usedTexIds);
    if (!prefetch_and_check_managed_textures_loaded(usedTexIds))
    {
      conerror("There was a problem loading the textures");
      return {};
    }
  }

  SCOPE_VIEW_PROJ_MATRIX;

  prepareTextures(gen_data);
  IPoint2 extent = get_extent(gen_data);

  int sliceHeight = SLICE_SIZE;
  int sliceWidth = SLICE_SIZE;

  ImpostorExportData exportData;
  exportData.vertexOffset.resize(NUM_SCALED_VERTICES, Point2{0, 0});

  UniqueTex impostorMask =
    dag::create_tex(nullptr, sliceWidth, sliceHeight, TEXFMT_L8 | TEXCF_RTARGET | TEXCF_READABLE, 1, (asset_name + "_mask").c_str());

  PackedImpostorSlices sliceLayout =
    PackedImpostorSlices(asset->getName(), gen_data.horizontalSamples, IPoint2(sliceWidth, sliceHeight));

  eastl::vector<float> scales(256, 0.f);
  const float startingAngle = 1.5f * 2 * static_cast<float>(PI) / NUM_SCALED_VERTICES;

  int sliceId = 0;
  const DataBlock *blkProp = asset->props.getBlockByNameEx("impostor");
  AOData aoData = generateAOData(res, *blkProp);
  treeCrownData = {};
  treeCrownData.crownCenter1 = aoData.crownCenter1;
  if (aoData.crownRad1.x > 0. && aoData.crownRad1.y > 0. && aoData.crownRad1.z > 0.)
  {
    treeCrownData.invCrownRad1.x = 1. / aoData.crownRad1.x;
    treeCrownData.invCrownRad1.y = 1. / aoData.crownRad1.y;
    treeCrownData.invCrownRad1.z = 1. / aoData.crownRad1.z;
  }
  treeCrownData.crownCenter2 = aoData.crownCenter2;
  if (aoData.crownRad2.x > 0. && aoData.crownRad2.y > 0. && aoData.crownRad2.z > 0.)
  {
    treeCrownData.invCrownRad2.x = 1. / aoData.crownRad2.x;
    treeCrownData.invCrownRad2.y = 1. / aoData.crownRad2.y;
    treeCrownData.invCrownRad2.z = 1. / aoData.crownRad2.z;
  }
  float aoBrightnessLocal, aoFalloffStartLocal, aoFalloffStopLocal;
  aoBrightnessLocal = blkProp->getReal("aoBrightness", aoBrightness);
  aoFalloffStartLocal = blkProp->getReal("aoFalloffStart", aoFalloffStart);
  aoFalloffStopLocal = blkProp->getReal("aoFalloffStop", aoFalloffStop);
  treeCrownData.brightness_falloff_start_stop_dist = Point4(aoBrightnessLocal, aoFalloffStartLocal, aoFalloffStopLocal, 0.);

  treeCrown->updateDataWithLock(0, sizeof(TreeCrown), (void *)&treeCrownData, VBLOCK_WRITEONLY);
  for (int v = 0; v < gen_data.verticalSamples; ++v)
  {
    for (int h = 0; h < gen_data.horizontalSamples; ++h, ++sliceId)
    {
      TIME_D3D_PROFILE(mask_impostor_slice);
      d3d::set_buffer(STAGE_VS, rendinstgenrender::INSTANCING_TEXREG, objectsBuffer.getBuf());
      get_impostor_texture_mgr()->setTreeCrownDataBuf(treeCrown.getBuf());
      if (gen_data.octahedralImpostor)
        get_impostor_texture_mgr()->generate_mask_octahedral(h, v, gen_data, res, maskRt.get(), impostorMask.getTex2D());
      else
        get_impostor_texture_mgr()->generate_mask_billboard(sliceId, res, maskRt.get(), impostorMask.getTex2D());
      get_impostor_texture_mgr()->setTreeCrownDataBuf(nullptr);
      int stride = 0;
      if (auto data = lock_texture<uint8_t>(impostorMask.getTex2D(), stride, 0, TEXLOCK_READ))
      {
        Point3 pointToEye;
        if (gen_data.octahedralImpostor)
        {
          pointToEye = ImpostorTextureManager::get_point_to_eye_octahedral(h, v, gen_data);
        }
        else
        {
          pointToEye = ImpostorTextureManager::get_point_to_eye_billboard(sliceId);
        }
        for (unsigned int y0 = 0; y0 < sliceHeight; ++y0)
        {
          for (unsigned int x0 = 0; x0 < sliceWidth; ++x0)
          {
            const size_t idx = static_cast<size_t>(y0 * stride) + x0; //-V1028
            const bool used = data.get()[idx] != 0;
            if (!used)
              continue;
            // px and py are in [-1, 1)
            const float px = (static_cast<float>(x0) / sliceWidth) * 2 - 1.f;
            const float py = (static_cast<float>(y0) / sliceHeight) * 2 - 1.f;
            float l = hypotf(px, py);
            float angle = l < 0.00001 ? 0.f : static_cast<float>(atan2(-py, px));
            static_assert(NUM_SCALED_VERTICES == 8, "update this line, if the number of vertices changes");
            angle = norm_ang(angle);
            const int id1 = int(angle / (2 * PI) * scales.size()) % scales.size();
            const int id2 = (id1 + 1) % scales.size();
            // inverse of lerp (used in rendinst_impostor_octahedral)
            float scale;
            if (::abs(ImpostorTextureManager::get_vertex_scaling(::abs(pointToEye.y))) > 0)
              scale = min((l - 1.f) / ImpostorTextureManager::get_vertex_scaling(::abs(pointToEye.y)) + 1, 1.f);
            else
              scale = 0.f;
            scales[id1] = std::max(scales[id1], scale);
            scales[id2] = std::max(scales[id2], scale);
          }
        }
        sliceLayout.setSlice(h, data.get(), stride);
      }
#if DAGOR_DBGLEVEL > 0
      else
        G_ASSERT(0);
#endif
    }
  }
  sliceLayout.packTexture(IPoint2(gen_data.textureWidth, gen_data.textureHeight));
  get_wrapping_polygon(startingAngle, scales, exportData.vertexOffset, asset_name.c_str());

  PostFxRenderer packTexturesShader;
  PostFxRenderer genMipShader;
  PostFxRenderer impostorPostprocessor;
  PostFxRenderer impostorColorPadding;
  packTexturesShader.init("pack_impostor_texture");
  genMipShader.init("impostor_gen_mip");
  impostorPostprocessor.init("impostor_postprocessor");
  impostorColorPadding.init("impostor_color_padding");
  int globalFrameBlockId = ShaderGlobal::getBlockId("global_frame");
  int rendinstSceneBlockId = ShaderGlobal::getBlockId("rendinst_scene");
  int impostor_sdf_tex_const_no = 0;
  bool variableExists = ShaderGlobal::get_int_by_name("impostor_sdf_tex_const_no", impostor_sdf_tex_const_no);
  G_ASSERTF(variableExists, "getting impostor_sdf_tex_const_no failed");

  // 8 is sufficient to avoid artifacts caused by block compression, but not too slow to calculate. Otherwise arbitrary
  ShaderGlobal::set_int(impostor_color_paddingVarId, 8);

  ImpostorTextureData packedTextures = prepareRt(extent, asset_name + "_packed", 1);
  if (!packedTextures)
    return {};

  ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::set_int(impostor_sdf_max_distanceVarId, RENDER_OVERSCALE * 8);
  UniqueTexHolder impostorSdfTex =
    dag::create_tex(nullptr, gen_data.textureWidth, gen_data.textureHeight, TEXFMT_R32UI | TEXCF_UNORDERED, 1, "impostor_sdf_tex");
  uint32_t maxDistance[4] = {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu};
  d3d::clear_rwtexi(impostorSdfTex.getTex2D(), maxDistance, 0, 0);
  for (uint32_t v = 0; v < gen_data.verticalSamples; ++v)
  {
    for (uint32_t h = 0; h < gen_data.horizontalSamples; ++h)
    {
      TIME_D3D_PROFILE(process_slice);
      unsigned int sliceId = gen_data.horizontalSamples * v + h;
      ShaderGlobal::set_int(slice_idVarId, sliceId);

      ImpostorBaker::SliceExportData sliceExportData;
      sliceExportData.transform.x = sliceLayout.xScaling * sliceWidth / gen_data.textureWidth;
      sliceExportData.transform.y = float(sliceHeight) / sliceLayout.slices[h].contentBbox.width().y;
      sliceExportData.transform.z =
        sliceLayout.xScaling * (sliceLayout.slices[h].xOffset - sliceLayout.slices[h].contentBbox.getMin().x) / gen_data.textureWidth;
      sliceExportData.transform.w = float(-sliceLayout.slices[h].contentBbox.getMin().y) / sliceLayout.slices[h].contentBbox.width().y;
      if (sliceLayout.slices[h].flipped)
      {
        sliceExportData.transform.y *= -1;
        sliceExportData.transform.w = 1 - sliceExportData.transform.w;
      }
      sliceExportData.clippingLines = sliceLayout.slices[h].normalizedSeparatorLines;

      exportData.sliceData.push_back(sliceExportData);

      {
        TIME_D3D_PROFILE(render_slice);
        SCOPE_RENDER_TARGET;
        d3d::set_buffer(STAGE_VS, rendinstgenrender::INSTANCING_TEXREG, objectsBuffer.getBuf());
        get_impostor_texture_mgr()->setTreeCrownDataBuf(treeCrown.getBuf());
        get_impostor_texture_mgr()->start_rendering_slices(rt.get());
        d3d::setview(0, 0, get_rt_extent(gen_data).x, get_rt_extent(gen_data).y, 0, 1);

        TMatrix viewToContent = sliceLayout.getViewToContent(sliceId, IPoint2(gen_data.textureWidth, gen_data.textureHeight));

        if (gen_data.octahedralImpostor)
          get_impostor_texture_mgr()->render_slice_octahedral(h, v, viewToContent, gen_data, res, rendinstSceneBlockId);
        else
          get_impostor_texture_mgr()->render_slice_billboard(sliceId, viewToContent, res, rendinstSceneBlockId);
        get_impostor_texture_mgr()->setTreeCrownDataBuf(nullptr);
        get_impostor_texture_mgr()->end_rendering_slices();
      }
      {
        TIME_D3D_PROFILE(pack_slice);
        SCOPE_RENDER_TARGET;
        d3d::set_render_target({nullptr, 0}, true,
          {{packedTextures.albedo_alpha.getTex2D(), 0}, {packedTextures.normal_translucency.getTex2D(), 0},
            {packedTextures.ao_smoothness.getTex2D(), 0}});
        d3d::setview(0, 0, gen_data.textureWidth, gen_data.textureHeight, 0, 1);
        rt->setVar();
        ShaderGlobal::set_color4(texture_sizeVarId, Color4(gen_data.textureWidth, gen_data.textureHeight, 0, 0));
        if (sliceLayout.slices[h].flipped)
          ShaderGlobal::set_color4(impostor_texture_transformVarId, Color4(1, -1, 0, 1));
        else
          ShaderGlobal::set_color4(impostor_texture_transformVarId, Color4(1, 1, 0, 0));
        // The STATE_GUARD_NULLPTR macro fails with these expressions
        Texture *const tex1 = rt->getRt(0);
        Texture *const tex2 = rt->getRt(1);
        Texture *const tex3 = rt->getRt(2);
        STATE_GUARD_NULLPTR(d3d::set_tex(STAGE_PS, dynamic_impostor_texture_const_no, VALUE), tex1);
        STATE_GUARD_NULLPTR(d3d::set_tex(STAGE_PS, dynamic_impostor_texture_const_no + 1, VALUE), tex2);
        STATE_GUARD_NULLPTR(d3d::set_tex(STAGE_PS, dynamic_impostor_texture_const_no + 2, VALUE), tex3);
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_PS, impostor_sdf_tex_const_no, VALUE, 0, 0), impostorSdfTex.getTex2D());
        packTexturesShader.render();
      }
    }
  }

  ImpostorTextureData prePaddingTextures = prepareRt(extent, asset_name + "_pre_padding", 0);
  if (!prePaddingTextures)
    return {};

  ImpostorData ret;
  ret.quality = sliceLayout.quality;
  ret.textures = prepareRt(extent, asset_name, 0);
  if (!ret.textures)
    return {};
  {
    SCOPE_RENDER_TARGET;
    TIME_D3D_PROFILE(postprocess_impostor);
    d3d::set_render_target({nullptr, 0}, true,
      {{prePaddingTextures.albedo_alpha.getTex2D(), 0}, {prePaddingTextures.normal_translucency.getTex2D(), 0},
        {prePaddingTextures.ao_smoothness.getTex2D(), 0}});
    d3d::setview(0, 0, gen_data.textureWidth, gen_data.textureHeight, 0, 1);
    ShaderGlobal::set_color4(texture_sizeVarId, Color4(gen_data.textureWidth, gen_data.textureHeight, 0, 0));
    ShaderGlobal::set_texture(impostor_packed_albedo_alphaVarId, packedTextures.albedo_alpha.getTexId());
    ShaderGlobal::set_texture(impostor_packed_normal_translucencyVarId, packedTextures.normal_translucency.getTexId());
    ShaderGlobal::set_texture(impostor_packed_ao_smoothnessVarId, packedTextures.ao_smoothness.getTexId());
    impostorPostprocessor.render();
  }

  perform_padding(impostorColorPadding, prePaddingTextures, ret.textures, 0);

  for (uint32_t mip = 1; mip < ret.textures.albedo_alpha->level_count(); ++mip)
  {
    {
      SCOPE_RENDER_TARGET;
      TIME_D3D_PROFILE(generate_mips);
      d3d::set_render_target({nullptr, 0}, true,
        {{prePaddingTextures.albedo_alpha.getTex2D(), mip}, {prePaddingTextures.normal_translucency.getTex2D(), mip},
          {prePaddingTextures.ao_smoothness.getTex2D(), mip}});
      d3d::setview(0, 0, gen_data.textureWidth >> mip, gen_data.textureHeight >> mip, 0, 1);
      ShaderGlobal::set_color4(texture_sizeVarId, Color4(gen_data.textureWidth >> mip, gen_data.textureHeight >> mip, 0, 0));
      ret.textures.albedo_alpha.getTex2D()->texmiplevel(mip - 1, mip - 1);
      ret.textures.normal_translucency.getTex2D()->texmiplevel(mip - 1, mip - 1);
      ret.textures.ao_smoothness.getTex2D()->texmiplevel(mip - 1, mip - 1);
      ShaderGlobal::set_texture(impostor_packed_albedo_alphaVarId, ret.textures.albedo_alpha.getTexId());
      ShaderGlobal::set_texture(impostor_packed_normal_translucencyVarId, ret.textures.normal_translucency.getTexId());
      ShaderGlobal::set_texture(impostor_packed_ao_smoothnessVarId, ret.textures.ao_smoothness.getTexId());
      genMipShader.render();
    }
    perform_padding(impostorColorPadding, prePaddingTextures, ret.textures, mip);
  }
  ret.textures.albedo_alpha.getTex2D()->texmiplevel(-1, -1);
  ret.textures.normal_translucency.getTex2D()->texmiplevel(-1, -1);
  ret.textures.ao_smoothness.getTex2D()->texmiplevel(-1, -1);
  if (impostor_blk)
    exportAssetBlk(asset, res, exportData, *impostor_blk, aoData);
  return ret;
}

AOData ImpostorBaker::generateAOData(RenderableInstanceLodsResource *res, const DataBlock &blk_in)
{
  AOData aoData;
  Point3 crownCenterOffset1 = blk_in.getPoint3("crownCenterOffset1", Point3(0, 0, 0));
  Point3 crownRadOffset1 = blk_in.getPoint3("crownRadOffset1", Point3(0, 0, 0));

  ShaderMesh *mesh = res->lods[0].scene->getMesh()->getMesh()->getMesh();
  dag::Span<ShaderMesh::RElem> elems = mesh->getElems(mesh->STG_atest, mesh->STG_atest);
  BBox3 bboxCrown;
  for (unsigned int elemNo = 0; elemNo < elems.size(); ++elemNo)
  {
    ShaderMesh::RElem &elem = elems[elemNo];
    if (!elem.e)
      continue;
    const GlobalVertexData *vd = elem.vertexData;
    eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>> dstVB, dstIB;
    dstVB.reset(d3d::create_sbuffer(1, elem.vertexData->getVbSize(), SBCF_MAYBELOST | SBCF_USAGE_READ_BACK, 0, "readBackVB"));
    vd->getVB()->copyTo(dstVB.get(), 0, elem.baseVertex * elem.vertexData->getStride(), elem.vertexData->getVbSize());

    uint8_t *memVB;
    if (dstVB->lock(0, dstVB->ressize(), (void **)&memVB, VBLOCK_READONLY))
    {
      Sbuffer *ib = elem.vertexData->getIB();
      uint8_t *memIB;
      uint32_t count = elem.numf;
      uint32_t start = 0;
      dstIB.reset(d3d::create_sbuffer(1, elem.vertexData->getIbSize(), SBCF_MAYBELOST | SBCF_USAGE_READ_BACK, 0, "readBackIB"));
      if (ib)
        ib->copyTo(dstIB.get(), 0, elem.si * elem.vertexData->getIbElemSz(), elem.vertexData->getIbSize());
#if DAGOR_DBGLEVEL > 0
      else
        G_ASSERT(0);
#endif
      if (dstIB->lock(0, dstIB->ressize(), (void **)&memIB, VBLOCK_READONLY))
      {
        for (int k = start * 3, i = 0; k < start * 3 + count * 3; k += 3, ++i)
        {
          uint8_t *ip = memIB + k * elem.vertexData->getIbElemSz();
          uint32_t idx[3];
          idx[0] = elem.vertexData->getIbElemSz() == 2 ? *(uint16_t *)ip : *(uint32_t *)ip;
          idx[1] = elem.vertexData->getIbElemSz() == 2 ? *((uint16_t *)ip + 1) : *((uint32_t *)ip + 1);
          idx[2] = elem.vertexData->getIbElemSz() == 2 ? *((uint16_t *)ip + 2) : *((uint32_t *)ip + 2);
          for (int j = 0; j < 3; ++j)
          {
            float *v = (float *)(memVB + idx[j] * elem.vertexData->getStride());
            const Point3 p = Point3(*v, *(v + 1), *(v + 2));
            bboxCrown += p;
          }
        }
        dstIB->unlock();
      }
      dstVB->unlock();
    }
    const BBox3 &riBBox = res->bbox;
    const Point3 center = riBBox.center();
    const Point3 width = riBBox.width();
    const Point3 norm = Point3(1.f / width.x, 1.f / width.y, 1.f / width.z);

    Point3 pos = bboxCrown.center() + crownCenterOffset1 - center;
    pos.x *= norm.x;
    pos.y *= norm.y;
    pos.z *= norm.z;
    Point3 crownWidth = bboxCrown.width() + crownRadOffset1;
    Point3 scale = Point3(crownWidth.x * norm.x, crownWidth.y * norm.y, crownWidth.z * norm.z);
    Point2 ao1TCPos = Point2(max(pos.x, pos.z), pos.y) * 2.f;
    Point2 ao1TCScale = Point2(max(scale.x, scale.z), scale.y);
    aoData.ao1PosScale = Color4(ao1TCPos.x, ao1TCPos.y, ao1TCScale.x, ao1TCScale.y);
    Point4 crownCenter, crownRad;
    crownCenter.set_xyz0(bboxCrown.center() + crownCenterOffset1);
    aoData.crownCenter1 = crownCenter;
    crownRad.set_xyz0((bboxCrown.width() + crownRadOffset1) * .5f);
    aoData.crownRad1 = crownRad;
    aoData.crownCenter2 = Point4(0, 0, 0, 0);
    aoData.crownRad2 = Point4(0, 0, 0, 0);
    if (blk_in.paramExists("crownCenter2") || blk_in.paramExists("crownRad2"))
    {
      Point3 crownCenter2 = blk_in.getPoint3("crownCenter2", Point3(0, 0, 0));
      aoData.crownCenter2.set_xyz0(crownCenter2);
      Point3 crownRad2 = blk_in.getPoint3("crownRad2", Point3(0, 0, 0));
      aoData.crownRad2.set_xyz0(crownRad2);
      Point3 pos = crownCenter2;
      pos.x *= norm.x;
      pos.y *= norm.y;
      pos.z *= norm.z;
      Point3 scale = Point3(crownRad2.x * norm.x, crownRad2.y * norm.y, crownRad2.z * norm.z);
      Point2 ao2TCPos = Point2(max(pos.x, pos.z), pos.y) * 2.f;
      Point2 ao2TCScale = Point2(max(scale.x, scale.z), scale.y);
      aoData.ao2PosScale = Color4(ao2TCPos.x, ao2TCPos.y, ao2TCScale.x, ao2TCScale.y);
    }
  }

  return aoData;
}

bool ImpostorBaker::saveImage(const char *filename, Texture *tex, int mip_offset, int num_channels)
{
  modifiedFiles.insert(filename);
  int mips = tex->level_count();
  G_ASSERT(mip_offset <= mips);
  mips = mips - mip_offset;
  TextureInfo info;
  tex->getinfo(info);
  int w = info.w >> mip_offset;
  int h = info.h >> mip_offset;
  const int imageHeight = h;
  const int imageWidth = mips > 1 ? (w * 2 - 1) : w;

  eastl::unique_ptr<TexImage32> image;
  image.reset(TexImage32::create(imageWidth, imageHeight));

  memset(image->getPixels(), 0, sizeof(uint32_t) * imageWidth * imageHeight);

  // Tiff exporter doesn't support 2 chanelled textures. daBuild will have to ignore the extra channel
  int numExportedChannels = num_channels == 2 ? 3 : num_channels;

  int xoffset = 0;
  for (int i = 0; i < mips; ++i)
  {
    int mipW = w >> i;
    int mipH = h >> i;

    G_ASSERT((xoffset + mipW) <= imageWidth);

    uint8_t *data;
    int texStride;
    if (tex->lockimg(reinterpret_cast<void **>(&data), texStride, i + mip_offset, TEXLOCK_READ) && data)
    {
      for (int y = 0; y < mipH; ++y)
      {
        for (int x = 0; x < mipW; ++x)
        {
          const size_t texIdx = size_t(y * texStride) + x * num_channels; //-V1028
          const size_t idx = size_t(y * imageWidth) + (x + xoffset);      //-V1028
          image->getPixels()[idx].b = data[texIdx + 0];
          image->getPixels()[idx].g = data[texIdx + 1];
          if (num_channels > 2)
            image->getPixels()[idx].r = data[texIdx + 2];
          else
            image->getPixels()[idx].r = data[texIdx + 0];
          if (numExportedChannels > 3)
            image->getPixels()[idx].a = data[texIdx + 3];
        }
      }
      if (!tex->unlockimg())
        return false;
    }
    else
      return false;
    xoffset += mipW;
  }
  if (numExportedChannels == 4)
    return save_tiff32(filename, image.get());
  else if (numExportedChannels == 3)
    return save_tiff24(filename, image.get());
  else
    G_ASSERTF(false, "Can't save tiff with %d channels", numExportedChannels);
  return false;
}

ImpostorBaker::ImpostorTextureData ImpostorBaker::prepareRt(IPoint2 extent, String asset_name, int mips) noexcept
{
  ImpostorTextureData ret;
  ret.albedo_alpha = dag::create_tex(nullptr, extent.x, extent.y,
    EXP_ALBEDO_ALPHA_FMT | TEXCF_RTARGET | TEXCF_SRGBREAD | TEXCF_SRGBWRITE | TEXCF_CLEAR_ON_CREATE, mips,
    (asset_name + "_albedo_alpha").c_str());
  ret.normal_translucency = dag::create_tex(nullptr, extent.x, extent.y,
    EXP_NORMAL_TRANSLUCENCY_FMT | TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE, mips, (asset_name + "_normal_translucency").c_str());
  ret.ao_smoothness = dag::create_tex(nullptr, extent.x, extent.y, EXP_AO_SMOOTHNESS_FMT | TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE, mips,
    (asset_name + "_ao_smoothness").c_str());
  if (!ret)
  {
    conerror("Textures could not be created");
    return {};
  }
  return ret;
}

void ImpostorBaker::prepareTextures(const ImpostorTextureManager::GenerationData &gen_data) noexcept
{
  static unsigned int textureCount = 0; // this helps with debugging
  const IPoint2 extent = get_rt_extent(gen_data);
  // These are temporary textures used as render targets
  if (rt && rt->getWidth() == extent.x && rt->getHeight() == extent.y)
    return;
  const static unsigned formats[] = {GBUF_DIFFUSE_FMT | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, GBUF_NORMAL_FMT, GBUF_SHADOW_FMT};
  rt.reset();
  rt = eastl::make_unique<DeferredRenderTarget>(nullptr, String(0, "impostor_baker_rt_%d", textureCount), extent.x, extent.y,
    DeferredRT::StereoMode::MonoOrMultipass, 0, 3, formats, GBUF_DEPTH_FMT);
  maskRt.reset();
  maskRt = eastl::make_unique<DeferredRenderTarget>(nullptr, String(0, "impostor_baker_mask_rt_%d", textureCount), SLICE_SIZE,
    SLICE_SIZE, DeferredRT::StereoMode::MonoOrMultipass, 0, 3, formats, GBUF_DEPTH_FMT);
  textureCount++;
}

IPoint2 ImpostorBaker::get_extent(const ImpostorTextureManager::GenerationData &gen_data)
{
  return IPoint2(gen_data.textureWidth, gen_data.textureHeight);
}

IPoint2 ImpostorBaker::get_rt_extent(const ImpostorTextureManager::GenerationData &gen_data)
{
  return IPoint2(gen_data.textureWidth * RENDER_OVERSCALE, gen_data.textureHeight * RENDER_OVERSCALE);
}

TexturePackingProfilingInfo ImpostorBaker::exportImpostor(DagorAsset *asset, const ImpostorOptions &options, DataBlock *impostor_blk)
{
  int resIdx = rendinst::addRIGenExtraResIdx(asset->getName(), -1, -1, rendinst::AddRIFlag::UseShadow);
  RenderableInstanceLodsResource *res = rendinst::getRIGenExtraRes(resIdx);
  if (res == nullptr)
  {
    conerror("Could not load rendInst: %s", asset->getName());
    return {};
  }
  ImpostorTextureManager::GenerationData genData;
  genData = ImpostorTextureManager::load_data(&asset->props, res, smallestMipSize, preshadowsEnabled, mipOffsets_hq_mq_lq,
    mobileMipOffsets_hq_mq_lq, defaultTextureQualityLevels.size(), defaultTextureQualityLevels.data());
  conlog(" Settings: textureWidth=%d, textureHeight=%d, horizontalSamples=%d, verticalSamples=%d", genData.textureWidth,
    genData.textureHeight, genData.horizontalSamples, genData.verticalSamples);
  ImpostorData impostorData = exportToFile(genData, asset, res, impostor_blk);
  if (!impostorData.textures)
  {
    conerror("Impostor could not be rendered: %s", asset->getName());
    return {};
  }
  if (displayExportedImages)
  {
    d3d::stretch_rect(impostorData.textures.albedo_alpha.getTex2D(), nullptr);
    d3d::update_screen();
  }
  return impostorData.quality;
}

eastl::string ImpostorBaker::getDDSxPackName(const char *folder_path)
{
  G_ASSERT(strlen(folder_path) > 0);
  int folderNameStart = strlen(folder_path) - 1;
  int folderNameSize = (folder_path[folderNameStart] == '/' || folder_path[folderNameStart] == '\\') ? 0 : 1;
  while (folderNameStart > 0 && folder_path[folderNameStart - 1] != '/' && folder_path[folderNameStart - 1] != '\\')
  {
    folderNameStart--;
    folderNameSize++;
  }
  return eastl::string(folder_path + folderNameStart, folderNameSize);
}

void ImpostorBaker::computeDDSxPackSizes(dag::ConstSpan<DagorAsset *> assets)
{
  for (auto *asset : assets)
  {
    if (!isSupported(asset))
      continue;
    const char *folderPath = asset->getFolderPath();
    eastl::string ddsxPackName = getDDSxPackName(folderPath);
    auto matched = ddsxPackSizes.find(ddsxPackName);
    if (matched != ddsxPackSizes.end())
      matched->second += 1;
    else
      ddsxPackSizes[ddsxPackName] = 1;
  }
}

void ImpostorBaker::generateFolderBlk(DagorAsset *asset, const ImpostorOptions &options)
{
  const char *folderPath = asset->getFolderPath();
  generateFolderBlk(getAssetFolder(asset), getDDSxPackName(folderPath).c_str(), options);
}

bool ImpostorBaker::hasSupportedType(DagorAsset *asset) const
{
  static int rendInstEntityClassId = DAEDITOR3.getAssetTypeId("rendInst");
  return asset->getType() == rendInstEntityClassId;
}

bool ImpostorBaker::isSupported(DagorAsset *asset) const
{
  if (!hasSupportedType(asset))
    return false;
  int nid = asset->props.getNameId("lod");
  for (int i = 0; DataBlock *lodBlk = asset->props.getBlock(i); i++)
  {
    if (lodBlk->getBlockNameId() != nid)
      continue;
    if (!lodBlk->paramExists("fname"))
      continue;
    if (strstr(lodBlk->getStr("fname", ""), "billboard_octagon_impostor") != nullptr)
      return true;
  }
  return false;
}

static int to_sh(float v) { return int(round(v * 1000.f)); }

ska::flat_hash_map<eastl::string, int> ImpostorBaker::getHashes(DagorAsset *asset) const
{
  ska::flat_hash_map<eastl::string, int> ret;
  const DataBlock *impostorBlock = asset->props.getBlockByNameEx("impostor");

  IPoint3 mipOffsets =
    impostorBlock->paramExists("mipOffsets_hq_mq_lq") ? impostorBlock->getIPoint3("mipOffsets_hq_mq_lq") : mipOffsets_hq_mq_lq;
  IPoint3 mobileMipOffsets = impostorBlock->paramExists("mobileMipOffsets_hq_mq_lq")
                               ? impostorBlock->getIPoint3("mobileMipOffsets_hq_mq_lq")
                               : mobileMipOffsets_hq_mq_lq;

  ret["mipHq"] = mipOffsets.x;
  ret["mipMq"] = mipOffsets.y;
  ret["mipLq"] = mipOffsets.z;
  ret["mobileMipHq"] = mobileMipOffsets.x;
  ret["mobileMipMq"] = mobileMipOffsets.y;
  ret["mobileMipLq"] = mobileMipOffsets.z;
  ret["smallestMip"] = smallestMipSize;
  ret["normalMipOffset"] = normalMipOffset;
  ret["aoSmoothnessMipOffset"] = aoSmoothnessMipOffset;
  ret["preshadow"] = impostorBlock->paramExists("preshadowEnabled") ? impostorBlock->getBool("preshadowEnabled") : preshadowsEnabled;
  ret["width"] = impostorBlock->paramExists("textureWidth") ? impostorBlock->getInt("textureWidth") : -1;
  if (impostorBlock->paramExists("textureHeight"))
  {
    ret["height"] = impostorBlock->getInt("textureHeight");
    ret["heightHash"] = -1;
  }
  else
  {
    ret["height"] = -1;
    int heightHash = 0;
    for (const auto &qualityLevel : defaultTextureQualityLevels)
      heightHash ^= to_sh(qualityLevel.minHeight) * qualityLevel.textureHeight;
    heightHash = heightHash > 0 ? -heightHash : heightHash;
    ret["heightHash"] = heightHash;
  }
  float aoBrightnessLocal, aoFalloffStartLocal, aoFalloffStopLocal;
  aoBrightnessLocal = impostorBlock->getReal("aoBrightness", aoBrightness);
  aoFalloffStartLocal = impostorBlock->getReal("aoFalloffStart", aoFalloffStart);
  aoFalloffStopLocal = impostorBlock->getReal("aoFalloffStop", aoFalloffStop);
  ret["aoBrightness"] = to_sh(aoBrightnessLocal);
  ret["aoFalloffStart"] = to_sh(aoFalloffStartLocal);
  ret["aoFalloffStart"] = to_sh(aoFalloffStopLocal);
  return ret;
}

ImpostorBaker::ImpostorData ImpostorBaker::exportToFile(const ImpostorTextureManager::GenerationData &gen_data, DagorAsset *asset,
  RenderableInstanceLodsResource *res, DataBlock *impostor_blk)
{
  ImpostorData impostor = generate(asset, gen_data, res, String(asset->getName()), impostor_blk);
  if (!impostor.textures)
  {
    conerror("Could not generate the impostor texture <%s>", asset->getName());
    return {};
  }

  String outFolder = getAssetFolder(asset);
  ::dd_mkpath(outFolder.c_str());

  String name(0, "%s", asset->getName());

  const IPoint2 extent = get_extent(gen_data);

  bool ret = true;
  ret =
    ret && saveImage((outFolder + name + "_" + ALBEDO_ALPHA_NAME + ".tiff").c_str(), impostor.textures.albedo_alpha.getTex2D(), 0, 4);
  ret = ret && saveImage((outFolder + name + "_" + NORMAL_TRANSLUCENCY_NAME + ".tiff").c_str(),
                 impostor.textures.normal_translucency.getTex2D(), normalMipOffset, 4);
  ret = ret && saveImage((outFolder + name + "_" + AO_SMOOTHNESS_NAME + ".tiff").c_str(), impostor.textures.ao_smoothness.getTex2D(),
                 aoSmoothnessMipOffset, 2);
  ret = ret && generateAssetTexBlk(asset, gen_data, outFolder.c_str(), name.c_str());
  if (ret)
    return impostor;
  else
    return {};
}

void ImpostorBaker::gatherExportedFiles(DagorAsset *asset, eastl::set<String, StrLess> &files) const
{
  if (!isSupported(asset))
    return;

  String baseFolder = getAssetFolder(asset);

  files.insert(String(0, "%s/%s_%s.tiff", baseFolder.c_str(), asset->getName(), ALBEDO_ALPHA_NAME));
  files.insert(String(0, "%s/%s_%s.tiff", baseFolder.c_str(), asset->getName(), NORMAL_TRANSLUCENCY_NAME));
  files.insert(String(0, "%s/%s_%s.tiff", baseFolder.c_str(), asset->getName(), AO_SMOOTHNESS_NAME));
  files.insert(String(0, "%s/%s_%s.tex.blk", baseFolder.c_str(), asset->getName(), ALBEDO_ALPHA_NAME));
  files.insert(String(0, "%s/%s_%s.tex.blk", baseFolder.c_str(), asset->getName(), NORMAL_TRANSLUCENCY_NAME));
  files.insert(String(0, "%s/%s_%s.tex.blk", baseFolder.c_str(), asset->getName(), AO_SMOOTHNESS_NAME));
}

void ImpostorBaker::clean(dag::ConstSpan<DagorAsset *> assets, const ImpostorOptions &options)
{
  std::regex impostorTexReg{FILE_REGEX};
  eastl::set<String, StrLess> files;
  eastl::set<String, StrLess> folders;
  bool stop = false;
  for (int i = 0; i < assets.size(); ++i)
    if (isSupported(assets[i]))
      gatherExportedFiles(assets[i], files);
  for (unsigned int i = 0; i < assets.size(); ++i)
  {
    String folder = getAssetFolder(assets[i]);
    if (folders.count(folder) > 0 || !::dd_dir_exist(folder.c_str()))
      continue;
    folders.insert(folder);
    alefind_t tex;
    if (!::dd_find_first(String(0, "%s/*", folder.c_str()).c_str(), DA_FILE, &tex))
      continue;
    conlog("Cleaning %s/*", folder.c_str());
    do
    {
      // .folder.blk files are allowed
      if (strcmp(tex.name, ".folder.blk") == 0)
        continue;
      if (!std::regex_match(tex.name, impostorTexReg))
      {
        // This might happen if the folder is configured incorrectly
        // If the folder is set to an existing asset folder
        // this process would clean up that folder
        conerror("Unexpected file (%s) in impostor folder. Aborting clean", tex.name);
        stop = true;
        break;
      }
      const String file(0, "%s/%s", folder.c_str(), tex.name);
      if (files.count(file) == 0)
      {
        conlog("Erasing file: %s%s", file.c_str(), options.dryMode ? " (dry running)" : "");
        if (!options.dryMode)
          if (::dd_erase(file.c_str()))
            removedFiles.insert(file.c_str());
      }
    } while (!stop && ::dd_find_next(&tex));
    ::dd_find_close(&tex);
  }
}

void ImpostorBaker::exportAssetBlk(DagorAsset *asset, RenderableInstanceLodsResource *res, const ImpostorExportData &export_data,
  DataBlock &blk, const AOData &ao_data) const
{
  G_ASSERT(asset != nullptr);
  const ImpostorTextureManager::GenerationData genData =
    ImpostorTextureManager::load_data(&asset->props, res, smallestMipSize, preshadowsEnabled, mipOffsets_hq_mq_lq,
      mobileMipOffsets_hq_mq_lq, defaultTextureQualityLevels.size(), defaultTextureQualityLevels.data());
  blk.setBool("preshadowEnabled", genData.preshadowEnabled);
  blk.setInt("horizontalSamples", genData.horizontalSamples);
  blk.setInt("verticalSamples", genData.verticalSamples);
  blk.setBool("allowParallax", genData.allowParallax);
  blk.setBool("allowTriView", genData.allowTriView);
  blk.setBool("octahedralImpostor", genData.octahedralImpostor);
  blk.setInt("rotationPaletteSize", genData.rotationPaletteSize);
  blk.setPoint3("tiltLimit", genData.tiltLimit);
  Point2 scale = ImpostorTextureManager::get_scale(res);
  blk.setPoint2("scale", scale);
  G_ASSERT(export_data.vertexOffset.size() == 8);
  blk.setPoint4("vertexOffsets1_2", Point4(export_data.vertexOffset[0].x, export_data.vertexOffset[0].y, export_data.vertexOffset[1].x,
                                      export_data.vertexOffset[1].y));
  blk.setPoint4("vertexOffsets3_4", Point4(export_data.vertexOffset[2].x, export_data.vertexOffset[2].y, export_data.vertexOffset[3].x,
                                      export_data.vertexOffset[3].y));
  blk.setPoint4("vertexOffsets5_6", Point4(export_data.vertexOffset[4].x, export_data.vertexOffset[4].y, export_data.vertexOffset[5].x,
                                      export_data.vertexOffset[5].y));
  blk.setPoint4("vertexOffsets7_8", Point4(export_data.vertexOffset[6].x, export_data.vertexOffset[6].y, export_data.vertexOffset[7].x,
                                      export_data.vertexOffset[7].y));
  for (int i = 0; i < export_data.sliceData.size(); i++)
  {
    blk.setPoint4(String(0, "tm_%d", i), export_data.sliceData[i].transform);
    blk.setPoint4(String(0, "lines_%d", i), export_data.sliceData[i].clippingLines);
  }
  blk.setPoint4("crownCenter1", ao_data.crownCenter1);
  blk.setPoint4("crownRad1", ao_data.crownRad1);
  if (ao_data.crownCenter2 != Point4(0, 0, 0, 0))
    blk.setPoint4("crownCenter2", ao_data.crownCenter2);
  else if (blk.paramExists("crownCenter2"))
    blk.removeParam("crownCenter2");
  if (ao_data.crownRad2 != Point4(0, 0, 0, 0))
    blk.setPoint4("crownRad2", ao_data.crownRad2);
  else if (blk.paramExists("crownRad2"))
    blk.removeParam("crownRad2");
}

String ImpostorBaker::getAssetFolder(const DagorAsset *asset) const { return String(0, "%s/impostors/", asset->getFolderPath()); }

void ImpostorBaker::generateFolderBlk(const char *folder, const char *dxp_prefix, const ImpostorOptions &options)
{
  if (exportedFolderBlks.count(eastl::string(folder)) > 0)
    return;
  exportedFolderBlks.emplace(folder);
  String folderBlk(0, "%s/.folder.blk", folder);
  bool createFolderBlk = false;
  if (options.folderBlkGenMode == ImpostorOptions::FolderBlkGenMode::REPLACE)
    createFolderBlk = true;
  else if (options.folderBlkGenMode == ImpostorOptions::FolderBlkGenMode::DONT_REPLACE && !::dd_file_exist(folderBlk.c_str()))
    createFolderBlk = true;
  if (createFolderBlk)
  {
    modifiedFiles.insert(folderBlk.c_str());
    if (!::dd_dir_exists(folder))
      ::dd_mkdir(folder);
    std::ofstream streamBlk(folderBlk.c_str());
    if (streamBlk)
    {
      streamBlk << "// This file is automatically generated by the impostor baker tool"
                   "(prog/tools/sceneTools/impostorBaker/impostorUtil/impostorBaker.cpp)\n";
      streamBlk << "export {\n"
                   "  ddsxTexPack:t=\""
                << dxp_prefix
                << "_impostors\"\n"
                   "}\n\n";
    }
    else
      conerror("Could not generate .folder.blk in the impostors folder");
  }
}

bool ImpostorBaker::generateAssetTexBlk(DagorAsset *asset, const ImpostorTextureManager::GenerationData &gen_data,
  const char *folder_path, const char *name)
{
  struct TextureData
  {
    const char *suffix;
    const char *type;
    const char *ios_type;
    bool srgb;
    int mipOffset;
    const char *swizzle;
  };
  const eastl::array<const TextureData, 3> tex_types = {TextureData{ALBEDO_ALPHA_NAME, "BC7", "ASTC", true, 0, nullptr},
    TextureData{NORMAL_TRANSLUCENCY_NAME, "BC7", "ASTC", false, normalMipOffset, nullptr},
    TextureData{AO_SMOOTHNESS_NAME, "ATI2N", "ASTC:8:rg", false, aoSmoothnessMipOffset, "GRGR"}};
  for (const TextureData &texData : tex_types)
  {
    String filename = String(0, "%s/%s_%s.tex.blk", folder_path, name, texData.suffix);
    modifiedFiles.insert(filename.c_str());

    DataBlock texBlk;
    int mipCnt = max(0, int(gen_data.mipCount) - 1 - texData.mipOffset); // -1 is because dabuild doesn't count the base level
    texBlk.setStr("name", String(0, "%s_%s.tiff", name, texData.suffix));
    texBlk.setInt("defMaxTexSz", 4096);
    texBlk.setStr("stubTexTag", "black");
    texBlk.setBool("convert", true);
    texBlk.setStr("addrU", "clamp");
    texBlk.setStr("addrV", "clamp");
    texBlk.setStr("mipmapType", "mipmapUseExisting");
    texBlk.setStr("fmt", texData.type);
    if (auto *iOS_blk = texBlk.addBlock("iOS"))
    {
      iOS_blk->setStr("fmt", texData.ios_type);
      iOS_blk->setInt("maxTexSz", 4096);
      iOS_blk->setInt("hqMip", gen_data.mobileMipOffsets_hq_mq_lq.x);
      iOS_blk->setInt("mqMip", gen_data.mobileMipOffsets_hq_mq_lq.y);
      iOS_blk->setInt("lqMip", gen_data.mobileMipOffsets_hq_mq_lq.z);
      iOS_blk->setBool("splitAtOverride", true);
      iOS_blk->setInt("splitAt", splitAtMobile);
    }
    texBlk.setInt("hqMip", gen_data.mipOffsets_hq_mq_lq.x);
    texBlk.setInt("mqMip", gen_data.mipOffsets_hq_mq_lq.y);
    texBlk.setInt("lqMip", gen_data.mipOffsets_hq_mq_lq.z);
    texBlk.setBool("splitAtOverride", true);
    texBlk.setInt("splitAt", splitAt);
    texBlk.setStr("mipFilter", "filterStripe");
    texBlk.setInt("mipCnt", mipCnt);

    for (auto packNamePattern : defaultPackNamePatterns)
    {
      const char *folderPath = asset->getFolderPath();
      eastl::string packName = getDDSxPackName(folderPath);
      uint32_t packSize = ddsxPackSizes[packName] * tex_types.size();
      bool needsSplit = packSize > ddsxMaxTexturesNum;
      if (needsSplit && std::regex_search(asset->getSrcFilePath().c_str(), packNamePattern.regex))
      {
        texBlk.setStr("ddsxTexPack", (packName + "_impostors_" + packNamePattern.packNameSuffix).c_str());
        break;
      }
    }
    if (texData.swizzle)
      texBlk.setStr("swizzleARGB", texData.swizzle);
    if (!texData.srgb)
      texBlk.setReal("gamma", 1);
    if (!dblk::save_to_text_file(texBlk, filename.c_str()))
    {
      conerror("Could not generate asset blk in the impostors folder: %s", filename.c_str());
      return false;
    }
  }
  return true;
}

bool ImpostorBaker::updateAssetTexBlk(DagorAsset *asset, int &out_processed, int &out_changed)
{
  const eastl::array<const char *, 3> tex_suff = {ALBEDO_ALPHA_NAME, NORMAL_TRANSLUCENCY_NAME, AO_SMOOTHNESS_NAME};
  eastl::string packName, old_packName;
  DataBlock blk;

  for (const char *tsuff : tex_suff)
  {
    String filename = String(0, "%s/%s_%s.tex.blk", getAssetFolder(asset), asset->getName(), tsuff);
    DEBUG_DUMP_VAR(filename);
    if (dd_file_exists(filename) && dblk::load(blk, filename))
    {
      out_processed++;
      old_packName = blk.getStr("ddsxTexPack", "");
      bool altered_pack = false;
      for (auto packNamePattern : defaultPackNamePatterns)
      {
        packName = getDDSxPackName(asset->getFolderPath());
        uint32_t packSize = ddsxPackSizes[packName] * tex_suff.size();
        bool needsSplit = packSize > ddsxMaxTexturesNum;
        if (needsSplit && std::regex_search(asset->getSrcFilePath().c_str(), packNamePattern.regex))
        {
          blk.setStr("ddsxTexPack", (packName + "_impostors_" + packNamePattern.packNameSuffix).c_str());
          altered_pack = true;
          break;
        }
      }
      if (!altered_pack)
        blk.removeParam("ddsxTexPack");

      if (old_packName != blk.getStr("ddsxTexPack", ""))
      {
        out_changed++;
        dblk::save_to_text_file(blk, filename);
      }
    }
  }
  return true;
}

void ImpostorBaker::setDefaultTextureQualityLevels(eastl::vector<ImpostorTextureQualityLevel> default_texture_quality_levels)
{
  defaultTextureQualityLevels = eastl::move(default_texture_quality_levels);
  eastl::sort(defaultTextureQualityLevels.begin(), defaultTextureQualityLevels.end(),
    [](const ImpostorTextureQualityLevel &lhs, const ImpostorTextureQualityLevel &rhs) { return lhs.minHeight < rhs.minHeight; });
}

void ImpostorBaker::setDefaultPackNamePatterns(eastl::vector<ImpostorPackNamePattern> default_pack_name_patterns)
{
  defaultPackNamePatterns = eastl::move(default_pack_name_patterns);
}

void ImpostorBaker::setAOBrightness(float ao_brightness) { aoBrightness = ao_brightness; }

void ImpostorBaker::setAOFalloffStart(float ao_falloff_start) { aoFalloffStart = ao_falloff_start; }

void ImpostorBaker::setAOFalloffStop(float ao_falloff_stop) { aoFalloffStop = ao_falloff_stop; }

void ImpostorBaker::setDDSXMaxTexturesNum(int ddsx_max_textures_num) { ddsxMaxTexturesNum = ddsx_max_textures_num; }
