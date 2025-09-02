// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/fast_grass.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <gameRes/dag_stdGameResId.h>
#include <heightmap/lodGrid.h>
#include <heightmap/heightmapCulling.h>
#include <render/globTMVars.h>
#include <render/noiseTex.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_info.h>
#include <perfMon/dag_statDrv.h>
#include <dag/dag_vectorSet.h>
#include <math/dag_math2d.h>

#define GLOBAL_VARS_LIST                \
  VAR(fast_grass_aspect_ratio)          \
  VAR(fast_grass_slice_step)            \
  VAR(fast_grass_num_samples)           \
  VAR(fast_grass_max_samples)           \
  VAR(fast_grass_fade_start)            \
  VAR(fast_grass_fade_range)            \
  VAR(fast_grass_step_scale)            \
  VAR(fast_grass_height_variance_scale) \
  VAR(fast_grass_smoothness_fade_start) \
  VAR(fast_grass_smoothness_fade_end)   \
  VAR(fast_grass_normal_fade_start)     \
  VAR(fast_grass_normal_fade_end)       \
  VAR(fast_grass_placed_fade_start)     \
  VAR(fast_grass_placed_fade_end)       \
  VAR(fast_grass_ao_max)                \
  VAR(fast_grass_ao_curve)              \
  VAR(fast_grass_num_clips)

#define VAR(a) static ShaderVariableInfo a(#a);
GLOBAL_VARS_LIST
#undef VAR

static inline float3 srgb_color4(E3DCOLOR ec)
{
  Color3 c = color3(ec);
  c.r = powf(c.r, 2.2f);
  c.g = powf(c.g, 2.2f);
  c.b = powf(c.b, 2.2f);
  return float3(c.r, c.g, c.b);
}

void FastGrassRenderer::close()
{
  if (hmapRenderer.isInited())
    release_64_noise();
  hmapRenderer.close();
  grassChannelsCB.close();
  clipmapRectsCB.close();
  hmapTex.close();
  gmapTex.close();
  cmapTex.close();
  clear_and_shrink(grassChannelData);
  alreadyLoaded = false;
  isInitialized = false;
  needUpdate = true;
}

void FastGrassRenderer::initOrUpdate(dag::Span<GrassTypeDesc> grass_types)
{
  eastl::array<uint8_t, GRASS_MAX_CHANNELS + 1> grassChannelMap;
  eastl::fill(grassChannelMap.begin(), grassChannelMap.end(), 255);

  dag::Vector<GrassTypeDesc *, framemem_allocator> usedTypes;
  usedTypes.reserve(grass_types.size());
  dag::VectorSet<eastl::string, eastl::less<eastl::string>, framemem_allocator> texNames;

  for (uint32_t ti = 0; ti < grass_types.size() && ti < GRASS_MAX_TYPES; ti++)
  {
    auto &gd = grass_types[ti];
    G_LOGERR_AND_DO(!gd.impostorName.empty(), continue, "no impostor specified for fast grass type <%s>", gd.name.c_str());

    bool used = false;
    for (const auto id : gd.biomes)
    {
      G_LOGERR_AND_DO(id < GRASS_MAX_CHANNELS, continue, "too big biome id = %d on fast grass type <%s>", id, gd.name.c_str());
      G_LOGERR_AND_DO(grassChannelMap[id] == 255, continue, "grass with id = %d is already initialized (on <%s> fast grass type)", id,
        gd.name.c_str());
      grassChannelMap[id] = usedTypes.size();
      used = true;
    }
    G_LOGERR_AND_DO(used, continue, "fast grass type <%s> is not used for any biomes", gd.name.c_str());

    usedTypes.push_back(&gd);
    texNames.insert(gd.impostorName);
  }

  if (usedTypes.empty())
  {
    debug("no fast grass");
    close();
    return;
  }

  G_ASSERT_LOG(usedTypes.size() <= GRASS_MAX_TYPES, "too many different grass types (%d) required! while maximum %d", usedTypes.size(),
    GRASS_MAX_TYPES);

  // map texture ids
  for (auto gd : usedTypes)
  {
    auto it = texNames.find(gd->impostorName);
    G_ASSERT_CONTINUE(it != texNames.end());
    gd->texId = it - texNames.begin();
  }

  // fill grass data to be uploaded to the buffer
  grassChannelData.resize(GRASS_MAX_CHANNELS + 1);
  needUpdate = true;

  for (int i = 0; i < grassChannelData.size(); i++)
  {
    auto &type = grassChannelData[i];
    auto ti = grassChannelMap[i];
    if (ti >= usedTypes.size() || i >= GRASS_MAX_CHANNELS)
    {
      memset(&type, 0, sizeof(type));
      type.texIndex = GRASS_MAX_TYPES;
      type.stiffness = 1;
    }
    else
    {
      auto &desc = *usedTypes[ti];
      type.mask_r_color0 = srgb_color4(desc.colors[0]);
      type.mask_r_color1 = srgb_color4(desc.colors[1]);
      type.mask_g_color0 = srgb_color4(desc.colors[2]);
      type.mask_g_color1 = srgb_color4(desc.colors[3]);
      type.mask_b_color0 = srgb_color4(desc.colors[4]);
      type.mask_b_color1 = srgb_color4(desc.colors[5]);
      type.height = desc.height;
      type.texIndex = desc.texId;
      type.w_to_height_add__height_var =
        uint32_t((1 - desc.w_to_height_mul) * 0xffff) | (uint32_t(desc.heightVariance * 0xffff) << 16);
      type.stiffness = desc.stiffness;
    }
  }

  // build array textures
  dag::Vector<eastl::string> nameList;
  dag::Vector<const char *> ptrList;
  nameList.resize(texNames.size());
  ptrList.resize(texNames.size());

  for (int i = 0; i < texNames.size(); i++)
  {
    nameList[i].sprintf("%s_grass_ad", texNames[i].c_str());
    ptrList[i] = nameList[i].c_str();
  }
  albedoTex.close();
  albedoTex = dag::add_managed_array_texture("fast_grass_albedo_texture*", ptrList, "fast_grass_tex_a");

  for (int i = 0; i < texNames.size(); i++)
  {
    nameList[i].sprintf("%s_grass_nt", texNames[i].c_str());
    ptrList[i] = nameList[i].c_str();
  }
  normalTex.close();
  normalTex = dag::add_managed_array_texture("fast_grass_normal_texture*", ptrList, "fast_grass_tex_n");

  if (!albedoTex || !normalTex) //-V1051
  {
    logerr("fast grass array texture not created");
    close();
    return;
  }

  alreadyLoaded = false;

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = d3d::AddressMode::Wrap;
    smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
    smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
    smpInfo.anisotropic_max = 1;
    ShaderGlobal::set_sampler(get_shader_variable_id("fast_grass_tex_a_samplerstate"), d3d::request_sampler(smpInfo));
    ShaderGlobal::set_sampler(get_shader_variable_id("fast_grass_tex_n_samplerstate"), d3d::request_sampler(smpInfo));
  }

  // init rendering
  if (!hmapRenderer.isInited())
  {
    if (!hmapRenderer.init("fast_grass", "", "hmap_tess_factor", true))
      return;
    init_and_get_argb8_64_noise().setVar();
  }

  if (!grassChannelsCB)
  {
    grassChannelsCB = dag::buffers::create_persistent_cb(dag::buffers::cb_array_reg_count<FastGrassType>(GRASS_MAX_CHANNELS + 1),
      "fast_grass_types_buf");
    if (!grassChannelsCB)
    {
      logerr("fast_grass: failed to create buffer");
      return;
    }
  }

  if (precompCascades > FAST_GRASS_MAX_CLIPMAP_CASCADES)
  {
    logerr("fast_grass: precompCascades=%d can't be more than max %d", precompCascades, FAST_GRASS_MAX_CLIPMAP_CASCADES);
    precompCascades = FAST_GRASS_MAX_CLIPMAP_CASCADES;
  }

  precompCascades = max(precompCascades, 1);
  int maxScale = int(ceilf(hmapRange * 2.0f / sliceStep / precompResolution));
  if (maxScale < (1 << (precompCascades - 1)))
  {
    int levels = __bsr_unsafe(maxScale) + 1;
    logerr("fast_grass: precompCascades=%d is too high, use %d", precompCascades, levels);
    precompCascades = levels;
  }

  clipmapScales.resize(precompCascades);
  clipmapScales[0] = 1;
  if (precompCascades > 1)
  {
    float b = expf(logf(maxScale) / (precompCascades - 1));
    debug("fast_grass: clipmap power base is %f, max scale is %d", b, maxScale);
    for (int i = 1; i < precompCascades; i++)
      clipmapScales[i] = int(ceilf(powf(b, i)));
  }

#if DAGOR_DBGLEVEL > 0
  String msg("fast_grass clipmap scales:");
  for (int s : clipmapScales)
    msg.aprintf(16, " %d", s);
  debug("%s", msg.c_str());
#endif

  if (!clipmapRectsCB)
  {
    clipmapRectsCB = dag::buffers::create_persistent_cb(
      dag::buffers::cb_array_reg_count<Point4>(FAST_GRASS_MAX_CLIPMAP_CASCADES * 2 + 1), "fast_grass_clip_rects");
    if (!clipmapRectsCB)
    {
      logerr("fast_grass: failed to create buffer");
      return;
    }
  }

  if (auto tex = hmapTex.getTex())
  {
    TextureInfo info;
    if (!tex->getinfo(info) || info.w != precompResolution || info.a != precompCascades)
    {
      hmapTex.close();
      gmapTex.close();
      cmapTex.close();
    }
  }

  if (!hmapTex.getTex())
  {
    hmapTex.set(d3d::create_array_tex(precompResolution, precompResolution, precompCascades, TEXCF_UNORDERED | TEXFMT_L16, 1,
                  "fast_grass_pre_hmap"),
      "fast_grass_pre_hmap");
    gmapTex.set(d3d::create_array_tex(precompResolution, precompResolution, precompCascades, TEXCF_UNORDERED | TEXFMT_R8UI, 1,
                  "fast_grass_pre_gmap"),
      "fast_grass_pre_gmap");
    cmapTex.set(d3d::create_array_tex(precompResolution, precompResolution, precompCascades, TEXCF_UNORDERED | TEXFMT_A8R8G8B8, 1,
                  "fast_grass_pre_cmap"),
      "fast_grass_pre_cmap");

    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = d3d::AddressMode::Border;
    smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
    smpInfo.anisotropic_max = 1;
    ShaderGlobal::set_sampler(get_shader_variable_id("fast_grass_pre_hmap_samplerstate"), d3d::request_sampler(smpInfo));
    ShaderGlobal::set_sampler(get_shader_variable_id("fast_grass_pre_cmap_samplerstate"), d3d::request_sampler(smpInfo));
  }

  invalidate();
  updateGrassTypes();

  isInitialized = true;
  debug("fast grass initialized");
}

void FastGrassRenderer::driverReset()
{
  invalidate();
  updateGrassTypes();
}

void FastGrassRenderer::invalidate() { needUpdate = true; }

FastGrassRenderer::FastGrassRenderer() : precompShader("fast_grass_precomp") {}

FastGrassRenderer::~FastGrassRenderer() { close(); }

void FastGrassRenderer::updateGrassTypes()
{
  if (grassChannelData.empty())
    return;

  bool ret = grassChannelsCB->updateData(0, data_size(grassChannelData), grassChannelData.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  if (ret)
    needUpdate = false;
}

#if DAGOR_DBGLEVEL > 0
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>

static void clipmap_debug() {}

REGISTER_IMGUI_WINDOW("Debug", "Fast Grass Clipmap Debug", clipmap_debug);
#endif

void FastGrassRenderer::render(const TMatrix4 &globtm, const Point3 &view_pos, float min_ht, float max_ht, float water_level,
  const BBox2 *clip_box)
{
  if (!isInitialized || !hmapRenderer.isInited())
    return;
  if (!alreadyLoaded && !check_managed_texture_loaded(normalTex.getTexId()))
    return;

  if (!alreadyLoaded)
  {
    if (auto tex = normalTex.getBaseTex())
    {
      TextureInfo info;
      tex->getinfo(info);
      ShaderGlobal::set_real(fast_grass_aspect_ratio, float(info.w) / info.h);
    }
    alreadyLoaded = true;
  }

  if (needUpdate)
  {
    updateGrassTypes();
    if (needUpdate)
      return;
  }

  TIME_D3D_PROFILE(render_fast_grass);

  Frustum frustum{globtm};

  int lodCount = int(ceilf(log2f(hmapRange / (hmapCellSize * hmapRenderer.getDim()))));
  float lod0AreaRadius = 0;

  Point2 centerOfHmap = Point2::xz(view_pos);

  const int lod0Rad = 1;
  const int lastLodRad = 1;

  int gridDim = hmapRenderer.getDim();
  float scaledCell = hmapCellSize;

  const int lod0TessFactor = 0;
  float alignSize = -1;

  LodGrid lodGrid;
  lodGrid.init(lodCount, lod0Rad, lod0TessFactor, lastLodRad);
  LodGridCullData defaultCullData(framemem_ptr());
  int hmap_tess_factorVarId = hmapRenderer.get_hmap_tess_factorVarId();
  BBox2 lodsRegion;
  cull_lod_grid(lodGrid, lodGrid.lodsCount, centerOfHmap.x, centerOfHmap.y, scaledCell, scaledCell, alignSize, alignSize, // alignment
    min_ht, max_ht, &frustum, clip_box, defaultCullData, NULL, lod0AreaRadius, hmap_tess_factorVarId, gridDim, false /*not used*/,
    nullptr, nullptr, &lodsRegion, water_level);
  if (!defaultCullData.getCount())
    return;

  // project camera and 4 far plane points to XZ
  shrink_frustum_zfar(frustum, v_make_vec3f(P3D(view_pos)), v_splats(hmapRange * 2));
  Point2 frustumPoints[5];
  for (int i = 0; i < 4; i++)
  {
    vec4f invalid;
    vec3f p = three_plane_intersection(frustum.camPlanes[i & 1], frustum.camPlanes[(i >> 1) | 2], frustum.camPlanes[4], invalid);
    frustumPoints[i] = Point2(v_extract_x(p), v_extract_z(p));
  }
  frustumPoints[4] = Point2::xz(view_pos);

  // start with 8 line segments (far quad and camera connections)
  dag::Vector<Point4, framemem_allocator> clipRects;
  clipRects.resize(clipmapScales.size() * 2 + 1);

  struct LineSeg
  {
    Point2 pa, pb;
  };
  dag::Vector<LineSeg, framemem_allocator> lines;
  lines.reserve(16);

  for (int i = 0; i < 4; i++)
    lines.push_back(LineSeg{frustumPoints[i], frustumPoints[4]});
  lines.push_back(LineSeg{frustumPoints[0], frustumPoints[1]});
  lines.push_back(LineSeg{frustumPoints[2], frustumPoints[3]});
  lines.push_back(LineSeg{frustumPoints[0], frustumPoints[2]});
  lines.push_back(LineSeg{frustumPoints[1], frustumPoints[3]});

#if DAGOR_DBGLEVEL > 0
  ImDrawList *drawList = nullptr;
  Point2 imguiOrigin;
  float imguiSize;

  auto imguiPos = [&](const Point2 p) {
    Point2 vp = p - Point2::xz(view_pos);
    return Point2(vp.x, -vp.y) * (imguiSize * 0.45f / hmapRange) + Point2(1, 1) * imguiSize * 0.5f + imguiOrigin;
  };

  if (imgui_window_is_visible("Debug", "Fast Grass Clipmap Debug"))
  {
    if (ImGui::Begin("Fast Grass Clipmap Debug"))
    {
      drawList = ImGui::GetWindowDrawList();
      imguiOrigin = ImGui::GetWindowPos();
      imguiSize = min(ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
    }
    ImGui::End();
  }
#endif

  // clip lines outside of heightmap range
  BBox2 rangeBox(frustumPoints[4], hmapRange * 2);
  for (int i = lines.size() - 1; i >= 0; i--)
  {
    float t0, t1;
    Point2 pa = lines[i].pa;
    Point2 pb = lines[i].pb;
    if (!isect_line_box(pa, pb - pa, rangeBox, t0, t1))
    {
      Point2 dir = pb - pa;
      float lsq = dir.lengthSq();
      if (lsq > 1e-6f)
        dir /= lsq;
      Point2 nearestCorner = rangeBox[0];
      float minDist = FLT_MAX;
      for (int j = 0; j < 4; j++)
      {
        Point2 corner(rangeBox[j & 1].x, rangeBox[j >> 1].y);
        float dist = (lerp(pa, pb, clamp(dir * (corner - pa), 0.0f, 1.0f)) - corner).lengthSq();
        if (dist < minDist)
        {
          minDist = dist;
          nearestCorner = corner;
        }
      }
      lines[i].pa = lines[i].pb = nearestCorner;
      continue;
    }

    if (t0 <= 0 && t1 >= 1) // fully inside the box
      continue;

    if (t0 >= 1) // fully outside the box, collapse to border point
    {
      lines[i].pa = lines[i].pb = lerp(pa, pb, t0);
      continue;
    }
    if (t1 <= 0) // fully outside the box, collapse to border point
    {
      lines[i].pa = lines[i].pb = lerp(pa, pb, t1);
      continue;
    }

    if (t0 > 0)
      lines[i].pa = lerp(pa, pb, t0);
    if (t1 < 1)
      lines[i].pb = lerp(pa, pb, t1);
  }

#if DAGOR_DBGLEVEL > 0
  if (drawList)
    for (const auto &l : lines)
      drawList->AddLine(imguiPos(l.pa), imguiPos(l.pb), IM_COL32(100, 100, 100, 255));
#endif

  // place clipmaps
  int numClips = 0;
  const int safeOfs = 2;

  for (int clip = 0; clip < clipmapScales.size() && !lines.empty(); clip++)
  {
    numClips = clip + 1;

    BBox2 box;
    for (const auto &l : lines)
    {
      box += l.pa;
      box += l.pb;
    }

    float step = clipmapScales[clip] * sliceStep;
    float size = step * (precompResolution - safeOfs * 2 - 1);

    Point2 p = floor(Point2::xz(view_pos) / step - Point2(0.5f, 0.5f) * precompResolution) * step;

    if (p.x < box[0].x)
    {
      if (p.x + size > box[1].x)
        p.x = floorf((box.center().x - size * 0.5f) / step) * step;
      else
        p.x = floorf(box[0].x / step) * step;
    }
    else if (p.x + size > box[1].x)
      p.x = ceilf(box[1].x / step) * step - size;

    if (p.y < box[0].y)
    {
      if (p.y + size > box[1].y)
        p.y = floorf((box.center().y - size * 0.5f) / step) * step;
      else
        p.y = floorf(box[0].y / step) * step;
    }
    else if (p.y + size > box[1].y)
      p.y = ceilf(box[1].y / step) * step - size;

    Point2 cp = p - Point2(step, step) * safeOfs;
    float factor = step * precompResolution;
    clipRects[clip * 2 + 0] = Point4(1 / factor, 1 / factor, -cp.x / factor, -cp.y / factor);
    clipRects[clip * 2 + 1] = Point4(step, step, cp.x + 0.5 * step, cp.y + 0.5 * step);

    if (clip + 1 == clipmapScales.size())
      break;

    // clip lines inside the box
    BBox2 clipBox(p, p + Point2(size, size));
    for (int i = lines.size() - 1; i >= 0; i--)
    {
      float t0, t1;
      if (!isect_line_box(lines[i].pa, lines[i].pb - lines[i].pa, clipBox, t0, t1))
        continue;
      if (t0 > 1 || t1 < 0) // fully outside the box
        continue;

      if (t0 <= 0 && t1 >= 1) // fully inside the box
      {
        lines.erase_unsorted(lines.begin() + i);
        continue;
      }

      if (t0 > 0)
      {
        if (t1 < 1)
        {
          // split line in two
          lines.push_back(LineSeg{lines[i].pb, lerp(lines[i].pa, lines[i].pb, t1)});
          lines[i].pb = lerp(lines[i].pa, lines[i].pb, t0);
        }
        else
          lines[i].pb = lerp(lines[i].pa, lines[i].pb, t0);
      }
      else
        lines[i].pa = lerp(lines[i].pa, lines[i].pb, t1);
    }
  }

  clipRects[numClips * 2] =
    Point4(0.5f / hmapRange, 0.5f / hmapRange, 0.5f - 0.5f * view_pos.x / hmapRange, 0.5f - 0.5f * view_pos.z / hmapRange);

#if DAGOR_DBGLEVEL > 0
  if (drawList)
  {
    drawList->AddRect(imguiPos(rangeBox[0]), imguiPos(rangeBox[1]), IM_COL32(255, 0, 0, 255));
    for (const auto &l : lines)
    {
      drawList->AddLine(imguiPos(l.pa), imguiPos(l.pb), IM_COL32(255, 255, 0, 255));
      drawList->AddCircle(imguiPos(l.pa), 5, IM_COL32(255, 255, 0, 255));
    }

    for (uint clip = 0; clip < numClips; clip++)
    {
      Point4 r = clipRects[clip * 2 + 1];
      drawList->AddRect(imguiPos(Point2(r.z, r.w)), imguiPos(Point2(r.z, r.w) + Point2::xy(r) * (precompResolution - 1)),
        IM_COL32(0, 255 * (clip + 1) / numClips, 0, 255));
    }
  }
#endif

  // cleanup
  clear_and_shrink(lines);

  // set shader params
  clipmapRectsCB->updateData(0, data_size(clipRects), clipRects.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  ShaderGlobal::set_int(fast_grass_num_clips, numClips);

  set_globtm_to_shader(globtm);

  ShaderGlobal::set_int(fast_grass_num_samples, numSamples);
  ShaderGlobal::set_int(fast_grass_max_samples, maxSamples);
  ShaderGlobal::set_real(fast_grass_slice_step, sliceStep);
  ShaderGlobal::set_real(fast_grass_fade_start, fadeStart);
  ShaderGlobal::set_real(fast_grass_fade_range, fadeEnd - fadeStart);
  ShaderGlobal::set_real(fast_grass_step_scale, stepScale);
  ShaderGlobal::set_real(fast_grass_height_variance_scale, heightVarianceScale);
  ShaderGlobal::set_real(fast_grass_smoothness_fade_start, smoothnessFadeStart);
  ShaderGlobal::set_real(fast_grass_smoothness_fade_end, smoothnessFadeEnd);
  ShaderGlobal::set_real(fast_grass_normal_fade_start, normalFadeStart);
  ShaderGlobal::set_real(fast_grass_normal_fade_end, normalFadeEnd);
  ShaderGlobal::set_real(fast_grass_placed_fade_start, placedFadeStart);
  ShaderGlobal::set_real(fast_grass_placed_fade_end, placedFadeEnd);
  ShaderGlobal::set_real(fast_grass_ao_max, 1 - aoMaxStrength);
  ShaderGlobal::set_real(fast_grass_ao_curve, aoCurve);

  hmapTex.setVar();
  gmapTex.setVar();
  cmapTex.setVar();

  precompShader.dispatchThreads(precompResolution, precompResolution, numClips);
  d3d::resource_barrier({hmapTex.getArrayTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({gmapTex.getArrayTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({cmapTex.getArrayTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});

  hmapRenderer.render(lodGrid, defaultCullData);
}
