// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsGenNoise.h"
#include "clouds2Common.h"
#include "cloudsShaderVars.h"
#include "shaders/clouds2/cloud_settings.hlsli"

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>

#include <util/dag_convar.h>
#include <osApiWrappers/dag_miscApi.h>

CONSOLE_INT_VAL("clouds", shape_res_compat_shift, 3, 0, 3);
CONSOLE_INT_VAL("clouds", detail_res_compat_shift, 2, 0, 2);
CONSOLE_INT_VAL("clouds", shape_res, 1, -2, 2); // 1<<(6+x), 1<<(4+x)
CONSOLE_INT_VAL("clouds", detail_res, 2, 0, 3); // 1<<(4+x)
CONSOLE_BOOL_VAL("clouds", compressedTextures, true);
static bool regen_noises = false;
static bool reload_noises = false;

void GenNoise::close()
{
  state = EMPTY;
  curlRendered = false;
  cloud1.close();
  cloud2.close();
  resCloud1.close();
  resCloud2.close();

  cloudsCurl2d.close();
  closeCompressor();
}

bool GenNoise::loadVolmap(const char *resname, int resolution, SharedTexHolder &tex)
{
  tex.close();

  if (compressSupported)
    tex = SharedTexHolder(dag::get_tex_gameres(String(128, "%s_%d", resname, resolution)), resname);

  if (!tex) // There may not be a compressed variant, in which case load the uncompressed variant, even if compression is supported.
  {
    tex.close();
    tex = SharedTexHolder(dag::get_tex_gameres(String(128, "%s_%d_raw", resname, resolution)), resname);
  }

  if (!tex)
    return false;

  return true;
}

void GenNoise::init()
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
      // prefer to use pregenerated noise for low power GPUs, to avoid GenNoise::extra load time & crashes
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
  ShaderGlobal::set_sampler(::get_shader_variable_id("gen_cloud_detail_samplerstate"), d3d::request_sampler({}));
  ShaderGlobal::set_sampler(::get_shader_variable_id("gen_cloud_shape_samplerstate"), d3d::request_sampler({}));

  if (!curlRendered)
  {
    if (VoltexRenderer::is_compute_supported())
      genCurl2d.reset(new_compute_shader("gen_curl_clouds_2d", true));
    else
      genCurl2dPs.init("gen_curl_clouds_2d_ps");
    cloudsCurl2d.close();
    cloudsCurl2d = dag::create_tex(0, CLOUD_CURL_RES, CLOUD_CURL_RES, TEXFMT_R8G8S | (genCurl2d ? TEXCF_UNORDERED : TEXCF_RTARGET), 1,
      "clouds_curl_2d");
    cloudsCurl2d->disableSampler();
    ShaderGlobal::set_sampler(::get_shader_variable_id("clouds_curl_2d_samplerstate"), d3d::request_sampler({}));
  }
}

void GenNoise::reset()
{
  if ((state == LOADED) || (state == LOADING))
    reload_noises = true;
  else
    regen_noises = true;

  curlRendered = false;
}

/*static*/ int GenNoise::get_blocked_mips(const ManagedTex &tex)
{
  int mip_count = tex->level_count();
  TextureInfo ti;
  tex->getinfo(ti);
  for (int minD = min(min(ti.w, ti.h), ti.d); mip_count > 1 && (minD >> mip_count) < 4;)
    --mip_count;
  return mip_count;
}

void GenNoise::closeCompressor()
{
  cloud1Compressed.close();
  cloud2Compressed.close();
  buffer.close();
}

void GenNoise::initCompressor()
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

void GenNoise::initCompressorBuffer()
{
  if (!compressSupported)
    return;

  compress3D.initVoltex(buffer, shapeRes / 4, shapeRes / 4, max(shapeRes / 4, detailRes), TEXFMT_R32G32UI, 1,
    "gen_cloud_shape_buffer");
}

void GenNoise::compressBC4(const ManagedTex &tex, const ManagedTex &dest)
{
  if (!compressSupported)
    return;

  G_ASSERT(dest->level_count() <= tex->level_count());
  TextureInfo ti;
  tex->getinfo(ti);
  ShaderGlobal::set_texture(compress_voltex_bc4_sourceVarId, tex);
  ShaderGlobal::set_sampler(compress_voltex_bc4_source_samplerstateVarId, d3d::request_sampler({}));
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

bool GenNoise::renderCurl()
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

void GenNoise::generateMips3d(const ManagedTex &tex)
{
  TIME_D3D_PROFILE(genMips);
  TextureInfo ti;
  tex->getinfo(ti);
  ShaderGlobal::set_int(clouds_gen_mips_3d_one_layerVarId, 0);
  ShaderGlobal::set_texture(clouds_gen_mips_3d_sourceVarId, tex.getTexId());
  ShaderGlobal::set_sampler(clouds_gen_mips_3d_source_samplerstateVarId, d3d::request_sampler({}));
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
bool GenNoise::isPrepareRequired() const
{
  if (state != LOADING)
    return false;

  TEXTUREID waitTextures[] = {resCloud1.getTexId(), resCloud2.getTexId()};
  return prefetch_and_check_managed_textures_loaded(dag::ConstSpan<TEXTUREID>(waitTextures, 2), true);
}

/* Returns true if at least one of noise textures was updated */
bool GenNoise::render()
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

  // we should split workload to avoid GenNoise::TDR on weak/inherently CS limited GPUs
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

void GenNoise::setVars()
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
      default: DAG_FATAL("unexpected state=%d", int(state));
    }
  }

  cloudsCurl2d.setVar();
}
