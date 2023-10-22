#pragma once
#include <EASTL/unique_ptr.h>
#include <math/dag_frustum.h>
#include <util/dag_simpleString.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_drv3dReset.h>
#include <3d/dag_quadIndexBuffer.h>
#include <render/computeShaderFallback/voltexRenderer.h>
#include "shaders/clouds2/cloud_settings.hlsli"
#include "shaders/clouds2/clouds_density_height_lut.hlsli"
#include <render/set_reprojection.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_lockTexture.h>
#include <3d/dag_eventQueryHolder.h>
#include "cloudsShaderVars.h"
#include <render/viewVecs.h>
#include <util/dag_convar.h> //remove me
#include <workCycle/dag_workCycle.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/dag_ringCPUTextureLock.h>

#define CLOUDS_ESRAM_ONLY 0 // TEXCF_ESRAM_ONLY
#ifndef USE_CONSOLE_BOOL_VAL

#define USE_CONSOLE_BOOL_VAL(domain, name, defVal)                         extern ConVarT<bool, false> name
#define USE_CONSOLE_INT_VAL(domain, name, defVal, minVal, maxVal)          extern ConVarT<int, true> name
#define USE_CONSOLE_FLOAT_VAL_MINMAX(domain, name, defVal, minVal, maxVal) extern ConVarT<float, true> name
#define USE_CONSOLE_FLOAT_VAL(domain, name, defVal)                        extern ConVarT<float, false> name

#endif

inline float clouds_sigma_from_extinction(float ext) { return -0.09f * ext; }

USE_CONSOLE_INT_VAL("clouds", weather_resolution, 128, 64, 768); // for 65536 meters 128 res is enough - sufficient details (minimum
                                                                 // clouds size is around 512meters)
enum CloudsChangeFlags
{
  CLOUDS_NO_CHANGE,
  CLOUDS_INVALIDATED = 1,
  CLOUDS_INCREMENTAL = 2
}; // these flags!
// afr ready:
struct GenWeather
{
  UniqueTexHolder clouds_weather_texture;
  PostFxRenderer gen_weather;
  int size = 128;
  bool frameValid = true;
  void init()
  {
    clouds_weather_texture.close();
    clouds_weather_texture = dag::create_tex(NULL, size, size, TEXCF_RTARGET | CLOUDS_ESRAM_ONLY, 1, "clouds_weather_texture");
    gen_weather.init("gen_weather");
    invalidate();
  }
  void invalidate() { frameValid = false; }
  CloudsChangeFlags render()
  {
    if (size != weather_resolution.get())
    {
      size = weather_resolution.get();
      init();
    }
    if (frameValid)
      return CLOUDS_NO_CHANGE;
    TIME_D3D_PROFILE(gen_weather);
    {
      SCOPE_RENDER_TARGET;
      d3d::set_render_target(clouds_weather_texture.getTex2D(), 0);
      gen_weather.render();
      d3d::resource_barrier({clouds_weather_texture.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
    }
    frameValid = true;
    return CLOUDS_INVALIDATED;
  }
};

struct CloudsFormLUT
{
  UniqueTexHolder clouds_types_lut;
  UniqueTex clouds_erosion_lut;
  PostFxRenderer gen_clouds_types_lut, gen_clouds_erosion_lut;
  bool frameValid = true;
  void init()
  {
    clouds_types_lut.close();
    clouds_erosion_lut.close();

    clouds_types_lut =
      dag::create_tex(NULL, CLOUDS_TYPES_HEIGHT_LUT, CLOUDS_TYPES_LUT, TEXCF_RTARGET | CLOUDS_ESRAM_ONLY, 1, "clouds_types_lut");
    clouds_types_lut->texaddru(TEXADDR_BORDER);
    clouds_types_lut->texaddrv(TEXADDR_CLAMP);
    gen_clouds_types_lut.init("gen_clouds_types_lut");

    clouds_erosion_lut = dag::create_tex(NULL, 32, 1, TEXCF_RTARGET | CLOUDS_ESRAM_ONLY | TEXFMT_R8G8, 1, "clouds_erosion_lut");
    clouds_erosion_lut->texaddr(TEXADDR_CLAMP);
    gen_clouds_erosion_lut.init("gen_clouds_erosion_lut");
    invalidate();
  }
  void invalidate() { frameValid = false; }
  CloudsChangeFlags render()
  {
    if (frameValid)
      return CLOUDS_NO_CHANGE;
    TIME_D3D_PROFILE(clouds_lut_gen);
    // todo: implement Compute version
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(clouds_types_lut.getTex2D(), 0);
    gen_clouds_types_lut.render();
    d3d::resource_barrier({clouds_types_lut.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    d3d::set_render_target(clouds_erosion_lut.getTex2D(), 0);
    gen_clouds_erosion_lut.render();
    frameValid = true;
    return CLOUDS_INVALIDATED;
  }
};


USE_CONSOLE_INT_VAL("clouds", shape_res_compat_shift, 3, 0, 3);
USE_CONSOLE_INT_VAL("clouds", detail_res_compat_shift, 2, 0, 2);
USE_CONSOLE_INT_VAL("clouds", shape_res, 1, -2, 2); // 1<<(6+x), 1<<(4+x)
USE_CONSOLE_INT_VAL("clouds", detail_res, 2, 0, 3); // 1<<(4+x)
USE_CONSOLE_BOOL_VAL("clouds", compressedTextures, true);
static bool regen_noises = false;
static bool reload_noises = false;


struct GenNoise
{
  int shapeRes = 128, detailRes = 64;
  int shapeResShift = 1, detailResShift = 2;
  static constexpr int MAX_CLOUD_DETAIL_MIPS = 4;
  enum GenNoiseState
  {
    EMPTY,
    LOADING,
    LOADED,
    RENDERED
  };
  GenNoiseState state = EMPTY;
  bool curlRendered = false;
  bool compressSupported = false;
  eastl::unique_ptr<ComputeShaderElement> genCurl2d, genCurl3d;
  PostFxRenderer genCurl2dPs;
  VoltexRenderer genCloudShape, genCloudDetail, genMips3d;
  UniqueTexHolder cloud1, cloud2;
  SharedTexHolder resCloud1, resCloud2;
  UniqueTexHolder cloudsCurl2d;
  VoltexRenderer compress3D;
  // TextureIDHolder cloudsCurl3d;
  UniqueTex cloud1Compressed, cloud2Compressed;
  UniqueTex buffer;

  void close()
  {
    state = EMPTY;
    curlRendered = false;
    cloud1.close();
    cloud2.close();
    resCloud1.close();
    resCloud2.close();
    // cloudsCurl3d.close();
    cloudsCurl2d.close();
    closeCompressor();
  }
  bool loadVolmap(const char *resname, int resolution, SharedTexHolder &tex)
  {
    String filename(128, "%s_%d%s", resname, resolution, compressSupported ? "" : "_raw");

    tex.close();
    tex = SharedTexHolder(dag::get_tex_gameres(filename), resname);
    if (!tex)
      return false;

    return true;
  }
  void init()
  {
    compressSupported =
      d3d::check_voltexformat(TEXFMT_ATI1N | CLOUDS_ESRAM_ONLY) && d3d::get_driver_desc().caps.hasResourceCopyConversion;

    if (!regen_noises && loadVolmap("gen_cloud_shape", shapeRes, resCloud1) && loadVolmap("gen_cloud_detail", detailRes, resCloud2))
    {
      cloud1.close();
      cloud2.close();
      state = LOADING;
      reload_noises = false;
      debug("daskies noise volmaps found, using from resources");
      TEXTUREID waitTextures[] = {resCloud1.getTexId(), resCloud2.getTexId()};
      prefetch_managed_textures(dag::ConstSpan<TEXTUREID>(waitTextures, 2));
    }
    else
    {
      resCloud1.close();
      resCloud2.close();
      state = EMPTY;
      bool useSimplifiedNoise = d3d::get_driver_desc().issues.hasComputeTimeLimited;
      if (useSimplifiedNoise)
      {
        // prefer to use pregenerated noise for low power GPUs, to avoid extra load time & crashes
        // but do not crash and use simplified generation as a safe fallback
        logerr("daskies noise volmaps not found, generating simplified noise");
        shapeRes = shapeRes >> shape_res_compat_shift.get();
        detailRes = detailRes >> detail_res_compat_shift.get();
      }
      else
        debug("daskies noise volmaps not found, generating");

      genCloudShape.init("gen_cloud_shape_cs", "gen_cloud_shape_ps");
      genCloudDetail.init("gen_cloud_detail_cs", "gen_cloud_detail_ps");
      genMips3d.init("clouds_gen_mips_3d_cs", "clouds_gen_mips_3d_ps");
      uint32_t fmt = TEXFMT_L8;
      const int shapeMips = min<int>(MAX_CLOUD_SHAPE_MIPS, get_log2w(shapeRes / 4));
      genCloudShape.initVoltex(cloud1, shapeRes, shapeRes, shapeRes / 4, fmt, shapeMips, "gen_cloud_shape");
      const int detailMips = min<int>(MAX_CLOUD_DETAIL_MIPS, get_log2w(detailRes / 2));
      genCloudDetail.initVoltex(cloud2, detailRes, detailRes, detailRes, fmt, detailMips, "gen_cloud_detail");
      cloud1.setVar();
      cloud2.setVar();
      initCompressor();
    }

    // genCurl3d.reset(new_compute_shader("gen_curl_clouds_3d", true));
    // cloudsCurl3d.set(d3d::create_voltex(32, 32, 32,//todo: ATI2N compression
    //                               TEXFMT_R8G8|TEXCF_UNORDERED, shapeMips, "clouds_curl_3d"), "clouds_curl_3d");
    // cloudsCurl3d.setVar();

    if (!curlRendered)
    {
      if (VoltexRenderer::is_compute_supported())
        genCurl2d.reset(new_compute_shader("gen_curl_clouds_2d", true));
      else
        genCurl2dPs.init("gen_curl_clouds_2d_ps");
      cloudsCurl2d.close();
      cloudsCurl2d = dag::create_tex(0, CLOUD_CURL_RES, CLOUD_CURL_RES, TEXFMT_R8G8S | (genCurl2d ? TEXCF_UNORDERED : TEXCF_RTARGET),
        1, "clouds_curl_2d");
    }
    d3d::resource_barrier({cloudsCurl2d.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  void reset()
  {
    if ((state == LOADED) || (state == LOADING))
      reload_noises = true;
    else
      regen_noises = true;

    curlRendered = false;
  }

  static int get_blocked_mips(const ManagedTex &tex)
  {
    int mip_count = tex->level_count();
    TextureInfo ti;
    tex->getinfo(ti);
    for (int minD = min(min(ti.w, ti.h), ti.d); mip_count > 1 && (minD >> mip_count) < 4;)
      --mip_count;
    return mip_count;
  }

  void closeCompressor()
  {
    cloud1Compressed.close();
    cloud2Compressed.close();
    buffer.close();
  }
  void initCompressor()
  {
    if (!compressSupported)
      return;

    cloud1Compressed.close();
    cloud2Compressed.close();
    auto cflg = TEXCF_UPDATE_DESTINATION | TEXFMT_ATI1N | CLOUDS_ESRAM_ONLY;
    cloud1Compressed =
      dag::create_voltex(shapeRes, shapeRes, shapeRes / 4, cflg, get_blocked_mips(cloud1), "gen_cloud_shape_compressed");
    cloud2Compressed =
      dag::create_voltex(detailRes, detailRes, detailRes, cflg, get_blocked_mips(cloud2), "gen_cloud_detail_compressed");

    compress3D.init("compress_voltex_bc4_cs", "compress_voltex_bc4_ps");
  }
  void initCompressorBuffer()
  {
    if (!compressSupported)
      return;

    compress3D.initVoltex(buffer, shapeRes / 4, shapeRes / 4, max(shapeRes / 4, detailRes), TEXFMT_R32G32UI, 1,
      "gen_cloud_shape_buffer");
  }

  void compressBC4(const ManagedTex &tex, const ManagedTex &dest)
  {
    if (!compressSupported)
      return;

    G_ASSERT(dest->level_count() <= tex->level_count());
    TextureInfo ti;
    tex->getinfo(ti);
    ShaderGlobal::set_texture(compress_voltex_bc4_sourceVarId, tex);
    for (int mips_count = dest->level_count(), i = 0; i < mips_count; ++i)
    {
      tex->texmiplevel(i, i);

      const int w = ti.w >> i, h = ti.h >> i, d = ti.d >> i;
      compress3D.render(buffer, 0, IPoint3(w, h, d));
      d3d::resource_barrier({buffer.getVolTex(), RB_RO_COPY_SOURCE, 0, 0});
      dest->updateSubRegion(buffer.getVolTex(), 0, 0, 0, 0, // source mip, x,y,z
        max(1, w / 4), max(1, h / 4), d,                    // width x height x depth
        i, 0, 0, 0);                                        // dest mip, x,y,z
    }
    tex->texmiplevel(-1, -1);
    ShaderElement::invalidate_cached_state_block();
  }
  /* Return true if curl was rendered */
  bool renderCurl()
  {
    if (curlRendered)
      return false;

    {
      TIME_D3D_PROFILE(cloud_curl2d);
      if (genCurl2d)
      {
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), cloudsCurl2d.getTex2D());
        genCurl2d->dispatchThreads(CLOUD_CURL_RES, CLOUD_CURL_RES, 1);
      }
      else
      {
        SCOPE_RENDER_TARGET;
        d3d::set_render_target(cloudsCurl2d.getTex2D(), 0);
        genCurl2dPs.getElem()->setStates();
        d3d::draw(PRIM_TRILIST, 0, 1);
      }
      d3d::resource_barrier({cloudsCurl2d.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    }
    curlRendered = true;
    return true;
  }
  void generateMips3d(const ManagedTex &tex)
  {
    TIME_D3D_PROFILE(genMips);
    TextureInfo ti;
    tex->getinfo(ti);
    ShaderGlobal::set_int(clouds_gen_mips_3d_one_layerVarId, 0);
    ShaderGlobal::set_texture(clouds_gen_mips_3d_sourceVarId, tex.getTexId());
    for (int mips_count = tex->level_count(), i = 1; i < mips_count; ++i)
    {
      d3d::resource_barrier({tex.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, (unsigned)(i - 1), 1});
      int d = ti.d >> i;
      if (d == 1)
        ShaderGlobal::set_int(clouds_gen_mips_3d_one_layerVarId, 1);
      tex->texmiplevel(i - 1, i - 1);
      genMips3d.render(tex, i);
    }
    d3d::resource_barrier({tex.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, (unsigned)(tex->level_count() - 1), 1});
    tex->texmiplevel(-1, -1);
    ShaderElement::invalidate_cached_state_block();
  }

  // Returns true, if noise textures have just been loaded
  bool isPrepareRequired() const
  {
    if (state != LOADING)
      return false;

    TEXTUREID waitTextures[] = {resCloud1.getTexId(), resCloud2.getTexId()};
    return prefetch_and_check_managed_textures_loaded(dag::ConstSpan<TEXTUREID>(waitTextures, 2), true);
  }

  // Returns true where noise loaded
  bool isReady() const { return (state != EMPTY && state != LOADING); }

  /* Returns true if at least one of noise textures was updated */
  bool render()
  {
    const bool resChanged = (shapeResShift != shape_res.get() || detailResShift != detail_res.get() || reload_noises);
    if (resChanged || regen_noises)
    {
      shapeRes = 1 << (6 + shape_res.get());
      detailRes = 1 << (4 + detail_res.get());
      shapeResShift = shape_res.get();
      detailResShift = detail_res.get();
      init();
    }

    // we must renderCurl it always, because curl never loads from resources
    // it uses `curlRendered` variable instead of `state`
    bool renderCurlNow = renderCurl();

    if (state == LOADING)
    {
      TEXTUREID waitTextures[] = {resCloud1.getTexId(), resCloud2.getTexId()};
      if (prefetch_and_check_managed_textures_loaded(dag::ConstSpan<TEXTUREID>(waitTextures, 2), true))
      {
        state = LOADED;
        return true;
      }
    }

    if (state != EMPTY)
      return renderCurlNow || resChanged;

    regen_noises = false;
    state = RENDERED;
    initCompressorBuffer(); // init buffer

    // we should split workload to avoid TDR on weak/inherently CS limited GPUs
    bool splitGenWorkload = d3d::get_driver_desc().issues.hasComputeTimeLimited;

    {
      d3d::GPUWorkloadSplit gpuWorkSplit(splitGenWorkload, false, "cloud_shape_gen");

      ShaderGlobal::set_int(clouds_shape_tex_size_xzVarId, shapeRes);
      ShaderGlobal::set_int(clouds_shape_tex_size_yVarId, shapeRes / 4);
      TIME_D3D_PROFILE(cloud1);
      genCloudShape.render(cloud1);
      generateMips3d(cloud1);
      compressBC4(cloud1, cloud1Compressed);
    }
    {
      d3d::GPUWorkloadSplit gpuWorkSplit(splitGenWorkload, true, "cloud_detail_gen");

      ShaderGlobal::set_int(clouds_detail_tex_sizeVarId, detailRes);
      TIME_D3D_PROFILE(cloud2);
      genCloudDetail.render(cloud2);
      generateMips3d(cloud2);
      compressBC4(cloud2, cloud2Compressed);
    }

    // After compression buffer isn't required
    buffer.close();

    return true;
  }
  void setVars()
  {
    G_ASSERT(state != EMPTY);
    if (state == RENDERED && compressSupported && compressedTextures.get())
    {
      ShaderGlobal::set_texture(cloud1.getVarId(), cloud1Compressed);
      ShaderGlobal::set_texture(cloud2.getVarId(), cloud2Compressed);
    }
    else // loaded or not compressed
    {
      switch (state)
      {
        case RENDERED:
          cloud1.setVar();
          cloud2.setVar();
          break;
        case LOADING:
        case LOADED:
          resCloud1.setVar();
          resCloud2.setVar();
          break;
        default: fatal("unexpected state=%d", int(state));
      }
    }

    cloudsCurl2d.setVar();
  }
};

USE_CONSOLE_BOOL_VAL("clouds", renderFullClouds, false);
USE_CONSOLE_BOOL_VAL("clouds", renderTemporalClouds, true);
USE_CONSOLE_BOOL_VAL("clouds", regenField, false);
#if DAGOR_DBGLEVEL > 0
USE_CONSOLE_BOOL_VAL("clouds", use_compute, true);
#endif
#include <render/noiseTex.h>

struct CloudsRendererData
{
  int w = 0, h = 0;
  carray<UniqueTex, 3> cloudsTextureColor = {};
  UniqueTex cloudsTextureDepth;
  carray<UniqueTex, 2> cloudsTextureWeight;
  // todo: reflection never needs clouds_color_close
  UniqueTex clouds_color_close;
  UniqueTex clouds_tile_distance, clouds_tile_distance_tmp;
  UniqueBuf clouds_close_layer_is_outside;
  UniqueBuf cloudsIndirectBuffer;
  uint32_t resetGen = 0;

  bool useCompute = true, taaUseCompute = false;
  bool lowresCloseClouds = true;
  static const uint32_t tileX = CLOUDS_TILE_W, tileY = CLOUDS_TILE_H;
  static void clear_black(ManagedTex &t)
  {
    if (!t)
      return;
    TextureInfo tinfo;
    t->getinfo(tinfo, 0);
    if (tinfo.cflg & TEXCF_RTARGET)
    {
      SCOPE_RENDER_TARGET;
      d3d::set_render_target(t.getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    }
    else if (tinfo.cflg & TEXCF_UNORDERED)
      d3d::clear_rwtexf(t.getTex2D(), ZERO_PTR<float>(), 0, 0);
  }
  void clearTemporalData(uint32_t gen)
  {
    if (resetGen == gen)
      return;
    resetGen = gen;
    frameValid = false;
    for (UniqueTex &tex : cloudsTextureColor)
      clear_black(tex);
    for (UniqueTex &tex : cloudsTextureWeight)
      clear_black(tex);
  }
  void close()
  {
    clouds_close_layer_is_outside.close();
    clouds_color_close.close();
    clouds_tile_distance.close();
    clouds_tile_distance_tmp.close();
    cloudsTextureDepth.close();
    for (auto &i : cloudsTextureWeight)
      i.close();
    for (auto &i : cloudsTextureColor)
      i.close();
    cloudsIndirectBuffer.close();
    w = h = 0;
  }
  void initTiledDist(const char *pr) // only needed when it is not 100% cloudy. todo: calc pixels count allocation
  {
    clouds_tile_distance.close();
    clouds_tile_distance_tmp.close();
    int dw = (w + tileX - 1) / tileX, dh = (h + tileY - 1) / tileY;
    if (dw * dh < 1920 * 720 / 4 / tileX / tileY)
      return;
    uint32_t flg = useCompute ? TEXCF_UNORDERED : TEXCF_RTARGET;
    String tn;
    tn.printf(64, "%s_tile_dist", pr);
    clouds_tile_distance = dag::create_tex(NULL, dw, dh, flg | TEXFMT_L16, 1, tn.c_str());
    clouds_tile_distance->texaddr(TEXADDR_CLAMP);
    clouds_tile_distance->texfilter(TEXFILTER_POINT);
    tn.printf(64, "%s_tile_dist_tmp", pr);
    clouds_tile_distance_tmp = dag::create_tex(NULL, dw, dh, flg | TEXFMT_L16, 1, tn.c_str());
    clouds_tile_distance_tmp->texaddr(TEXADDR_CLAMP);
    clouds_tile_distance_tmp->texfilter(TEXFILTER_POINT);
  }
  void setVars()
  {
    const int dw = bool(clouds_tile_distance) ? (w + tileX - 1) / tileX : 0, dh = dw != 0 ? (h + tileY - 1) / tileY : 0;
    G_ASSERT(w > 0 && h > 0);
    ShaderGlobal::set_color4(clouds_tiled_resVarId, dw, dh, 0, 0);
    ShaderGlobal::set_color4(clouds2_resolutionVarId, w, h, lowresCloseClouds ? w / 2 : w, lowresCloseClouds ? h / 2 : h);
    ShaderGlobal::set_texture(clouds_colorVarId, cloudsTextureColor[2]);
    ShaderGlobal::set_texture(clouds_color_closeVarId, clouds_color_close);
    ShaderGlobal::set_texture(clouds_tile_distanceVarId, clouds_tile_distance);
    ShaderGlobal::set_texture(clouds_tile_distance_tmpVarId, clouds_tile_distance_tmp);
    ShaderGlobal::set_texture(clouds_depthVarId, cloudsTextureDepth);
    ShaderGlobal::set_buffer(clouds_close_layer_is_outsideVarId, clouds_close_layer_is_outside);
  }
  void init(int w_, int h_, const char *pr, bool can_be_in_clouds, CloudsResolution clouds_resolution)
  {
    ShaderGlobal::set_int(clouds_use_fullresVarId, (clouds_resolution == CloudsResolution::ForceFullresClouds));
    const bool changedSize = (w != w_ || h != h_);
    const bool changedCanBeInClouds = (can_be_in_clouds != bool(clouds_color_close)) || changedSize;
    if (!changedSize && !changedCanBeInClouds)
      return;
    if (d3d::get_driver_desc().shaderModel < 5.0_sm)
      useCompute = taaUseCompute = false;
    if (!w_)
    {
      close();
      return;
    }

    w = w_;
    h = h_;
    frameValid = false;
    resetGen = 0;
    lowresCloseClouds =
      clouds_resolution != CloudsResolution::FullresCloseClouds && clouds_resolution != CloudsResolution::ForceFullresClouds;
    uint32_t fmt = TEXFMT_A16B16G16R16F;
    uint32_t rtflg = useCompute ? TEXCF_UNORDERED : TEXCF_RTARGET; //
    String tn;
    if (d3d::get_driver_desc().shaderModel >= 5.0_sm && d3d::get_driver_desc().caps.hasIndirectSupport && !cloudsIndirectBuffer)
    {
      tn.printf(64, "%s_clouds_indirect", pr);
      cloudsIndirectBuffer = dag::create_sbuffer(4, CLOUDS_APPLY_COUNT * 4, SBCF_UA_INDIRECT, 0, tn.c_str());
    }
    if (changedSize)
    {
      uint32_t taaRtflg = taaUseCompute ? TEXCF_UNORDERED : TEXCF_RTARGET; //
      for (auto &i : cloudsTextureColor)
        i.close();
      for (auto &i : cloudsTextureWeight)
        i.close();
      cloudsTextureDepth.close();
      for (int i = 0; i < cloudsTextureWeight.size(); ++i)
      {
        // fmt = i==2 ? TEXCF_SRGBREAD|TEXCF_SRGBWRITE|TEXCF_ESRAM_ONLY : TEXFMT_A16B16G16R16F;//unless we have exposure
        tn.printf(64, "%s_clouds_weight_%d", pr, i);
        cloudsTextureWeight[i] = dag::create_tex(NULL, w, h, taaRtflg | TEXFMT_L8, 1, tn.c_str());
        cloudsTextureWeight[i]->texaddr(TEXADDR_CLAMP);
      }

      for (int i = 0; i < cloudsTextureColor.size(); ++i)
      {
        // fmt = i==2 ? TEXCF_SRGBREAD|TEXCF_SRGBWRITE|TEXCF_ESRAM_ONLY : TEXFMT_A16B16G16R16F;//unless we have exposure
        fmt = TEXFMT_A16B16G16R16F;
        tn.printf(64, "%s_clouds2_%d", pr, i);
        cloudsTextureColor[i] = dag::create_tex(NULL, w, h, (i == 2 ? rtflg : taaRtflg) | fmt, 1, tn.c_str());
        cloudsTextureColor[i]->texaddr(TEXADDR_CLAMP);
      }
      tn.printf(64, "%s_clouds_depth", pr);
      cloudsTextureDepth.close();
      cloudsTextureDepth = dag::create_tex(NULL, w, h, rtflg | TEXFMT_L16, 1, tn.c_str());
      cloudsTextureDepth->texaddr(TEXADDR_CLAMP);
      cloudsTextureDepth->texfilter(TEXFILTER_POINT);
      initTiledDist(pr);
    }
    if (changedCanBeInClouds)
    {
      clouds_color_close.close();
      clouds_close_layer_is_outside.close();
      if (can_be_in_clouds)
      {
        tn.printf(64, "%s_clouds_close", pr);
        clouds_color_close = dag::create_tex(NULL, lowresCloseClouds ? w / 2 : w, lowresCloseClouds ? h / 2 : h,
          rtflg | fmt | TEXCF_CLEAR_ON_CREATE, 1, tn.c_str());
        clouds_color_close->texaddr(TEXADDR_CLAMP);
      }
    }

    tn.printf(64, "%s_close_layer_is_outside", pr);

    if (d3d::get_driver_desc().shaderModel >= 5.0_sm)
    {
      if (!clouds_close_layer_is_outside.getBuf())
        clouds_close_layer_is_outside =
          dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 2, tn.c_str(), dag::buffers::Init::Zero);
    } // NOTE: we don't use clouds_close_layer_is_outside optimization without compute shaders
  }
  bool frameValid = false;

  TMatrix4 prevGlobTm = TMatrix4::IDENT;
  Point4 prevViewVecLT = {0, 0, 0, 0};
  Point4 prevViewVecRT = {0, 0, 0, 0};
  Point4 prevViewVecLB = {0, 0, 0, 0};
  Point4 prevViewVecRB = {0, 0, 0, 0};
  DPoint3 prevWorldPos{0., 0., 0.};
  unsigned int frameNo = 0;

  DPoint3 cloudsCameraOrigin{0., 0., 0.};
  double currentCloudsOffset = 0.;
  void update(const DPoint3 &dir, const DPoint3 &origin)
  {
    double offset = dir * (origin - cloudsCameraOrigin);
    currentCloudsOffset += offset;
    cloudsCameraOrigin = origin;
  }
};
/*USE_CONSOLE_BOOL_VAL("clouds", clouds_cs, true);
USE_CONSOLE_BOOL_VAL("clouds", taaCompute, false);
    if (clouds_cs.get() != data.useCompute || taaCompute.get() != data.taaUseCompute)
    {
      data.useCompute = clouds_cs.get();
      data.taaUseCompute = taaCompute.get();
      data.init(data.w, data.h);
    }
*/

struct CloudsRenderer
{
  PostFxRenderer clouds2_direct;
  PostFxRenderer clouds2_temporal_ps, clouds2_close_temporal_ps;
  PostFxRenderer clouds2_taa_ps, clouds2_taa_ps_has_empty, clouds2_taa_ps_no_empty;
  PostFxRenderer clouds2_apply, clouds2_apply_has_empty, clouds2_apply_no_empty;

  eastl::unique_ptr<ComputeShaderElement> clouds_create_indirect;

  eastl::unique_ptr<ComputeShaderElement> clouds2_temporal_cs, clouds2_close_temporal_cs;
  eastl::unique_ptr<ComputeShaderElement> clouds2_taa_cs, clouds2_taa_cs_has_empty, clouds2_taa_cs_no_empty;
  eastl::unique_ptr<ComputeShaderElement> clouds_tile_dist_prepare_cs, clouds_tile_dist_min_cs;
  PostFxRenderer clouds_tile_dist_prepare_ps, clouds_tile_dist_min_ps;

  eastl::unique_ptr<ComputeShaderElement> clouds_close_layer_outside, clouds_close_layer_clear;
  eastl::unique_ptr<ComputeShaderElement> clouds_tile_dist_count_cs;
  void renderTiledDist(CloudsRendererData &data)
  {
    if (!data.clouds_tile_distance)
      return;
    TIME_D3D_PROFILE(tiledDist);
    if (data.useCompute)
    {
      TextureInfo ti;
      data.clouds_tile_distance_tmp->getinfo(ti, 0);
      d3d::set_rwtex(STAGE_CS, 0, data.clouds_tile_distance_tmp.getTex2D(), 0, 0);
      clouds_tile_dist_prepare_cs->dispatchThreads(ti.w, ti.h, 1);
      d3d::set_rwtex(STAGE_CS, 0, data.clouds_tile_distance.getTex2D(), 0, 0);
      clouds_tile_dist_min_cs->dispatchThreads(ti.w, ti.h, 1);
      d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    }
    else
    {
      d3d::set_render_target(data.clouds_tile_distance_tmp.getTex2D(), 0, 0);
      clouds_tile_dist_prepare_ps.render();
      d3d::set_render_target(data.clouds_tile_distance.getTex2D(), 0, 0);
      clouds_tile_dist_min_ps.render();
    }
    if (clouds_tile_dist_count_cs && data.clouds_close_layer_is_outside)
    {
      TIME_D3D_PROFILE(non_empty_tiles);
      TextureInfo ti;
      data.clouds_tile_distance_tmp->getinfo(ti, 0);
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), data.clouds_close_layer_is_outside.getBuf());
      clouds_tile_dist_count_cs->dispatchThreads(ti.w, ti.h, 1);
    }
  }
  void init()
  {
    clouds2_apply.init("clouds2_apply");
    if (d3d::get_driver_desc().shaderModel >= 5.0_sm)
    {
#define INIT(a) a.reset(new_compute_shader(#a, false));
      INIT(clouds2_temporal_cs);
      INIT(clouds2_close_temporal_cs);
      INIT(clouds2_taa_cs);
      INIT(clouds2_taa_cs_has_empty);
      INIT(clouds2_taa_cs_no_empty);
      INIT(clouds_close_layer_outside);
      INIT(clouds_close_layer_clear);
      INIT(clouds_tile_dist_count_cs);
      INIT(clouds_tile_dist_prepare_cs);
      INIT(clouds_tile_dist_min_cs);
      INIT(clouds_create_indirect);
#undef INIT
    }
#define INIT(a) a.init(#a);
    INIT(clouds2_temporal_ps)
    INIT(clouds2_close_temporal_ps)
    INIT(clouds2_taa_ps)
    INIT(clouds2_taa_ps_has_empty)
    INIT(clouds2_taa_ps_no_empty)
    INIT(clouds2_direct)
    INIT(clouds_tile_dist_prepare_ps)
    INIT(clouds_tile_dist_min_ps)
    INIT(clouds2_apply_has_empty)
    INIT(clouds2_apply_no_empty)
#undef INIT
  }

  DPoint2 cloudsOfs = {0, 0}, currentCloudsOfs = {0, 0};

  void setCloudsOfs(const DPoint2 &xz) { cloudsOfs += xz; }
  DPoint2 getCloudsOfs() const { return cloudsOfs; }
  Point2 currentCloudsOffsetCalc(float worldSize) const
  {
    G_ASSERT_RETURN(worldSize != 0.f, point2(cloudsOfs));
    return point2(cloudsOfs - floor(cloudsOfs / worldSize) * worldSize);
  }

  struct DispatchGroups2D
  {
    int x, y, z, w;
  };
  DispatchGroups2D set_dispatch_groups(int w, int h, int w_sz, int h_sz, bool lowres_close_clouds)
  {
    union
    {
      Color4 dgf;
      struct
      {
        int x, y, z, w;
      } dg;
    } castDG;
    castDG.dg.x = (w + w_sz - 1) / w_sz;
    castDG.dg.y = (h + h_sz - 1) / h_sz;
    castDG.dg.z = ((lowres_close_clouds ? w / 2 : w) + w_sz - 1) / w_sz;
    castDG.dg.w = ((lowres_close_clouds ? h / 2 : h) + h_sz - 1) / h_sz;
    ShaderGlobal::set_color4(clouds2_dispatch_groupsVarId, castDG.dgf);
    return DispatchGroups2D{castDG.dg.x, castDG.dg.y, castDG.dg.z, castDG.dg.w};
  }
  int getNotLesserDepthLevel(CloudsRendererData &data, int &depth_levels, Texture *depth)
  {
    depth_levels = 1;
    if (!depth)
      return 0;
    depth_levels = depth->level_count();
    if (!data.w)
      return 0;
    TextureInfo depthInfo;
    depth->getinfo(depthInfo, 0);
    int level = 0;
    for (int mw = depthInfo.w, mh = depthInfo.h; mw >= data.w && mh >= data.h && level < depth_levels; mw >>= 1, mh >>= 1, level++)
      ;
    level = max(0, level - 1);
    return level;
  }
  void setCloudsOriginOffset(float worldSize)
  {
    ShaderGlobal::set_color4(clouds_origin_offsetVarId, P2D(currentCloudsOffsetCalc(worldSize)), 0, 0);
  }
  void setCloudsOffsetVars(float currentCloudsOffset, float worldSize)
  {
    ShaderGlobal::set_real(clouds_offsetVarId, currentCloudsOffset);
    setCloudsOriginOffset(worldSize);
  }

  void setCloudsOffsetVars(CloudsRendererData &data, float worldSize) { setCloudsOffsetVars(data.currentCloudsOffset, worldSize); }
  void render(CloudsRendererData &data, const TextureIDPair &depth, const TextureIDPair &prevDepth, const Point2 &windChangeOfs,
    float worldSize, const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 *world_pos = nullptr)
  {
    if (!data.cloudsTextureColor[2])
      return;
    TIME_D3D_PROFILE(render_clouds);
    ScopeReprojection reprojectionScope;
    data.setVars();
    set_viewvecs_to_shader(view_tm, proj_tm);
    data.clearTemporalData(get_d3d_full_reset_counter());
    ShaderGlobal::set_real(clouds_restart_taaVarId, data.frameValid ? 0 : 2);
    data.frameValid = true;
    data.frameNo++;

    ShaderGlobal::set_int(clouds_infinite_skiesVarId, depth.getTex2D() ? 0 : 1);
    ShaderGlobal::set_int(clouds2_current_frameVarId, data.frameNo & 0xFFFF);
    setCloudsOffsetVars(data, worldSize);
    const bool canBeInsideClouds = bool(data.clouds_color_close) && depth.getTex();

    if (data.clouds_close_layer_is_outside)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), data.clouds_close_layer_is_outside.getBuf());
      if (canBeInsideClouds)
      {
        TIME_D3D_PROFILE(clouds_close_layer_is_outside_calc);
        TMatrix4 globtm;
        d3d::calcglobtm(view_tm, proj_tm, globtm);
        extern void set_frustum_planes(const Frustum &frustum);
        set_frustum_planes(Frustum(globtm));
        clouds_close_layer_outside->dispatch(1, 1, 1);
      }
      else
      {
        TIME_D3D_PROFILE(clouds_close_layer_clear);
        clouds_close_layer_clear->dispatch(1, 1, 1);
      }
    }
    bool useCompute = data.useCompute, taaUseCompute = data.taaUseCompute;
#if DAGOR_DBGLEVEL > 0
    useCompute &= use_compute.get();
    taaUseCompute &= use_compute.get();
#endif

    struct MyScopeRenderTarget
    {
      Driver3dRenderTarget prevRT;
      bool should;
      MyScopeRenderTarget(bool should_) : should(should_)
      {
        if (should)
          d3d_get_render_target(prevRT);
      }
      ~MyScopeRenderTarget()
      {
        if (should)
          d3d_set_render_target(prevRT);
      }
    } scoped(!useCompute || !taaUseCompute);
    renderTiledDist(data);
    int depthLevels = 1;
    int level = getNotLesserDepthLevel(data, depthLevels, depth.getTex2D());
    ShaderGlobal::set_texture(clouds_depth_gbufVarId, depth.getId());
    ShaderGlobal::set_int(clouds_has_close_sequenceVarId, bool(data.clouds_color_close) ? 1 : 0);
    DispatchGroups2D dg = set_dispatch_groups(data.w, data.h, CLOUD_TRACE_WARP_X, CLOUD_TRACE_WARP_Y, data.lowresCloseClouds);
    {
      if (level != 0 && depthLevels != 1 && depth.getTex2D())
        depth.getTex2D()->texmiplevel(level, level);
      TIME_D3D_PROFILE(current_clouds);
      if (useCompute)
      {
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), data.cloudsTextureColor[2].getTex2D());
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), data.cloudsTextureDepth.getTex2D());
        clouds2_temporal_cs->dispatch(dg.x, dg.y, 1); // todo: measure optimal warp size
      }
      else
      {
        d3d::set_render_target(data.cloudsTextureColor[2].getTex2D(), 0);
        d3d::set_render_target(1, data.cloudsTextureDepth.getTex2D(), 0);
        clouds2_temporal_ps.render();
      }
    }

    if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
    {
      TIME_D3D_PROFILE(create_indirect);
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), data.cloudsIndirectBuffer.getBuf());
      clouds_create_indirect->dispatch(1, 1, 1);
    }
    if (canBeInsideClouds)
    {
      // todo: use indirect
      if (depthLevels != 1 && level + 1 < depthLevels && depth.getTex2D() && data.lowresCloseClouds)
        depth.getTex2D()->texmiplevel(level + 1, level + 1);
      TIME_D3D_PROFILE(current_clouds_near);
      if (useCompute)
      {
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), data.clouds_color_close.getTex2D());
        if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
        {
          clouds2_close_temporal_cs->dispatch_indirect(data.cloudsIndirectBuffer.getBuf(),
            4 * 4 * (CLOUDS_HAS_CLOSE_LAYER + CLOUDS_APPLY_COUNT_PS));
        }
        else
        {
          clouds2_close_temporal_cs->dispatch(dg.z, dg.w, 1);
        }
      }
      else
      {
        d3d::set_render_target(data.clouds_color_close.getTex2D(), 0);
        if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
        {
          clouds2_close_temporal_ps.getElem()->setStates();
          d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_HAS_CLOSE_LAYER);
        }
        else
          clouds2_close_temporal_ps.render();
      }
      d3d::resource_barrier({data.clouds_color_close.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }

    {
      if (depthLevels != 1)
      {
        if (depth.getTex2D())
          depth.getTex2D()->texmiplevel(level, level);
        if (prevDepth.getTex2D())
        {
          G_ASSERT(prevDepth.getTex2D()->level_count() == depth.getTex2D()->level_count());
          prevDepth.getTex2D()->texmiplevel(level, level);
        }
      }
      if (prevDepth.getTex2D())
        prevDepth.getTex2D()->texfilter(TEXFILTER_POINT);
      if (depth.getTex2D())
        depth.getTex2D()->texfilter(TEXFILTER_POINT);
      TIME_D3D_PROFILE(taa_clouds);
      ShaderGlobal::set_texture(clouds_prev_depth_gbufVarId, prevDepth.getId());
      data.prevWorldPos.x -= cloudsOfs.x + windChangeOfs.x - currentCloudsOfs.x;
      data.prevWorldPos.z -= cloudsOfs.y + windChangeOfs.y - currentCloudsOfs.y;
      set_reprojection(view_tm, proj_tm, data.prevWorldPos, data.prevGlobTm, data.prevViewVecLT, data.prevViewVecRT,
        data.prevViewVecLB, data.prevViewVecRB, world_pos);
      ShaderGlobal::set_texture(clouds_color_prevVarId, data.cloudsTextureColor[1 - (data.frameNo & 1)].getTexId());
      ShaderGlobal::set_texture(clouds_colorVarId, data.cloudsTextureColor[2]);
      ShaderGlobal::set_texture(clouds_prev_taa_weightVarId, data.cloudsTextureWeight[1 - (data.frameNo & 1)]);
      if (taaUseCompute)
      {
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), data.cloudsTextureColor[(data.frameNo & 1)].getTex2D());
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), data.cloudsTextureWeight[(data.frameNo & 1)].getTex2D());
        if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
        {
          clouds2_taa_cs_has_empty->dispatch_indirect(data.cloudsIndirectBuffer.getBuf(),
            4 * 4 * (CLOUDS_HAS_EMPTY + CLOUDS_APPLY_COUNT_PS));
          clouds2_taa_cs_no_empty->dispatch_indirect(data.cloudsIndirectBuffer.getBuf(),
            4 * 4 * (CLOUDS_NO_EMPTY + CLOUDS_APPLY_COUNT_PS));
        }
        else
          clouds2_taa_cs->dispatch(dg.x, dg.y, 1); // todo: measure optimal warp size
      }
      else
      {
        d3d::set_render_target(data.cloudsTextureColor[(data.frameNo & 1)].getTex2D(), 0);
        d3d::set_render_target(1, data.cloudsTextureWeight[(data.frameNo & 1)].getTex2D(), 0);
        if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
        {
          clouds2_taa_ps_has_empty.getElem()->setStates();
          d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_HAS_EMPTY);
          set_program(clouds2_taa_ps_has_empty.getElem(), clouds2_taa_ps_no_empty.getElem());
          // clouds2_taa_ps_no_empty.getElem()->setStates();
          d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_NO_EMPTY);
        }
        else
          clouds2_taa_ps.render();

        d3d::resource_barrier({data.cloudsTextureColor[(data.frameNo & 1)].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      }
      if (depthLevels != 1)
      {
        if (depth.getTex2D())
          depth.getTex2D()->texmiplevel(-1, -1);
        if (prevDepth.getTex2D())
          prevDepth.getTex2D()->texmiplevel(-1, -1);
      }
    }
    currentCloudsOfs = cloudsOfs;
  }
  void renderDirect(CloudsRendererData &data)
  {
    TIME_D3D_PROFILE(render_clouds_direct);
    G_UNUSED(data);
    clouds2_direct.render();
  }
  static void set_program(ShaderElement *oe, ShaderElement *ne)
  {
    uint32_t program, state_index;
    shaders::RenderStateId rstate;
    uint32_t cstate, tstate;
    int curVariant = get_dynamic_variant_states(ne->native(), program, state_index, rstate, cstate, tstate);
    uint32_t program2, state_index2;
    shaders::RenderStateId rstate2;
    uint32_t cstate2, tstate2;
    int curVariant2 = get_dynamic_variant_states(oe->native(), program2, state_index2, rstate2, cstate2, tstate2);
    if (curVariant2 != curVariant || rstate2 != rstate || state_index2 != state_index)
      ne->setStates();
    else
      d3d::set_program(program);
  }
  void renderFull(CloudsRendererData &data, const TextureIDPair &downsampledDepth, TEXTUREID targetDepth,
    const Point4 &targetDepthTransform, const TMatrix &view_tm, const TMatrix4 &proj_tm)
  {
    TIME_D3D_PROFILE(render_clouds);
    set_viewvecs_to_shader(view_tm, proj_tm);
    if (downsampledDepth.getTex2D())
      downsampledDepth.getTex2D()->texfilter(TEXFILTER_POINT);
    ShaderGlobal::set_texture(clouds_target_depth_gbufVarId, targetDepth);
    ShaderGlobal::set_color4(clouds_target_depth_gbuf_transformVarId, targetDepthTransform);
    ShaderGlobal::set_int(clouds_has_close_sequenceVarId, data.clouds_color_close.getTex2D() ? 1 : 0);
    ShaderGlobal::set_texture(clouds_depth_gbufVarId, downsampledDepth.getId());
    if (!data.cloudsTextureColor[2])
    {
      renderDirect(data);
      return;
    }
    TIME_D3D_PROFILE(apply_bilateral);
    data.setVars();
    const int frameNo = (data.frameNo & 1);
    ShaderGlobal::set_texture(clouds_colorVarId, data.cloudsTextureColor[frameNo]);
    int depthLevels;
    int level = getNotLesserDepthLevel(data, depthLevels, downsampledDepth.getTex2D());
    if (level != 0)
      downsampledDepth.getTex2D()->texmiplevel(level, level);
    if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
    {
      clouds2_apply_has_empty.getElem()->setStates();
      d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_HAS_EMPTY);
      set_program(clouds2_apply_has_empty.getElem(), clouds2_apply_no_empty.getElem());
      // clouds2_apply_no_close_no_empty.getElem()->setStates();
      d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_NO_EMPTY);
    }
    else
    {
      clouds2_apply.render();
    }
    if (level != 0)
      downsampledDepth.getTex2D()->texmiplevel(-1, -1);
  }
};

USE_CONSOLE_BOOL_VAL("clouds", regenShadows, false);
struct CloudsShadows
{
  VoltexRenderer gen_cloud_shadows_volume_partial, copy_cloud_shadows_volume;
  UniqueTexHolder cloudsShadowsTemp;
  enum
  {
    CLOUDS_MAX_SHADOW_TEMPORAL_XZ = 8,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y = 8,
    CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD = CLOUDS_MAX_SHADOW_TEMPORAL_XZ * CLOUD_SHADOWS_WARP_SIZE_XZ,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD = CLOUDS_MAX_SHADOW_TEMPORAL_Y * CLOUD_SHADOWS_WARP_SIZE_Y,
    CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS =
      (CLOUD_SHADOWS_VOLUME_RES_XZ + CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD - 1) / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y_STEPS =
      (CLOUD_SHADOWS_VOLUME_RES_Y + CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD - 1) / CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD,
  };
  uint32_t resetGen = 0;
  void initTemporal()
  {
    copy_cloud_shadows_volume.init("copy_cloud_shadows_volume", "copy_cloud_shadows_volume_ps");
    gen_cloud_shadows_volume_partial.init("gen_cloud_shadows_volume_partial_cs", "gen_cloud_shadows_volume_partial_ps");
    cloudsShadowsTemp.close();
    copy_cloud_shadows_volume.initVoltex(cloudsShadowsTemp, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
      CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD, TEXFMT_R32UI, 1, "cloud_shadows_old_values");

    ShaderGlobal::set_color4(clouds_compute_widthVarId, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
      CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD, 0);
    resetGen = 0;
  }
  void updateTemporalStep(int x, int y, int z)
  {
    ShaderGlobal::set_color4(clouds_start_compute_offsetVarId, x, y, z, 0);
    {
      TIME_D3D_PROFILE(cloud_shadows_vol_copy);
      copy_cloud_shadows_volume.render(cloudsShadowsTemp);
      d3d::resource_barrier({cloudsShadowsTemp.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    }
    {
      TIME_D3D_PROFILE(cloud_shadows_vol_calc);
      gen_cloud_shadows_volume_partial.render(cloudsShadowsVol, 0,
        CLOUD_SHADOWS_WARP_SIZE_XZ *
          IPoint3(CLOUDS_MAX_SHADOW_TEMPORAL_XZ, CLOUDS_MAX_SHADOW_TEMPORAL_XZ, CLOUDS_MAX_SHADOW_TEMPORAL_Y),
        IPoint3(x, y, z));
      d3d::resource_barrier({cloudsShadowsVol.getVolTex(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
      // generateMips2d(cloud1);
    }
  }
  enum
  {
    TEMPORAL_STEPS = CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS * CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS * CLOUDS_MAX_SHADOW_TEMPORAL_Y_STEPS,
    TEMPORAL_STEPS_LERP = 1, // can be used for smoother update light
    ALL_TEMPORAL_STEPS_LERP = TEMPORAL_STEPS * TEMPORAL_STEPS_LERP,
    TEMPORAL_STEP_FORCED = ~0u,
  };
  uint32_t temporalStep = 0, temporalStepFinal = 0;
  bool updateAmbient = false;
  bool updateTemporal()
  {
    bool changed = false;
    if (temporalStep < temporalStepFinal)
    {
      TIME_D3D_PROFILE(cloud_shadows_vol_gradual);
      const uint32_t lerpStep = (temporalStepFinal - temporalStep - 1) / TEMPORAL_STEPS;
      float effect = 1.f - float(lerpStep) / (lerpStep + 1.f);
      ShaderGlobal::set_color4(clouds_new_shadow_ambient_weightVarId, effect, updateAmbient ? effect : 0.f, 0, 0);
      int xs = (temporalStep % CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS),
          ys = (temporalStep / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS) % CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS,
          zs = (temporalStep / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS) %
               CLOUDS_MAX_SHADOW_TEMPORAL_Y_STEPS;
      updateTemporalStep(xs * CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD, ys * CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
        zs * CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD);
      if (++temporalStep != temporalStepFinal)
        return true;
      rerenderShadows2D = true;
      changed = true;
    }
    else if (temporalStep == TEMPORAL_STEP_FORCED)
    {
      TIME_D3D_PROFILE(cloud_shadows_vol_all);
      ShaderGlobal::set_color4(clouds_new_shadow_ambient_weightVarId, 1, updateAmbient ? 1 : 0, 0, 0);
      for (int z = 0; z < CLOUD_SHADOWS_VOLUME_RES_Y; z += CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD)
        for (int y = 0; y < CLOUD_SHADOWS_VOLUME_RES_XZ; y += CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD)
          for (int x = 0; x < CLOUD_SHADOWS_VOLUME_RES_XZ; x += CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD)
            updateTemporalStep(x, y, z);
      rerenderShadows2D = true;
      changed = true;
    }
    validate(); // we re finally finished
    return changed;
  }

  VoltexRenderer genCloudShadowsVolume;
  UniqueTexHolder cloudsShadowsVol;
  void init()
  {
    initTemporal();
    genCloudShadowsVolume.init("gen_cloud_shadows_volume_cs", "gen_cloud_shadows_volume_ps");
    genCloudShadowsVolume.initVoltex(cloudsShadowsVol, CLOUD_SHADOWS_VOLUME_RES_XZ, CLOUD_SHADOWS_VOLUME_RES_XZ,
      CLOUD_SHADOWS_VOLUME_RES_Y, TEXFMT_R8G8, 1, "clouds_shadows_volume");
    cloudsShadowsVol->texaddru(TEXADDR_WRAP);
    cloudsShadowsVol->texaddrv(TEXADDR_WRAP);
    cloudsShadowsVol->texaddrw(TEXADDR_CLAMP);
  }
  Point3 lastLightDir = {0, 0, 0};
  void invalidate() { resetGen = 0; }
  void validate()
  {
    temporalStep = temporalStepFinal = 0;
    updateAmbient = false;
  }
  void addRecalcAll()
  {
    temporalStepFinal += ALL_TEMPORAL_STEPS_LERP;
    while (temporalStepFinal - temporalStep > ALL_TEMPORAL_STEPS_LERP) // if we added more than once
      temporalStepFinal -= ALL_TEMPORAL_STEPS_LERP;
    if (temporalStep > ALL_TEMPORAL_STEPS_LERP) // if we added more than once
    {
      uint32_t fullSteps = temporalStep / ALL_TEMPORAL_STEPS_LERP;
      temporalStepFinal -= fullSteps * ALL_TEMPORAL_STEPS_LERP;
      temporalStep -= fullSteps * ALL_TEMPORAL_STEPS_LERP;
    }
  }
  void forceFullUpdate(const Point3 &main_light_dir, bool update_ambient)
  {
    lastLightDir = main_light_dir;
    updateAmbient |= update_ambient;
    addRecalcAll();
  }
  CloudsChangeFlags render(const Point3 &main_light_dir) // todo: use main_light_dir for next temporality
  {
    const uint32_t cresetGen = get_d3d_full_reset_counter();
    if (lastLightDir != main_light_dir)
      forceFullUpdate(main_light_dir, false);
    if (cresetGen == resetGen) // and sun light changed
    {
      if (regenShadows.get())
        temporalStep = temporalStepFinal = TEMPORAL_STEP_FORCED;
      CloudsChangeFlags ret = CLOUDS_NO_CHANGE;
      if (updateTemporal())
        ret = CLOUDS_INCREMENTAL;
      if (rerenderShadows2D)
        renderShadows2D();
      return ret;
    }
    {
      TIME_D3D_PROFILE(cloud_shadows_vol);

      bool splitWorkload = d3d::get_driver_desc().issues.hasComputeTimeLimited != 0 && genCloudShadowsVolume.isComputeLoaded();
      if (splitWorkload)
      {
        TextureInfo tinfo;
        cloudsShadowsVol->getinfo(tinfo);
        const int splits = 8;

        int splitStep = tinfo.w / splits;
        G_ASSERT((splitStep / CLOUD_SHADOWS_WARP_SIZE_XZ >= 1) && (splitStep % CLOUD_SHADOWS_WARP_SIZE_XZ == 0));
        for (int i = 0; i < splits; ++i)
        {
          if (is_main_thread())
            // this code runs on main thread in blocking manner, avoid ANR
            ::dagor_process_sys_messages();
          else
          {
            // even if we run on non main thread, GPU ownership will block it,
            // release and reacquire ownership after frame is rendered in supposed to be "loading screen"
            int lockedFrame = dagor_frame_no();
            d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
            spin_wait([lockedFrame]() { return lockedFrame >= dagor_frame_no(); });
            d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);
          }

          d3d::GPUWorkloadSplit gpuWorkSplit(true /*do_split*/, true /*split_at_end*/, String(32, "cloud_shadows_vol_split%u", i));
          ShaderGlobal::set_color4(clouds_start_compute_offsetVarId, splitStep * i, 0, 0, 0);
          genCloudShadowsVolume.render(cloudsShadowsVol, 0, {splitStep, -1, -1});
        }
        ShaderGlobal::set_color4(clouds_start_compute_offsetVarId, 0, 0, 0, 0);
      }
      else
      {
        ShaderGlobal::set_color4(clouds_start_compute_offsetVarId, 0, 0, 0, 0);
        genCloudShadowsVolume.render(cloudsShadowsVol);
      }

      d3d::resource_barrier({cloudsShadowsVol.getVolTex(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
      renderShadows2D();
      validate();
      // generateMips2d(cloud1);
    }
    resetGen = cresetGen;
    return CLOUDS_INVALIDATED;
  }
  UniqueTexHolder cloudsShadows2d;
  PostFxRenderer build_shadows_2d_ps; // todo: implement compute version
  bool rerenderShadows2D = false;
  void renderShadows2D()
  {
    if (!cloudsShadows2d)
      return;
    TIME_D3D_PROFILE(clouds_2d_shadows);
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(cloudsShadows2d.getTex2D(), 0);
    build_shadows_2d_ps.render();
    rerenderShadows2D = false;
  }
  void closeShadows2D() { cloudsShadows2d.close(); }
  void initShadows2D()
  {
    if (cloudsShadows2d)
      return;
    // may be build a mip chain?
    cloudsShadows2d = dag::create_tex(NULL, CLOUD_SHADOWS_VOLUME_RES_XZ, CLOUD_SHADOWS_VOLUME_RES_XZ, TEXFMT_L8 | TEXCF_RTARGET, 1,
      "clouds_shadows_2d");
    build_shadows_2d_ps.init("build_shadows_2d_ps");
    rerenderShadows2D = true;
  }
};

#define CLOUDS_LAYERS_USE_CB 0 // seem to be slower with cb

// todo: move it to gameParams
USE_CONSOLE_INT_VAL("clouds", downsampled_field_res, 1, 1, 3); // 1<<(4+x)

struct CloudsField
{
  int resXZ = 512, resY = 64;
  int targetXZ = 512, targetY = 64;
  int downsampleRatio = 8;
  float averaging = 0.65;
  bool useCompression = false;
  VoltexRenderer genCloudsField, genCloudsFieldCmpr, downsampleCloudsField;
  DynamicShaderHelper genCloudLayersNonEmpty, genCloudLayersNonEmptyCmpr;
  eastl::unique_ptr<ComputeShaderElement> refineAltitudes;
  PostFxRenderer refineAltitudesPs;
  UniqueTexHolder cloudsFieldVol, cloudsDownSampledField;
  UniqueTexHolder layersPixelCount, layersHeights;
  struct
  {
    EventQueryHolder query;
    float layerAltStart = 0, layerAltTop = 0;
    bool valid = false;
  } readbackData;
  bool getReadbackData(float &alt_start, float &alt_top) const
  {
    if (!readbackData.valid)
      return false;
    alt_start = readbackData.layerAltStart;
    alt_top = readbackData.layerAltTop;
    return true;
  }

  bool frameValid = false;

  UniqueTex cloudsFieldVolCompressed;
  UniqueTexHolder cloudsFieldVolTemp;
  void closedCompressed()
  {
    cloudsFieldVolTemp.close();
    cloudsFieldVolCompressed.close();
  }
  void initCompressed()
  {
    genCloudsFieldCmpr.init("gen_cloud_field_cmpr", "gen_cloud_field_cmpr_ps");
    if (!genCloudsFieldCmpr.isComputeLoaded())
      genCloudLayersNonEmptyCmpr.init("gen_cloud_layers_non_empty_cmpr", nullptr, 0, "gen_cloud_layers_non_empty_cmpr", false);
    // todo: on consoles we don't need temp, we can alias memory
    genCloudsFieldCmpr.initVoltex(cloudsFieldVolTemp, resXZ / 4, resXZ / 4, resY, TEXFMT_R32G32UI | CLOUDS_ESRAM_ONLY, 1,
      "clouds_field_volume_tmp");
    cloudsFieldVolCompressed =
      dag::create_voltex(resXZ, resXZ, resY, TEXFMT_ATI1N | TEXCF_UPDATE_DESTINATION, 1, "clouds_field_volume_compressed");
    cloudsFieldVolCompressed->texaddru(TEXADDR_WRAP);
    cloudsFieldVolCompressed->texaddrv(TEXADDR_WRAP);
    cloudsFieldVolCompressed->texaddrw(TEXADDR_BORDER);
    ShaderGlobal::set_texture(clouds_field_volumeVarId, cloudsFieldVolCompressed);
  }

  void genFieldGeneral(VoltexRenderer &renderer, DynamicShaderHelper non_empty_fill, ManagedTex &voltex)
  {
    if (renderer.isComputeLoaded())
      d3d::set_rwtex(STAGE_CS, 1, layersPixelCount.getTex2D(), 0, 0);
    renderer.render(voltex);
    if (renderer.isComputeLoaded())
      d3d::set_rwtex(STAGE_CS, 1, nullptr, 0, 0);

    d3d::resource_barrier({voltex.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    if (!renderer.isComputeLoaded()) // we didn't fill layersPixelCount
    {
      SCOPE_RENDER_TARGET;
      ShaderGlobal::set_color4(dispatch_sizeVarId, resXZ, resXZ, resY, 0);
      d3d::set_render_target(layersPixelCount.getTex2D(), 0);
      non_empty_fill.shader->setStates(0, true);
      d3d::setvsrc(0, 0, 0);
      d3d::draw_instanced(PRIM_TRILIST, 0, 1, resXZ * resXZ);
    }
    d3d::resource_barrier({layersPixelCount.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }

  void genFieldCompressed()
  {
    TIME_D3D_PROFILE(cloud_field_vol_cmpr);
    genFieldGeneral(genCloudsFieldCmpr, genCloudLayersNonEmptyCmpr, cloudsFieldVolTemp);

    // todo: remove me on consoles - we can alias memory
    TIME_D3D_PROFILE(copy_compr);
    cloudsFieldVolCompressed->updateSubRegion(cloudsFieldVolTemp.getVolTex(), 0, 0, 0, 0, // source mip, x,y,z
      max(1, resXZ / 4), max(1, resXZ / 4), resY,                                         // width x height x depth
      0, 0, 0, 0);                                                                        // dest mip, x,y,z
    // todo: we can remove temp texture
  }

  void genField()
  {
    TIME_D3D_PROFILE(cloud_field_vol);
    genFieldGeneral(genCloudsField, genCloudLayersNonEmpty, cloudsFieldVol);
    d3d::resource_barrier({cloudsFieldVol.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }

  void initDownsampledField()
  {
    cloudsDownSampledField.close();
    downsampleCloudsField.initVoltex(cloudsDownSampledField, (resXZ + downsampleRatio - 1) / downsampleRatio,
      (resXZ + downsampleRatio - 1) / downsampleRatio, (resY + downsampleRatio - 1) / downsampleRatio, TEXFMT_L8, 1,
      "clouds_field_volume_low");
    cloudsDownSampledField->texaddru(TEXADDR_WRAP);
    cloudsDownSampledField->texaddrv(TEXADDR_WRAP);
    cloudsDownSampledField->texaddrw(TEXADDR_BORDER);
    cloudsDownSampledField.setVar();
    d3d::resource_barrier({cloudsDownSampledField.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }
  void renderDownsampledField()
  {
    TIME_D3D_PROFILE(downsample_field_vol);
    ShaderGlobal::set_int(clouds_field_downsample_ratioVarId, downsampleRatio);
    downsampleCloudsField.render(cloudsDownSampledField);
    d3d::resource_barrier({cloudsDownSampledField.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }
  void init()
  {
    initCloudsVolumeRenderer();
    cloudsFieldVol.close();
    closedCompressed();
    if (useCompression)
    {
      initCompressed();
    }
    if (!cloudsFieldVolTemp)
    {
      genCloudsField.init("gen_cloud_field", "gen_cloud_field_ps");
      genCloudsField.initVoltex(cloudsFieldVol, resXZ, resXZ, resY, TEXFMT_L8, 1, "clouds_field_volume");
      if (!genCloudsField.isComputeLoaded())
        genCloudLayersNonEmpty.init("gen_cloud_layers_non_empty", nullptr, 0, "gen_cloud_layers_non_empty", false);
      cloudsFieldVol->texaddru(TEXADDR_WRAP);
      cloudsFieldVol->texaddrv(TEXADDR_WRAP);
      cloudsFieldVol->texaddrw(TEXADDR_BORDER);
      cloudsFieldVol.setVar();
      d3d::resource_barrier({cloudsFieldVol.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    }
    downsampleCloudsField.init("downsample_cloud_field", "downsample_cloud_field_ps", 1, 1, 1);
    initDownsampledField();

    layersPixelCount.close();
    layersPixelCount = dag::create_tex(nullptr, resY, 1,
      (genCloudsField.isComputeLoaded() ? TEXCF_UNORDERED : 0) | TEXCF_RTARGET | TEXFMT_R32F, 1, "cloud_layers_non_empty");

    if (VoltexRenderer::is_compute_supported())
      refineAltitudes.reset(new_compute_shader("clouds_refine_altitudes", false));
    else
      refineAltitudesPs.init("clouds_refine_altitudes_ps");
    if (!layersHeights)
      layersHeights = dag::create_tex(nullptr, 2, 1, (refineAltitudes ? TEXCF_UNORDERED : TEXCF_RTARGET) | TEXFMT_A32B32G32R32F, 1,
        "cloud_layers_altitudes_tex");
    if (!readbackData.query)
    {
      readbackData.query.reset(d3d::create_event_query());
      readbackData.valid = false;
    }

    frameValid = false;
  }
  void layersHeightsBarrier() { d3d::resource_barrier({layersHeights.getTex2D(), RB_RO_SRV | RB_STAGE_ALL_SHADERS, 0, 0}); }
  void setParams(const DaSkies::CloudsSettingsParams &params)
  {
    const int xz = clamp(256 * params.quality, 128, 2048), y = clamp(32, 32 * params.quality, 192);
    const bool nextCompression = !params.fastEvolution && d3d::check_voltexformat(TEXFMT_ATI1N | CLOUDS_ESRAM_ONLY) &&
                                 d3d::get_driver_desc().caps.hasResourceCopyConversion;
    const bool compressionChanged = nextCompression != useCompression;
    useCompression = nextCompression;

    if (xz != resXZ || y != resY || compressionChanged)
    {
      resXZ = xz;
      resY = y;
      init();
    }
    int txz = xz, ty = y;
    if (params.competitive_advantage)
    {
      txz = clamp(256 * params.target_quality, 128, 2048), ty = clamp(32, 32 * params.target_quality, 192);
      if (targetXZ != txz || targetY != ty || averaging != params.maximum_averaging_ratio)
        invalidate();
    }
    averaging = params.maximum_averaging_ratio;
    targetXZ = txz;
    targetY = ty;
  }
  void invalidate()
  {
    frameValid = false;
    readbackData.valid = false;
  }

  void doReadback()
  {

    if (!readbackData.valid && d3d::get_event_query_status(readbackData.query.get(), false))
    {
      int stride;
      TIME_PROFILE(end_clouds_gpu_readback);
      if (auto lockedTex = lock_texture<Point4>(layersHeights.getTex2D(), stride, 0, TEXLOCK_READ))
      {
        readbackData.layerAltStart = lockedTex[1].x;
        readbackData.layerAltTop = lockedTex[1].y;
      }
      readbackData.valid = true;
    }
  }

  CloudsChangeFlags render()
  {
    if (frameValid && !regenField.get())
    {
      int ratio = resXZ >= 512 ? 3 : 2;
      ratio = downsampled_field_res.get() == 1 ? ratio : downsampled_field_res.get();
      if ((1 << ratio) != downsampleRatio)
      {
        downsampleRatio = (1 << ratio);
        initDownsampledField();
        renderDownsampledField();
      }
      doReadback();
      return CLOUDS_NO_CHANGE;
    }
    {
      TIME_D3D_PROFILE(clear_layer);
      SCOPE_RENDER_TARGET;
      d3d_err(d3d::set_render_target(layersPixelCount.getTex2D(), 0));
      d3d_err(d3d::clearview(CLEAR_TARGET, E3DCOLOR(), 0, 0));
      d3d::resource_barrier({layersPixelCount.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    }
    {
      ShaderGlobal::set_color4(clouds_field_resVarId, resXZ, resY, targetXZ, targetY);
      ShaderGlobal::set_real(clouds_average_weightVarId, averaging);
      if (useCompression)
        genFieldCompressed();
      else
        genField();
      renderDownsampledField();
      TIME_D3D_PROFILE(refineLayers);
      if (refineAltitudes)
      {
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), layersHeights.getTex2D());
        refineAltitudes->dispatch(1, 1, 1);
      }
      else
      {
        SCOPE_RENDER_TARGET;
        d3d::set_render_target(layersHeights.getTex2D(), 0);
        refineAltitudesPs.render();
      }
      d3d::resource_barrier({layersHeights.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

      readbackData.valid = false;
      TIME_D3D_PROFILE(start_gpu_readback);
      /* NOTE: this optimization does not work on dx12 driver and disabled
      After prefetch lockimg(nullptr, ...) texture have a state 0x0 (in a split barrier), so we cannot use the
      texture in any shaders after prefetch and before real fetch (in function doReadback).
      The texture is used in macros DISTANCE_TO_CLOUDS2 in several shaders.
      int stride;
      if (layersHeights.getTex2D()->lockimg(nullptr, stride, 0, TEXLOCK_READ | TEXLOCK_NOSYSLOCK))//start readback
        layersHeights.getTex2D()->unlockimg();*/
      d3d::issue_event_query(readbackData.query.get());
    }
    frameValid = true;
    return CLOUDS_INVALIDATED;
  }

  eastl::unique_ptr<ComputeShaderElement> build_dacloud_volume_cs;
  PostFxRenderer build_dacloud_volume_ps;
  void initCloudsVolumeRenderer()
  {
    if (d3d::get_driver_desc().shaderModel >= 5.0_sm)
      build_dacloud_volume_cs.reset(new_compute_shader("build_dacloud_volume_cs", false));
    else
      build_dacloud_volume_ps.init("build_dacloud_volume_ps");
  }

  void renderCloudVolume(VolTexture *cloudVolume, float max_dist, const TMatrix &view_tm, const TMatrix4 &proj_tm)
  {
    if (!cloudVolume)
      return;
    TextureInfo tinfo;
    cloudVolume->getinfo(tinfo);
    set_viewvecs_to_shader(view_tm, proj_tm);
    ShaderGlobal::set_color4(cloud_volume_resVarId, tinfo.w, tinfo.h, tinfo.d, max_dist);
    if ((tinfo.cflg & TEXCF_UNORDERED) && build_dacloud_volume_cs.get())
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), cloudVolume);
      build_dacloud_volume_cs->dispatchThreads(tinfo.w, tinfo.h, 1);
    }
    else
    {
      SCOPE_RENDER_TARGET;
      d3d::set_render_target(0, cloudVolume, d3d::RENDER_TO_WHOLE_ARRAY, 0);
      build_dacloud_volume_ps.getElem()->setStates();
      d3d::draw_instanced(PRIM_TRILIST, 0, 1, tinfo.d);
    }
  }
};

struct CloudsLightRenderer
{
  UniqueTexHolder clouds_light_color;
  PostFxRenderer gen_clouds_light_texture_ps;
  eastl::unique_ptr<ComputeShaderElement> gen_clouds_light_texture_cs;
  uint32_t resetGen = 0;
  // float blendTimeLeft = 0.;
  // float lastDt = 0.f;
  // const float defaultBlendTime = 0.25;//new light changes in this time.
  // Color3 last_sun_light_color={1,1,1};
  float lastMainLightDirY = 0, lastSecondLightDirY = 0;
  void init()
  {
    if (VoltexRenderer::is_compute_supported())
      gen_clouds_light_texture_cs.reset(new_compute_shader("gen_clouds_light_texture_cs", true));
    else
      gen_clouds_light_texture_ps.init("gen_clouds_light_texture_ps");
    clouds_light_color.close();
    int texflags = gen_clouds_light_texture_cs ? TEXCF_UNORDERED : TEXCF_RTARGET;
    int texfmt =
      (d3d::get_texformat_usage(TEXFMT_R11G11B10F, RES3D_TEX) & texflags) == texflags ? TEXFMT_R11G11B10F : TEXFMT_A16B16G16R16F;
    clouds_light_color = dag::create_voltex(8, 8, 2 * CLOUDS_LIGHT_TEXTURE_WIDTH, texfmt | texflags, 1, "clouds_light_color");
    clouds_light_color->texaddr(TEXADDR_CLAMP);
    resetGen = 0;
  }
  void update(float) {}
  // void update(float dt) { lastDt = dt; }
  // void invalidate() {blendTimeLeft = defaultBlendTime;}
  void invalidate() { resetGen = 0; }

  CloudsChangeFlags render(const Point3 &main_light_dir, const Point3 &second_light_dir)
  {
    if (fabsf(lastMainLightDirY - main_light_dir.y) > 1e-4f || fabsf(lastSecondLightDirY - second_light_dir.y) > 1e-4f) // todo: check
                                                                                                                        // for
                                                                                                                        // brightness
                                                                                                                        // as well?
      invalidate();
    const uint32_t cresetGen = get_d3d_full_reset_counter();
    if (resetGen == cresetGen) // nothing to update
      return CLOUDS_NO_CHANGE;
    TIME_D3D_PROFILE(render_clouds_light);
    lastMainLightDirY = main_light_dir.y;
    lastSecondLightDirY = second_light_dir.y;

    if (gen_clouds_light_texture_cs)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), clouds_light_color.getVolTex());
      gen_clouds_light_texture_cs->dispatch(1, 1, 2 * CLOUDS_LIGHT_TEXTURE_WIDTH);
    }
    else
    {
      SCOPE_RENDER_TARGET;
      d3d::set_render_target(0, clouds_light_color.getVolTex(), d3d::RENDER_TO_WHOLE_ARRAY, 0);
      gen_clouds_light_texture_ps.getElem()->setStates();
      d3d::draw_instanced(PRIM_TRILIST, 0, 1, 2 * CLOUDS_LIGHT_TEXTURE_WIDTH);
    }
    d3d::resource_barrier({clouds_light_color.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    resetGen = cresetGen;
    return CLOUDS_INCREMENTAL; // although it can be that its first time, but then shadows will return INVALIDATED
  }
};

struct Clouds2
{
  void setCloudsOfs(const DPoint2 &xz) { clouds.setCloudsOfs(xz); }
  DPoint2 getCloudsOfs() const { return clouds.getCloudsOfs(); }
  // use GPU readback if available needed!
  float minimalStartAlt() const { return cloudsStartAt; }
  float maximumTopAlt() const { return cloudsEndAt; }
  float getCloudsShadowCoverage() const { return cloudsShadowCoverage; }
  float effectiveStartAlt() const
  {
    float altStart, altTop;
    return field.getReadbackData(altStart, altTop) ? altStart : cloudsStartAt;
  }
  float effectiveTopAlt() const
  {
    float altStart, altTop;
    return field.getReadbackData(altStart, altTop) ? altTop : cloudsEndAt;
  }
  bool hasVisibleClouds() const
  {
    if (weatherParams.layers[0].coverage > 0 || weatherParams.layers[1].coverage > 0 || weatherParams.cumulonimbusCoverage > 0 ||
        weatherParams.epicness > 0)
    {
      float altStart, altTop;
      return field.getReadbackData(altStart, altTop) ? altStart <= altTop : true;
    }
    return false;
  }
  // GPU readback!

  void updateErosionNoiseWind(float dt)
  {
    erosionWindChange = windDir * (rendering.erosionWindSpeed * dt);
    clouds_erosion_noise_wind_ofs += erosionWindChange;
    float erosionNoiseSize = rendering.erosion_noise_size * 32.f;
    clouds_erosion_noise_wind_ofs =
      clouds_erosion_noise_wind_ofs - floor(clouds_erosion_noise_wind_ofs / erosionNoiseSize) * erosionNoiseSize;
  }
  void setErosionNoiseWindDir(const Point2 &wind_dir) { windDir = wind_dir; } // normalized
  // sets erosion noise texel size
  static Point2 alt_to_diap(float bottom, float thickness, float bigger_bottom, float bigger_thickness)
  {
    const float zero = (bottom - bigger_bottom) / bigger_thickness;
    const float one = (bottom + thickness - bigger_bottom) / bigger_thickness;
    return Point2(1 / (one - zero), -zero / (one - zero));
  }
  void calcCloudsAlt()
  {
    cloudsStartAt = min(form.layers[0].startAt, form.layers[1].startAt);
    cloudsEndAt = max(form.layers[0].thickness + form.layers[0].startAt, form.layers[1].thickness + form.layers[1].startAt);
    cloudsEndAt = max(cloudsStartAt + 0.001f, cloudsEndAt);
  }

  void setCloudsShadowCoverage()
  {
    calcCloudsAlt();
    const float cloudsThickness = cloudsEndAt - cloudsStartAt;
    // we can calculate exact coverage with compute shaders. But not as easy on pixel shaders.
    // so, instead we use CPU approximation
    // and for better quality we need godrays anyway, so it is just plan B
    // magic numbers obtained with empirical data
    auto safePow = [](float v, float p) { return v > 0.f ? powf(v, p) : 0.f; };
    float averageCoverage =
      safePow(weatherParams.layers[0].coverage * 2 - 1, 8) * 0.25 * lerp(0.17, 0.31, form.layers[0].clouds_type) *
        max(0.f, form.layers[0].thickness - (2.f / cloudsThickness / 64)) / cloudsThickness // 2/64 is two texels
      + safePow(weatherParams.layers[1].coverage * 2 - 1, 8) * 0.25 * lerp(0.17, 0.31, form.layers[1].clouds_type) *
          max(0.f, form.layers[1].thickness - (2.f / cloudsThickness / 64)) / cloudsThickness; // 2/64 is two texels
    float averageCumulonimbusCoverage = safePow(weatherParams.cumulonimbusCoverage * 2 - 1, 4) * 0.5;
    cloudsShadowCoverage = lerp(averageCoverage, averageCumulonimbusCoverage, weatherParams.cumulonimbusCoverage);
    ShaderGlobal::set_real(clouds_shadow_coverageVarId, cloudsShadowCoverage);
  }
  void setCloudLightVars()
  {
    // todo: if layer is invisible, hide one inside other

    ShaderGlobal::set_int(clouds_shape_scaleVarId,
      max(int(1), (int)floorf((form.shapeNoiseScale * weatherParams.worldSize) / 65536.0 + 0.5)));

    ShaderGlobal::set_int(clouds_cumulonimbus_shape_scaleVarId,
      max(int(1), (int)floorf((form.cumulonimbusShapeScale * weatherParams.worldSize) / 65536.0 + 0.5)));
    ShaderGlobal::set_int(clouds_turbulence_freqVarId, form.turbulenceFreq);
    ShaderGlobal::set_real(clouds_turbulence_scaleVarId, form.turbulenceStrength);
    calcCloudsAlt();
    const float cloudsThickness = cloudsEndAt - cloudsStartAt;
    ShaderGlobal::set_color4(clouds_layers_typesVarId, form.layers[0].clouds_type, form.layers[0].clouds_type_variance,
      form.layers[1].clouds_type, form.layers[1].clouds_type_variance);
    float global_clouds_sigma = clouds_sigma_from_extinction(form.extinction);
    ShaderGlobal::set_real(global_clouds_sigmaVarId, global_clouds_sigma);
    ShaderGlobal::set_real(clouds_first_layer_densityVarId, form.layers[0].density);
    ShaderGlobal::set_real(clouds_second_layer_densityVarId, form.layers[1].density);
    ShaderGlobal::set_real(clouds_thickness2VarId, cloudsThickness);
    ShaderGlobal::set_real(clouds_start_altitude2VarId, cloudsStartAt);
    ShaderGlobal::set_color4(clouds_height_fractionsVarId,
      P2D(alt_to_diap(form.layers[0].startAt, form.layers[0].thickness, cloudsStartAt, cloudsThickness)),
      P2D(alt_to_diap(form.layers[1].startAt, form.layers[1].thickness, cloudsStartAt, cloudsThickness)));

    // TODO: until node based shaders have preshader support, we calculate the necessary shadervars by hand here:
    float clouds_start_altitude2 = cloudsStartAt;
    float clouds_thickness2 = cloudsThickness;
    float clouds_weather_size = weatherParams.worldSize;
    ShaderGlobal::set_color4(nbs_world_pos_to_clouds_alt__inv_clouds_weather_size__neg_clouds_thickness_mVarId,
      0.001f / clouds_thickness2, -clouds_start_altitude2 / clouds_thickness2, 1.0f / clouds_weather_size, -1000 * clouds_thickness2);
    ShaderGlobal::set_real(nbs_clouds_start_altitude2_metersVarId, cloudsStartAt * 1000);

    setCloudsShadowCoverage();
    // ShaderGlobal::set_color4(clouds_wind_alt_gradientVarId,
    //   cloudsLight.windGradientDir.x*(top-bottom)*form.gradient_per1km,
    //   cloudsLight.windGradientDir.y*(top-bottom)*form.gradient_per1km, 0,0);
  }
  void setCloudRenderingVars()
  {
    ShaderGlobal::set_real(clouds_forward_eccentricityVarId, rendering.forward_eccentricity);
    ShaderGlobal::set_real(clouds_back_eccentricityVarId, -rendering.back_eccentricity);
    ShaderGlobal::set_real(clouds_forward_eccentricity_weightVarId, rendering.forward_eccentricity_weight);
    float erosionNoiseSize = rendering.erosion_noise_size * 32.f;
    ShaderGlobal::set_real(clouds_erosion_noise_tile_sizeVarId, erosionNoiseSize);
    ShaderGlobal::set_real(clouds_ambient_desaturationVarId, rendering.ambient_desaturation);
    ShaderGlobal::set_real(clouds_ms_contributionVarId, rendering.ms_contribution);
    ShaderGlobal::set_real(clouds_ms_attenuationVarId, rendering.ms_attenuation);
    ShaderGlobal::set_real(clouds_ms_eccentricity_attenuationVarId, rendering.ms_ecc_attenuation);
  }
  void setWeatherGenVars()
  {
    ShaderGlobal::set_real(clouds_weather_sizeVarId, weatherParams.worldSize);
    ShaderGlobal::set_real(clouds_layer1_coverageVarId, weatherParams.layers[0].coverage);
    ShaderGlobal::set_real(clouds_layer1_freqVarId, weatherParams.layers[0].freq);
    ShaderGlobal::set_real(clouds_layer1_seedVarId, weatherParams.layers[0].seed);
    ShaderGlobal::set_real(clouds_layer2_coverageVarId, weatherParams.layers[1].coverage);
    ShaderGlobal::set_real(clouds_layer2_freqVarId, weatherParams.layers[1].freq);
    ShaderGlobal::set_real(clouds_layer2_seedVarId, weatherParams.layers[1].seed);
    ShaderGlobal::set_real(clouds_epicnessVarId, weatherParams.epicness);
    ShaderGlobal::set_real(clouds_rain_clouds_amountVarId, weatherParams.cumulonimbusCoverage);
    ShaderGlobal::set_real(clouds_rain_clouds_seedVarId, weatherParams.cumulonimbusSeed);
  }
  void setGameParams(const DaSkies::CloudsSettingsParams &game_params)
  {
    if (gameParams == game_params)
      return;
    gameParams = game_params;
    field.setParams(gameParams);
    field.invalidate();
    cloudShadows.invalidate(); // todo: instead force temporal recalc!
    holeFound = false;
  }

  void setWeatherGen(const DaSkies::CloudsWeatherGen &weather_params)
  {
    if (weatherParams == weather_params)
      return;
    weatherParams = weather_params;
    invalidateWeather();
  }
  void invalidateWeather()
  {
    setWeatherGenVars();
    weather.invalidate();
    invalidateShadows();
  }
  void invalidateLight() { light.invalidate(); }
  void invalidateShadows()
  {
    setCloudLightVars();

    cloudsForm.invalidate(); // actually we need to invalidate only if something except extinction. Extinction shouldn't invalidate
                             // lut.
    // but it is so fast to compute

    field.invalidate();
    cloudShadows.invalidate(); // todo: instead force temporal recalc!
    holeFound = false;
    // light.invalidate();
  }
  void setCloudsForm(const DaSkies::CloudsForm &form_)
  {
    if (form == form_)
      return;
    form = form_;
    calcCloudsAlt();
    invalidateShadows();
  }

  void setCloudsRendering(const DaSkies::CloudsRendering &rendering_)
  {
    if (rendering == rendering_)
      return;
    rendering = rendering_;
    // fixme: only ambient desturation should cause this
    invalidateLight();
    setCloudRenderingVars();

    // fixme: invalidate panorama, if we change everything besides erosion speed
  }

  // temporary function for a workaround on A8 iPads (gpu hang)
  void setRenderingEnabled(bool enabled) { renderingEnabled = enabled; }

  // causes clouds to change lighting
  // void setWindAltGradient(float x, float z) {cloudsLight.windGradientDir = Point2(x,z);}
  void invalidateAll()
  {
    invalidateWeather();
    invalidateLight();
    invalidateShadows();
    setCloudLightVars();
    setCloudRenderingVars();
  }

  void init(bool useHole = true)
  {
#define VAR(a, opt) a##VarId = ::get_shader_variable_id(#a, opt);
    CLOUDS_VARS_LIST
#undef VAR

    noises.init();
    weather.init();
    clouds.init();
    field.init();
    cloudShadows.init();
    light.init();
    cloudsForm.init();
    calcCloudsAlt();
    if (useHole)
      initHole();

    invalidateWeather();
    invalidateLight();
    invalidateShadows();
    setCloudLightVars();
    setCloudRenderingVars();
  }
  void update(float dt)
  {
    updateErosionNoiseWind(dt);
    light.update(dt);

    ShaderGlobal::set_color4(clouds_erosion_noise_wind_ofsVarId, clouds_erosion_noise_wind_ofs.x, 0, clouds_erosion_noise_wind_ofs.y,
      0);
  }
  void setCloudsOffsetVars(CloudsRendererData &data) { clouds.setCloudsOffsetVars(data, weatherParams.worldSize); }
  void setCloudsOffsetVars(float currentCloudsOffset) { clouds.setCloudsOffsetVars(currentCloudsOffset, weatherParams.worldSize); }
  IPoint2 getResolution(CloudsRendererData &data) { return IPoint2(data.w, data.h); }
  void renderClouds(CloudsRendererData &data, const TextureIDPair &depth, const TextureIDPair &prev_depth, const TMatrix &view_tm,
    const TMatrix4 &proj_tm)
  {
    if (!renderingEnabled)
    {
      return;
    }
    noises.setVars();
    clouds.render(data, depth, prev_depth, erosionWindChange, weatherParams.worldSize, view_tm, proj_tm);
  }
  void renderCloudsScreen(CloudsRendererData &data, const TextureIDPair &downsampledDepth, TEXTUREID targetDepth,
    const Point4 &targetDepthTransform, const TMatrix &view_tm, const TMatrix4 &proj_tm)
  {
    if (!renderingEnabled)
    {
      return;
    }
    clouds.renderFull(data, downsampledDepth, targetDepth, targetDepthTransform, view_tm, proj_tm);
  }

  bool isPrepareRequired() const { return noises.isPrepareRequired(); }

  enum
  {
    HOLE_UPDATED = 0,
    HOLE_PROCESSED = 1,
    HOLE_CREATED = 2
  };

  void processHole()
  {
    if (needHoleCPUUpdate == HOLE_UPDATED)
      return;

    if (needHoleCPUUpdate == HOLE_CREATED)
    {
      int frame1;
      Texture *currentTarget = ringTextures.getNewTarget(frame1);
      if (currentTarget)
      {
        d3d::stretch_rect(holePosTex.getTex2D(), currentTarget, NULL, NULL);
        ringTextures.startCPUCopy();
        needHoleCPUUpdate = HOLE_PROCESSED;
      }
    }

    if (needHoleCPUUpdate == HOLE_PROCESSED)
    {
      uint32_t frame2;
      int stride;
      uint8_t *data = ringTextures.lock(stride, frame2);
      if (!data)
        return;
      Point4 destData;
      memcpy(&destData, data, sizeof(Point4));

      ringTextures.unlock();
      ShaderGlobal::set_color4(clouds_hole_posVarId, destData.x, destData.y, destData.z, destData.w);
      needHoleCPUUpdate = HOLE_UPDATED;
    }
  }

  CloudsChangeFlags prepareLighting(const Point3 &main_light_dir, const Point3 &second_light_dir)
  {
    uint32_t changes = CLOUDS_NO_CHANGE;
    if (noises.render())
    {
      invalidateWeather();
      invalidateLight();
      changes = CLOUDS_INVALIDATED;
    }
    changes |= uint32_t(weather.render());
    changes |= uint32_t(cloudsForm.render());
    changes |= uint32_t(field.render());

    if (noises.isReady())
    {
      const uint32_t cloudsShadowsUpdateFlags = cloudShadows.render(main_light_dir);
      changes |= uint32_t(cloudsShadowsUpdateFlags);
      changes |= uint32_t(light.render(main_light_dir, second_light_dir)); // not afr ready

      if (cloudsShadowsUpdateFlags == CLOUDS_INVALIDATED)
        holeFound = false;
    }

    changes |= validateHole(main_light_dir) ? uint32_t(CLOUDS_INVALIDATED) : uint32_t(0);

    processHole();

    return CloudsChangeFlags(changes);
  }
  void renderCloudVolume(VolTexture *cloudVolume, float max_dist, const TMatrix &view_tm, const TMatrix4 &proj_tm)
  {
    if (!renderingEnabled)
    {
      return;
    }
    field.renderCloudVolume(cloudVolume, max_dist, view_tm, proj_tm);
  }
  void setUseShadows2D(bool on)
  {
    if (on)
      cloudShadows.initShadows2D();
    else
      cloudShadows.closeShadows2D();
  }
  void reset() { noises.reset(); }
  void initHole()
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::BLEND_OP);
    state.blendOp = BLENDOP_MAX;
    blendMaxState.reset(shaders::overrides::create(state));

    int posTexFlags = TEXFMT_A32B32G32R32F;
    if (VoltexRenderer::is_compute_supported())
    {
      clouds_hole_cs.reset(new_compute_shader("clouds_hole_cs", true));
      holeBuf = dag::buffers::create_ua_sr_structured(4, 1, "clouds_hole_buf");

      clouds_hole_pos_cs.reset(new_compute_shader("clouds_hole_pos_cs", true));
      posTexFlags |= TEXCF_UNORDERED;
    }
    else
    {
      clouds_hole_ps.init("clouds_hole_ps");
      clouds_hole_pos_ps.init("clouds_hole_pos_ps");

      holeTex = dag::create_tex(nullptr, 1, 1, TEXCF_RTARGET | TEXFMT_R32F, 1, "clouds_hole_tex");
      posTexFlags |= TEXCF_RTARGET;
    }

    holePosTex = dag::create_tex(nullptr, 1, 1, posTexFlags | TEXCF_CLEAR_ON_CREATE, 1, "clouds_hole_pos_tex");
    ringTextures.close();
    ringTextures.init(1, 1, 1, "CPU_clouds_hole_tex",
      TEXFMT_A32B32G32R32F | TEXCF_RTARGET | TEXCF_LINEAR_LAYOUT | TEXCF_CPU_CACHED_MEMORY);
#if !_TARGET_PC_WIN && !_TARGET_XBOX
    d3d::GpuAutoLock gpuLock;
    if (VoltexRenderer::is_compute_supported())
    {
      Point4 zeroPos(0, 0, 0, 0);
      d3d::clear_rwtexf(holePosTex.getTex2D(), reinterpret_cast<float *>(&zeroPos), 0, 0);
    }
    else
    {
      SCOPE_RENDER_TARGET;
      d3d::set_render_target(holePosTex.getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, E3DCOLOR(), 0, 0);
    }
#endif
    d3d::resource_barrier({holePosTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_ALL_GRAPHICS, 0, 0});
    holePosTexStaging = dag::create_tex(nullptr, 1, 1, TEXFMT_A32B32G32R32F, 1, "clouds_hole_pos_tex_1");
  }

  void layersHeightsBarrier() { field.layersHeightsBarrier(); }

  bool validateHole(const Point3 &main_light_dir)
  {
    if (holeFound)
      return false;

    holeFound = true;
    if (!findHole(main_light_dir))
    {
      Point2 hole(0, 0);
      setHole(hole);
    }
    needHoleCPUUpdate = HOLE_CREATED;
    ringTextures.reset();
    return true;
  }

  bool findHole(const Point3 &main_light_dir)
  {
    if (!((holeBuf || holeTex) && cloudShadows.cloudsShadowsVol))
      return false;

    ShaderGlobal::set_real(clouds_hole_densityVarId, holeDensity);
    TIME_D3D_PROFILE(look_for_hole);
    const float alt = (minimalStartAlt() + (maximumTopAlt() - minimalStartAlt()) * 0.5 / 32) * 1000.f - holeTarget.y;
    if (clouds_hole_cs && clouds_hole_pos_cs)
    {
      {
        uint32_t v[4] = {0xFFFFFFFF};
        d3d::clear_rwbufi(holeBuf.getBuf(), v);
        d3d::resource_barrier({holeBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
        STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), holeBuf.getBuf());
        clouds_hole_cs->dispatchThreads(CLOUDS_HOLE_GEN_RES, CLOUDS_HOLE_GEN_RES, 1);
        d3d::resource_barrier({holeBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
      }
      {
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), holePosTex.getTex2D());
        ShaderGlobal::set_color4(clouds_hole_target_altVarId, P3D(holeTarget), alt);
        ShaderGlobal::set_color4(clouds_hole_light_dirVarId, P3D(main_light_dir), 0);
        clouds_hole_pos_cs->dispatch();
        d3d::resource_barrier({holePosTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_ALL_GRAPHICS, 0, 0});
      }
    }
    else if (clouds_hole_ps.getElem() && clouds_hole_pos_ps.getElem())
    {
      SCOPE_RENDER_TARGET;
      d3d::set_render_target(holeTex.getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, E3DCOLOR(), 0, 0);
      shaders::overrides::set(blendMaxState.get());
      clouds_hole_ps.getElem()->setStates();
      d3d::draw_instanced(PRIM_TRILIST, 0, 1, CLOUDS_HOLE_GEN_RES * CLOUDS_HOLE_GEN_RES);
      shaders::overrides::reset();

      ShaderGlobal::set_color4(clouds_hole_target_altVarId, P3D(holeTarget), alt);
      ShaderGlobal::set_color4(clouds_hole_light_dirVarId, P3D(main_light_dir), 0);
      d3d::set_render_target(holePosTex.getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, E3DCOLOR(), 0, 0);
      clouds_hole_pos_ps.getElem()->setStates();
      d3d::draw_instanced(PRIM_TRILIST, 0, 1, 1);
      d3d::resource_barrier({holePosTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_ALL_GRAPHICS, 0, 0});
    }
    return true;
  }
  void resetHole(const Point3 &hole_target, const float &hole_density)
  {
    holeTarget = hole_target;
    holeDensity = hole_density;
    holeFound = false;
  }
  Point2 getCloudsHole() const
  {
    Point2 point(0, 0);
    if (holePosTex.getTex2D() && holeFound)
    {
      int stride;
      if (auto lockedTex = lock_texture<Point2>(holePosTex.getTex2D(), stride, 0, TEXLOCK_READ))
      {
        point = lockedTex[0];
        d3d::resource_barrier({holePosTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_ALL_GRAPHICS, 0, 0});
      }
    }
    return point;
  }
  void setHole(const Point2 &p)
  {
    Point4 bufferData(p.x, p.y, p.x / weatherParams.worldSize, p.y / weatherParams.worldSize);
    Point4 *holePos;
    int stride;
    if (holePosTexStaging && holePosTexStaging->lockimgEx(&holePos, stride, 0, TEXLOCK_WRITE) && holePos)
    {
      *holePos = bufferData;
      holePosTexStaging->unlockimg();
      holePosTex->update(holePosTexStaging.getTex2D());
      d3d::resource_barrier({holePosTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_ALL_GRAPHICS, 0, 0});
      holeFound = true;
    }
  }
  void resetHole()
  {
    Point2 zero(0, 0);
    setHole(zero);
  }
  void getTextureResourceDependencies(Tab<TEXTUREID> &dependencies) const
  {
    if (noises.resCloud1.getTexId() != BAD_TEXTUREID)
    {
      dependencies.push_back(noises.resCloud1.getTexId());
    }
    if (noises.resCloud2.getTexId() != BAD_TEXTUREID)
    {
      dependencies.push_back(noises.resCloud2.getTexId());
    }
  }

protected:
  GenWeather weather;
  GenNoise noises;
  CloudsFormLUT cloudsForm;
  CloudsRenderer clouds;
  CloudsShadows cloudShadows;
  CloudsField field;
  CloudsLightRenderer light;
  float cloudsStartAt = 0.7f, cloudsEndAt = 8.0f;
  float cloudsShadowCoverage = 0;
  DaSkies::CloudsSettingsParams gameParams;
  DaSkies::CloudsWeatherGen weatherParams;
  DaSkies::CloudsForm form;
  DaSkies::CloudsRendering rendering;


  // causes clouds to change lighting
  // struct CloudsParamsLighting { Point2 windGradientDir={0,0}; } cloudsLight;
  Point2 clouds_erosion_noise_wind_ofs = {0, 0};
  Point2 windDir = {0, 0}, erosionWindChange = {0, 0};
  Point3 holeTarget = {0, 0, 0};
  float holeDensity = 0;
  bool holeFound = true;
  UniqueBufHolder holeBuf;
  UniqueTexHolder holeTex;
  UniqueTexHolder holePosTex;
  UniqueTex holePosTexStaging;
  eastl::unique_ptr<ComputeShaderElement> clouds_hole_cs;
  eastl::unique_ptr<ComputeShaderElement> clouds_hole_pos_cs;
  PostFxRenderer clouds_hole_ps;
  PostFxRenderer clouds_hole_pos_ps;
  shaders::UniqueOverrideStateId blendMaxState;
  bool renderingEnabled = true;
  RingCPUTextureLock ringTextures;
  int needHoleCPUUpdate = HOLE_UPDATED;
};
