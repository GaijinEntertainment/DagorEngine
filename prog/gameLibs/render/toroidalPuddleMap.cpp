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
#include <shaders/dag_shaderVariableInfo.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IBBox2.h>
#include <3d/dag_render.h>
#include <image/dag_texPixel.h>
#include <render/toroidal_update.h>
#include <render/toroidal_update_regions.h>
#include <render/toroidalPuddleMap.h>

void ToroidalPuddles::close() { toroidalPuddles.close(); }

void ToroidalPuddles::invalidate()
{
  invalidated = true;

  for (int j = 0; j < LOD_COUNT; ++j)
  {
    lodData[j].invalidRegions.clear();
    lodData[j].quadRegions.clear();
  }
}

static ShaderVariableInfo toroidalPuddles_world2uv_1VarId("toroidalPuddles_world2uv_1", true);
static ShaderVariableInfo toroidalPuddles_world2uv_2VarId("toroidalPuddles_world2uv_2", true);
static ShaderVariableInfo toroidalPuddles_world_offsetsVarId("toroidalPuddles_world_offsets", true);
static ShaderVariableInfo toroidalPuddles_texarray_samplerVarId("toroidal_puddles_texarray_samplerstate", true);

int ToroidalPuddles::getBufferSize() { return puddlesCacheSize; }

void ToroidalPuddles::init(int puddles_size, float near_lod_size, float far_lod_size, int treshold, E3DCOLOR clear_value)
{
  close();
  puddlesCacheSize = puddles_size;
  lodData[0].puddlesRadius = near_lod_size;
  lodData[1].puddlesRadius = far_lod_size;
  pixelTreshold = treshold;
  clearValue = clear_value;

  toroidalPuddles =
    dag::create_array_tex(puddlesCacheSize, puddlesCacheSize, LOD_COUNT, TEXFMT_R8 | TEXCF_RTARGET, 1, "toroidal_puddles_texarray");
  toroidalPuddles_texarray_samplerVarId.set_sampler(d3d::request_sampler({}));

  for (int j = 0; j < LOD_COUNT; ++j)
  {
    lodData[j].torHelper.texSize = puddlesCacheSize;
    lodData[j].torHelper.curOrigin = IPoint2(-1000000, 100000);

    lodData[j].regions.clear();
    lodData[j].invalidRegions.clear();
    lodData[j].quadRegions.clear();

    lodData[j].worldSize = lodData[j].puddlesRadius * 2.f;
    lodData[j].texelSize = lodData[j].worldSize / (float)puddlesCacheSize;
  }
}


void ToroidalPuddles::addRegionToRender(const ToroidalQuadRegion &reg, int cascade)
{
  append_items(lodData[cascade].quadRegions, 1, &reg);
}

void ToroidalPuddles::invalidateBox(const BBox2 box, bool flush_regions)
{
  flushRegions |= flush_regions;

  for (int cascadeNo = LOD_COUNT - 1; cascadeNo >= 0; cascadeNo--)
  {
    IBBox2 ibox(ipoint2(floor(box[0] / lodData[cascadeNo].texelSize)), ipoint2(ceil(box[1] / lodData[cascadeNo].texelSize)));

    toroidal_clip_region(lodData[cascadeNo].torHelper, ibox);
    if (ibox.isEmpty())
      return;

    add_non_intersected_box(lodData[cascadeNo].invalidRegions, ibox);
  }
}

void ToroidalPuddles::updateTile(ToroidalPuddlesRenderer &renderer, const ToroidalQuadRegion &reg, float texel_size)
{
  BBox2 box(point2(reg.texelsFrom) * texel_size, point2(reg.texelsFrom + reg.wd) * texel_size);

  d3d::setview(reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y, 0, 1);
  d3d::clearview(CLEAR_TARGET, clearValue, 0.f, 0);

  renderer.renderTile(box);
}

void ToroidalPuddles::updatePuddles(ToroidalPuddlesRenderer &renderer, const Point3 &camera_position, float max_move)
{
  // start from the largest cascade
  for (int cascadeNo = LOD_COUNT - 1; cascadeNo >= 0; cascadeNo--)
  {
    // update
    lodData[cascadeNo].regions.clear();

    ToroidalHelper &torHelper = lodData[cascadeNo].torHelper;

    IPoint2 newTexelOrigin = torHelper.curOrigin;

    IPoint2 center_pos;
    center_pos.x = 4 * floorf(camera_position.x / (4.0f * lodData[cascadeNo].texelSize));
    center_pos.y = 4 * floorf(camera_position.z / (4.0f * lodData[cascadeNo].texelSize));

    if (invalidated)
      torHelper.curOrigin = IPoint2(-10000000, -1000000);

    // movement is too small to update
    if ((abs(torHelper.curOrigin.x - center_pos.x) < pixelTreshold) && (abs(torHelper.curOrigin.y - center_pos.y) < pixelTreshold))
      continue;

    // if possible, move center only on one axis in time, so we need to update only 1 region instead of 3
    if (abs(torHelper.curOrigin.x - center_pos.x) > abs(torHelper.curOrigin.y - center_pos.y))
      newTexelOrigin.x = center_pos.x;
    else
      newTexelOrigin.y = center_pos.y;

    // movement is way too big, so we can't move center by one axis direction
    if (
      max(abs(torHelper.curOrigin.x - newTexelOrigin.x), abs(torHelper.curOrigin.y - newTexelOrigin.y)) > puddlesCacheSize * max_move)
      newTexelOrigin = center_pos;

    ToroidalGatherCallback cb(lodData[cascadeNo].regions);
    toroidal_update(newTexelOrigin, torHelper, 0.33f * puddlesCacheSize, cb);

    Point2 worldSpaceOrigin = point2(torHelper.curOrigin) * lodData[cascadeNo].texelSize;

    lodData[cascadeNo].worldToToroidal = Color4(1.f / lodData[cascadeNo].worldSize, 1.f / lodData[cascadeNo].worldSize,
      0.5f - worldSpaceOrigin.x / lodData[cascadeNo].worldSize, 0.5f - worldSpaceOrigin.y / lodData[cascadeNo].worldSize);

    lodData[cascadeNo].uvOffset = -point2((torHelper.mainOrigin - torHelper.curOrigin) % torHelper.texSize) / torHelper.texSize;

    for (int i = 0; i < lodData[cascadeNo].regions.size(); ++i)
    {
      const ToroidalQuadRegion &reg = lodData[cascadeNo].regions[i];
      addRegionToRender(reg, cascadeNo);
    }

    if (!invalidated)
      break; // update only one cascade at time to avoid spikes
  }
  // update dynamic regions
  for (int cascadeNo = LOD_COUNT - 1; cascadeNo >= 0; cascadeNo--)
  {
    // clip invalidated regions and choose one to render
    toroidal_clip_regions(lodData[cascadeNo].torHelper, lodData[cascadeNo].invalidRegions);
    do
    {
      int invalid_reg_id = get_closest_region_and_split(lodData[cascadeNo].torHelper, lodData[cascadeNo].invalidRegions);
      if (invalid_reg_id >= 0)
      {
        // update chosen invalid region.
        ToroidalQuadRegion quad(
          transform_point_to_viewport(lodData[cascadeNo].invalidRegions[invalid_reg_id][0], lodData[cascadeNo].torHelper),
          lodData[cascadeNo].invalidRegions[invalid_reg_id].width() + IPoint2(1, 1),
          lodData[cascadeNo].invalidRegions[invalid_reg_id][0]);

        addRegionToRender(quad, cascadeNo);

        // and remove it from update list
        erase_items(lodData[cascadeNo].invalidRegions, invalid_reg_id, 1);
      }
    } while (flushRegions && lodData[cascadeNo].invalidRegions.size() > 0);
  }
  flushRegions = false;
  invalidated = false; // invalidation perfomed only on zero afr

  // update all generated regions for all cascades
  for (int cascadeNo = LOD_COUNT - 1; cascadeNo >= 0; cascadeNo--)
  {
    if (lodData[cascadeNo].quadRegions.size() == 0)
      continue;

    d3d::set_render_target(toroidalPuddles.getArrayTex(), cascadeNo, 0);

    // set matrix
    renderer.startRenderTiles(Point2::xz(camera_position));

    // toroidal update & render
    for (int i = 0; i < lodData[cascadeNo].quadRegions.size(); ++i)
    {
      updateTile(renderer, lodData[cascadeNo].quadRegions[i], lodData[cascadeNo].texelSize);
    }
    lodData[cascadeNo].quadRegions.clear();
    renderer.endRenderTiles();
  }

  // shader var
  ShaderGlobal::set_color4(toroidalPuddles_world2uv_1VarId, lodData[0].worldToToroidal);
  ShaderGlobal::set_color4(toroidalPuddles_world2uv_2VarId, lodData[1].worldToToroidal);

  ShaderGlobal::set_color4(toroidalPuddles_world_offsetsVarId, lodData[0].uvOffset.x, lodData[0].uvOffset.y, lodData[1].uvOffset.x,
    lodData[1].uvOffset.y);

  toroidalPuddles.setVar();
  d3d::resource_barrier({toroidalPuddles.getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}
