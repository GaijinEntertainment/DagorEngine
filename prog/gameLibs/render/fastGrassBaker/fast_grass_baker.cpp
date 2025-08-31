// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <ioSys/dag_dataBlock.h>
#include <image/dag_texPixel.h>
#include <image/dag_tiff.h>
#include <ispc_texcomp.h>
#include <3d/ddsxTex.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <ecs/gameres/commonLoadingJobMgr.h>
#include <folders/folders.h>
#include <math/integer/dag_IPoint2.h>
#include <math/random/dag_random.h>
#include <render/fast_grass.h>
#include <render/deferredRT.h>
#include <shaders/dag_IRenderWrapperControl.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <shaders/dag_postFxRenderer.h>
#include <rendInst/rendInstExtraRender.h>
#include <gameRes/dag_stdGameResId.h>
#include <fx/dag_leavesWind.h>
#include "../shaders/pack_impostor_texture.hlsl"
#include "render/genRender.h"

#if DAGOR_DBGLEVEL <= 0
#error This module is for developer version only.
#endif

namespace fast_grass_baker
{

enum PreviewMode
{
  ALBEDO,
  DEPTH,
  NORMAL,
  TRANSLUCENCY,
  ROUGHNESS
};

#define VAR(a) static ShaderVariableInfo a##VarId(#a);
VAR(texture_size)
VAR(impostor_depth_scale)
VAR(imgui_channel_mask)
VAR(imgui_draw_function)
VAR(impostor_color_padding)
VAR(impostor_packed_albedo_alpha)
VAR(impostor_packed_normal_translucency)
#undef VAR

static constexpr int MIN_RESOLUTION = 8;

static IPoint2 resolution(512, 128);
static char data_folder[DAGOR_MAX_PATH] = "";
static char texture_folder[DAGOR_MAX_PATH] = "";

static SharedTex preview_ad_texid, preview_nt_texid;
static int preview_mode = DEPTH;


static void update_impostor_image(const String &tex_name, dag::ConstSpan<rgba_surface> mipmaps);


struct FastGrassImpostorData
{
  DataBlock blk;
  String name;
  eastl::unique_ptr<struct BakedImpostor> baked;
  bool changed = false;

  FastGrassImpostorData(const char *n, DataBlock &&b) : blk(b)
  {
    const char *p = strchr(n, '.');
    name.setStr(n, p ? p - n : strlen(n));
  }

  FastGrassImpostorData(const FastGrassImpostorData &from) : blk(from.blk), name(from.name) {}
  FastGrassImpostorData &operator=(const FastGrassImpostorData &from)
  {
    blk = from.blk;
    name = from.name;
    return *this;
  }

  String makeTextureName(const char *type) { return String(0, "%s_grass_%s*", name.c_str(), type); }
  String makeTiffFilename(const char *type) { return String(0, "%s%s_grass_%s.tiff", texture_folder, name.c_str(), type); }
  String makeBlkFilename() { return String(0, "%s%s.fg_impostor.blk", data_folder, name.c_str()); }
};

static dag::Vector<FastGrassImpostorData> impostor_data;
static uint32_t selected_impostor = 0;
static bool selection_changed = true;

static String popup_message;
static bool popup_message_is_error = false, popup_message_was_set = false;

static dag::Vector<SimpleString> asset_names;

static void clear_popup_message()
{
  popup_message.clear();
  popup_message_is_error = false;
  popup_message_was_set = false;
}

static void append_popup_message(const String &s)
{
  popup_message.append(s);
  popup_message.append("\n");
  popup_message_was_set = true;
}

static void append_popup_error(const String &s)
{
  append_popup_message(s);
  popup_message_is_error = true;
}


struct BakingData
{
  FastGrassImpostorData impostor;
  struct AssetData
  {
    const DataBlock &blk;
    Ptr<RenderableInstanceLodsResource> model;
    float weight;
    Tab<TMatrix> tms;
  };
  Tab<AssetData> assets;
  TextureIdSet usedTexIds;
  int ready = 0;

  struct LoadingJob : public cpujobs::IJob
  {
    BakingData &baking;
    LoadingJob(BakingData &b) : baking(b) {}

    const char *getJobName(bool & /*copystr*/) const override { return "BakingLoadingJob"; }
    void releaseJob() override { delete this; }
    void doJob() override
    {
      for (auto &asset : baking.assets)
      {
        auto res = get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(asset.blk.getStr("name")), RendInstGameResClassId);
        if (!res)
          logerr("failed to load '%s' for baking of grass impostor '%s'", asset.blk.getStr("name"), baking.impostor.name.c_str());
        asset.model = reinterpret_cast<RenderableInstanceLodsResource *>(res);
        if (asset.model)
          asset.model->gatherUsedTex(baking.usedTexIds);
        release_game_resource(res);
      }

      interlocked_or(baking.ready, 1);
    }
  };

  BakingData(const FastGrassImpostorData &imp) : impostor(imp) {}

  void loadResources()
  {
    assets.clear();
    if (int assetNameId = impostor.blk.getNameId("asset"); assetNameId != -1)
    {
      for (int bi = -1;;)
      {
        bi = impostor.blk.findBlock(assetNameId, bi);
        if (bi == -1)
          break;
        if (const auto *b = impostor.blk.getBlock(bi))
          if (const char *name = b->getStr("name", nullptr); name && *name)
            assets.emplace_back(AssetData{*b, nullptr, b->getReal("weight", 1)});
      }
    }

    cpujobs::add_job(ecs::get_common_loading_job_mgr(), new LoadingJob(*this));
  }
};

static eastl::unique_ptr<BakingData> impostor_to_bake;


struct BakedImpostor
{
  struct BakedImage
  {
    eastl::unique_ptr<TexImage32> image;
    dag::Vector<rgba_surface> mipmaps;
  };

  BakedImage baked_ad, baked_nt;

  TexImage32 *getImage(const char *type)
  {
    if (strcmp(type, "ad") == 0)
      return baked_ad.image.get();
    else if (strcmp(type, "nt") == 0)
      return baked_nt.image.get();
    else
      return nullptr;
  }
};

static bool readback_image(BakedImpostor::BakedImage &baked_image, Texture *tex)
{
  TextureInfo info;
  tex->getinfo(info);

  const int w = info.w;
  const int h = info.h;
  const int mips = info.mipLevels;
  const int imageHeight = h;
  const int imageWidth = mips > 1 ? (w * 2 - 1) : w;

  baked_image.image.reset(TexImage32::create(imageWidth, imageHeight));
  memset(baked_image.image->getPixels(), 0, sizeof(uint32_t) * imageWidth * imageHeight);

  baked_image.mipmaps.reserve(mips);

  int xoffset = 0;
  for (int i = 0; i < mips; ++i)
  {
    int mipW = w >> i;
    int mipH = h >> i;

    G_ASSERT((xoffset + mipW) <= imageWidth);

    uint8_t *data;
    int texStride;
    if (tex->lockimg(reinterpret_cast<void **>(&data), texStride, i, TEXLOCK_READ) && data)
    {
      for (int y = 0; y < mipH; ++y)
        memcpy(baked_image.image->getPixels() + y * imageWidth + xoffset, data + y * texStride, mipW * sizeof(TexPixel32));

      if (!tex->unlockimg())
      {
        append_popup_error(String(0, "Cannot unlock tex=%p: err='%s'\n", tex, d3d::get_last_error()));
        return false;
      }
    }
    else
    {
      append_popup_error(String(0, "Cannot lock tex=%p: err='%s'\n", tex, d3d::get_last_error()));
      return false;
    }

    baked_image.mipmaps.emplace_back(rgba_surface{
      reinterpret_cast<uint8_t *>(baked_image.image->getPixels() + xoffset), mipW, mipH, int(imageWidth * sizeof(TexPixel32))});
    xoffset += mipW;
  }

  return true;
}

static void perform_padding(const PostFxRenderer &padder, UniqueTex &src_ad, UniqueTex &src_nt, UniqueTex &dst_ad, UniqueTex &dst_nt,
  uint32_t mip)
{
  SCOPE_RENDER_TARGET;
  TIME_D3D_PROFILE(impostor_padding);
  d3d::set_render_target({nullptr, 0}, DepthAccess::SampledRO, {{dst_ad.getTex2D(), mip}, {dst_nt.getTex2D(), mip}});
  TextureInfo info;
  dst_ad.getTex2D()->getinfo(info, mip);
  d3d::setview(0, 0, info.w, info.h, 0, 1);
  ShaderGlobal::set_color4(texture_sizeVarId, Color4(info.w, info.h, 0, 0));
  src_ad.getTex2D()->texmiplevel(mip, mip);
  src_nt.getTex2D()->texmiplevel(mip, mip);
  ShaderGlobal::set_texture(impostor_packed_albedo_alphaVarId, src_ad.getTexId());
  ShaderGlobal::set_texture(impostor_packed_normal_translucencyVarId, src_nt.getTexId());
  padder.render();
}

void fast_grass_baker_on_render()
{
  if (!impostor_to_bake || interlocked_acquire_load(impostor_to_bake->ready) != 1)
    return;
  if (!prefetch_and_check_managed_textures_loaded(impostor_to_bake->usedTexIds))
    return;

  clear_popup_message();

  auto impostor = eastl::find_if(impostor_data.begin(), impostor_data.end(),
    [](const auto &d) { return d.name == impostor_to_bake->impostor.name; });
  if (impostor == impostor_data.end())
  {
    append_popup_error(String(0, "can't find fast grass impostor '%s' in the list\n", impostor_to_bake->impostor.name.c_str()));
    return;
  }

  const auto &blk = impostor_to_bake->impostor.blk;

  double sum = 0;
  for (auto &a : impostor_to_bake->assets)
  {
    if (a.model)
      sum += a.weight;
    a.weight = sum;
  }
  if (sum != 0)
    for (auto &a : impostor_to_bake->assets)
      a.weight /= sum;

  int mips = get_log2i(min(resolution.x, resolution.y) / MIN_RESOLUTION) + 1;
  UniqueTex albedo_alpha = dag::create_tex(nullptr, resolution.x, resolution.y,
    TEXFMT_R8G8B8A8 | TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE, mips, "fast_grass_baker_rt_albedo_alpha");
  UniqueTex padded_albedo_alpha = dag::create_tex(nullptr, resolution.x, resolution.y,
    TEXFMT_R8G8B8A8 | TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE, mips, "fast_grass_baker_padded_albedo_alpha");
  UniqueTex normal_translucency = dag::create_tex(nullptr, resolution.x, resolution.y,
    TEXFMT_R8G8B8A8 | TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE, mips, "fast_grass_baker_rt_normal_translucency");
  UniqueTex padded_normal_translucency = dag::create_tex(nullptr, resolution.x, resolution.y,
    TEXFMT_R8G8B8A8 | TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE, mips, "fast_grass_baker_padded_normal_translucency");

  const static unsigned formats[] = {TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, TEXFMT_A8R8G8B8, TEXFMT_A8R8G8B8};
  DeferredRT defrt("fast_grass_baker", resolution.x * RENDER_OVERSCALE, resolution.y * RENDER_OVERSCALE,
    DeferredRT::StereoMode::MonoOrMultipass, 0, 3, formats, TEXFMT_DEPTH16);

  eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>> rendinstMatrixBuffer(
    d3d::create_sbuffer(sizeof(Point4), 4u, SBCF_BIND_SHADER_RES, TEXFMT_A32B32G32R32F, "fast_grass_baker_matrix_buffer"));

  PostFxRenderer packTexturesShader;
  PostFxRenderer genMipShader;
  PostFxRenderer impostorPostprocessor;
  PostFxRenderer impostorColorPadding;
  packTexturesShader.init("pack_grass_impostor_texture");
  genMipShader.init("grass_impostor_gen_mip");
  impostorPostprocessor.init("grass_impostor_postprocessor");
  impostorColorPadding.init("grass_impostor_color_padding");

  const float Z_NEAR = -10, Z_FAR = 10;

  {
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;

    defrt.setRt();
    d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL | CLEAR_TARGET, 0, 0, 0);

    float scaleX = float(resolution.x) / resolution.y;
    TMatrix4 projTm = matrix_ortho_lh_reverse(scaleX, 1, Z_NEAR, Z_FAR);
    d3d::settm(TM_PROJ, &projTm);

    TMatrix viewItm = TMatrix::IDENT;
    viewItm.setcol(3, Point3(scaleX * 0.5f, 0.5f, 0));
    TMatrix viewTm = orthonormalized_inverse(viewItm);
    d3d::settm(TM_VIEW, viewTm);

    Point3_vec4 col[3];
    for (int i = 0; i < 3; ++i)
      col[i] = viewItm.getcol(i);
    LeavesWindEffect::setNoAnimShaderVars(col[0], col[1], col[2]);

    ShaderGlobal::set_int(rendinst::render::rendinstRenderPassVarId, eastl::to_underlying(rendinst::RenderPass::ImpostorColor));

    const int lastFrameBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(rendinst::render::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

    rendinst::render::startRenderInstancing();
    rendinst::render::setCoordType(rendinst::render::COORD_TYPE_TM);
    d3d::setind(unitedvdata::riUnitedVdata.getIB());

    d3d::setview(0, 0, resolution.x * RENDER_OVERSCALE, resolution.y * RENDER_OVERSCALE, 0, 1);

    int seed = DefaultOAHasher<false>().hash_data(impostor_to_bake->impostor.name.c_str(), impostor_to_bake->impostor.name.length());
    seed = blk.getInt("seed", seed);
    int numInst = max(blk.getInt("instances", resolution.x * 2 / resolution.y), 1);
    int numRows = max(blk.getInt("rows", 1), 1);
    float rowStep = blk.getReal("row_step", 0.5f);
    float jitter = blk.getReal("jitter", 0.5f);
    float globalScaleVar = blk.getReal("scale_variance", 0.1f);
    float depthOffset = blk.getReal("depth_offset", 0);

    for (int row = 0; row < numRows; row++)
      for (int col = 0; col < numInst; col++)
      {
        float r = _frnd(seed);
        int i;
        for (i = 0; i < impostor_to_bake->assets.size(); i++)
          if (impostor_to_bake->assets[i].model && r <= impostor_to_bake->assets[i].weight)
            break;
        if (i >= impostor_to_bake->assets.size())
          continue;

        auto &asset = impostor_to_bake->assets[i];
        float scale = asset.blk.getReal("scale", 0);
        float scaleVar = asset.blk.getReal("scale_variance", 0.1f);
        float tilt = DegToRad(asset.blk.getReal("tilt", 15));
        float yOffset = asset.blk.getReal("y_offset", 0);

        if (scale <= 0)
        {
          scale = (1.0f - yOffset) / asset.model->bbox[1].y;
          const char *name = asset.blk.getStr("name");
          DataBlock *orgBlk = nullptr;
          dblk::iterate_blocks_by_name(impostor->blk, "asset", [name, &orgBlk](const DataBlock &b) {
            if (strcmp(b.getStr("name", ""), name) == 0)
              orgBlk = const_cast<DataBlock *>(&b);
          });
          if (orgBlk)
          {
            orgBlk->setReal("scale", scale);
            impostor->changed = true;
          }
        }

        TMatrix tm;
        float ha = _frnd(seed) * TWOPI;
        tm = (makeTM(Point3(cosf(ha), 0, sinf(ha)), _frnd(seed) * tilt) * rotyTM(_frnd(seed) * TWOPI)) *
             ((1 - _frnd(seed) * scaleVar) * (1 - _frnd(seed) * globalScaleVar) * scale);
        tm.col[3] =
          Point3(scaleX * (col + _srnd(seed) * jitter) / numInst, yOffset, (row + _srnd(seed) * jitter) * rowStep + depthOffset);
        asset.tms.push_back(tm);
        tm.col[3][0] -= scaleX;
        asset.tms.push_back(tm);
        tm.col[3][0] += scaleX * 2;
        asset.tms.push_back(tm);
      }

    for (const auto &asset : impostor_to_bake->assets)
    {
      auto lods = asset.model.get();
      if (!lods)
        continue;

      RenderableInstanceResource *riRes = lods->lods[lods->getQlBestLod()].scene;
      ShaderMesh *mesh = riRes->getMesh()->getMesh()->getMesh();
      if (!mesh)
        continue;

      for (const auto &tm : asset.tms)
      {
        SCENE_LAYER_GUARD(rendinst::render::rendinstSceneBlockId);

        Point4 data[4];
        data[0] = Point4(tm.getcol(0).x, tm.getcol(1).x, tm.getcol(2).x, tm.getcol(3).x),
        data[1] = Point4(tm.getcol(0).y, tm.getcol(1).y, tm.getcol(2).y, tm.getcol(3).y),
        data[2] = Point4(tm.getcol(0).z, tm.getcol(1).z, tm.getcol(2).z, tm.getcol(3).z);
        data[3] = Point4(0, 0, 0, 0);
        rendinstMatrixBuffer.get()->updateData(0, sizeof(Point4) * 4u, &data, 0);
        d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, rendinstMatrixBuffer.get());

        rendinst::render::RiShaderConstBuffers cb;
        cb.setBBoxZero();
        cb.setOpacity(0, 1);
        cb.setBoundingSphere(0, 0, 1, 1, 0);
        cb.setInstancing(0, 3, 0, 0);
        cb.flushPerDraw();
        d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);

        mesh->render();
        mesh->render_trans();
      }
    }

    rendinst::render::endRenderInstancing();
    d3d::set_depth(nullptr, DepthAccess::RW);
    shaders::overrides::reset();
    d3d::setind(nullptr);
    d3d::setvdecl(BAD_VDECL);
    d3d::setvsrc(0, nullptr, 0);
    d3d::set_render_target(0, nullptr, 0);
    d3d::set_depth(nullptr, DepthAccess::SampledRO);
    ShaderElement::invalidate_cached_state_block();

    ShaderGlobal::setBlock(lastFrameBlockId, ShaderGlobal::LAYER_FRAME);
  }

  // 8 is sufficient to avoid artifacts caused by block compression, but not too slow to calculate. Otherwise arbitrary
  ShaderGlobal::set_int(impostor_color_paddingVarId, 16);

  {
    TIME_D3D_PROFILE(pack_slice);
    SCOPE_RENDER_TARGET;
    d3d::set_render_target({nullptr, 0}, DepthAccess::SampledRO, {{albedo_alpha.getTex2D(), 0}, {normal_translucency.getTex2D(), 0}});
    d3d::setview(0, 0, resolution.x, resolution.y, 0, 1);
    defrt.setVar();
    ShaderGlobal::set_color4(texture_sizeVarId, Color4(resolution.x, resolution.y, 0, 0));
    // map -0.5..1 to 0..3 after unpacking z near/far, for better depth representation in 8 bit
    ShaderGlobal::set_color4(impostor_depth_scaleVarId, Color4((Z_NEAR - Z_FAR) * 2 * 2, Z_FAR * 2 + 1, 0, 0));
    // The STATE_GUARD_NULLPTR macro fails with these expressions
    Texture *const tex1 = defrt.getRt(0);
    Texture *const tex2 = defrt.getRt(1);
    Texture *const tex3 = defrt.getRt(2);
    STATE_GUARD_NULLPTR(d3d::set_tex(STAGE_PS, rendinst::render::dynamic_impostor_texture_const_no, VALUE), tex1);
    STATE_GUARD_NULLPTR(d3d::set_tex(STAGE_PS, rendinst::render::dynamic_impostor_texture_const_no + 1, VALUE), tex2);
    STATE_GUARD_NULLPTR(d3d::set_tex(STAGE_PS, rendinst::render::dynamic_impostor_texture_const_no + 2, VALUE), tex3);
    packTexturesShader.render();
  }

  perform_padding(impostorColorPadding, albedo_alpha, normal_translucency, padded_albedo_alpha, padded_normal_translucency, 0);

  for (uint32_t mip = 1; mip < padded_albedo_alpha->level_count(); ++mip)
  {
    {
      SCOPE_RENDER_TARGET;
      TIME_D3D_PROFILE(generate_mips);
      d3d::set_render_target({nullptr, 0}, DepthAccess::SampledRO,
        {{albedo_alpha.getTex2D(), mip}, {normal_translucency.getTex2D(), mip}});
      d3d::setview(0, 0, resolution.x, resolution.y, 0, 1);
      ShaderGlobal::set_color4(texture_sizeVarId, Color4(resolution.x, resolution.y, 0, 0));
      padded_albedo_alpha.getTex2D()->texmiplevel(mip - 1, mip - 1);
      padded_normal_translucency.getTex2D()->texmiplevel(mip - 1, mip - 1);
      ShaderGlobal::set_texture(impostor_packed_albedo_alphaVarId, padded_albedo_alpha.getTexId());
      ShaderGlobal::set_texture(impostor_packed_normal_translucencyVarId, padded_normal_translucency.getTexId());
      genMipShader.render();
    }
    perform_padding(impostorColorPadding, albedo_alpha, normal_translucency, padded_albedo_alpha, padded_normal_translucency, mip);
  }

  d3d::driver_command(Drv3dCommand::D3D_FLUSH);

  auto baked = eastl::make_unique<BakedImpostor>();
  if (!readback_image(baked->baked_ad, padded_albedo_alpha.getTex2D()))
    return;
  if (!readback_image(baked->baked_nt, padded_normal_translucency.getTex2D()))
    return;

  update_impostor_image(impostor->makeTextureName("ad"), make_span_const(baked->baked_ad.mipmaps));
  update_impostor_image(impostor->makeTextureName("nt"), make_span_const(baked->baked_nt.mipmaps));

  impostor->baked = eastl::move(baked);
  impostor_to_bake.reset();
}


static void reload_impostor_data()
{
  impostor_data.clear();

  for (const alefind_t &ff : dd_find_iterator(String(0, "%s*.fg_impostor.blk", data_folder).c_str(), DA_FILE))
  {
    char filename[DAGOR_MAX_PATH];
    strcpy(filename, data_folder);
    strcat(filename, ff.name);
    DataBlock blk;
    if (!blk.load(filename))
    {
      append_popup_error(String(0, "error loading %s\n", filename));
      continue;
    }
    impostor_data.emplace(impostor_data.end(), ff.name, eastl::move(blk));
  }

  eastl::sort(impostor_data.begin(), impostor_data.end(), [](const auto &a, const auto &b) { return a.name < b.name; });

  selected_impostor = min<uint32_t>(selected_impostor, impostor_data.size() - 1);
  selection_changed = true;
}


static class DdsxProviderFactory : public TextureFactory
{
  eastl::vector_map<TEXTUREID, int> texToIndex;
  dag::Vector<Tab<char>> ddsxData;

public:
  void addData(TEXTUREID id, Tab<char> &&data)
  {
    auto [it, inserted] = texToIndex.insert(id);
    if (inserted)
    {
      it->second = ddsxData.size();
      ddsxData.push_back();
    }

    ddsxData[it->second].swap(data);

    change_managed_texture(id, nullptr, this);
  }

  BaseTexture *createTexture(TEXTUREID) override { return nullptr; }
  void releaseTexture(BaseTexture *, TEXTUREID) override {}

  void texFactoryActiveChanged(bool active) override
  {
    if (active)
      return;
    clear_and_shrink(texToIndex);
    clear_and_shrink(ddsxData);
  }

  bool getTextureDDSx(TEXTUREID id, Tab<char> &out_ddsx) override
  {
    auto it = texToIndex.find(id);
    if (it == texToIndex.end())
      return false;
    auto &data = ddsxData[it->second];
    out_ddsx.assign(data.begin(), data.end());
    return true;
  }

  void onUnregisterTexture(TEXTUREID id) override
  {
    auto it = texToIndex.find(id);
    if (it == texToIndex.end())
      return;
    clear_and_shrink(ddsxData[it->second]);
    texToIndex.erase(it);
  }
} ddsx_provider;


static void update_impostor_image(const String &tex_name, dag::ConstSpan<rgba_surface> mipmaps)
{
  G_ASSERT(!mipmaps.empty());

  // compress to BC7 DDSx
  bc7_enc_settings bc7Settings;
  GetProfile_alpha_basic(&bc7Settings);

  size_t packedSize = sizeof(ddsx::Header);
  for (const auto &m : mipmaps)
  {
    if ((m.width & 3) != 0 || (m.height & 3) != 0)
    {
      append_popup_error(String(0, "invalid mip size %dx%d, must be multiple of 4\n", m.width, m.height));
      return;
    }
    packedSize += m.width * m.height; // BC7 is 8 bit per pixel
  }

  Tab<char> packed(midmem);
  packed.resize_noinit(packedSize);
  char *ptr = &packed[0];

  auto &header = *reinterpret_cast<ddsx::Header *>(ptr);
  ptr += sizeof(ddsx::Header);

  memset(&header, 0, sizeof(ddsx::Header));
  header.label = _MAKE4C('DDSx');
  header.d3dFormat = _MAKE4C('BC7 ');
  header.flags = header.FLG_GAMMA_EQ_1;
  header.w = mipmaps[0].width;
  header.h = mipmaps[0].height;
  header.levels = mipmaps.size();
  header.depth = 1;
  header.dxtShift = 4;
  header.memSz = packedSize - sizeof(ddsx::Header);

  for (const auto &m : mipmaps)
  {
    CompressBlocksBC7(&m, reinterpret_cast<uint8_t *>(ptr), &bc7Settings);
    ptr += m.width * m.height; // BC7 is 8 bit per pixel
  }

  G_ASSERT(ptr - &packed[0] == packed.size());

  // update texture
  TEXTUREID texId = get_managed_texture_id(tex_name.c_str());
  if (texId)
  {
    if (get_managed_texture_refcount(texId) <= 0)
      evict_managed_tex_id(texId);
    else if (!change_managed_texture(texId, nullptr, &ddsx_provider))
    {
      append_popup_error(String(0, "failed to change managed texture '%s'\n", tex_name.c_str()));
      return;
    }
  }

  if (!texId)
  {
    texId = add_managed_texture(tex_name.c_str(), &ddsx_provider);
    if (!texId)
    {
      append_popup_error(String(0, "can't add managed texture '%s'\n", tex_name.c_str()));
      return;
    }
  }

  ddsx_provider.addData(texId, eastl::move(packed));
  reload_managed_array_textures_for_changed_slice(tex_name.c_str());
}


static void add_empty_impostor_image(const String &name)
{
  eastl::unique_ptr<TexImage32> image;
  image.reset(TexImage32::create(resolution.x * 2 - 1, resolution.y));
  memset(image->getPixels(), 0, sizeof(uint32_t) * image->w * image->h);

  dag::Vector<rgba_surface> mipmaps;
  for (int x = 0, w = resolution.x, h = resolution.y; w >= MIN_RESOLUTION && h >= MIN_RESOLUTION && x + w <= image->w;
       x += w, w >>= 1, h >>= 1)
    mipmaps.emplace_back(rgba_surface{reinterpret_cast<uint8_t *>(image->getPixels() + x), w, h, int(image->w * sizeof(TexPixel32))});

  update_impostor_image(name, mipmaps);
}


static void set_image_mode(const ImDrawList *, const ImDrawCmd *)
{
  ShaderGlobal::set_int(imgui_channel_maskVarId, preview_mode);
  ShaderGlobal::set_int(imgui_draw_functionVarId, 2);
}

static void reset_image_mode(const ImDrawList *, const ImDrawCmd *)
{
  ShaderGlobal::set_int(imgui_channel_maskVarId, 15);
  ShaderGlobal::set_int(imgui_draw_functionVarId, 0);
}


static void baker_window()
{
  if (popup_message_was_set && !popup_message.empty())
  {
    popup_message_was_set = false;
    ImGui::OpenPopup("notification");
  }

  if (ImGui::BeginPopup("notification"))
  {
    ImGui::TextColored(popup_message_is_error ? ImVec4(1, 0, 0, 1) : ImGui::GetStyleColorVec4(ImGuiCol_Text), "%s",
      popup_message.c_str());
    ImGui::EndPopup();
  }
  else
  {
    popup_message.clear();
    popup_message_is_error = false;
  }

  const float buttonSize = ImGui::GetFrameHeight();

  ImGui::BeginGroup();

  if (ImGui::Button("Reload##reload_blks"))
    reload_impostor_data();

  bool shouldFocusAddImpostor = false;
  ImGui::SameLine();
  static String newImpostorName;
  if (ImGui::Button("Add impostor"))
  {
    newImpostorName.clear();
    ImGui::OpenPopup("add_impostor");
    shouldFocusAddImpostor = true;
  }

  if (ImGui::BeginPopup("add_impostor"))
  {
    if (shouldFocusAddImpostor)
    {
      ImGui::SetKeyboardFocusHere();
      shouldFocusAddImpostor = false;
    }
    if (ImGuiDagor::InputText("Impostor name", &newImpostorName,
          ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue))
    {
      if (!newImpostorName.empty())
      {
        impostor_data.emplace(impostor_data.begin(), newImpostorName.c_str(), DataBlock());
        selected_impostor = 0;
        selection_changed = true;
      }
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  if (ImGui::BeginListBox("##impostor_list", ImVec2(300, -1)))
  {
    for (int i = 0; i < impostor_data.size(); i++)
    {
      if (impostor_data[i].changed)
        ImGui::Bullet();
      if (ImGui::Selectable(impostor_data[i].name.c_str(), i == selected_impostor))
      {
        selected_impostor = i;
        selection_changed = true;
      }
    }
    ImGui::EndListBox();
  }

  ImGui::EndGroup();

  if (selected_impostor < impostor_data.size())
  {
    auto &imp = impostor_data[selected_impostor];

    ImGui::SameLine();
    ImGui::BeginGroup();

    ImGui::BeginDisabled(!!impostor_to_bake);
    if (ImGui::Button("Bake", ImVec2(200, ImGui::GetFrameHeightWithSpacing() + ImGui::GetFrameHeight())))
    {
      impostor_to_bake.reset(new BakingData(imp));
      impostor_to_bake->loadResources();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginGroup();
    if (ImGui::Button("Reload TIFF"))
    {
      clear_popup_message();

      for (const auto type : {"ad", "nt"})
      {
        String fname = imp.makeTiffFilename(type);
        eastl::unique_ptr<TexImage32> image;
        image.reset(load_tiff32(fname.c_str(), tmpmem));
        if (!image)
        {
          append_popup_error(String(0, "failed to load %s\n", fname.c_str()));
          continue;
        }

        // extract mip-maps
        if (image->h != resolution.y || image->w < resolution.x)
        {
          append_popup_error(
            String(0, "TIFF image %dx%d doesn't match impostor resolution %dx%d\n", image->w, image->h, resolution.x, resolution.y));
          continue;
        }

        dag::Vector<rgba_surface> mipmaps;
        for (int x = 0, w = resolution.x, h = resolution.y; w >= MIN_RESOLUTION && h >= MIN_RESOLUTION && x + w <= image->w;
             x += w, w >>= 1, h >>= 1)
          mipmaps.emplace_back(
            rgba_surface{reinterpret_cast<uint8_t *>(image->getPixels() + x), w, h, int(image->w * sizeof(TexPixel32))});

        update_impostor_image(imp.makeTextureName(type), make_span_const(mipmaps));
      }

      selection_changed = true;
    }

    ImGui::BeginDisabled(!imp.baked);
    ImGui::SameLine();
    if (ImGui::Button("Save TIFF"))
    {
      clear_popup_message();
      bool ok = true;
      for (const auto type : {"ad", "nt"})
      {
        String fname = imp.makeTiffFilename(type);
        if (!save_tiff32(fname.c_str(), imp.baked->getImage(type)))
        {
          append_popup_error(String(0, "failed to save %s\n", fname.c_str()));
          ok = false;
        }
      }

      if (ok)
        imp.baked.reset();
    }
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!imp.changed);
    ImGui::SameLine();
    if (ImGui::Button("Save BLK"))
    {
      clear_popup_message();
      auto fname = imp.makeBlkFilename();
      if (!imp.blk.saveToTextFile(fname.c_str()))
        append_popup_error(String(0, "failed to save %s\n", fname.c_str()));
      else
        imp.changed = false;
    }
    ImGui::EndDisabled();

    if (selection_changed)
    {
      selection_changed = false;

      String name;
      name = imp.makeTextureName("ad");
      const char *ptr = name.c_str();
      if (!get_managed_texture_id(ptr))
        add_empty_impostor_image(name);
      preview_ad_texid = dag::add_managed_array_texture("fast_grass_baker_preview_ad", make_span(&ptr, 1));

      name = imp.makeTextureName("nt");
      ptr = name.c_str();
      if (!get_managed_texture_id(ptr))
        add_empty_impostor_image(name);
      preview_nt_texid = dag::add_managed_array_texture("fast_grass_baker_preview_nt", make_span(&ptr, 1));
    }

    ImGui::RadioButton("Albedo", &preview_mode, ALBEDO);
    ImGui::SameLine();
    ImGui::RadioButton("Depth", &preview_mode, DEPTH);
    ImGui::SameLine();
    ImGui::RadioButton("Normal", &preview_mode, NORMAL);
    ImGui::SameLine();
    ImGui::RadioButton("Translucency", &preview_mode, TRANSLUCENCY);
    ImGui::SameLine();
    ImGui::RadioButton("Roughness", &preview_mode, ROUGHNESS);

    ImGui::EndGroup();

    ImGui::GetWindowDrawList()->AddCallback(set_image_mode, nullptr);
    float imw = ImGui::GetContentRegionAvail().x - 2;
    ImGui::Image(
      ImTextureID(unsigned(((preview_mode == ALBEDO || preview_mode == DEPTH) ? preview_ad_texid : preview_nt_texid).getTexId())),
      ImVec2(imw, imw * resolution.y / resolution.x), ImVec2(-0.5f, 0), ImVec2(0.5f, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 1));
    ImGui::GetWindowDrawList()->AddCallback(reset_image_mode, nullptr);
    if (preview_mode == DEPTH)
      ImGui::SetItemTooltip("White - front-clipped, should be minimal.\n"
                            "Cyan - 75%% of values.\n"
                            "Red - 25%% of far values, should be edges only.\n"
                            "Purple - background.");

    ImGui::Separator();

    if (ImGui::BeginChild("settings"))
    {
#define DO_PARAM(BLK, N, T, W, DV, ...)                          \
  {                                                              \
    auto v = (BLK).get##T(#N, DV);                               \
    if (ImGui::W(#N, &v, __VA_ARGS__) || !(BLK).paramExists(#N)) \
    {                                                            \
      (BLK).set##T(#N, v);                                       \
      imp.changed = true;                                        \
    }                                                            \
  }
      DO_PARAM(imp.blk, instances, Int, SliderInt, resolution.x * 2 / resolution.y, 1, 100)
      DO_PARAM(imp.blk, rows, Int, SliderInt, 1, 1, 10)
      DO_PARAM(imp.blk, row_step, Real, SliderFloat, 0.5f, 0, 2)
      DO_PARAM(imp.blk, jitter, Real, SliderFloat, 0.5f, 0, 2)
      DO_PARAM(imp.blk, scale_variance, Real, SliderFloat, 0.1f, 0, 0.99f, "%.2f")
      DO_PARAM(imp.blk, depth_offset, Real, SliderFloat, 0, -1, 2)
      DO_PARAM(imp.blk, seed, Int, InputInt, DefaultOAHasher<false>().hash_data(imp.name.c_str(), imp.name.length()), 1)

      if (int assetNameId = imp.blk.getNameId("asset"); assetNameId != -1)
      {
        int idToRemove = -1;
        for (int bi = -1;;)
        {
          bi = imp.blk.findBlock(assetNameId, bi);
          if (bi == -1)
            break;
          auto &b = *imp.blk.getBlock(bi);

          ImGui::PushID(bi);
          if (ImGui::Button("X##remove_asset", ImVec2(buttonSize, buttonSize)))
            idToRemove = bi;
          ImGui::SetItemTooltip("Remove this asset");
          ImGui::SameLine();
          const char *name = b.getStr("name", "???");
          if (ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
          {
            DO_PARAM(b, scale, Real, SliderFloat, 0, 0, 10)
            ImGui::SetItemTooltip("Set to 0 and bake to set default scale.");
            DO_PARAM(b, weight, Real, SliderFloat, 1, 0, 100, "%.1f")
            DO_PARAM(b, scale_variance, Real, SliderFloat, 0.1f, 0, 0.99f, "%.2f")
            DO_PARAM(b, tilt, Real, SliderFloat, 15, 0, 180, "%.1f deg")
            DO_PARAM(b, y_offset, Real, SliderFloat, 0, -0.5f, 0.5f)
          }
          ImGui::PopID();
        }

        if (idToRemove != -1)
        {
          imp.blk.removeBlock(idToRemove);
          imp.changed = true;
        }
      }

      bool shouldSetAssetFocus = false;
      if (ImGui::Button("Add asset"))
      {
        asset_names.clear();
        iterate_gameres_names_by_class(RendInstGameResClassId, [](const char *name) { asset_names.emplace_back(name); });
        eastl::sort(asset_names.begin(), asset_names.end());
        ImGui::OpenPopup("select_asset");
        shouldSetAssetFocus = true;
      }

      if (ImGui::BeginPopup("select_asset"))
      {
        static ImGuiTextFilter name_filter;
        if (shouldSetAssetFocus)
        {
          ImGui::SetKeyboardFocusHere();
          shouldSetAssetFocus = false;
        }
        bool textChanged =
          ImGui::InputTextWithHint("##name-filter", "Filter (-exc,inc)", name_filter.InputBuf, IM_ARRAYSIZE(name_filter.InputBuf));
        ImGui::SameLine(0, 0);
        if (ImGui::Button("x##clear-name-filter", ImVec2(buttonSize, buttonSize)))
        {
          name_filter.Clear();
          textChanged = true;
        }

        if (textChanged)
          name_filter.Build();

        if (ImGui::BeginListBox("##asset_list", ImVec2(300, 600)))
        {
          for (const auto &name : asset_names)
          {
            if (!name_filter.PassFilter(name.c_str()))
              continue;
            if (ImGui::Selectable(name.c_str()))
            {
              if (auto *b = imp.blk.addNewBlock("asset"))
              {
                b->setStr("name", name.c_str());
                imp.changed = true;
              }
              ImGui::CloseCurrentPopup();
              break;
            }
          }
          ImGui::EndListBox();
        }
        ImGui::EndPopup();
      }
#undef DO_PARAM
    }
    ImGui::EndChild();
    ImGui::EndGroup();
  }
}


static void on_imgui_state_change(ImGuiState, ImGuiState new_state)
{
  if (new_state == ImGuiState::OFF)
  {
    preview_ad_texid.close();
    preview_nt_texid.close();
    impostor_to_bake.reset();
    selection_changed = true;
  }
}

void init_fast_grass_baker()
{
  imgui_register_on_state_change_handler(on_imgui_state_change);

  char appFname[DAGOR_MAX_PATH];
  strcpy(appFname, folders::get_game_dir().c_str());
  strcat(appFname, "/../application.blk");
  dd_simplify_fname_c(appFname);

  DataBlock appBlk;
  if (!dd_file_exists(appFname) || !appBlk.load(appFname))
    logwarn("error loading %s", appFname);

  auto settingsBlk = appBlk.getBlockByNameEx("assets")->getBlockByNameEx("grass_impostor");
  resolution = settingsBlk->getIPoint2("resolution", resolution);

  dd_get_fname_location(data_folder, appFname);
  dd_append_slash_c(data_folder);
  strcat(data_folder, settingsBlk->getStr("data_folder", "develop"));
  dd_simplify_fname_c(data_folder);
  dd_append_slash_c(data_folder);

  dd_get_fname_location(texture_folder, appFname);
  dd_append_slash_c(texture_folder);
  strcat(texture_folder, settingsBlk->getStr("texture_folder", settingsBlk->getStr("data_folder", "develop")));
  dd_simplify_fname_c(texture_folder);
  dd_append_slash_c(texture_folder);

  debug("fast grass baker data folder: %s", data_folder);
  debug("fast grass baker texture folder: %s", texture_folder);
  reload_impostor_data();
}

} // namespace fast_grass_baker

REGISTER_IMGUI_WINDOW("Grass", "Fast Grass Baker", fast_grass_baker::baker_window);
