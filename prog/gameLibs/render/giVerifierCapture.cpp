// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/giVerifierCapture.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_lock.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_lockTexture.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_direct.h>
#include <math/dag_TMatrix.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

namespace gi_verify
{

// the write can throw where exceptions are enabled, and a texture left CPU
// locked while its handle is released is driver-defined behaviour
struct AutoTexUnlock
{
  BaseTexture *tex;
  ~AutoTexUnlock() { tex->unlockimg(); }
};

static bool dump_tex2d(BaseTexture *t, int w, int h, int bpp, const char *fn)
{
  int stride = 0;
  uint8_t *ptr = nullptr;
  if (!t->lockimg((void **)&ptr, stride, 0, TEXLOCK_READ) || !ptr)
  {
    logerr("gi_verify capture: GPU lock failed for '%s'", fn);
    return false;
  }
  AutoTexUnlock unlock{t};
  FullFileSaveCB cb(fn);
  if (!cb.fileHandle)
  {
    logerr("gi_verify capture: cannot open '%s' for write", fn);
    return false;
  }
  for (int y = 0; y < h; ++y)
    cb.write(ptr + y * stride, w * bpp);
  return true;
}

// The daGI2 albedo voxel scene is the same resource in every game: the sparse
// block atlas + indirection buffer + the shader vars that address them.
// Capture them verbatim so giVerifier can rebind and sample identically.
// three outcomes, not two: a game with no daGI2 has nothing to capture, which
// is not the same as a capture that was attempted and failed
enum class AlbedoScene
{
  Absent,
  Saved,
  Failed
};

static AlbedoScene save_albedo_scene(const char *dir, DataBlock &blk)
{
  DataBlock *ab = blk.addBlock("albedoScene");
  ab->setBool("valid", false);
  TEXTUREID atlasId = ShaderGlobal::get_tex(get_shader_variable_id("dagi_albedo_atlas", true));
  D3DRESID indId = ShaderGlobal::get_buf(get_shader_variable_id("dagi_albedo_indirection__free_indices_list", true));
  BaseTexture *atlas = acquire_managed_tex(atlasId);
  Sbuffer *ind = acquire_managed_buf(indId);
  if (!atlas || !ind)
  {
    // no daGI2 in this game (or not built yet): the blk records valid:b=no and
    // the replay falls back to its own material for disoccluded pixels
    debug("gi_verify capture: no daGI2 albedo scene bound, capturing without it");
    release_managed_tex(atlasId);
    release_managed_buf(indId);
    return AlbedoScene::Absent;
  }
  TextureInfo ti;
  atlas->getinfo(ti, 0);
  const int numElements = ind->getNumElements();

  // read both resources into memory, then release the managed refs before
  // touching the filesystem: a write failure (SaveException in dev builds)
  // must not skip the release and leak a reference to the daGI2 resources,
  // which would block their free on level unload
  Tab<uint8_t> atlasData;
  {
    int rowStride = 0, sliceStride = 0;
    uint8_t *ptr = nullptr;
    if (atlas->lockbox((void **)&ptr, rowStride, sliceStride, 0, TEXLOCK_READ) && ptr)
    {
      atlasData.resize((size_t)ti.d * ti.h * ti.w * 4);
      uint8_t *dst = atlasData.data();
      for (int z = 0; z < ti.d; ++z)
        for (int y = 0; y < ti.h; ++y, dst += ti.w * 4)
          memcpy(dst, ptr + z * sliceStride + y * rowStride, ti.w * 4);
      atlas->unlockbox();
    }
    else
      logerr("gi_verify capture: albedo atlas lockbox failed");
  }
  // the indirection buffer is a plain UAV: stage it through a readback copy
  Tab<uint32_t> indData;
  {
    UniqueBuf staging = dag::buffers::create_ua_structured_readback(sizeof(uint32_t), numElements, "gi_verify_cap_ind");
    // a failed copy still leaves a lockable staging buffer, so without this the
    // undefined contents would be written out as a valid indirection table
    const bool copied = staging && ind->copyTo(staging.getBuf());
    uint32_t *data = nullptr;
    if (copied && staging.getBuf()->lock(0, numElements * sizeof(uint32_t), (void **)&data, VBLOCK_READONLY) && data)
    {
      indData.assign(data, data + numElements);
      staging.getBuf()->unlock();
    }
    else
      logerr("gi_verify capture: albedo indirection readback failed");
  }
  release_managed_tex(atlasId);
  release_managed_buf(indId);

  bool ok = !atlasData.empty() && !indData.empty();
  if (!atlasData.empty())
  {
    FullFileSaveCB cb(String(0, "%s/albedo_atlas.bin", dir));
    ok &= cb.fileHandle != nullptr;
    if (cb.fileHandle)
      cb.write(atlasData.data(), data_size(atlasData));
  }
  if (!indData.empty())
  {
    FullFileSaveCB cb(String(0, "%s/albedo_indirection.bin", dir));
    ok &= cb.fileHandle != nullptr;
    if (cb.fileHandle)
      cb.write(indData.data(), data_size(indData));
  }

  ab->setInt("atlasW", ti.w);
  ab->setInt("atlasH", ti.h);
  ab->setInt("atlasD", ti.d);
  ab->setInt("atlasFmt", ti.cflg & TEXFMT_MASK);
  ab->setInt("indirectionDwords", numElements);
  ab->setIPoint4("clipmap_sizei", ShaderGlobal::get_int4(get_shader_variable_id("dagi_albedo_clipmap_sizei", true)));
  ab->setIPoint4("clipmap_sizei_np2", ShaderGlobal::get_int4(get_shader_variable_id("dagi_albedo_clipmap_sizei_np2", true)));
  ab->setIPoint4("atlas_sizei", ShaderGlobal::get_int4(get_shader_variable_id("dagi_albedo_atlas_sizei", true)));
  for (int i = 0; i < 4; ++i)
    ab->setIPoint4(String(0, "lt_coord_%d", i),
      ShaderGlobal::get_int4(get_shader_variable_id(String(0, "dagi_albedo_clipmap_lt_coord_%d", i), true)));
  Color4 inv = ShaderGlobal::get_float4(get_shader_variable_id("dagi_albedo_inv_atlas_size_blocks_texels", true));
  ab->setPoint4("inv_atlas_size_blocks_texels", Point4(inv.r, inv.g, inv.b, inv.a));
  Color4 ibs = ShaderGlobal::get_float4(get_shader_variable_id("dagi_albedo_internal_block_size_tc_border", true));
  ab->setPoint4("internal_block_size_tc_border", Point4(ibs.r, ibs.g, ibs.b, ibs.a));
  ab->setBool("valid", ok);
  return ok ? AlbedoScene::Saved : AlbedoScene::Failed;
}

bool save_capture(const char *dir, const TMatrix &view_itm, const Driver3dPerspective &persp, int width, int height,
  const Point3 &dir_to_sun, const char *source_dump, const char *level_bin, EnvWriter env_writer)
{
  if (width <= 0 || height <= 0)
  {
    logerr("gi_verify capture: bad resolution %dx%d", width, height);
    return false;
  }
  if (!dd_mkdir(dir))
    logerr("gi_verify capture: could not create '%s'", dir); // the writes below will say what failed
  DataBlock blk;
  blk.setInt("version", 1);

  DataBlock *cam = blk.addBlock("camera");
  cam->setTm("itm", view_itm);
  cam->setReal("wk", persp.wk);
  cam->setReal("hk", persp.hk);
  cam->setReal("zn", persp.zn);
  cam->setReal("zf", persp.zf);
  cam->setReal("fovDeg", 2.f * RadToDeg(atanf(1.f / persp.hk)));
  cam->setInt("width", width);
  cam->setInt("height", height);

  // sun: caller-supplied to-sun direction + sun_light_color as every game
  // shades with it (dagor convention: E/PI; the replay multiplies by PI)
  DataBlock *sun = blk.addBlock("sun");
  Color4 sunCol = ShaderGlobal::get_float4(get_shader_variable_id("sun_light_color", true));
  const Point3 sunColor(sunCol.r, sunCol.g, sunCol.b);
  const bool haveSunDir = dir_to_sun.lengthSq() > 1e-12f;
  // the replay turns the sun on from its COLOUR, so a live colour with a
  // degenerate direction lights nothing: the replay would then differ from the
  // captured frame by exactly the sun, and nothing would say so
  if (sunColor.lengthSq() > 0 && !haveSunDir)
  {
    logerr("gi_verify capture: sun_light_color is set but dir_to_sun is zero - the replay would lose the sun");
    return false;
  }
  sun->setPoint3("dirToSun", haveSunDir ? normalize(dir_to_sun) : Point3(0, 1, 0)); // replay uses it as a unit direction
  sun->setPoint3("colorDivPi", sunColor);

  // canonical gbuffer planes through the decode pass: 8-bit planes for 8-bit
  // sources (albedo kept in sRGB), fp16 normals, fp32 linear depth
  UniqueTex capA = dag::create_tex(NULL, width, height, TEXFMT_A8R8G8B8 | TEXCF_SRGBWRITE | TEXCF_SRGBREAD | TEXCF_RTARGET, 1,
    "gi_verify_cap_albedo");
  UniqueTex capN = dag::create_tex(NULL, width, height, TEXFMT_A16B16G16R16F | TEXCF_RTARGET, 1, "gi_verify_cap_normal");
  UniqueTex capM = dag::create_tex(NULL, width, height, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1, "gi_verify_cap_material");
  UniqueTex capD = dag::create_tex(NULL, width, height, TEXFMT_R32F | TEXCF_RTARGET, 1, "gi_verify_cap_depth");
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(capA.getTex2D(), 0);
    d3d::set_render_target(1, capN.getTex2D(), 0);
    d3d::set_render_target(2, capM.getTex2D(), 0);
    d3d::set_render_target(3, capD.getTex2D(), 0);
    PostFxRenderer capPass;
    capPass.init("gi_verify_capture");
    if (!capPass.getElem())
    {
      // render() would be a silent no-op and the planes below would dump
      // whatever the fresh targets contain - garbage that reports success
      logerr("gi_verify capture: shader 'gi_verify_capture' is missing - add "
             "gameLibs/render/shaders/gi_verify_capture.dshl to this game's shader list");
      return false;
    }
    capPass.render();
  }
  // &= rather than &&: every plane is attempted even if an earlier one failed,
  // so the log names each broken plane instead of only the first
  bool gbufOk = true;
  gbufOk &= dump_tex2d(capA.getTex2D(), width, height, 4, String(0, "%s/gbuf_albedo_trans.srgba8.bin", dir));
  gbufOk &= dump_tex2d(capN.getTex2D(), width, height, 8, String(0, "%s/gbuf_normal.rgba16f.bin", dir));
  gbufOk &= dump_tex2d(capM.getTex2D(), width, height, 4, String(0, "%s/gbuf_material.rgba8.bin", dir));
  gbufOk &= dump_tex2d(capD.getTex2D(), width, height, 4, String(0, "%s/gbuf_depth.r32f.bin", dir));
  blk.addBlock("gbuf")->setBool("valid", gbufOk);
  if (!gbufOk)
    logerr("gi_verify capture: gbuffer plane readback failed");

  // environment as a lat-long plane, encoded by the caller (see EnvWriter)
  const bool envRequested = env_writer != nullptr;
  bool envWritten = false;
  {
    const int ew = 512, eh = 256;
    if (env_writer)
    {
      UniqueTex capE = dag::create_tex(NULL, ew, eh, TEXFMT_A16B16G16R16F | TEXCF_RTARGET, 1, "gi_verify_cap_env");
      bool envShaderOk = false;
      {
        SCOPE_RENDER_TARGET;
        d3d::set_render_target(capE.getTex2D(), 0);
        PostFxRenderer envPass;
        envPass.init("gi_verify_env_latlong");
        envShaderOk = envPass.getElem() != nullptr;
        if (envShaderOk)
          envPass.render();
        else // render() would no-op and the readback below would dump the fresh target
          logerr("gi_verify capture: shader 'gi_verify_env_latlong' is missing - add "
                 "gameLibs/render/shaders/gi_verify_capture.dshl to this game's shader list");
      }
      int stride = 0;
      uint8_t *ptr = nullptr;
      if (envShaderOk && capE.getTex2D()->lockimg((void **)&ptr, stride, 0, TEXLOCK_READ) && ptr)
      {
        // the lock stride is driver-chosen, so pack before handing it over
        Tab<uint16_t> rgba;
        rgba.resize(ew * eh * 4);
        for (int y = 0; y < eh; ++y)
          memcpy(&rgba[y * ew * 4], ptr + y * stride, ew * 4 * sizeof(uint16_t));
        capE.getTex2D()->unlockimg();
        envWritten = env_writer(String(0, "%s/env.exr", dir), rgba.data(), ew, eh);
        if (!envWritten)
          logerr("gi_verify capture: env write failed");
      }
      else
        logerr("gi_verify capture: env readback failed");
    }
    // valid means env.exr is on disk, so a skipped environment records no
    blk.addBlock("env")->setBool("valid", envWritten);
  }

  // scene sources by reference: the collision dump and level bin are large and
  // immutable, same-machine replays read the originals (copy them next to
  // capture.blk manually when archiving a capture)
  DataBlock *bvh = blk.addBlock("bvh");
  bvh->setStr("sourceDump", source_dump ? source_dump : "");
  bvh->setStr("level", level_bin ? level_bin : "");
  // the replay traces the collision dump: without it there is no geometry to
  // project the captured gbuffer onto, so the folder is unusable even though
  // every plane in it is fine. Say so here rather than at replay time.
  const bool haveDump = source_dump && source_dump[0];
  if (!haveDump)
    logerr("gi_verify capture: no collision dump path (check the ri_collisions setting) - "
           "the capture cannot be replayed");

  const AlbedoScene albedo = save_albedo_scene(dir, blk);

  // every component that was attempted has to have succeeded: callers read a
  // bool as "the capture is complete", and the blk keeps the per-file detail
  const bool blkOk = blk.saveToTextFile(String(0, "%s/capture.blk", dir));
  return blkOk && haveDump && gbufOk && (!envRequested || envWritten) && albedo != AlbedoScene::Failed;
}

} // namespace gi_verify
