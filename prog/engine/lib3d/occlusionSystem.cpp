// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_frustum.h>
#include <3d/dag_maskedOcclusionCulling.h>
#include <3d/dag_occlusionSystem.h>
#include <3d/dag_ringCPUTextureLock.h>
#include <shaders/dag_postFxRenderer.h>
#include <generic/dag_carray.h>
#include <generic/dag_smallTab.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include "dag_occlusionRenderer.h"
#include <EASTL/unique_ptr.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_convar.h>

#define debug(...) logmessage(_MAKE4C('OCCL'), __VA_ARGS__)

#if DAGOR_DBGLEVEL > 0
CONSOLE_BOOL_VAL("occlusion", debug_reprojection, false);
#endif

class OcclusionSystemImpl final : public OcclusionSystem
{
public:
  OcclusionSystemImpl() {}
  ~OcclusionSystemImpl() { close(); }
  void init() override;
  void close() override;
  void reset() override;
  bool hasGPUFrame() const override { return currentCheckingFrame > 0; }
  void readGPUframe() override; // can be run from any thread, but will read from gpu texture (gpu readback)
  void prepareGPUDepthFrame(vec3f pos, mat44f_cref view, mat44f_cref proj, mat44f_cref viewProj, float cockpit_distance,
    CockpitReprojectionMode cockpit_mode, const mat44f &cockpit_anim) override; // performs reprojection of latest GPU frame
  void clear() override;                                                        // performs reprojection of latest GPU frame
  void buildMips() override;
  void prepareDebug() override;
  void prepareNextFrame(vec3f view_pos, mat44f_cref view, mat44f_cref proj, mat44f_cref view_proj, float zn, float zf,
    TextureIDPair mipped_depth, Texture *depth) override;
  void prepareNextFrame(vec3f view_pos, mat44f_cref view, mat44f_cref proj, mat44f_cref view_proj, float zn, float zf,
    TextureIDPair mipped_depth, Texture *depth, StereoIndex stereo_index) override;
  void setReprojectionUseCameraTranslatedSpace(bool enabled) override;
  bool getReprojectionUseCameraTranslatedSpace() const override;
  void initSWRasterization() override;
  void startRasterization(float zn) override;
  void rasterizeBoxes(mat44f_cref viewproj, const bbox3f *box, const mat43f *mat, int cnt) override;
  void rasterizeBoxes(mat44f_cref viewproj, const bbox3f *box, const mat44f *mat, int cnt) override;
  void rasterizeQuads(mat44f_cref viewproj, const vec4f *verts, int cnt) override;
  void rasterizeMesh(mat44f_cref viewproj, vec3f bmin, vec3f bmax, const vec4f *verts, int vert_cnt, const unsigned short *faces,
    int tri_count) override;
  void combineWithSWRasterization() override;
  void prepareDebugSWRasterization() override;
  void mergeOcclusionsZmin(MaskedOcclusionCulling **another_occl, uint32_t occl_count, uint32_t first_tile,
    uint32_t last_tile) override
  {
    moc->mergeOcclusionsZmin(another_occl, occl_count, first_tile, last_tile);
  }
  void getMaskedResolution(uint32_t &_width, uint32_t &_height) const
  {
    _width = width * 4;
    _height = height * 4;
  }

protected:
  template <bool ignoreVr>
  void prepareNextFrameImpl(vec3f view_pos, mat44f_cref view, mat44f_cref proj, mat44f_cref view_proj, float zn, float zf,
    TextureIDPair mipped_depth, Texture *depth, StereoIndex stereo_index);

  void initVrRes();
  bool process(float *destData, uint32_t &frame);
  SmallTab<float, MidmemAlloc> lastHWdepth; // we need that in case next frame is not ready
  class MaskedOcclusionCulling *moc = 0;

  static constexpr int FRAMES_HISTORY = 8;
  struct Frame
  {
    mat44f viewproj;
    vec4f pos;
    float zn, zf;
  };
  carray<Frame, FRAMES_HISTORY> globtmHistory;
  carray<mat44f, FRAMES_HISTORY> cockpitAnimHistory = {};
  int lastRenderedFrame = 0;
  bool isVrResInited = false;

  int width = 0, height = 0;
  RingCPUTextureLock ringTextures;
  PostFxRenderer downsample;
  eastl::unique_ptr<ComputeShaderElement> downsample_vr;
  uint32_t currentCheckingFrame = 0;
#if DAGOR_DBGLEVEL > 0
  TextureIDHolder reprojected; // debug
  TextureIDHolder masked;      // debug
#endif

  bool nextFramePrepared = false;

  bool reprojectionUseCameraTranslatedSpace = false;

  Texture *currentTarget = nullptr;
  TEXTUREID currentTargetId = BAD_TEXTUREID;
  d3d::SamplerHandle clampPointSampler = d3d::INVALID_SAMPLER_HANDLE;
};

OcclusionSystem *OcclusionSystem::create() { return new OcclusionSystemImpl; }

void OcclusionSystemImpl::close()
{
  ringTextures.close();
  downsample.clear();
  isVrResInited = false;
  downsample_vr.reset();
  if (moc)
    MaskedOcclusionCulling::Destroy(moc);
  moc = 0;
#if DAGOR_DBGLEVEL > 0
  reprojected.close();
  masked.close();
#endif
}

void OcclusionSystemImpl::initSWRasterization()
{
  if (!moc)
  {
    moc = MaskedOcclusionCulling::Create();
    moc->SetResolution(width * 4, height * 4);
    ////////////////////////////////////////////////////////////////////////////////////////
    // Print which version (instruction set) is being used
    ////////////////////////////////////////////////////////////////////////////////////////

    MaskedOcclusionCulling::Implementation implementation = moc->GetImplementation();
    switch (implementation)
    {
      case MaskedOcclusionCulling::SSE2: debug("Using SSE2 version\n"); break;
      case MaskedOcclusionCulling::SSE41: debug("Using SSE41 version\n"); break;
      case MaskedOcclusionCulling::NEON: debug("Using NEON version\n"); break;
      case MaskedOcclusionCulling::AVX2: debug("Using AVX2 version\n"); break;
    }
  }
}

void OcclusionSystemImpl::init()
{
  close();
  width = WIDTH;
  height = HEIGHT;
  currentCheckingFrame = 0;
  if (shader_exists("downsample_depth_hzb")) // we don't support GPU readback on this platform
  {
    downsample.init("downsample_depth_hzb");
    ringTextures.init(width, height, 3, "hzb",
      TEXCF_LINEAR_LAYOUT | TEXFMT_R32F | TEXCF_RTARGET | TEXCF_CPU_CACHED_MEMORY | TEXCF_UNORDERED);
    ringTextures.texaddr(TEXADDR_CLAMP);
    clear_and_resize(lastHWdepth, width * height);
    debug("use Coverage buffer/GPU HZB occlusion");
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    clampPointSampler = d3d::request_sampler(smpInfo);
  }
}

void OcclusionSystemImpl::reset()
{
  clear();
  ringTextures.reset();
  lastRenderedFrame = currentCheckingFrame = 0;
}

static const uint16_t box_indices[] = {
  1,
  3,
  2, // index for top
  0,
  1,
  2,
  7,
  5,
  6, // index for bottom
  5,
  4,
  6,
  0,
  6,
  4, // index for left
  0,
  2,
  6,
  1,
  5,
  7, // index for right
  1,
  7,
  3,
  0,
  4,
  5, // index for back
  0,
  5,
  1,
  2,
  7,
  6, // index for front
  2,
  3,
  7,
};

void OcclusionSystemImpl::rasterizeBoxes(mat44f_cref viewproj, const bbox3f *box, const mat43f *mat, int cnt)
{
  if (!moc)
    return;
  TIME_PROFILE(rasterize_boxes)
  for (int i = 0; i < cnt; ++i, box++, mat++)
  {
    mat44f clip;
    v_mat44_mul43(clip, viewproj, *mat);

    vec3f bmin = box->bmin, bmax = box->bmax;

    // int result = v_frustum_intersect(v_add(bmax, bmin), v_sub(bmax, bmin), clip);
    // if (!result)
    //   continue;
    vec4f minmax_x = v_perm_xXxX(bmin, bmax);
    vec4f minmax_y = v_perm_yyYY(bmin, bmax);
    vec4f minmax_z_0 = v_splat_z(bmin);
    vec4f minmax_z_1 = v_splat_z(bmax);

    // transform points to clip space
    vec4f points_cs[8];

    vis_transform_points_4(points_cs + 0, minmax_x, minmax_y, minmax_z_0, clip);
    v_mat44_transpose(points_cs[0], points_cs[1], points_cs[2], points_cs[3]);

    vis_transform_points_4(points_cs + 4, minmax_x, minmax_y, minmax_z_1, clip);
    v_mat44_transpose(points_cs[4], points_cs[5], points_cs[6], points_cs[7]);

    moc->RenderTriangles((float *)points_cs, box_indices, 12, nullptr, MaskedOcclusionCulling::BACKFACE_CW,
      MaskedOcclusionCulling::CLIP_PLANE_ALL);
  }
}

void OcclusionSystemImpl::rasterizeBoxes(mat44f_cref viewproj, const bbox3f *box, const mat44f *mat, int cnt)
{
  if (!moc)
    return;
  TIME_PROFILE(rasterize_boxes)
  for (int i = 0; i < cnt; ++i, box++, mat++)
  {
    mat44f clip;
    v_mat44_mul43(clip, viewproj, *mat);

    vec3f bmin = box->bmin, bmax = box->bmax;

    // int result = v_frustum_intersect(v_add(bmax, bmin), v_sub(bmax, bmin), clip);
    // if (!result)
    //   continue;
    vec4f minmax_x = v_perm_xXxX(bmin, bmax);
    vec4f minmax_y = v_perm_yyYY(bmin, bmax);
    vec4f minmax_z_0 = v_splat_z(bmin);
    vec4f minmax_z_1 = v_splat_z(bmax);

    // transform points to clip space
    vec4f points_cs[8];

    vis_transform_points_4(points_cs + 0, minmax_x, minmax_y, minmax_z_0, clip);
    v_mat44_transpose(points_cs[0], points_cs[1], points_cs[2], points_cs[3]);

    vis_transform_points_4(points_cs + 4, minmax_x, minmax_y, minmax_z_1, clip);
    v_mat44_transpose(points_cs[4], points_cs[5], points_cs[6], points_cs[7]);

    moc->RenderTriangles((float *)points_cs, box_indices, 12, nullptr, MaskedOcclusionCulling::BACKFACE_CW,
      MaskedOcclusionCulling::CLIP_PLANE_ALL);
  }
}

// both culling
static const uint16_t quad_indices[12] = {0, 1, 2, 1, 2, 3, 0, 2, 1, 1, 3, 2};

void OcclusionSystemImpl::rasterizeQuads(mat44f_cref viewproj, const vec4f *vert, int cnt)
{
  if (!moc)
    return;
  TIME_PROFILE(rasterize_quads)
  for (int i = 0; i < cnt; ++i, vert += 4)
  {
    vec4f points_cs[4] = {vert[0], vert[1], vert[2], vert[3]};
    v_mat44_transpose(points_cs[0], points_cs[1], points_cs[2], points_cs[3]);
    vis_transform_points_4(points_cs, points_cs[0], points_cs[1], points_cs[2], viewproj);
    // vec4f zclip = v_cmp_ge(points_cs[2], points_cs[3]);
    v_mat44_transpose(points_cs[0], points_cs[1], points_cs[2], points_cs[3]);
    // since, very limited amount of tris, better clip them all
    moc->RenderTriangles((float *)points_cs, quad_indices, 4, nullptr, MaskedOcclusionCulling::BACKFACE_CW,
      MaskedOcclusionCulling::CLIP_PLANE_ALL);
    // v_signmask(zclip) ? MaskedOcclusionCulling::CLIP_PLANE_ALL : MaskedOcclusionCulling::CLIP_PLANE_NONE);//CLIP_PLANE_ALL
  }
}

void OcclusionSystemImpl::rasterizeMesh(mat44f_cref viewproj, vec3f bmin, vec3f bmax, const vec4f *verts, int /*vert_cnt*/,
  const unsigned short *faces, int tri_count)
{
  if (!moc)
    return;
  TIME_PROFILE(rasterize_mesh)
  Frustum frustum(viewproj);
  int ret = frustum.testBox(bmin, bmax);
  if (!ret)
    return;
  moc->RenderTriangles((float *)verts, faces, tri_count, (float *)&viewproj, MaskedOcclusionCulling::BACKFACE_CW,
    ret == Frustum::INTERSECT ? MaskedOcclusionCulling::CLIP_PLANE_ALL : MaskedOcclusionCulling::CLIP_PLANE_NONE); // CLIP_PLANE_ALL
}

void OcclusionSystemImpl::startRasterization(float zn)
{
  if (!moc)
    return;
  moc->SetNearClipPlane(zn);
  moc->ClearBuffer();
}

void OcclusionSystemImpl::prepareNextFrame(vec3f view_pos, mat44f_cref view, mat44f_cref proj, mat44f_cref view_proj, float zn,
  float zf, TextureIDPair depth, Texture *base_depth)
{
  prepareNextFrameImpl<true>(view_pos, view, proj, view_proj, zn, zf, depth, base_depth, StereoIndex::Mono);
}

void OcclusionSystemImpl::prepareNextFrame(vec3f view_pos, mat44f_cref view, mat44f_cref proj, mat44f_cref view_proj, float zn,
  float zf, TextureIDPair depth, Texture *base_depth, StereoIndex stereo_index)
{
  prepareNextFrameImpl<false>(view_pos, view, proj, view_proj, zn, zf, depth, base_depth, stereo_index);
}

template <bool ignoreVr>
void OcclusionSystemImpl::prepareNextFrameImpl(vec3f view_pos, mat44f_cref view, mat44f_cref proj, mat44f_cref view_proj, float zn,
  float zf, TextureIDPair depth, Texture *base_depth, StereoIndex stereo_index)
{
  G_ASSERT_RETURN(depth.getTex2D(), );
  if (ignoreVr || stereo_index != StereoIndex::Right)
    currentTarget = ringTextures.getNewTargetAndId(lastRenderedFrame, currentTargetId);
  if (ignoreVr || stereo_index != StereoIndex::Left)
    nextFramePrepared = !!currentTarget;
  if (!currentTarget)
    return;

  if (ignoreVr || stereo_index == StereoIndex::Mono)
  {
    TIME_D3D_PROFILE(downsample_hzb);
    SCOPE_RENDER_TARGET;
    if (base_depth)
      d3d::resource_barrier({base_depth, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    TextureInfo tinfo;
    depth.getTex2D()->getinfo(tinfo, 0);

    const int w = tinfo.w, h = tinfo.h;

    int lod = 0;
    while (((w >> (lod + 1)) > width || (h >> (lod + 1)) > height) && lod < depth.getTex2D()->level_count() - 1)
      lod++;

    static int depthSourceSzVarId = get_shader_variable_id("depth_source_sz", true);
    static int depthTargetSzVarId = get_shader_variable_id("depth_target_sz", true);
    static int downsampleTypeIdVarId = get_shader_variable_id("downsampleTypeId", true);
    static int source_tex_const_no = -1;
    if (source_tex_const_no < 0)
    {
      if (!ShaderGlobal::get_int_by_name("downsample_depth_from_const_no", source_tex_const_no))
        source_tex_const_no = 0;
      debug("source_tex_const_no = %d", source_tex_const_no);
    }

    if (base_depth)
    {
      TextureInfo base_tinfo;
      base_depth->getinfo(base_tinfo, 0);
      G_ASSERTF((base_tinfo.w / 2 == tinfo.w && base_tinfo.h / 2 == tinfo.h) || (!tinfo.mipLevels && d3d::is_stub_driver()),
        "base_tinfo=%dx%d,L%d tinfo=%dx%d,L%d", base_tinfo.w, base_tinfo.h, base_tinfo.mipLevels, tinfo.w, tinfo.h, tinfo.mipLevels);

      Texture *sourceTex = base_depth;
      int srcSizeW = base_tinfo.w, srcSizeH = base_tinfo.h;
      bool finalRendered = false;
      for (int i = 0, e = depth.getTex2D()->level_count();; i++) // current lod
      {
        int targetW = srcSizeW >> 1, targetH = srcSizeH >> 1;
        int downsampleTypeId = 0;                    // 0 = final2x2, 1 = final4x4, 2 = down2x2, 3 = down4x4
        if ((targetW > width) || (targetH > height)) // we can skip one mip, by downsample 4x4.
        {
          downsampleTypeId = 0b01;
          targetW >>= 1;
          targetH >>= 1;
        }

        G_ASSERT(srcSizeW == (base_tinfo.w >> i) && srcSizeH == (base_tinfo.h >> i));
        Texture *targetTex;
        int renderTargetLod = 0;
        const bool final = (targetW <= width && targetH <= height) || (i == e);
        if (final) // we found final target
        {
          i = e;
          targetW = width;
          targetH = height;
          targetTex = currentTarget;
          finalRendered = true;
        }
        else
        {
          i += downsampleTypeId;
          downsampleTypeId |= 0b10;
          targetTex = depth.getTex2D();
          renderTargetLod = i;
          G_ASSERT((tinfo.w >> renderTargetLod) == targetW && (tinfo.h >> renderTargetLod) == targetH);
        }
        d3d::set_render_target(targetTex, renderTargetLod);
        d3d::settex(source_tex_const_no, sourceTex);
        d3d::set_sampler(STAGE_PS, source_tex_const_no, clampPointSampler);
        sourceTex = depth.getTex2D();

        ShaderGlobal::set_int(downsampleTypeIdVarId, downsampleTypeId);

        ShaderGlobal::set_color4(depthTargetSzVarId, targetW, targetH, 0, 0);
        ShaderGlobal::set_color4(depthSourceSzVarId, srcSizeW, srcSizeH, 0, 0);

        downsample.render();

        if (i >= e)
          break;

        srcSizeW = tinfo.w >> i;
        srcSizeH = tinfo.h >> i;
        depth.getTex2D()->texmiplevel(i, i);
        d3d::resource_barrier({depth.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(i), 1});
      }

      G_ASSERT(finalRendered == true);

      ShaderGlobal::set_int(downsampleTypeIdVarId, 0);
    }
    else
    {
      ShaderGlobal::set_color4(depthSourceSzVarId, tinfo.w >> lod, tinfo.h >> lod, 0, 0);
      ShaderGlobal::set_color4(depthTargetSzVarId, width, height, 0, 0);
      ShaderGlobal::set_int(downsampleTypeIdVarId, 0);

      depth.getTex2D()->texmiplevel(lod, lod);
      d3d::settex(source_tex_const_no, depth.getTex2D());
      d3d::set_sampler(STAGE_PS, source_tex_const_no, clampPointSampler);
      d3d::set_render_target(currentTarget, 0);

      downsample.render();
    }
    depth.getTex2D()->texmiplevel(-1, -1);
  }
  else
  {
    initVrRes();
    if (stereo_index != StereoIndex::Right)
    {
      TIME_D3D_PROFILE(clear_hzb);
      float clearValue[4] = {1.0f};
      d3d::clear_rwtexf(currentTarget, clearValue, 0, 0);
    }

    TIME_D3D_PROFILE(downsample_hzb);
    TextureInfo tinfo;
    depth.getTex2D()->getinfo(tinfo, 0);

    static int depthSourceSzVarId = get_shader_variable_id("depth_source_sz");
    static int depthTargetSzVarId = get_shader_variable_id("depth_target_sz");
    static int depthSourceVarId = get_shader_variable_id("source_depth_hzb_vr");
    static int depthTargetVarId = get_shader_variable_id("target_depth_hzb_vr");
    static int depthLodVarId = get_shader_variable_id("depth_lod");

    int widthLod = static_cast<int>(log2(float(tinfo.w) / width));
    int heightLod = static_cast<int>(log2(float(tinfo.h) / height));
    int lod = eastl::min<int>({widthLod, heightLod, depth.getTex2D()->level_count() - 1});

    {
      IPoint2 sourceSize(tinfo.w >> lod, tinfo.h >> lod);
      ShaderGlobal::set_color4(depthSourceSzVarId, sourceSize.x, sourceSize.y, 0, 0);
      ShaderGlobal::set_color4(depthTargetSzVarId, width, height, 0, 0);

      ShaderGlobal::set_int(depthLodVarId, lod);

      ShaderGlobal::set_texture(depthSourceVarId, depth.getId());
      ShaderGlobal::set_texture(depthTargetVarId, currentTargetId);

      downsample_vr->dispatchThreads(sourceSize.x, sourceSize.y, 1);
    }
  }

  if (ignoreVr || stereo_index != StereoIndex::Left)
  {
    TIME_D3D_PROFILE(start_cpu_copy_hzb);
    ringTextures.startCPUCopy();

    if (reprojectionUseCameraTranslatedSpace)
    {
      mat44f viewTm = view;
      viewTm.col3 = V_C_UNIT_0001;
      v_mat44_mul(globtmHistory[lastRenderedFrame % globtmHistory.size()].viewproj, proj, viewTm);
    }
    else
      globtmHistory[lastRenderedFrame % globtmHistory.size()].viewproj = view_proj;

    globtmHistory[lastRenderedFrame % globtmHistory.size()].pos = view_pos;
    globtmHistory[lastRenderedFrame % globtmHistory.size()].zn = zn;
    globtmHistory[lastRenderedFrame % globtmHistory.size()].zf = zf;
  }
}

void OcclusionSystemImpl::setReprojectionUseCameraTranslatedSpace(bool enabled) { reprojectionUseCameraTranslatedSpace = enabled; }

bool OcclusionSystemImpl::getReprojectionUseCameraTranslatedSpace() const { return reprojectionUseCameraTranslatedSpace; }

void OcclusionSystemImpl::initVrRes()
{
  if (isVrResInited)
    return;
  G_ASSERT(!downsample_vr);
  isVrResInited = true;
  downsample_vr.reset(new_compute_shader("downsample_depth_hzb_vr"));
}

bool OcclusionSystemImpl::process(float *destData, uint32_t &frame)
{
  TIME_D3D_PROFILE(readback_hzb);
  int stride;
  const uint8_t *data = ringTextures.lock(stride, frame);
  if (!data)
    return false;
  {
    TIME_PROFILE_DEV(cpu_copy_hzb);
    if (DAGOR_LIKELY(stride == width * sizeof(float)))
      memcpy(destData, data, height * stride);
    else
      for (int y = 0; y < height; ++y, destData += width, data += stride)
        memcpy(destData, data, width * sizeof(float));
  }
  ringTextures.unlock();
  return true;
}

void OcclusionSystemImpl::readGPUframe()
{
  if (!lastHWdepth.size()) // we don't support GPU readback on this platform
    return;
  process(lastHWdepth.data(), currentCheckingFrame);
}

void OcclusionSystemImpl::clear()
{
  TIME_PROFILE(occlusion_clear);
  OcclusionTest<WIDTH, HEIGHT>::clear();
}

void OcclusionSystemImpl::prepareGPUDepthFrame(vec3f pos, mat44f_cref view, mat44f_cref proj, mat44f_cref viewProj,
  float cockpit_distance, CockpitReprojectionMode cockpit_mode, const mat44f &cockpit_anim)
{
  {
    TIME_PROFILE(occlusion_clear);
    OcclusionRenderer<WIDTH, HEIGHT>::clear();
  }
  if (currentCheckingFrame > 0) // occlusion is only valid after first frame
  {
    {
      TIME_PROFILE(occlusion_reproject);
      int idx = currentCheckingFrame % globtmHistory.size();
      mat44f toWorldHW;
      v_mat44_inverse(toWorldHW, globtmHistory[idx].viewproj);

      mat44f &lastAnimHistoryMatrix = cockpitAnimHistory[lastRenderedFrame % cockpitAnimHistory.size()];
      if (nextFramePrepared)
        lastAnimHistoryMatrix = cockpit_anim;
      else
        v_mat44_mul43(lastAnimHistoryMatrix, cockpit_anim, lastAnimHistoryMatrix);
      mat44f actualAnim = lastAnimHistoryMatrix;
      for (int i = lastRenderedFrame - 1; i >= (int)currentCheckingFrame; --i)
        v_mat44_mul43(actualAnim, actualAnim, cockpitAnimHistory[i % cockpitAnimHistory.size()]);

      if (reprojectionUseCameraTranslatedSpace)
      {
        mat44f translationTm;
        v_mat44_ident(translationTm);
        translationTm.col3 = v_perm_xyzd(v_sub(globtmHistory[idx].pos, pos), V_C_ONE);
        mat44f viewTm = view;
        viewTm.col3 = V_C_UNIT_0001;
        v_mat44_mul(viewTm, viewTm, translationTm);
        mat44f viewProjTm;
        v_mat44_mul(viewProjTm, proj, viewTm);

        OcclusionRenderer<WIDTH, HEIGHT>::reprojectHWDepthBuffer(toWorldHW, v_zero(), globtmHistory[idx].zn, globtmHistory[idx].zf,
          viewProjTm, 0, height, lastHWdepth.data(), cockpit_distance, cockpit_mode, actualAnim);
      }
      else
      {
        OcclusionRenderer<WIDTH, HEIGHT>::reprojectHWDepthBuffer(toWorldHW, globtmHistory[idx].pos, globtmHistory[idx].zn,
          globtmHistory[idx].zf, viewProj, 0, height, lastHWdepth.data(), cockpit_distance, cockpit_mode, actualAnim);
      }
    }
  }
}

void OcclusionSystemImpl::combineWithSWRasterization()
{
  if (moc)
  {
    float *zb = OcclusionTest<WIDTH, HEIGHT>::getZbuffer(0);
    if (currentCheckingFrame > 0)
      moc->CombinePixelDepthBuffer2W(zb, WIDTH, HEIGHT);
    else
      moc->DecodePixelDepthBuffer2W(zb, WIDTH, HEIGHT);
  }
}

void OcclusionSystemImpl::buildMips()
{
  TIME_PROFILE(occlusion_buildMips);
  OcclusionTest<WIDTH, HEIGHT>::buildMips();
}

void OcclusionSystemImpl::prepareDebug()
{
#if DAGOR_DBGLEVEL > 0
  TIME_D3D_PROFILE(debug_occlusion);
  constexpr auto mip_chain_cnt = OcclusionTest<WIDTH, HEIGHT>::mip_chain_count;
  if (!reprojected.getTex2D())
  {
    uint32_t flags = TEXCF_READABLE | TEXCF_DYNAMIC | TEXCF_LINEAR_LAYOUT | TEXFMT_R32F;
    reprojected.set(d3d::create_tex(NULL, width, height, flags, mip_chain_cnt, "hzb_reprojected"), "hzb_reprojected");
    reprojected.getTex2D()->texaddr(TEXADDR_CLAMP);
    reprojected.getTex2D()->texfilter(TEXFILTER_POINT);
  }

  uint8_t *data;
  int stride;
  for (int mip = 0; mip < mip_chain_cnt; ++mip)
    if (reprojected.getTex2D()->lockimg((void **)&data, stride, mip, TEXLOCK_UPDATEFROMSYSTEX | TEXLOCK_RWMASK | TEXLOCK_SYSTEXLOCK))
    {
      const float *zbuffer = OcclusionTest<WIDTH, HEIGHT>::getZbuffer(mip);
      if (mip == 0 && debug_reprojection.get())
      {
        for (int i = 0; i < height; ++i, data += stride)
          memcpy(data, OcclusionRenderer<WIDTH, HEIGHT>::reprojectionDebug + i * width, width * sizeof(float));
      }
      else
      {
        for (int i = 0; i < (height >> mip); ++i, data += stride)
          memcpy(data, zbuffer + i * (width >> mip), (width >> mip) * sizeof(float));
      }
      reprojected.getTex2D()->unlockimg();
    }
#endif
}

void OcclusionSystemImpl::prepareDebugSWRasterization()
{
#if DAGOR_DBGLEVEL > 0
  if (!moc)
    return;
  TIME_D3D_PROFILE(debug_sw_occlusion);
  unsigned int mw, mh;
  moc->GetResolution(mw, mh);
  if (!masked.getTex2D())
  {
    uint32_t flags = TEXCF_READABLE | TEXCF_DYNAMIC | TEXCF_LINEAR_LAYOUT | TEXFMT_R32F;
    masked.set(d3d::create_tex(NULL, mw, mh, flags, 1, "masked_z"), "masked_z");
    masked.getTex2D()->texaddr(TEXADDR_CLAMP);
    masked.getTex2D()->texfilter(TEXFILTER_POINT);
  }

  uint8_t *data;
  int stride;
  if (masked.getTex2D()->lockimg((void **)&data, stride, 0, TEXLOCK_UPDATEFROMSYSTEX | TEXLOCK_RWMASK | TEXLOCK_SYSTEXLOCK))
  {
    float *depth = new float[mw * mh];
    moc->ComputePixelDepthBuffer(depth, false /* flipY */);

    for (int i = 0; i < mh; ++i, data += stride)
    {
      memcpy(data, depth + i * mw, mw * sizeof(float));
    }
    delete[] depth;
    masked.getTex2D()->unlockimg();
  }
#endif
}
