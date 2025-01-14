// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_bounds2.h>
#include <util/dag_globDef.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IBBox2.h>
#include <3d/dag_render.h>
#include <image/dag_texPixel.h>
#include <render/toroidal_update.h>
#include <render/toroidal_update_regions.h>
#include <render/toroidalHeightmap.h>

void ToroidalHeightmap::close() { toroidalHeightmap.close(); }

void ToroidalHeightmap::invalidate()
{
  invalidated = true;

  for (int j = 0; j < LOD_COUNT; ++j)
  {
    invalidRegions[j].clear();
    quadRegions[j].clear();
  }
}

Point2 ToroidalHeightmap::getWorldSize() { return Point2(heightmapSizeLod0, heightmapSizeLod1); }

int ToroidalHeightmap::getBufferSize() { return heightmapCacheSize; }


void ToroidalHeightmap::init(int heightmap_size, float near_lod_size, float far_lod_size, uint32_t format, int treshold,
  E3DCOLOR clear_value)
{
  close();
  heightmapCacheSize = heightmap_size;
  heightmapSizeLod0 = near_lod_size;
  heightmapSizeLod1 = far_lod_size;
  pixelTreshold = treshold;
  clearValue = clear_value;

  uint32_t heightmapFormat;
  if ((d3d::get_texformat_usage(format) & (d3d::USAGE_RTARGET | d3d::USAGE_BLEND)) == (d3d::USAGE_RTARGET | d3d::USAGE_BLEND))
    heightmapFormat = format;
  else
  {
    heightmapFormat = TEXFMT_A8R8G8B8;
    logerr("Can't use format for toroidal heightmap");
  }

  toroidalHeightmap = dag::create_array_tex(heightmapCacheSize, heightmapCacheSize, LOD_COUNT, heightmapFormat | TEXCF_RTARGET, 1,
    "toroidal_heightmap_texarray");
  toroidalHeightmap.getArrayTex()->disableSampler();

  // shader variables
  toroidalClipmap_world2uv_1VarId = ::get_shader_glob_var_id("toroidalClipmap_world2uv_1", true);
  toroidalClipmap_world2uv_2VarId = ::get_shader_glob_var_id("toroidalClipmap_world2uv_2", true);
  toroidalClipmap_world_offsetsVarId = ::get_shader_glob_var_id("toroidalClipmap_world_offsets", true);
  toroidalHeightmapSamplerVarId = ::get_shader_glob_var_id("toroidal_heightmap_texarray_samplerstate", true);
  setTexFilter(d3d::FilterMode::Default);

  for (int j = 0; j < LOD_COUNT; ++j)
  {
    torHelpers[j].texSize = heightmapCacheSize;
    torHelpers[j].curOrigin = IPoint2(-1000000, 100000);

    worldToToroidal[j] = Color4(0, 0, 0, 0);
    uvOffset[j] = Point2(0, 0);
    regions[j].clear();
    invalidRegions[j].clear();
    quadRegions[j].clear();
  }
}

void ToroidalHeightmap::setBlackTex(TEXTUREID black_tex_array)
{
  // always sample center of texture
  ShaderGlobal::set_color4(toroidalClipmap_world2uv_1VarId, Color4(0, 0, 0.5, 0.5));
  ShaderGlobal::set_texture(toroidalHeightmap.getVarId(), black_tex_array);
}

void ToroidalHeightmap::setHeightmapTex()
{
  ShaderGlobal::set_color4(toroidalClipmap_world2uv_1VarId, worldToToroidal[0]); // restore correct coords
  toroidalHeightmap.setVar();
}

void ToroidalHeightmap::setTexFilter(d3d::FilterMode filter)
{
  heightmapSampler.filter_mode = filter;
  ShaderGlobal::set_sampler(toroidalHeightmapSamplerVarId, d3d::request_sampler(heightmapSampler));
}

void ToroidalHeightmap::addRegionToRender(const ToroidalQuadRegion &reg, int cascade) { append_items(quadRegions[cascade], 1, &reg); }

void ToroidalHeightmap::invalidateBox(const BBox2 box, bool flush_regions)
{
  flushRegions |= flush_regions;

  // constants
  const float torDistances[LOD_COUNT] = {heightmapSizeLod0, heightmapSizeLod1};

  for (int cascadeNo = LOD_COUNT - 1; cascadeNo >= 0; cascadeNo--)
  {
    float texelSize = torDistances[cascadeNo] * 2.f / (float)heightmapCacheSize;

    IBBox2 ibox(ipoint2(floor(box[0] / texelSize)), ipoint2(ceil(box[1] / texelSize)));

    toroidal_clip_region(torHelpers[cascadeNo], ibox);
    if (ibox.isEmpty())
      return;

    add_non_intersected_box(invalidRegions[cascadeNo], ibox);
  }
}

void ToroidalHeightmap::updateTile(ToroidalHeightmapRenderer &renderer, const ToroidalQuadRegion &reg, float texel_size)
{
  BBox2 box(point2(reg.texelsFrom) * texel_size, point2(reg.texelsFrom + reg.wd) * texel_size);

  // render tile
  d3d::setview(reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y, 0, 1);
  d3d::clearview(CLEAR_TARGET, clearValue, 0.f, 0);

  renderer.renderTile(box);
}

void ToroidalHeightmap::updateHeightmap(ToroidalHeightmapRenderer &renderer, const Point3 &camera_position, float min_texel_size,
  float max_move)
{

  // constants
  const float torDistances[LOD_COUNT] = {heightmapSizeLod0, heightmapSizeLod1};

  // start from the largest cascade
  for (int cascadeNo = LOD_COUNT - 1; cascadeNo >= 0; cascadeNo--)
  {
    // update
    regions[cascadeNo].clear();

    // vars
    ToroidalHelper &torHelper = torHelpers[cascadeNo];
    float toroidalWorldSize = torDistances[cascadeNo] * 2.f;
    float texelSize = torDistances[cascadeNo] * 2.f / (float)heightmapCacheSize;

    if (texelSize < min_texel_size)
      continue;

    IPoint2 newTexelOrigin = torHelpers[cascadeNo].curOrigin;

    IPoint2 center_pos;
    center_pos.x = 4 * floorf(camera_position.x / (4.0f * texelSize));
    center_pos.y = 4 * floorf(camera_position.z / (4.0f * texelSize));

    if (invalidated)
      torHelpers[cascadeNo].curOrigin = IPoint2(-10000000, -1000000);

    if ((abs(torHelpers[cascadeNo].curOrigin.x - center_pos.x) < pixelTreshold) &&
        (abs(torHelpers[cascadeNo].curOrigin.y - center_pos.y) < pixelTreshold))
      continue;
    else
    {
      if (abs(torHelpers[cascadeNo].curOrigin.x - center_pos.x) > abs(torHelpers[cascadeNo].curOrigin.y - center_pos.y))
        newTexelOrigin.x = center_pos.x;
      else
        newTexelOrigin.y = center_pos.y;
    }

    if (max(abs(torHelpers[cascadeNo].curOrigin.x - newTexelOrigin.x), abs(torHelpers[cascadeNo].curOrigin.y - newTexelOrigin.y)) >
        heightmapCacheSize * max_move)
      newTexelOrigin = center_pos;

    ToroidalGatherCallback cb(regions[cascadeNo]);
    toroidal_update(newTexelOrigin, torHelper, 0.33f * heightmapCacheSize, cb);

    Point2 worldSpaceOrigin = point2(torHelper.curOrigin) * texelSize;

    worldToToroidal[cascadeNo] = Color4(1.f / toroidalWorldSize, 1.f / toroidalWorldSize,
      0.5f - worldSpaceOrigin.x / toroidalWorldSize, 0.5f - worldSpaceOrigin.y / toroidalWorldSize);

    uvOffset[cascadeNo] = -point2((torHelper.mainOrigin - torHelper.curOrigin) % torHelper.texSize) / torHelper.texSize;

    for (int i = 0; i < regions[cascadeNo].size(); ++i)
    {
      const ToroidalQuadRegion &reg = regions[cascadeNo][i];
      addRegionToRender(reg, cascadeNo);
    }

    if (!invalidated)
      break; // update only one cascade at time to avoid spikes
  }
  // update dynamic regions
  for (int cascadeNo = LOD_COUNT - 1; cascadeNo >= 0; cascadeNo--)
  {
    // clip invalidated regions and choose one to render
    toroidal_clip_regions(torHelpers[cascadeNo], invalidRegions[cascadeNo]);
    do
    {
      int invalid_reg_id = get_closest_region_and_split(torHelpers[cascadeNo], invalidRegions[cascadeNo]);
      if (invalid_reg_id >= 0)
      {
        // update chosen invalid region.
        ToroidalQuadRegion quad(transform_point_to_viewport(invalidRegions[cascadeNo][invalid_reg_id][0], torHelpers[cascadeNo]),
          invalidRegions[cascadeNo][invalid_reg_id].width() + IPoint2(1, 1), invalidRegions[cascadeNo][invalid_reg_id][0]);

        addRegionToRender(quad, cascadeNo);

        // and remove it from update list
        erase_items(invalidRegions[cascadeNo], invalid_reg_id, 1);
      }
    } while (flushRegions && invalidRegions[cascadeNo].size() > 0);
  }
  flushRegions = false;
  invalidated = false; // invalidation perfomed only on zero afr

  // save states
  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;

  // update all generated regions for all cascades
  for (int cascadeNo = LOD_COUNT - 1; cascadeNo >= 0; cascadeNo--)
  {
    // vars
    float texelSize = torDistances[cascadeNo] * 2.f / (float)heightmapCacheSize;

    if ((texelSize < min_texel_size) || (quadRegions[cascadeNo].size() == 0))
      continue;

    d3d::set_render_target(toroidalHeightmap.getArrayTex(), cascadeNo, 0);

    // set matrix
    renderer.startRenderTiles(Point2::xz(camera_position));

    // toroidal update & render
    for (int i = 0; i < quadRegions[cascadeNo].size(); ++i)
    {
      updateTile(renderer, quadRegions[cascadeNo][i], texelSize);
    }
    quadRegions[cascadeNo].clear();
    renderer.endRenderTiles();
  }

  // shader var
  ShaderGlobal::set_color4(toroidalClipmap_world2uv_1VarId, worldToToroidal[0]);
  ShaderGlobal::set_color4(toroidalClipmap_world2uv_2VarId, worldToToroidal[1]);

  ShaderGlobal::set_color4(toroidalClipmap_world_offsetsVarId, uvOffset[0].x, uvOffset[0].y, uvOffset[1].x, uvOffset[1].y);

  toroidalHeightmap.setVar();
  d3d::resource_barrier({toroidalHeightmap.getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}
