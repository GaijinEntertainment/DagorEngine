// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_draw.h>
#include <generic/dag_sort.h>
#include <math/dag_vecMathCompatibility.h>
#include <3d/dag_textureIDHolder.h>
#include <util/dag_bitArray.h>
#include <memory/dag_framemem.h>
#include <EASTL/fixed_vector.h>
#include <math/dag_adjpow2.h>
#include <shaders/dag_postFxRenderer.h>
#include <perfMon/dag_statDrv.h>
#include <render/shadowSystem.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaders.h>
#include <render/spotLight.h>
#include <shaders/dag_shaderBlock.h>

static const float downgrade_quality_step = 0.85f, upgrade_quality_step = 1.05f;
static const float occupancy_to_upgrade_quality = 0.5f;
static const float min_downgraded_quality = 0.125f;  // 1/8 for each side = 1/64
static const float max_upgraded_quality = 2.0f;      // 2 for each side = 4x
static const unsigned frames_to_change_quality = 31; // do not upgrade quality more often than each second

#define SHADOW_SYSTEM_SHADER_VARS         \
  VAR(octahedral_texture_size)            \
  VAR(octahedral_shadow_zn_zfar)          \
  VAR(dynamic_light_shadows)              \
  VAR(dynamic_light_shadows_samplerstate) \
  VAR(octahedral_temp_shadow)

#define VAR(a) static int a##VarId = -1;
SHADOW_SYSTEM_SHADER_VARS
#undef VAR

#define DEBUG_OCCUPANCY(...)
// #define DEBUG_OCCUPANCY debug

static mat44f buildTexTM(uint16_t ofsX, uint16_t ofsY, uint16_t sw, uint16_t sh, uint16_t tex_w, uint16_t tex_h);

ShadowSystem::ShadowSystem(const char *name_suffix) : nameSuffix(name_suffix)
{
  octahedralPacker.init("octahedral_shadow_packer");
  v_bbox3_init_empty(activeShadowVolume);
  volumesToUpdate.reserve(32); // allocate 64 bytes, should be enough for 99% cases
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  smpInfo.filter_mode = d3d::FilterMode::Compare;
  shadowSampler = d3d::request_sampler(smpInfo);

  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_FUNC);
  state.zFunc = CMPF_ALWAYS;
  cmpfAlwaysNoBiasStateId.reset(shaders::overrides::create(state));
}

void ShadowSystem::changeResolution(int atlasW, int max_shadow_size, int min_shadow_size, int shadow_size_step,
  bool dynamic_shadow_32bit)
{
  shadow_size_step = (shadow_size_step + 15) & (~15); // we can use filter of 8 texels, so 16 pixels is minimum reasonable size
  shadow_size_step = min(shadow_size_step, int(16));
  atlasW = atlasW - (atlasW % shadow_size_step);
  max_shadow_size += shadow_size_step - 1;
  max_shadow_size -= max_shadow_size % shadow_size_step;
  min_shadow_size += shadow_size_step - 1;
  min_shadow_size -= min_shadow_size % shadow_size_step;
  TextureInfo tinfo;
  tinfo.w = 0;
  if (dynamic_light_shadows.getTex2D())
    dynamic_light_shadows.getTex2D()->getinfo(tinfo, 0);

  G_ASSERT(shadow_size_step < max_shadow_size);
  G_ASSERT(atlasW >= max_shadow_size);
  G_ASSERT(min_shadow_size <= max_shadow_size);

  shadowStep = shadow_size_step;
  maxShadow = max_shadow_size;
  minShadow = min_shadow_size;
  atlasWidth = atlasW;
  atlasHeight = atlasW;
  maxOctahedralTempShadow = max_shadow_size;
  bool texFormatChanged = uses32bitShadow != dynamic_shadow_32bit;
  uses32bitShadow = dynamic_shadow_32bit;
  uint32_t textureFormat = uses32bitShadow ? TEXFMT_DEPTH32 : TEXFMT_DEPTH16;


#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  SHADOW_SYSTEM_SHADER_VARS
#undef VAR

  if (tinfo.w != atlasWidth || tinfo.h != atlasHeight || texFormatChanged)
  {
    dynamic_light_shadows.close();
    dynamic_light_shadows =
      dag::create_tex(NULL, atlasWidth, atlasHeight, textureFormat | TEXCF_RTARGET, 1, getResName("dynamic_light_shadows").c_str());
    dynamic_light_shadows->disableSampler();
    ShaderGlobal::set_texture(dynamic_light_shadowsVarId, dynamic_light_shadows);
    ShaderGlobal::set_sampler(dynamic_light_shadows_samplerstateVarId, shadowSampler);
    d3d::resource_barrier({dynamic_light_shadows.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
    invalidateAllVolumes();
  }
  tinfo.w = 0;
  if (tempCopy.getTex2D())
    tempCopy.getTex2D()->getinfo(tinfo, 0);
  // todo: we only need it on old API, otherwise can just copy
  if (maxShadow != tinfo.w || texFormatChanged)
  {
    tempCopy.close();
    tempCopy =
      dag::create_tex(NULL, maxShadow, maxShadow, textureFormat | TEXCF_RTARGET, 1, getResName("temp_dynamic_light_shadows").c_str());
    tempCopy->disableSampler();

    octahedral_temp_shadow.close();
    octahedral_temp_shadow = dag::create_array_tex(maxOctahedralTempShadow, maxOctahedralTempShadow, 6, textureFormat | TEXCF_RTARGET,
      1, getResName("octahedral_temp_shadow").c_str());
    octahedral_temp_shadow->disableSampler();
    ShaderGlobal::set_texture(octahedral_temp_shadowVarId, octahedral_temp_shadow);
  }
  if (!copyDepth)
  {
    copyDepth = new PostFxRenderer;
    copyDepth->init("copy_depth_region");
  }
  if (!trace_shadow_depth_region.getElem())
    trace_shadow_depth_region.init("trace_shadow_depth_region", true);
}

void ShadowSystem::close()
{
  cmpfAlwaysStateId.reset();
  cmpfAlwaysNoBiasStateId.reset();
  pruneFreeVolumes();
  G_ASSERTF(volumes.size() == 0 && freeVolumes.size() == 0, "some volumes were not removed %d volumes left (%d free)", volumes.size(),
    freeVolumes.size());
  maxShadow = minShadow = shadowStep = maxOctahedralTempShadow = 0;
  dynamic_light_shadows.close();
  octahedral_temp_shadow.close();
  octahedralPacker.clear();
  del_it(copyDepth);
}

void ShadowSystem::setOverrideState(const shaders::OverrideState &baseState)
{
  shaders::OverrideState state = baseState;
  state.set(shaders::OverrideState::Z_FUNC);
  state.zFunc = CMPF_ALWAYS;
  cmpfAlwaysStateId.reset(shaders::overrides::create(state));
}

int ShadowSystem::allocateVolume(ShadowCastersFlag casters, bool hint_dynamic, uint16_t quality, uint8_t priority,
  uint8_t shadow_size_srl, DynamicShadowRenderGPUObjects render_gpu_objects)
{
  int id = -1;
  if (freeVolumes.size())
  {
    id = freeVolumes.back();
    freeVolumes.pop_back();
    G_ASSERT(volumes[id].isDestroyed());
  }
  else
  {
    id = appendVolume();
  }
  auto &volume = volumes[id];
  volume.flags = ((casters & DYNAMIC_CASTERS) ? 0 : volume.ONLY_STATIC) | (hint_dynamic ? volume.DYNAMIC_LIGHT : 0) |
                 ((casters & APPROXIMATE_STATIC_CASTERS) ? volume.APPROXIMATE_STATIC : 0);
  volume.quality = (uint8_t)min(quality + 1, 255);
  volume.sizeShift = min(shadow_size_srl, uint8_t(get_log2i(maxShadow / minShadow)));
  volume.priority = min(priority, (uint8_t)MAX_PRIORITY);
  volume.renderGPUObjects = render_gpu_objects;
  volume.invalidate();
  return id;
}

void ShadowSystem::destroyVolume(uint32_t id)
{
  G_ASSERT_RETURN(id < volumes.size(), );
  G_ASSERT_RETURN(!volumes[id].isDestroyed(), );
  {
    Volume &volume = volumes[id];
    atlasFree(volume.dynamicShadow);
    volume.dynamicShadow.reset();

    atlasFree(volume.shadow);
    volume.shadow.reset();
  }
  freeVolumes.push_back(id);
  volumes[id].invalidate();
  volumes[id].lastFrameUsed = 0;
  volumesToUpdate.erase(id);
}

bool ShadowSystem::getShadowProperties(uint32_t id, ShadowCastersFlag &casters, bool &hint_dynamic, uint16_t &quality,
  uint8_t &priority, uint8_t &shadow_size_srl, DynamicShadowRenderGPUObjects &render_gpu_objects) const
{
  G_ASSERT_RETURN(id < volumes.size() && !volumes[id].isDestroyed(), false);
  auto &volume = volumes[id];
  hint_dynamic = volume.isDynamic();
  casters = ShadowCastersFlag((volume.isApproximatelyTracedStaticCasters() ? APPROXIMATE_STATIC_CASTERS : STATIC_CASTERS) |
                              (volume.hasOnlyStaticCasters() ? 0 : DYNAMIC_CASTERS));
  quality = volumes[id].quality - 1;
  shadow_size_srl = volumes[id].sizeShift;
  priority = volumes[id].priority;
  render_gpu_objects = volumes[id].renderGPUObjects;
  return true;
}

Point2 ShadowSystem::getShadowUvSize(uint32_t id) const
{
  const AtlasRect &shadow = volumes[id].dynamicShadow.isEmpty() ? volumes[id].shadow : volumes[id].dynamicShadow;
  return Point2(safediv((float)shadow.width, (float)atlasWidth), safediv((float)shadow.height, (float)atlasHeight));
}

Point4 ShadowSystem::getShadowUvMinMax(uint32_t id) const
{
  const AtlasRect &shadow = volumes[id].dynamicShadow.isEmpty() ? volumes[id].shadow : volumes[id].dynamicShadow;
  return Point4(safediv((float)shadow.x, (float)atlasWidth), safediv((float)shadow.y, (float)atlasHeight),
    safediv((float)shadow.x + (float)shadow.width, (float)atlasWidth),
    safediv((float)shadow.y + (float)shadow.height, (float)atlasHeight));
}

void ShadowSystem::Volume::buildProj(mat44f &proj) const { v_mat44_make_persp_reverse(proj, wk, wk, zn, zf); }

uint32_t ShadowSystem::Volume::getNumViews() const { return isOctahedral() ? 6 : 1; }

void ShadowSystem::Volume::buildView(mat44f &view, mat44f &invView, uint32_t view_id) const
{
  invView = invTm;
  if (isOctahedral() && view_id > 0)
  {
    // view_id = 0 => forward
    if (view_id < 4)
    {
      static const Point2 cosSinValues[] = {{0, 1}, {-1, 0}, {0, -1}};
      float c = cosSinValues[view_id - 1].x;
      float s = cosSinValues[view_id - 1].y;
      vec4f vs = v_splats(s);
      vec4f vc = v_splats(c);
      invView.col0 = v_msub(vc, invTm.col0, v_mul(vs, invTm.col2));
      invView.col2 = v_madd(vs, invTm.col0, v_mul(vc, invTm.col2));
    }
    else if (view_id == 4)
    {
      // Rotate around X in either direction
      invView.col1 = v_neg(invTm.col2);
      invView.col2 = invTm.col1;
    }
    else
    {
      invView.col1 = invTm.col2;
      invView.col2 = v_neg(invTm.col1);
    }
  }
  v_mat44_orthonormal_inverse43_to44(view, invView);
}

void ShadowSystem::Volume::buildViewProj(mat44f &viewproj, uint32_t view_id) const
{
  mat44f proj, view, invView;
  buildView(view, invView, view_id);
  buildProj(proj);
  v_mat44_mul(viewproj, proj, view);
}

void ShadowSystem::invalidateVolumeShadow(uint32_t id)
{
  G_ASSERT_RETURN(id < volumes.size() != 0, );
  G_ASSERT_RETURN(!volumes[id].isDestroyed(), );
  volumes[id].lastFrameChanged = volumes[id].lastFrameUpdated + 1; // invalidate
}

void ShadowSystem::setShadowVolume(uint32_t id, mat44f_cref viewITM, float zn, float zf, float wk, bbox3f_cref obb)
{
  G_ASSERT_RETURN(id < volumes.size() != 0, );
  G_ASSERT_RETURN(!volumes[id].isDestroyed(), );

  Volume &volume = volumes[id];
  volume.invTm = viewITM;
  volume.zn = zn;
  volume.zf = zf;
  volume.wk = wk;
  volume.lastFrameChanged = currentFrame; // may be check is set same params
  volume.flags &= ~volume.OCTAHEDRAL;

  // wk is 1/tan(halfAngle)
  float tanHalfAngle = 1. / wk;
  float cosHalfAngle = 1. / sqrtf(1 + tanHalfAngle * tanHalfAngle);
  float sinHalfAngle = tanHalfAngle * cosHalfAngle;
  SpotLight::BoundingSphereDescriptor boundingSphereDesc = SpotLight::get_bounding_sphere_description(zf, sinHalfAngle, cosHalfAngle);
  vec3f boundingSphereCenter = v_madd(viewITM.col2, v_splats(boundingSphereDesc.boundingSphereOffset), viewITM.col3);
  volumesSphere[id] = v_perm_xyzd(boundingSphereCenter, v_rot_1(v_set_x(boundingSphereDesc.boundSphereRadius)));

  mat44f viewproj, proj, view, invView;
  volume.buildView(view, invView, 0);
  volume.buildProj(proj);
  v_mat44_mul(viewproj, proj, view);

  Frustum frustum(viewproj);
  bbox3f box;
  frustum.calcFrustumBBox(box);
  volumesFrustum[id] = frustum;
  if (v_test_vec_x_ge(obb.bmax, obb.bmin)) // not empty
  {
    bbox3f wbb;
    v_bbox3_init(wbb, invView, obb);
    volumesBox[id].bmax = v_min(box.bmax, wbb.bmax);
    volumesBox[id].bmin = v_max(box.bmin, wbb.bmin);
  }
  else
    volumesBox[id] = box;
}

void ShadowSystem::setOctahedralShadowVolume(uint32_t id, vec3f pos, float zn, float zf, bbox3f_cref obb)
{
  G_ASSERT_RETURN(id < volumes.size() != 0, );
  G_ASSERT_RETURN(!volumes[id].isDestroyed(), );

  mat44f viewITM;
  v_mat44_ident(viewITM);
  viewITM.col3 = pos;

  Volume &volume = volumes[id];
  volume.invTm = viewITM;
  volume.zn = zn;
  volume.zf = zf;
  volume.wk = 1;
  volume.lastFrameChanged = currentFrame; // may be check is set same params
  volume.flags |= volume.OCTAHEDRAL;


  vec4f pos_radius = v_perm_xyzd(pos, v_rot_1(v_set_x(M_SQRT2 * zf)));
  volumesSphere[id] = pos_radius;

  mat44f viewproj, proj, view, invView;
  volume.buildView(view, invView, 0);
  view.col3 = v_madd(view.col2, v_splats(zf), view.col3);
  v_mat44_make_ortho(proj, volume.wk * zf * 2, volume.wk * zf * 2, 0, zf * 2);
  v_mat44_mul(viewproj, proj, view);

  // orthogonal frustum that contains exactly the 6 frustums from the 6 sides
  Frustum frustum(viewproj);
  bbox3f box;
  vec4f boxSize = v_splats(zf);
  box.bmin = v_sub(pos_radius, boxSize);
  box.bmax = v_add(pos_radius, boxSize);

  bbox3f box2;
  frustum.calcFrustumBBox(box2);
  volumesFrustum[id] = frustum;
  if (v_test_vec_x_ge(obb.bmax, obb.bmin)) // not empty
  {
    bbox3f wbb;
    v_bbox3_init(wbb, invView, obb);
    volumesBox[id].bmax = v_min(box.bmax, wbb.bmax);
    volumesBox[id].bmin = v_max(box.bmin, wbb.bmin);
  }
  else
    volumesBox[id] = box;
}

void ShadowSystem::setShadowVolumeDynamicContentChanged(uint32_t id)
{
  G_ASSERT_RETURN(id < volumes.size() != 0, );
  G_ASSERT_RETURN(!volumes[id].isDestroyed(), );
  // G_ASSERTF(0, "not implemented yet, mark as dynamic content has changed");
  volumes[id].lastFrameHasDynamicContent = currentFrame;
}

void ShadowSystem::invalidateStaticObjects(bbox3f_cref box)
{
  for (int i = 0; i < volumes.size(); ++i)
  {
    if (volumes[i].isDestroyed() || !volumes[i].isValidContent()) // if it is invalid it will be re-rendered anyway
      continue;
    if (!v_bbox3_test_box_intersect(volumesBox[i], box))
      continue;
    volumes[i].lastFrameChanged = volumes[i].lastFrameUpdated + 1;
  }
}

void ShadowSystem::setDynamicObjectsContent(const bbox3f *boxes, int count)
{
  if (!count || !volumesToUpdate.size())
    return;

  bbox3f visibleVolumesBox;
  v_bbox3_init_empty(visibleVolumesBox);
  for (const auto &id : volumesToUpdate)
  {
    if (!volumes[id].hasOnlyStaticCasters())
      v_bbox3_add_box(visibleVolumesBox, volumesBox[id]);
  }
  bbox3f allBoxes;
  v_bbox3_init_empty(allBoxes);

  Tab<bbox3f> boxesAround2(framemem_ptr());
  boxesAround2.reserve(count);
  for (int j = 0; j < count; ++j)
  {
    if (!v_bbox3_test_box_intersect(visibleVolumesBox, boxes[j]))
      continue;
    v_bbox3_add_box(allBoxes, boxes[j]);
    bbox3f box2;
    box2.bmin = v_add(boxes[j].bmax, boxes[j].bmin);
    box2.bmax = v_sub(boxes[j].bmax, boxes[j].bmin);
    boxesAround2.push_back(box2);
  }
  vec3f center2 = v_add(allBoxes.bmax, allBoxes.bmin);
  vec3f extent2 = v_sub(allBoxes.bmax, allBoxes.bmin);
  for (const auto &id : volumesToUpdate)
  {
    if (volumes[id].hasOnlyStaticCasters())
      continue;
    const Frustum &frustum = volumesFrustum[id];
    if (!frustum.testBoxExtentB(center2, extent2))
      continue;
    int j = 0;
    for (; j < boxesAround2.size(); ++j)
      if (frustum.testBoxExtentB(boxesAround2[j].bmin, boxesAround2[j].bmax))
        break;
    if (j != boxesAround2.size())
      setShadowVolumeDynamicContentChanged(id);
  }
  if (v_bbox3_test_box_intersect(allBoxes, visibleVolumesBox))
  {
    activeShadowVolume.bmax = v_min(allBoxes.bmax, visibleVolumesBox.bmax);
    activeShadowVolume.bmin = v_max(allBoxes.bmin, visibleVolumesBox.bmin);
  }
  else
    v_bbox3_init_empty(activeShadowVolume);
}


struct TraceLightInfo
{
  uint16_t x, y, w, h;
  float zn, zf;
  float posType[4];
  float extentOrView0[4];
  float viewHor[4], viewVer[4];
};

void ShadowSystem::startRenderVolumes(const dag::ConstSpan<uint16_t> &volumesToRender)
{
  if (!volumesToRender.size())
    return;
  if (trace_shadow_depth_region.getElem())
  {
    TIME_D3D_PROFILE(trace_shadow_depth);
    TextureInfo tinfo;
    dynamic_light_shadows.getTex2D()->getinfo(tinfo, 0);
    G_STATIC_ASSERT(sizeof(TraceLightInfo) % 16 == 0);
    eastl::fixed_vector<TraceLightInfo, 8, true, framemem_allocator> traceAreas; // we are usually not updating all volumes at once
    for (auto i = volumesToRender.end() - 1, e = volumesToRender.begin() - 1; i > e; --i)
    {
      auto id = *i;
      const Volume &volume = volumes[id];
      if (!volume.isApproximatelyTracedStaticCasters())
        continue;
      traceAreas.emplace_back(prepareApproximateStaticCasters(volume, tinfo.w, tinfo.h));
      // auto flags = getVolumeRenderFlags(id);
      // if (!(flags & RENDER_DYNAMIC))
      //   volumes.erase_unsorted(i);
    }
    if (!traceAreas.empty())
    {
      // trace all lights in one render

      //! gets currently set shader block ID for a layer
      const int oldBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
      shaders::OverrideStateId originalState = shaders::overrides::get_current();
      shaders::overrides::reset();
      shaders::overrides::set(cmpfAlwaysNoBiasStateId);
      d3d::set_render_target((Texture *)NULL, 0);
      d3d::set_depth(dynamic_light_shadows.getTex2D(), DepthAccess::RW);
      const int sizeInRegs = sizeof(TraceLightInfo) / 16;
      const int quant = d3d::set_vs_constbuffer_size(51 * sizeInRegs) / sizeInRegs; // 51 must match shader
      G_ASSERT(quant > 0);
      ShaderGlobal::set_color4(octahedral_texture_sizeVarId, tinfo.w, tinfo.h, 1. / tinfo.w, 1. / tinfo.h);

      d3d::setvsrc(0, 0, 0);
      trace_shadow_depth_region.getElem()->setStates(0, true);
      for (int i = 0, e = traceAreas.size(); i < e; i += quant)
      {
        const uint32_t cnt = min<int>(e - i, quant);
        d3d::set_vs_const(0, &traceAreas[i], cnt * sizeInRegs);
        // todo: render in quads on consoles, saves shader ops
        d3d::draw(PRIM_TRILIST, 0, cnt * 2);
      }
      shaders::overrides::set(originalState);
      d3d::resource_barrier({dynamic_light_shadows.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
      d3d::set_vs_constbuffer_size(0);
      ShaderGlobal::setBlock(oldBlock, ShaderGlobal::LAYER_FRAME);
    }
  }
  {
    TIME_D3D_PROFILE(copy_depth);
    for (auto id : volumesToRender)
    {
      Volume &volume = volumes[id];
      // todo: make condition more clear
      if (!volume.isValidContent() || !volume.isStaticLightWithDynamicContent(currentFrame) || volume.dynamicShadow.isEmpty())
        continue;
      copyStaticToDynamic(volume);
    }
  }
  d3d::set_render_target((Texture *)NULL, 0);
  d3d::set_depth(dynamic_light_shadows.getTex2D(), DepthAccess::RW);
}

void ShadowSystem::copyStaticToDynamic(const Volume &volume)
{
  G_ASSERT_RETURN(!volume.shadow.isEmpty() && !volume.dynamicShadow.isEmpty(), );
  copyAtlasRegion(volume.shadow.x, volume.shadow.y, volume.dynamicShadow.x, volume.dynamicShadow.y, volume.shadow.width,
    volume.shadow.height);
}

void ShadowSystem::copyAtlasRegion(int src_x, int src_y, int dst_x, int dst_y, int w, int h)
{
  // can be done on modern API without renders
  // or can be done on compute
  shaders::overrides::set(cmpfAlwaysStateId);
  G_ASSERT_RETURN(w <= maxShadow && h <= maxShadow, );
  G_ASSERT_RETURN(copyDepth, );
  d3d::set_render_target((Texture *)NULL, 0);
  d3d::set_depth(tempCopy.getTex2D(), DepthAccess::RW);
  int v_from[4] = {src_x, src_y, 0, 0};
  d3d::set_ps_const(77, (float *)v_from, 1);
  if (w < maxShadow || h < maxShadow)
    d3d::setview(0, 0, w, h, 0, 1);
  d3d::resource_barrier({dynamic_light_shadows.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  d3d::settex(15, dynamic_light_shadows.getTex2D());
  d3d::set_sampler(STAGE_PS, 15, shadowSampler);
  copyDepth->render();

  d3d::set_render_target((Texture *)NULL, 0);
  d3d::set_depth(dynamic_light_shadows.getTex2D(), DepthAccess::RW);
  d3d::setview(dst_x, dst_y, w, h, 0, 1);
  d3d::resource_barrier({tempCopy.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  d3d::settex(15, tempCopy.getTex2D());
  int v[4] = {-dst_x, -dst_y, 0, 0};
  d3d::set_ps_const(77, (float *)v, 1);
  copyDepth->render();
  shaders::overrides::reset();
}

void ShadowSystem::endRenderVolumes()
{
  d3d::resource_barrier({dynamic_light_shadows.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
}

void ShadowSystem::startRenderTempShadow()
{
  d3d::set_render_target((Texture *)NULL, 0);
  d3d::set_depth(tempCopy.getTex2D(), DepthAccess::RW);
  d3d::setview(0, 0, maxShadow, maxShadow, 0, 1);
  d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0);
}

void ShadowSystem::getVolumeUpdateData(uint32_t id, dynamic_shadow_render::FrameUpdate &result)
{
  const auto &volume = volumes[id];
  const uint32_t numViews = volume.getNumViews();
  G_ASSERT(numViews <= result.views.size());

  for (uint32_t viewId = 0; viewId < numViews; ++viewId)
    volume.buildView(result.views[viewId].view, result.views[viewId].invView, viewId);

  volume.buildProj(result.proj);
  result.maxDrawDistance = volume.zf;
  result.numViews = static_cast<int>(numViews);
  result.renderGPUObjects = volume.renderGPUObjects;
}

ShadowSystem::RenderFlags ShadowSystem::getVolumeRenderFlags(uint32_t id)
{
  Volume &volume = volumes[id];
  const RenderFlags staticFlag =
    trace_shadow_depth_region.getElem() && volume.isApproximatelyTracedStaticCasters() ? RENDER_NONE : RENDER_STATIC;
  if (volume.isDynamicOrOnlyStaticCasters())
  {
    G_ASSERT(volume.isDynamic() || !volume.isValidContent());
    return volume.hasOnlyStaticCasters() ? staticFlag : RenderFlags(staticFlag | RENDER_DYNAMIC);
  }

  if (!volume.dynamicShadow.isEmpty())
  {
    if (volume.hasDynamicContent(currentFrame))
      return volume.isValidContent() ? RENDER_DYNAMIC : RenderFlags(staticFlag | RENDER_DYNAMIC);
    else
    {
      G_ASSERT(!volume.isValidContent());
      return staticFlag;
    }
  }

  G_ASSERT(!volume.isValidContent());
  return staticFlag;
}

uint32_t ShadowSystem::startRenderVolume(uint32_t id, mat44f &proj, RenderFlags &render_flags)
{
  G_ASSERT_RETURN(id < volumes.size() != 0, 0);
  Volume &volume = volumes[id];
  G_ASSERTF(!volume.shadow.isEmpty(), "volume %d", id);
  G_ASSERT_RETURN(!volume.shadow.isEmpty(), 0);
  volume.buildProj(proj);
  render_flags = getVolumeRenderFlags(id);

  if (!volume.isOctahedral())
  {
    mat44f viewproj;
    mat44f invView;
    mat44f view;
    volume.buildView(view, invView, 0);
    v_mat44_mul(viewproj, proj, view);
    mat44f texTM;
    if (volume.isStaticLightWithDynamicContent(currentFrame) && !volume.dynamicShadow.isEmpty())
    {
      texTM = buildTexTM(volume.dynamicShadow.x, volume.dynamicShadow.y, volume.dynamicShadow.width, volume.dynamicShadow.height,
        atlasWidth, atlasHeight);
    }
    else
      texTM = buildTexTM(volume.shadow.x, volume.shadow.y, volume.shadow.width, volume.shadow.height, atlasWidth, atlasHeight);

    mat44f valumeTexTM;
    v_mat44_mul(valumeTexTM, texTM, viewproj);
    v_mat44_transpose(volumesTexTM[id], valumeTexTM);
    G_ASSERTF(uint32_t(volume.shadow.x) < uint32_t(atlasWidth) && uint32_t(volume.shadow.y) < atlasHeight &&
                volume.shadow.x + volume.shadow.width <= atlasWidth && volume.shadow.y + volume.shadow.height <= atlasHeight,
      "rect %dx%d %dx%d", volume.shadow.x, volume.shadow.y, volume.shadow.width, volume.shadow.height);
  }
  else
  {
    int offsetX;
    int offsetY;
    int sizeX;
    int sizeY;
    if (volume.isStaticLightWithDynamicContent(currentFrame) && !volume.dynamicShadow.isEmpty())
    {
      offsetX = volume.dynamicShadow.x;
      offsetY = volume.dynamicShadow.y;
      sizeX = volume.dynamicShadow.width;
      sizeY = volume.dynamicShadow.height;
    }
    else
    {
      offsetX = volume.shadow.x;
      offsetY = volume.shadow.y;
      sizeX = volume.shadow.width;
      sizeY = volume.shadow.height;
    }
    G_ASSERTF(OMNI_SHADOW_MARGIN * 2 < sizeX && OMNI_SHADOW_MARGIN * 2 < sizeY, "The margin is larger than the texture itself");
    int margin = min(OMNI_SHADOW_MARGIN, min(sizeX, sizeY) / 2);
    volumesOctahedralData[id] = Point4(float(offsetX + margin) / atlasWidth, float(offsetY + margin) / atlasHeight,
      float(sizeX - margin * 2) / atlasWidth, float(sizeY - margin * 2) / atlasHeight);
  }
  if (!volume.isValidContent() || volume.isDynamicOrOnlyStaticCasters())
  {
    d3d::setview(volume.shadow.x, volume.shadow.y, volume.shadow.width, volume.shadow.height, 0, 1);
    // octahedral shadows are rendered to intermediate texture if using rasterization, or with z-always if using tracing
    // so depth clear not needed at all, as first pack/(trace) to octahedral dest now uses z-func always
    if (!volume.isOctahedral() && !volume.isApproximatelyTracedStaticCasters())
      d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0);
  }
  else if (volume.isStaticLightWithDynamicContent(currentFrame))
  {
    G_ASSERTF(!volume.dynamicShadow.isEmpty(), "if we here, we actually should not render anything at all");
    d3d::setview(volume.dynamicShadow.x, volume.dynamicShadow.y, volume.dynamicShadow.width, volume.dynamicShadow.height, 0, 1);
  }
  else
  {
    G_ASSERT(0);
  }

  return volume.getNumViews();
}

IPoint2 ShadowSystem::getOctahedralTempShadowExtent(const Volume &volume) const
{
  if (!volume.isValidContent() || volume.isDynamicOrOnlyStaticCasters())
  {
    return IPoint2(min(volume.shadow.width, maxOctahedralTempShadow), min(volume.shadow.height, maxOctahedralTempShadow));
  }
  else if (volume.isStaticLightWithDynamicContent(currentFrame))
  {
    G_ASSERTF(!volume.dynamicShadow.isEmpty(), "if we here, we actually should not render anything at all");
    return IPoint2(min(volume.dynamicShadow.width, maxOctahedralTempShadow),
      min(volume.dynamicShadow.height, maxOctahedralTempShadow));
  }
  else
  {
    G_ASSERT(0);
  }
  return IPoint2(maxOctahedralTempShadow, maxOctahedralTempShadow);
}

void ShadowSystem::startRenderVolumeView(uint32_t id, uint32_t view_id, mat44f &invView, mat44f &view, RenderFlags render_flags,
  RenderFlags current_render_type)
{
  G_ASSERT(current_render_type == RENDER_STATIC || current_render_type == RENDER_DYNAMIC);
  Volume &volume = volumes[id];
  volume.buildView(view, invView, view_id);

  if (volume.isOctahedral())
  {
    d3d::set_render_target(nullptr, 0);
    d3d::set_depth(octahedral_temp_shadow.getArrayTex(), view_id, DepthAccess::RW);
    IPoint2 extent = getOctahedralTempShadowExtent(volume);
    d3d::setview(0, 0, extent.x, extent.y, 0, 1);
    if (current_render_type == RENDER_STATIC || !(render_flags & RENDER_STATIC))
      d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0);
  }
}

void ShadowSystem::endRenderVolumeView(uint32_t, uint32_t) {}

void ShadowSystem::startRenderDynamic(uint32_t id)
{
  G_ASSERT_RETURN(id < volumes.size() != 0, );
  Volume &volume = volumes[id];
  G_ASSERT(volume.isDynamic() || (volume.isStaticLightWithDynamicContent(currentFrame) && !volume.dynamicShadow.isEmpty()));
  if (volume.isStaticLightWithDynamicContent(currentFrame) && !volume.dynamicShadow.isEmpty())
    copyStaticToDynamic(volume);
}

void ShadowSystem::packOctahedral(uint16_t id, bool dynamic_content)
{
  Volume &volume = volumes[id];
  if (!volume.isOctahedral())
    return;
  d3d::set_depth(dynamic_light_shadows.getTex2D(), DepthAccess::RW);
  d3d::resource_barrier({octahedral_temp_shadow.getArrayTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  if (dynamic_content)
  {
    G_ASSERTF(!volume.dynamicShadow.isEmpty(), "if we here, we actually should not render anything at all");
    d3d::setview(volume.dynamicShadow.x, volume.dynamicShadow.y, volume.dynamicShadow.width, volume.dynamicShadow.height, 0, 1);
  }
  else
    d3d::setview(volume.shadow.x, volume.shadow.y, volume.shadow.width, volume.shadow.height, 0, 1);

  IPoint2 extent = getOctahedralTempShadowExtent(volume);
  G_ASSERTF(OMNI_SHADOW_MARGIN * 2 < extent.x && OMNI_SHADOW_MARGIN * 2 < extent.y, "The margin is larger than the texture itself");
  int margin = min(OMNI_SHADOW_MARGIN, min(extent.x, extent.y) / 2);
  ShaderGlobal::set_color4(octahedral_texture_sizeVarId, extent.x, extent.y, float(margin) / extent.x, float(margin) / extent.y);
  ShaderGlobal::set_color4(octahedral_shadow_zn_zfarVarId, volume.zn, volume.zf, 0, 0);

  ShaderGlobal::set_texture(octahedral_temp_shadowVarId, octahedral_temp_shadow);

  shaders::OverrideStateId originalState = shaders::overrides::get_current();
  shaders::overrides::reset();
  if (!dynamic_content) // we only need z-test _additional_ content, otherwise just immediately write
    shaders::overrides::set(cmpfAlwaysNoBiasStateId);
  // depth test is only needed when we use static-light-with-dynamic content,
  // and only on second draw (where we 'add' dynamic content)
  octahedralPacker.render();
  if (!dynamic_content)
    shaders::overrides::reset();
  shaders::overrides::set(originalState);
}

TraceLightInfo ShadowSystem::prepareApproximateStaticCasters(const Volume &volume, uint16_t atlas_w, uint16_t atlas_h)
{
  DA_PROFILE_GPU;
  TraceLightInfo info;
  info.x = volume.shadow.x;
  info.y = volume.shadow.y;
  info.w = volume.shadow.width;
  info.h = volume.shadow.height;
  info.zn = volume.zn;
  info.zf = volume.zf;
  memcpy(info.posType, &volume.invTm.col3, sizeof(float) * 3);
  info.posType[3] = float(volume.isOctahedral());
  if (!volume.isOctahedral())
  {
    mat44f invViewProj, viewproj, proj;
    mat44f invView;
    mat44f view;
    volume.buildProj(proj);
    volume.buildView(view, invView, 0);
    view.col3 = V_C_UNIT_0001;
    v_mat44_mul(viewproj, proj, view);
    v_mat44_inverse(invViewProj, viewproj);
    vec3f lt = v_mat44_mul_vec3p(invViewProj, v_make_vec4f(-1.f, +1.f, 1.f, 1)),
          rt = v_mat44_mul_vec3p(invViewProj, v_make_vec4f(+1.f, +1.f, 1.f, 1)),
          lb = v_mat44_mul_vec3p(invViewProj, v_make_vec4f(-1.f, -1.f, 1.f, 1));
    vec3f hor = v_sub(rt, lt), ver = v_sub(lb, lt);
    v_stu(info.extentOrView0, lt);
    v_stu(info.viewHor, hor);
    v_stu(info.viewVer, ver);
  }
  else
  {
    const IPoint2 extent = getOctahedralTempShadowExtent(volume);
    G_ASSERTF(OMNI_SHADOW_MARGIN * 2 < extent.x && OMNI_SHADOW_MARGIN * 2 < extent.y, "The margin is larger than the texture itself");
    const int margin = min(OMNI_SHADOW_MARGIN, min(extent.x, extent.y) / 2);
    info.extentOrView0[0] = extent.x;
    info.extentOrView0[1] = extent.y;
    info.extentOrView0[2] = float(margin) / extent.x;
    info.extentOrView0[3] = float(margin) / extent.y;
    memset(info.viewHor, 0, sizeof(info.viewHor));
    memset(info.viewVer, 0, sizeof(info.viewVer));
  }
  info.viewHor[3] = 1.f / atlas_w;
  info.viewVer[3] = 1.f / atlas_h;
  return info;
}

void ShadowSystem::endRenderStatic(uint32_t id)
{
  G_ASSERT_RETURN(id < volumes.size() != 0, );
  Volume &volume = volumes[id];
  if (volume.isStaticLightWithDynamicContent(currentFrame) && !volume.dynamicShadow.isEmpty())
  {
    G_ASSERT(!volume.isApproximatelyTracedStaticCasters());
    packOctahedral(id, false);
  }
}

void ShadowSystem::endRenderVolume(uint32_t id)
{
  G_ASSERT_RETURN(id < volumes.size() != 0, );
  Volume &volume = volumes[id];
  if (volume.isOctahedral())
  {
    const bool dynamic = volume.isStaticLightWithDynamicContent(currentFrame) && !volume.dynamicShadow.isEmpty();
    if (dynamic || !volume.isApproximatelyTracedStaticCasters()) // otherwise it is already written directly to target
      packOctahedral(id, dynamic);
  }

  volume.lastFrameUpdated = currentFrame;
  volumesToUpdate.erase(id);
}

void ShadowSystem::validateConsistency()
{
#if DAGOR_DBGLEVEL > 0
  G_ASSERT(volumesFrustum.size() == volumes.size());
  G_ASSERT(volumesBox.size() == volumes.size());
  Bitarray unused;
  unused.resize(volumes.size());
  unused.set();
  for (int i = freeVolumes.size() - 1; i >= 0; --i)
    if (freeVolumes[i] >= volumes.size() || !volumes[freeVolumes[i]].isDestroyed())
    {
      G_ASSERTF(0, "invalid freeVolume in freeVolume list index: %d id=%d, frameIsUsed = %d, currentFrame = %d", i, freeVolumes[i],
        volumes[freeVolumes[i]].lastFrameUsed, currentFrame);
      erase_items(freeVolumes, i, 1);
    }
    else
    {
      unused.set(freeVolumes[i]);
    }
  for (int i = volumes.size() - 1; i >= 0; --i)
  {
    if (volumes[i].isDestroyed() && !unused.get(i))
    {
      G_ASSERTF(0, "invalid volume in volume list: id=%d, frameIsUsed = %d, currentFrame = %d", i, volumes[i].lastFrameUsed,
        currentFrame);
      freeVolumes.push_back(i);
    }
  }
#endif
}

void ShadowSystem::pruneFreeVolumes()
{
  validateConsistency();
  int lastUsed = -1;
  for (int id = volumes.size() - 1; id >= 0; --id)
  {
    if (!volumes[id].isDestroyed())
    {
      lastUsed = id;
      break;
    }
  }
  if (lastUsed == -1)
  {
    freeVolumes.clear();
    resizeVolumes(0);
  }
  else
  {
    resizeVolumes(lastUsed + 1);
    struct AscCompare
    {
      bool operator()(const uint16_t a, const uint16_t b) const { return a < b; }
    };
    fast_sort(freeVolumes, AscCompare());
    for (int i = freeVolumes.size() - 1; i >= 0; --i)
      if (freeVolumes[i] <= lastUsed)
      {
        freeVolumes.resize(i + 1);
        break;
      }
  }
  validateConsistency();
}

void ShadowSystem::Volume::invalidate()
{
  lastFrameUsed = lastFrameUpdated = 1;
  shadow.reset();
  dynamicShadow.reset();
}

void ShadowSystem::invalidateAllVolumes()
{
  lastFrameStaticLightsOptimized = lruListFrame = 0;
  atlas.init(atlasWidth / maxShadow, atlasHeight / maxShadow, maxShadow / minShadow);
  for (int i = 0; i < volumes.size(); ++i)
    if (!volumes[i].isDestroyed())
      volumes[i].invalidate();
}

void ShadowSystem::startPrepareShadows()
{
  G_ASSERTF(currentState == UPDATE_ENDED, "start without end has been called");
  currentState = UPDATE_STARTED;
  currentFrame++;
  G_ASSERTF(currentFrame != 1,
    "we have wrapped 32bit counter. Can happen in once per 4 months with 200 fps. todo: Invalidate all volumes");
}

void ShadowSystem::useShadowOnFrame(int id)
{
  G_ASSERT_RETURN(id < volumes.size() != 0, );
  G_ASSERT_RETURN(!volumes[id].isDestroyed(), );
  volumes[id].lastFrameUsed = currentFrame;
  volumesToUpdate.insert(id);
}

/*static float projected_sphere_area( const Point3 &sphere_view_space, float r2, float fl ) // projection (focal length)
{
  // transform to camera space
  const Point3 &o = sphere_view_space;
  float z2 = o.z*o.z;
  float l2 = dot(o, o);
  float area = -PI*fl*fl*r2*sqrtf(fabsf((l2-r2)/(r2-z2))) / (r2-z2);
  return area;
}*/

static float projected_sphere_area_look_center(float z2, float r2, float fl) // projection (focal length)
{
  // transform to camera space
  float area = z2 > r2 ? PI * fl * fl * r2 / (z2 - r2) : PI * r2;
  return area;
}

void ShadowSystem::endPrepareShadows(dynamic_shadow_render::VolumesVector &volumesToRender, int max_shadow_volumes_to_update,
  float max_area_part_to_update, const Point3 &viewPos, float cameraFocal, mat44f_cref clip)
{
  G_ASSERTF(currentState == UPDATE_STARTED, "start without end has been called");
  volumesToRender.clear();
  if (!volumesToUpdate.size())
  {
    currentState = UPDATE_ENDED;
    return;
  }
  // vec3f threshold = v_splats(0/720.0);
  float threshold = 4. / (1280.0 * 720.f);
  const float full_screen_side_size = 2.0f, lowest_quality_scale = 0.25f;             // verify
  float screen_to_shadow_texels_ratio = lowest_quality_scale / full_screen_side_size; //
  // vec4f screen_to_shadow_texels = v_splats(screen_to_shadow_texels_ratio*0.5);
  int totalUpdateArea = max_area_part_to_update * atlasWidth * atlasHeight; // since all volumes are quads
  // 1. sort volumesToUpdate by (time_not_updated)*volumes.priority

  Tab<LRUEntry> lruUpdateList(framemem_ptr()); // only valid for lruListFrame
  lruUpdateList.reserve(volumesToUpdate.size());

  for (auto it = volumesToUpdate.begin(); it != volumesToUpdate.end();)
  {
    int id = *it;
    G_ASSERT(!volumes[id].isDestroyed());
    if (volumes[id].lastFrameUsed < currentFrame) // do not update if it is not used anymore
    {
      it = volumesToUpdate.erase(it);
      continue;
    }
    int age = min((currentFrame - volumes[id].lastFrameUpdated) * (volumes[id].priority + 1), uint32_t(65535));
    lruUpdateList.push_back(LRUEntry(id, age));
    it++;
  }
  fast_sort(lruUpdateList, LRUEntryCompare());

  // 2. check if anything has changed within volume bbox, including required resolution
  // 3. manage atlas for visible volumes.
  Frustum cameraFrustum(clip);
  vec4f cameraSphere = v_make_vec4f(viewPos.x, viewPos.y, viewPos.z, 0.1);
  // TMatrix view = orthonormalized_inverse(viewItm);
  int volumesChecked = 0;
  for (; volumesChecked < lruUpdateList.size(); volumesChecked++)
  {
    const uint16_t id = lruUpdateList[volumesChecked].id;
    if (!cameraFrustum.testSphereB(volumesSphere[id], v_splat_w(volumesSphere[id])) ||
        !cameraFrustum.testBoxB(volumesBox[id].bmin, volumesBox[id].bmax))
      continue;
    Volume &volume = volumes[id];
    // float screenSize = 16.f*16.f;
    const Point4 &sphC = as_point4(&volumesSphere[id]);
    float area = 10000.0;
    // use rotation stable
    // todo: use already rendered static data as heurestics for static lights. We can rely on minimum/maximum volume distance/area or
    // average volume distance.
    //  ideally, project area to screen. Doing so for each light is a bit tough
    //  so just use rendered area/distance to light as heurestics
    const bool isInside = volumesFrustum[id].testSphereB(cameraSphere, v_splat_w(cameraSphere));
    if (!isInside)
    {
      // we are trying to find texel density.
      // for omni's that's more or less correct
      area = projected_sphere_area_look_center(lengthSq(viewPos - Point3::xyz(sphC)), sphC.w * sphC.w, cameraFocal);
      // todo:
      // we need projected cone area
      // for spot, we better use something like projected triangle surface + area of part of sphere(not bounding sphere)
      // this is way less than projected area of bounding sphere
    }
    G_ASSERT(area > 0);
    if (area > threshold)
    {
      int maxShadowSize = maxShadow >> volume.sizeShift;
      float sideSize = min(sqrtf(area) * maxShadowSize * screen_to_shadow_texels_ratio * volume.quality + shadowStep / 2,
        float(maxShadowSize + shadowStep));

      sideSize *= notEnoughSpaceMul;
      int targetShadowSize = clamp(int(sideSize), minShadow - shadowStep, maxShadowSize + shadowStep);
      targetShadowSize = get_bigger_pow2(targetShadowSize);
      if (volume.shadow.isEmpty() || abs(targetShadowSize - volume.shadow.height) > shadowStep) // not about same size, we have to
                                                                                                // update it
      {
        targetShadowSize = clamp(targetShadowSize, minShadow, maxShadowSize);
        insertToAtlas(id, targetShadowSize);
        // debug("real target size %d", volume.shadow.height);
      }
      if (!volume.shadow.isEmpty())
      {
        if (volume.isStaticLightWithDynamicContent(currentFrame))
        {
          if (volume.shadow.width != volume.dynamicShadow.width) // we have to allocate exactly same size
          {
            atlasFree(volume.dynamicShadow);
            volume.dynamicShadow = atlasInsert(volume.shadow.width);
            if (volume.dynamicShadow.isEmpty()) // remove unused
            {
              replaceUnused(atlasWidth << 1);                          // remove unused
              volume.dynamicShadow = atlasInsert(volume.shadow.width); // try again
              if (volume.dynamicShadow.isEmpty())                      // still no space
              {
                degradeQuality();                                        // degrade quality
                volume.dynamicShadow = atlasInsert(volume.shadow.width); // no worries if it fails. Just there won't be dynamic
                                                                         // objects in shadow.
              }
            }
          }
        }

        if (volume.shouldBeRendered(currentFrame))
        {
          volumesToRender.push_back(id);
          totalUpdateArea -= volume.shadow.width * volume.shadow.width;
          if (volumesToRender.size() >= max_shadow_volumes_to_update || totalUpdateArea < 0)
            break;
        }
        else if (!volume.dynamicShadow.isEmpty())
        {
          if (!volume.isOctahedral())
          {
            mat44f texTM =
              buildTexTM(volume.shadow.x, volume.shadow.y, volume.shadow.width, volume.shadow.height, atlasWidth, atlasHeight);
            mat44f viewProj;
            volume.buildViewProj(viewProj, 0);
            mat44f valumeTexTM;
            v_mat44_mul(valumeTexTM, texTM, viewProj); // todo: do that only if last time was different matrix
            v_mat44_transpose(volumesTexTM[id], valumeTexTM);
          }
          else
          {
            G_ASSERTF(OMNI_SHADOW_MARGIN * 2 < volume.shadow.width && OMNI_SHADOW_MARGIN * 2 < volume.shadow.height,
              "The margin is larger than the texture itself");
            int margin = min(OMNI_SHADOW_MARGIN, min(volume.shadow.width, volume.shadow.height) / 2);
            volumesOctahedralData[id] =
              Point4(float(volume.shadow.x + margin) / atlasWidth, float(volume.shadow.y + margin) / atlasHeight,
                float(volume.shadow.width - margin * 2) / atlasWidth, float(volume.shadow.height - margin * 2) / atlasHeight);
          }

          atlasFree(volume.dynamicShadow); // free space
          volume.dynamicShadow.reset();
        }
      }
      else
      {
        DEBUG_OCCUPANCY("no space for shadow volume");
      }
    }
  }

  const int framesToUpdateScaled = frames_to_change_quality * (volumesChecked ? lruUpdateList.size() / volumesChecked : 16);
  if (
    notEnoughSpaceMul < max_upgraded_quality / upgrade_quality_step && currentFrame - lastFrameQualityUpgraded > framesToUpdateScaled)
  {
    upgradeQuality();
  }
  currentState = UPDATE_ENDED;

  ShaderGlobal::set_texture(dynamic_light_shadowsVarId, dynamic_light_shadows);
}

void ShadowSystem::debugValidate()
{
#if RBP_DEBUG_OVERLAP || DAGOR_DBGLEVEL > 1
  atlas.validate();
  for (int i = 0; i < volumes.size(); ++i)
  {
    if (volumes[i].isDestroyed())
      continue;
    if (volumes[i].shadow.isEmpty())
      continue;
    G_ASSERTF(atlasIsAllocated(volumes[i].shadow), "volume not allocated%d(%dx%d %dx%d)", i, volumes[i].shadow.x, volumes[i].shadow.y,
      volumes[i].shadow.width, volumes[i].shadow.width);
    rbp::Rect with;
    G_ASSERTF(!atlasIntersectsWithFree(volumes[i].shadow, with), "volumes intersect free space! %d(%dx%d %dx%d) (%dx%d %dx%d) ", i,
      volumes[i].shadow.x, volumes[i].shadow.y, volumes[i].shadow.width, volumes[i].shadow.width, with.x, with.y, with.width,
      with.height);
    for (int j = i + 1; j < volumes.size(); ++j)
    {
      if (volumes[j].isDestroyed())
        continue;
      if (volumes[j].shadow.isEmpty())
        continue;
      if (rbp::DisjointRectCollection::Disjoint(volumes[i].shadow, volumes[j].shadow))
        continue;
      G_ASSERTF(0, "volumes intersect! %d(%dx%d %dx%d) vs %d(%dx%d %dx%d) ", i, volumes[i].shadow.x, volumes[i].shadow.y,
        volumes[i].shadow.width, volumes[i].shadow.width, j, volumes[j].shadow.x, volumes[j].shadow.y, volumes[j].shadow.width,
        volumes[j].shadow.width);
    }
  }
#endif
}

bool ShadowSystem::insertToAtlas(uint16_t id, uint16_t targetSize)
{
  // debug("atlas occupied %d / %d", atlas.occupiedArea(atlasWidth*atlasHeight), atlasWidth*atlasHeight);
  Volume &volume = volumes[id];
  if (volume.shadow.height == targetSize)
    return true;
  // bool wasEmpty = volume.shadow.isEmpty();
  AtlasRect oldStaticRect;

  if (!volume.isDynamicOrOnlyStaticCasters() && targetSize > volume.shadow.width)
  {
    oldStaticRect = volume.shadow;
    volume.shadow.reset();
  }

  atlasFree(volume.dynamicShadow);
  volume.dynamicShadow.reset();

  atlasFree(volume.shadow);
  volume.shadow.reset();

  volume.shadow = atlasInsert(targetSize);

  // we are out of space in atlas! remove unused;
  if (volume.shadow.isEmpty())
  {
    volume.shadow = replaceUnused(targetSize); // will return Rect to Replace, if it is best candidate
    if (volume.shadow.isEmpty())
      volume.shadow = atlasInsert(targetSize);
  }
  int downgradedSize = targetSize;
  if (volume.shadow.isEmpty() && downgradedSize > minShadow)
  {
    downgradedSize >>= 1;
    volume.shadow = atlasInsert(downgradedSize);
    lastFrameQualityUpgraded = currentFrame;
    DEBUG_OCCUPANCY("%d: not enough space in atlas for %d shadow, downgrade quality %d notEnoughSpaceMul=%f!", id, targetSize,
      downgradedSize, notEnoughSpaceMul);
  }

  if (volume.shadow.isEmpty()) // there is not enough space even for half res
  {
    degradeQuality();
    downgradedSize >>= 1;
    DEBUG_OCCUPANCY("degradeQuality! %d: not enough space in atlas for %d shadow, downgrade quality %d, notEnoughSpaceMul=%f!", id,
      targetSize, downgradedSize, notEnoughSpaceMul);
    // try to allocate quarter res
    volume.shadow = atlasInsert(downgradedSize);
  }
  debugValidate();

  if (!oldStaticRect.isEmpty() && volume.shadow.width <= oldStaticRect.width) // nothing has changed in terms of quality, we'd better
                                                                              // reuse old shadow
  {
    DEBUG_OCCUPANCY("%d: use old rect %d", id, oldStaticRect.width);
    atlasFree(volume.shadow);
    volume.shadow = oldStaticRect;
  }
  else
  {
    atlasFree(oldStaticRect);
    if (volume.shadow.isEmpty())
    {
      DEBUG_OCCUPANCY("still not enough space in atlas for %d shadow!", targetSize);
    }
    volume.lastFrameUpdated = 1;
    volume.lastFrameChanged = volume.lastFrameUpdated + 1;
    memset(&volumesTexTM[id], 0, sizeof(volumesTexTM[id]));
    memset(&volumesOctahedralData[id], 0, sizeof(volumesOctahedralData[id]));
  }
  return !volume.shadow.isEmpty();
}

void ShadowSystem::buildLRU()
{
  if (lruListFrame == currentFrame)
    return;
  lruList.clear();
  for (int i = 0; i < volumes.size(); ++i)
    if (!volumes[i].shadow.isEmpty() && volumes[i].lastFrameUsed != currentFrame)
    {
      G_ASSERT(volumes[i].lastFrameUsed < currentFrame);
      lruList.push_back(LRUEntry(i, min((currentFrame - volumes[i].lastFrameUsed), uint32_t(65535))));
    }

  fast_sort(lruList, LRUEntryCompare());
  lruListFrame = currentFrame;
}

bool ShadowSystem::degradeQuality()
{
  if (lastFrameQualityDegraded == currentFrame)
    return false;
  notEnoughSpaceMul = max(downgrade_quality_step * notEnoughSpaceMul, min_downgraded_quality);
  lastFrameQualityDegraded = lastFrameQualityUpgraded = currentFrame;
  return true;
}

bool ShadowSystem::upgradeQuality()
{
  if (lastFrameQualityDegraded == currentFrame || lastFrameQualityUpgraded == currentFrame)
    return false;
  lastFrameQualityUpgraded = currentFrame;

  float occupancy = atlasOccupancy();
  float thresholdOccupancy = occupancy_to_upgrade_quality / (upgrade_quality_step * upgrade_quality_step);
  if (occupancy > thresholdOccupancy)
  {
    buildLRU();
    if (lruList.size())
    {
      replaceUnused(atlasWidth << 1); //
      occupancy = atlasOccupancy();
    }
  }
  if (occupancy < thresholdOccupancy)
  {
    notEnoughSpaceMul = min(max_upgraded_quality, notEnoughSpaceMul * upgrade_quality_step);
    DEBUG_OCCUPANCY("occupancy = %f upgrade quality %f", occupancy, notEnoughSpaceMul);
    return true;
  }
  return false;
}

bool ShadowSystem::freeUnusedDynamicDataForStaticLights()
{
  if (lastFrameStaticLightsOptimized == currentFrame)
    return false;
  bool ret = false;
  for (int i = 0; i < volumes.size(); ++i)
  {
    if (volumes[i].isDestroyed())
      continue;
    Volume &volume = volumes[i];
    if (!volume.isStaticLightWithDynamicContent(currentFrame) && !volume.dynamicShadow.isEmpty())
    {
      atlasFree(volume.dynamicShadow);
      volume.dynamicShadow.reset();
      ret = true;
    }
  }
  lastFrameStaticLightsOptimized = currentFrame;
  return ret;
}

ShadowSystem::AtlasRect ShadowSystem::replaceUnused(uint16_t targetShadowSize)
{
  buildLRU();
  AtlasRect retRect;
  if (!lruList.size())
    return retRect; // nothing to free here!

  // try to remove one best fit
  int bestIndex = -1, bestScore = max(atlasHeight, atlasWidth) * 16;
  if (targetShadowSize < atlasWidth && targetShadowSize < atlasHeight)
  {
    for (int i = lruList.size() - 1; i >= 0; --i)
    {
      int id = lruList[i].id;
      auto &s = volumes[id].shadow;
      if (s.height == targetShadowSize && s.width == targetShadowSize) // we can fit here
      {
        bestScore = 0;
        bestIndex = i;
        break;
      }
    }
    if (bestIndex == -1)
    {
      for (int i = 0, e = lruList.size(); i < e; ++i)
      {
        int id = lruList[i].id;
        auto &s = volumes[id].shadow;
        if (s.height >= targetShadowSize && s.width >= targetShadowSize) // we can fit here
        {
          const int score = (s.height - targetShadowSize) * (s.width - targetShadowSize);
          if (score < bestScore) // find most efficient one
          {
            bestScore = score;
            bestIndex = i;
          }
        }
      }
    }
  }
  // we have found one big enough (but smallest possible) option.
  if (bestScore == 0) // can be just replaced inplace
  {
    DEBUG_OCCUPANCY("found old entry to replace free = %d,  %d out of %d", atlas.freeSpaceArea(), bestIndex, lruList.size());
    const int id = lruList[bestIndex].id;
    retRect = volumes[id].shadow;
    volumes[id].shadow.reset();
    volumes[id].valid = false;          // actually not important, it will be invalidated on insert
    erase_items(lruList, bestIndex, 1); // nothing to remove
    return retRect;
  }
  freeUnusedDynamicDataForStaticLights();
  // we have to free all small
  if (bestIndex < 0)
    bestIndex = lruList.size() - 1;
  DEBUG_OCCUPANCY("atlas free = %d, free Small lru %d out of %d", atlas.freeSpaceArea(), bestIndex, lruList.size());
  for (int i = 0; i <= bestIndex; ++i)
  {
    int id = lruList[i].id;
    DEBUG_OCCUPANCY("free volume %d, %dx%d %d", id, volumes[id].shadow.x, volumes[id].shadow.y, volumes[id].shadow.height);
    if (volumes[id].shadow.isEmpty())
      continue;
    atlasFree(volumes[id].shadow);
    volumes[id].shadow.reset();
    volumes[id].valid = false; // actually not important, it will be invalidated on insert
  }
  debugValidate();
  erase_items(lruList, 0, bestIndex + 1); // nothing to remove
  // atlas.MergeFreeList();
  // atlas.MergeFreeList();
  DEBUG_OCCUPANCY("atlas free after lru clear %d", atlas.freeSpaceArea());
  return retRect;
}

int ShadowSystem::appendVolume()
{
  int id = append_items(volumes, 1);
  G_VERIFY(append_items(volumesFrustum, 1) == id);
  G_VERIFY(append_items(volumesBox, 1) == id);
  G_VERIFY(append_items(volumesTexTM, 1) == id);
  G_VERIFY(append_items(volumesOctahedralData, 1) == id);
  G_VERIFY(append_items(volumesSphere, 1) == id);
  return id;
}

void ShadowSystem::resizeVolumes(int cnt)
{
  volumes.resize(cnt);
  volumesFrustum.resize(cnt);
  volumesBox.resize(cnt);
  volumesTexTM.resize(cnt);
  volumesOctahedralData.resize(cnt);
  volumesSphere.resize(cnt);
}

void ShadowSystem::afterReset()
{
  d3d::resource_barrier({dynamic_light_shadows.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
}

static mat44f buildTexTM(uint16_t ofsX, uint16_t ofsY, uint16_t sw, uint16_t sh, uint16_t tex_w, uint16_t tex_h)
{
  float x = ((float)ofsX) / (float)tex_w;
  float y = ((float)ofsY) / (float)tex_h;
  float w = ((float)sw) / (float)tex_w;
  float h = ((float)sh) / (float)tex_h;
  mat44f vret;
  vret.col0 = v_make_vec4f(0.5f * w, 0, 0, 0);
  vret.col1 = v_make_vec4f(0, -0.5f * h, 0, 0);
  vret.col2 = v_make_vec4f(0, 0, 1, 0);
  vret.col3 = v_make_vec4f(x + 0.5f * w, h + y - 0.5 * h, 0, 1);
  return vret;
}
