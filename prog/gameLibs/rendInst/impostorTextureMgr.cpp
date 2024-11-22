// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/impostorTextureMgr.h>
#include "render/genRender.h"
#include "riGen/riRotationPalette.h"

#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_consts.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_e3dColor.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_IRenderWrapperControl.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderBlock.h>
#include <render/deferredRenderer.h>
#include <startup/dag_globalSettings.h>
#include <render/bcCompressor.h>
#include <fx/dag_leavesWind.h>

#include <math/dag_hlsl_floatx.h>

#include <render/rendinst_impostor_dirs.hlsli>


static int globalFrameBlockId = -1;
static int rendinstSceneBlockId = -1;

#define GLOBAL_VARS_LIST VAR(treeCrown_buf_slot)

#define GLOBAL_OPTIONAL_VARS_LIST       \
  VAR(impostor_bounding_sphere)         \
  VAR(impostor_shadow_texture)          \
  VAR(impostor_data_offset)             \
  VAR(rendinst_render_pass)             \
  VAR(impostor_atlas_mask)              \
  VAR(impostor_atlas_mask_samplerstate) \
  VAR(impostor_shadow_x)                \
  VAR(impostor_shadow_y)                \
  VAR(impostor_shadow_z)                \
  VAR(impostor_options)                 \
  VAR(impostor_shadow)                  \
  VAR(impostor_scale)                   \
  VAR(impostor_slice)                   \
  VAR(texture_size)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
GLOBAL_OPTIONAL_VARS_LIST
#undef VAR

static eastl::unique_ptr<ImpostorTextureManager> impostorTextureManager;

void init_impostor_texture_mgr()
{
  close_impostor_texture_mgr();
  impostorTextureManager = eastl::make_unique<ImpostorTextureManager>();
}

void close_impostor_texture_mgr() { impostorTextureManager.reset(nullptr); }

ImpostorTextureManager *get_impostor_texture_mgr() { return impostorTextureManager.get(); }

static TMatrix tranform_point4_to_view_to_content_matrix(const Point4 &transform)
{
  TMatrix flipY;
  flipY.setcol(0, {1, 0, 0});
  flipY.setcol(1, {0, -1, 0});
  flipY.setcol(2, {0, 0, 1});
  flipY.setcol(3, {0, 0, 0});

  // ((-1,-1):(1,1) -> (0,0);(1,1))
  TMatrix a;
  a.setcol(0, {0.5, 0, 0});
  a.setcol(1, {0, 0.5, 0});
  a.setcol(2, {0, 0, 1});
  a.setcol(3, {0.5, 0.5, 0});

  // ((0,0);(1,1) -> (-1,-1):(1,1))
  TMatrix b;
  b.setcol(0, {2, 0, 0});
  b.setcol(1, {0, 2, 0});
  b.setcol(2, {0, 0, 1});
  b.setcol(3, {-1, -1, 0});

  TMatrix sliceTm;
  sliceTm.setcol(0, {transform.x, 0, 0});
  sliceTm.setcol(1, {0, transform.y, 0});
  sliceTm.setcol(2, {0, 0, 1});
  sliceTm.setcol(3, {transform.z, transform.w, 0});

  return flipY * b * sliceTm * a * flipY;
}

ImpostorTextureManager::ImpostorTextureManager()
{
  debug("Initializing impostor texture manager");
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR

#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_OPTIONAL_VARS_LIST
#undef VAR

  treeCrownBufSlot = ShaderGlobal::get_int(treeCrown_buf_slotVarId);

  ::init_rendinst_impostor_shader();

  globalFrameBlockId = ShaderGlobal::getBlockId("global_frame");
  rendinstSceneBlockId = ShaderGlobal::getBlockId("rendinst_scene");

  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_FUNC);
  state.zFunc = CMPF_GREATEREQUAL;
  state.set(shaders::OverrideState::SCISSOR_ENABLED);
  impostorShaderState = shaders::overrides::create(state);

  if (shader_exists("impostor_mask_shader"))
    impostorMaskShader.init("impostor_mask_shader");

  if (shader_exists("impostor_shadow_atlas"))
    impostorShadowShader.init("impostor_shadow_atlas");

  if (prefer_bc_compression())
  {
    shadowAtlasCompressor = eastl::make_unique<BcCompressor>(BcCompressor::COMPRESSION_BC4, 0, 0, 0, 1, "bc4_compressor");
    if (!shadowAtlasCompressor->isValid())
      shadowAtlasCompressor.reset();
  }
}

bool ImpostorTextureManager::hasBcCompression() const { return shadowAtlasCompressor != nullptr; }

ImpostorTextureManager::~ImpostorTextureManager() { debug("Closing impostor texture manager"); }

class ImpostorGenRenderWrapperControl final : public IRenderWrapperControl
{
public:
  int checkVisible(const BSphere3 & /*sph*/, const BBox3 * /*bbox*/) override { return 1; }
  void beforeRender(int /*value*/, bool /*trans*/) override {}
  void afterRender(int /*value*/, bool /*trans*/) override {}
  void destroy() override {}
};

ImpostorTextureManager::GenerationData ImpostorTextureManager::load_data(const DataBlock *props, RenderableInstanceLodsResource *res,
  int smallest_mip, bool default_preshadows_enabled, const IPoint3 &default_mip_offsets_hq_mq_lq,
  const IPoint3 &default_mobile_mip_offsets_hq_mq_lq, int num_levels, const ImpostorTextureQualityLevel *sorted_levels)
{
  const DataBlock *blk = props->getBlockByNameEx("impostor");
  GenerationData genData;

  int nid_lod = props->getNameId("lod");
  const char *defaultImpostorMode = "billboard";
  if (nid_lod >= 0)
  {
    for (int i = 0; i < props->blockCount(); i++)
    {
      const DataBlock *lodBlk = props->getBlock(i);
      const int blockNameId = lodBlk->getBlockNameId();
      if (blockNameId == nid_lod)
      {
        if (strstr(lodBlk->getStr("fname", ""), "octahedral") != nullptr)
        {
          // set default value (it can be overwritten)
          defaultImpostorMode = "octahedral";
          break;
        }
      }
    }
  }

  genData.octahedralImpostor = strcmp(blk->getStr("impostorType", defaultImpostorMode), "octahedral") == 0;
  if (!genData.octahedralImpostor)
  {
    genData.horizontalSamples = 9; // -V1048
    genData.verticalSamples = 1;   // -V1048
  }
  genData.allowParallax = blk->getBool("allowParallax", genData.allowParallax);
  genData.allowTriView = blk->getBool("allowTriView", genData.allowTriView);
  genData.mipCount = blk->getInt("mipCount", genData.mipCount);
  genData.textureWidth = blk->getInt("textureWidth", genData.textureWidth);
  genData.textureHeight = blk->getInt("textureHeight", genData.textureHeight);
  genData.horizontalSamples = blk->getInt("horizontalSamples", genData.horizontalSamples);
  genData.verticalSamples = blk->getInt("verticalSamples", genData.verticalSamples);
  genData.rotationPaletteSize = blk->getInt("rotationPaletteSize", genData.rotationPaletteSize);
  genData.tiltLimit = blk->getPoint3("tiltLimit", genData.tiltLimit * RAD_TO_DEG) * DEG_TO_RAD;
  genData.preshadowEnabled = blk->getBool("preshadowEnabled", default_preshadows_enabled);
  genData.bottomGradient = blk->getReal("bottomGradient", genData.bottomGradient);
  genData.mipOffsets_hq_mq_lq = blk->getIPoint3("mipOffsets_hq_mq_lq", default_mip_offsets_hq_mq_lq);
  genData.mobileMipOffsets_hq_mq_lq = blk->getIPoint3("mobileMipOffsets_hq_mq_lq", default_mobile_mip_offsets_hq_mq_lq);

  if (genData.textureHeight == 0)
  {
    int i = 0;
    float height = res->bbox.width().y;
    // with the expected sizes, linear search is better than binary search
    while (i + 1 < num_levels && sorted_levels[i + 1].minHeight <= height)
      ++i;
    genData.textureHeight = sorted_levels[i].textureHeight;
  }

  if (genData.textureWidth == 0)
  {
    float bboxAspectRatio = 1;
    if (const RenderableInstanceLodsResource::ImpostorData *imp = res->getImpostorData())
    {
      bboxAspectRatio = imp->bboxAspectRatio;
    }
    else
    {
      const BBox3 &lbb = res->bbox;
      bboxAspectRatio = safediv(lbb.width().y, max(lbb.width().x, lbb.width().z));
    }
    if (bboxAspectRatio > 1.75)
      genData.textureWidth = genData.textureHeight * 4;
    else
      genData.textureWidth = genData.textureHeight * 8;
  }

  if (genData.mipCount == 0)
  {
    genData.mipCount = 1;
    int w = genData.textureWidth;
    int h = genData.textureHeight;
    while (w > smallest_mip && h > smallest_mip)
    {
      genData.mipCount++;
      w >>= 1;
      h >>= 1;
    }
  }
  return genData;
}

static void get_impostor_tight_matrix(RenderableInstanceLodsResource *res, const Point3 &point_to_eye, TMatrix &viewItm,
  TMatrix &viewTm, float &zn, float &zf, Point2 &scale)
{
  // Align the camera on the trunk
  Point3 rotationCenter = Point3(0, res->bsphCenter.y, 0);
  const Point3 viewVec = -point_to_eye;
  Point3 up = viewVec.x == 0.f && viewVec.z == 0.f ? Point3(1, 0, 0) : Point3(0, 1, 0);
  Point3 side = normalize(cross(up, viewVec));
  up = cross(viewVec, side);
  viewItm.setcol(0, side);
  viewItm.setcol(1, up);
  viewItm.setcol(2, viewVec);
  viewItm.setcol(3, rotationCenter - viewVec * res->bsphRad);
  viewTm = orthonormalized_inverse(viewItm);

  Point3 centerOffset = res->bsphCenter - rotationCenter;
  float zOffset = dot(centerOffset, viewVec);
  zn = zOffset;
  zf = zn + 2 * res->bsphRad;
  Point2 scaleOffset = abs(Point2{dot(side, centerOffset), dot(up, centerOffset)});
  scale = ImpostorTextureManager::get_scale(res) + scaleOffset;
}

Point2 ImpostorTextureManager::get_scale(RenderableInstanceLodsResource *res)
{
  Point3 boxSize;
  for (int i = 0; i < 3; ++i)
    boxSize[i] = ::max(::abs(res->bbox[1][i] - res->bsphCenter[i]), ::abs(res->bbox[0][i] - res->bsphCenter[i]));
  return Point2(Point2{boxSize.x, boxSize.z}.length(), res->bsphRad);
}

Point3 ImpostorTextureManager::get_point_to_eye_billboard(uint32_t sliceId)
{
  G_ASSERT(sliceId < 9);
  return -IMPOSTOR_SLICE_DIRS[sliceId];
}

Point3 ImpostorTextureManager::get_point_to_eye_octahedral(uint32_t h, uint32_t v, unsigned int h_samples, unsigned int v_samples)
{
  float x = (h + 0.5f) / h_samples * 2 - 1;
  float y = (v + 0.5f) / v_samples * 2 - 1;
  { // upper hemisphere only
    // Transformation: rotation about 45 degrees and scaling by 1/sqrt(2)
    // The top half of the octahedron is mapped to a square, defined by connecting the
    // middle point of the sides of the rectangle (-1,-1):(1,1)
    // this transformation transforms this rectangle to the rectangle of the upper half
    float xx = x;
    x = (x + y) / 2;
    y = (y - xx) / 2;
  }
  x = clamp(x, -1.f, 1.f);
  y = clamp(y, -1.f, 1.f);

  float z = 1.0f - (fabs(x) + fabs(y));
  if (fabs(x) + fabs(y) > 1)
  {
    float absX = fabs(x);
    x = -(fabs(y) - 1) * sign(x);
    y = -(absX - 1) * sign(y);
  }
  return normalize(Point3(x, z, y));
}

Point3 ImpostorTextureManager::get_point_to_eye_octahedral(uint32_t h, uint32_t v, const GenerationData &gen_data)
{
  return get_point_to_eye_octahedral(h, v, gen_data.horizontalSamples, gen_data.verticalSamples);
}

void ImpostorTextureManager::render(const Point3 &point_to_eye, const TMatrix &view_to_content, RenderableInstanceLodsResource *res,
  int block_id, int lod) const
{
  if (lod < 0)
    lod = res->getQlMinAllowedLod();
  G_ASSERT(lod >= res->getQlBestLod());
  TIME_D3D_PROFILE(RenderImpostor);

  rendinst::gen::ScopedDisablePaletteRotation disableRotation;

  TMatrix viewTm;
  TMatrix viewItm;
  float zn, zf;
  Point2 scale;
  get_impostor_tight_matrix(res, point_to_eye, viewItm, viewTm, zn, zf, scale);
  d3d::settm(TM_VIEW, viewTm);
  Point3_vec4 col[3];
  for (int i = 0; i < 3; ++i)
    col[i] = viewItm.getcol(i);
  LeavesWindEffect::setNoAnimShaderVars(col[0], col[1], col[2]);

  TMatrix4 postProjTm = TMatrix4(view_to_content);
  TMatrix4 projTm = matrix_ortho_lh_reverse(scale.x * 2, scale.y * 2, zn, zf) * postProjTm;
  d3d::settm(TM_PROJ, &projTm);

  IPoint2 viewOffset;
  IPoint2 viewSize;
  float zmin, zmax;
  d3d::getview(viewOffset.x, viewOffset.y, viewSize.x, viewSize.y, zmin, zmax);

  if (abs(point_to_eye.y) < 0.1)
  {
    TMatrix4 globTmUnaligned;
    d3d::calcglobtm(viewTm, projTm, globTmUnaligned);
    mat44f globTm;
    memcpy(&globTm, &globTmUnaligned, sizeof(globTm));
    vec4f groundPoint = v_make_vec4f(0, 0, 0, 1);
    vec4f groundDir = v_make_vec4f(0, -1, 0, 0);
    vec4f groundPointInScreen = v_mat44_mul_vec4(globTm, groundPoint);
    vec4f groundDirInScreen = v_mat44_mul_vec4(globTm, groundDir);
    float groundYPos =
      saturate(1.0 - (v_extract_y(v_div(groundPointInScreen, v_perm_wwww(groundPointInScreen))) * 0.5 + 0.5)) * viewSize.y;
    if (v_extract_y(groundDirInScreen) < 0)
    {
      int groundOffset = floorf(viewSize.y - groundYPos);
      d3d::setscissor(viewOffset.x, viewOffset.y, viewSize.x, viewSize.y - groundOffset);
    }
    else
    {
      int groundOffset = floorf(groundYPos);
      d3d::setscissor(viewOffset.x, viewOffset.y + groundOffset, viewSize.x, viewSize.y - groundOffset);
    }
  }
  else
    d3d::setscissor(viewOffset.x, viewOffset.y, viewSize.x, viewSize.y);


  {
    SCENE_LAYER_GUARD(block_id);
    res->lods[lod].scene->getMesh()->getMesh()->getMesh()->acquireTexRefs();

    static const E3DCOLOR defaultColors[] = {E3DCOLOR(127, 127, 127, 127), E3DCOLOR(127, 127, 127, 127)};

    rendinst::render::RiShaderConstBuffers cb;
    cb.setOpacity(0, 1);
    cb.setCrossDissolveRange(0);
    const Point3 &sphereCenter = res->bsphCenter;
    float sphereRadius = res->bsphRad + sqrtf(sphereCenter.x * sphereCenter.x + sphereCenter.z * sphereCenter.z);
    // sphere radius is needed for ellipsoid normal
    cb.setBoundingSphere(0, 0, sphereRadius, 1, 0);
    cb.setRandomColors(defaultColors);
    cb.setInstancing(0, 3, 0, 0);
    cb.flushPerDraw();

    // We update rendinst::render::perDrawCB inside cb.flushPerDraw() if it's not null.
    // It causes split of renderpass in Vulkan cause of stage change.
    // Just have to accept it unless we allocate multiple buffers and update them in advance.
    d3d::allow_render_pass_target_load();

    d3d::set_buffer(STAGE_PS, treeCrownBufSlot, treeCrownDataBuf);
    ImpostorGenRenderWrapperControl rwc;
    res->lods[lod].scene->render(TMatrix(1), rwc);
    res->lods[lod].scene->renderTrans(TMatrix(1), rwc);
    d3d::set_buffer(STAGE_PS, treeCrownBufSlot, nullptr);
  }
}

float ImpostorTextureManager::get_vertex_scaling(float cos_phi) { return 1 - cos_phi; }

void ImpostorTextureManager::start_rendering_slices(DeferredRenderTarget *rt)
{
  G_ASSERT(rt && rt->getRt(0));
  G_ASSERT(VariableMap::isGlobVariablePresent(texture_sizeVarId));
  shaders::overrides::set(impostorShaderState);
  ShaderGlobal::set_int(rendinst_render_passVarId, eastl::to_underlying(rendinst::RenderPass::ImpostorColor));
  ShaderGlobal::setBlock(rendinst::render::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

  rt->setRt();
  d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL | CLEAR_TARGET, 0, 0, 0);
  rendinst::render::setCoordType(rendinst::render::COORD_TYPE_TM);
  rendinst::render::startRenderInstancing();
  d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);
}

void ImpostorTextureManager::start_rendering_branches(Texture *rt)
{
  G_ASSERT(rt);
  G_ASSERT(VariableMap::isGlobVariablePresent(texture_sizeVarId));
  shaders::overrides::set(impostorShaderState);
  ShaderGlobal::set_int(rendinst_render_passVarId, eastl::to_underlying(rendinst::RenderPass::ImpostorColor));
  ShaderGlobal::setBlock(rendinst::render::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

  d3d::set_render_target({nullptr, 0}, DepthAccess::SampledRO, {{rt, 0}});
  d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL | CLEAR_TARGET, E3DCOLOR(255, 255, 255, 255), 0, 0);
  rendinst::render::setCoordType(rendinst::render::COORD_TYPE_TM);
  rendinst::render::startRenderInstancing();
  d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);
}

void ImpostorTextureManager::end_rendering_slices()
{
  rendinst::render::endRenderInstancing();
  d3d::set_depth(nullptr, DepthAccess::RW);
  shaders::overrides::reset();
  d3d::setind(nullptr);
  d3d::setvdecl(BAD_VDECL);
  d3d::setvsrc(0, nullptr, 0);
  d3d::set_render_target(0, nullptr, 0);
  d3d::set_depth(nullptr, DepthAccess::SampledRO);
  ShaderElement::invalidate_cached_state_block();
}

void ImpostorTextureManager::generate_mask(const Point3 &point_to_eye, RenderableInstanceLodsResource *res, DeferredRenderTarget *rt,
  Texture *mask_tex, int lod)
{
  G_ASSERT(impostorMaskShader.getElem());
  SCOPE_RENDER_TARGET;
  start_rendering_slices(rt);
  TMatrix postView;
  postView.identity();
  render(point_to_eye, postView, res, rendinst::render::rendinstSceneBlockId, lod);
  end_rendering_slices();
  TextureInfo info;
  rt->getRt(0)->getinfo(info, 0);
  TextureInfo info2;
  mask_tex->getinfo(info2, 0);
  G_ASSERT(info.w == info2.w && info.h == info2.h);
  ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

  d3d::set_render_target({nullptr, 0}, DepthAccess::SampledRO, {{mask_tex, 0}});
  d3d::setview(0, 0, info.w, info.h, 0, 1);
  d3d::setscissor(0, 0, info.w, info.h);
  d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL | CLEAR_TARGET, 0, 0, 0);
  rt->setVar();
  // The STATE_GUARD_NULLPTR macro fails with these expressions
  Texture *const tex1 = rt->getRt(0);
  Texture *const tex2 = rt->getRt(1);
  Texture *const tex3 = rt->getRt(2);
  STATE_GUARD_NULLPTR(d3d::set_tex(STAGE_PS, rendinst::render::dynamic_impostor_texture_const_no, VALUE), tex1);
  STATE_GUARD_NULLPTR(d3d::set_tex(STAGE_PS, rendinst::render::dynamic_impostor_texture_const_no + 1, VALUE), tex2);
  STATE_GUARD_NULLPTR(d3d::set_tex(STAGE_PS, rendinst::render::dynamic_impostor_texture_const_no + 2, VALUE), tex3);
  impostorMaskShader.render();
}

void ImpostorTextureManager::render_slice_octahedral(uint32_t h, uint32_t v, const TMatrix &view_to_content,
  const GenerationData &gen_data, RenderableInstanceLodsResource *res, int block_id, int lod)
{
  render(get_point_to_eye_octahedral(h, v, gen_data), view_to_content, res, block_id, lod);
}

void ImpostorTextureManager::render_slice_billboard(uint32_t sliceId, const TMatrix &view_to_content,
  RenderableInstanceLodsResource *res, int block_id, int lod)
{
  render(get_point_to_eye_billboard(sliceId), view_to_content, res, block_id, lod);
}

void ImpostorTextureManager::generate_mask_billboard(uint32_t sliceId, RenderableInstanceLodsResource *res, DeferredRenderTarget *rt,
  Texture *mask_tex, int lod)
{
  generate_mask(get_point_to_eye_billboard(sliceId), res, rt, mask_tex, lod);
}

void ImpostorTextureManager::generate_mask_octahedral(uint32_t h, uint32_t v, const GenerationData &gen_data,
  RenderableInstanceLodsResource *res, DeferredRenderTarget *rt, Texture *mask_tex, int lod)
{
  generate_mask(get_point_to_eye_octahedral(h, v, gen_data), res, rt, mask_tex, lod);
}

UniqueTex ImpostorTextureManager::renderDepthAtlasForShadow(RenderableInstanceLodsResource *res)
{
  TIME_D3D_PROFILE(render_depth_atlas_for_shadow);

  SharedTex impostorAlbedo = SharedTex(res->getImpostorTextures().albedo_alpha);
  TextureInfo info;
  impostorAlbedo->getinfo(info, 0);
  // TODO Reduce the number of creation and destruction of this texture. Currently it's created once for each asset with impostor
  UniqueTex impostorDepthBuffer =
    dag::create_tex(nullptr, info.w, info.h, TEXFMT_DEPTH16 | TEXCF_RTARGET, 1, "tmp_impostor_depth_atlas");
  G_ASSERT_RETURN(impostorDepthBuffer, {});
  impostorDepthBuffer->disableSampler();
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    ShaderGlobal::set_sampler(get_shader_variable_id("impostor_shadow_texture_samplerstate", true), d3d::request_sampler(smpInfo));
  }

  // start_rendering_slices with depth only but assuming that instancing has already been set up
  SCOPE_RESET_SHADER_BLOCKS;
  SCOPE_RENDER_TARGET;
  shaders::overrides::set(impostorShaderState);
  ShaderGlobal::set_int(rendinst_render_passVarId, eastl::to_underlying(rendinst::RenderPass::Depth));
  ShaderGlobal::setBlock(rendinst::render::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

  d3d::set_render_target(nullptr, 0);
  d3d::set_depth(impostorDepthBuffer.getTex2D(), DepthAccess::RW);
  d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0);
  rendinst::render::setCoordType(rendinst::render::COORD_TYPE_TM);

  const auto &params = res->getImpostorParams();
  int sliceId = 0;
  for (int v = 0; v < params.verticalSamples; ++v)
  {
    for (int h = 0; h < params.horizontalSamples; ++h, ++sliceId)
    {
      TMatrix viewToContent = tranform_point4_to_view_to_content_matrix(params.perSliceParams[sliceId].sliceTm);

      get_impostor_texture_mgr()->render_slice_billboard(sliceId, viewToContent, res, rendinst::render::rendinstDepthSceneBlockId, -1);
    }
  }

  // end_rendering_slices but without breaking instancing global state
  shaders::overrides::reset();

  return impostorDepthBuffer;
}

bool ImpostorTextureManager::update_shadow(RenderableInstanceLodsResource *res, const ShadowGenInfo &info, int layer_id, int ri_id,
  int palette_id, const UniqueTex &impostorDepthBuffer)
{
  if (!res->getImpostorParams().preshadowEnabled)
    return true;
  if (!impostorDepthBuffer)
    return false;
  bool compressionEnabled = hasBcCompression();
  SCOPE_RESET_SHADER_BLOCKS;
  bool ret = true;
  TIME_D3D_PROFILE(render_impostor_shadow_atlas);
  const rendinst::gen::RotationPaletteManager::Palette palette =
    rendinst::gen::get_rotation_palette_manager()->getPalette({layer_id, ri_id});
  TMatrix paletteRotation = rendinst::gen::get_rotation_palette_manager()->get_tm(palette, palette_id);
  TMatrix shadowMatrix;
  for (uint32_t i = 0; i < 4; ++i)
  {
    shadowMatrix.setcol(i, Point3(info.impostor_shadow_x[i], info.impostor_shadow_y[i], info.impostor_shadow_z[i]));
  }
  const TMatrix modelToShadow = shadowMatrix * paletteRotation;
  ArrayTexture *arrayTex = acquire_managed_tex(res->getImpostorTextures().shadowAtlas);
  G_ASSERT_RETURN(arrayTex != nullptr, false);
  TextureInfo texInfo;
  arrayTex->getinfo(texInfo);
  BaseTexture *targetTex = arrayTex;
  int targetSlice = palette_id;
  if (compressionEnabled)
  {
    TextureInfo compressionInfo;
    if (impostorCompressionBuffer)
      impostorCompressionBuffer->getinfo(compressionInfo);
    if (!impostorCompressionBuffer || compressionInfo.w != texInfo.w || compressionInfo.h != texInfo.h)
    {
      if (impostorCompressionBuffer)
        logwarn("Compression buffer is recreated in impostorTextureMgr");
      impostorCompressionBuffer = dag::create_tex(nullptr, texInfo.w, texInfo.h, TEXCF_RTARGET | TEXFMT_L8 | TEXCF_GENERATEMIPS,
        arrayTex->level_count(), "impostorCompressionBuffer");
    }
    targetTex = impostorCompressionBuffer.getTex2D();
    targetSlice = 0;
  }
  if (!targetTex)
    return false;
  {
    G_ASSERT(impostorShadowShader.getElem());
    SCOPE_RENDER_TARGET;
    const auto &params = res->getImpostorParams();
    ShaderGlobal::set_texture(impostor_atlas_maskVarId, res->getImpostorTextures().albedo_alpha);
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.anisotropic_max = 1;
      smpInfo.filter_mode = d3d::FilterMode::Default;
      smpInfo.mip_map_bias = 0;
      ShaderGlobal::set_sampler(impostor_atlas_mask_samplerstateVarId, d3d::request_sampler(smpInfo));
    }
    ShaderGlobal::set_color4(impostor_optionsVarId, params.horizontalSamples, params.verticalSamples, 0, 0);
    ShaderGlobal::set_color4(impostor_scaleVarId, Color4(params.scale.x, params.scale.y, 0, 0));
    ShaderGlobal::set_color4(impostor_bounding_sphereVarId,
      Color4(res->bsphCenter.x, res->bsphCenter.y, res->bsphCenter.z, res->bsphRad));
    ShaderGlobal::set_int(impostor_data_offsetVarId, palette.impostor_data_offset);
    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::set_color4(impostor_shadow_xVarId, modelToShadow.getcol(0).x, modelToShadow.getcol(1).x, modelToShadow.getcol(2).x,
      modelToShadow.getcol(3).x);
    ShaderGlobal::set_color4(impostor_shadow_yVarId, modelToShadow.getcol(0).y, modelToShadow.getcol(1).y, modelToShadow.getcol(2).y,
      modelToShadow.getcol(3).y);
    ShaderGlobal::set_color4(impostor_shadow_zVarId, modelToShadow.getcol(0).z, modelToShadow.getcol(1).z, modelToShadow.getcol(2).z,
      modelToShadow.getcol(3).z);
    ShaderGlobal::set_texture(impostor_shadowVarId, info.impostor_shadowID);
    d3d::resource_barrier({impostorDepthBuffer.getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    d3d::set_render_target(0, targetTex, targetSlice, 0);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    for (int v = 0; v < params.verticalSamples; ++v)
    {
      for (int h = 0; h < params.horizontalSamples; ++h)
      {
        SCOPE_RENDER_TARGET;
        TIME_D3D_PROFILE(render_impostor_shadow_atlas_slice);
        d3d::set_render_target(0, targetTex, targetSlice, 0);
        d3d::setview(0, 0, texInfo.w, texInfo.h, 0, 1);
        ShaderGlobal::set_texture(impostor_shadow_textureVarId, impostorDepthBuffer.getTexId());
        ShaderGlobal::set_color4(impostor_sliceVarId, h, v, 0, 0);
        impostorShadowShader.render();
      }
    }
    ShaderGlobal::set_texture(impostor_shadowVarId, BAD_TEXTUREID);
    targetTex->generateMips();
  }
  if (compressionEnabled)
  {
    G_ASSERT(shadowAtlasCompressor);
    if (!shadowAtlasCompressor)
      ret = false;
    else
    {
      shadowAtlasCompressor->resetBuffer(arrayTex->level_count(), texInfo.w, texInfo.h, 1);

      for (int i = 0; i < arrayTex->level_count(); ++i)
      {
        TextureInfo ti;
        arrayTex->getinfo(ti, i);

        shadowAtlasCompressor->updateFromMip(impostorCompressionBuffer.getTexId(), i, i);
        shadowAtlasCompressor->copyToMip(arrayTex, i + palette_id * arrayTex->level_count(), 0, 0, i, 0, 0, ti.w, ti.h);
      }
    }
  }
  release_managed_tex(res->getImpostorTextures().shadowAtlas);
  return ret;
}


bool ImpostorTextureManager::prefer_bc_compression()
{
  if (!BcCompressor::isAvailable(BcCompressor::ECompressionType::COMPRESSION_BC4))
    return false;
#if _TARGET_ANDROID
  bool defaultCompression = false;
#else
  bool defaultCompression = true;
#endif
  return ::dgs_get_game_params()->getBool("enableImpostorPreshadowCompression", defaultCompression);
}

int ImpostorTextureManager::getPreferredShadowAtlasMipOffset() const { return hasBcCompression() ? 1 : 2; }
