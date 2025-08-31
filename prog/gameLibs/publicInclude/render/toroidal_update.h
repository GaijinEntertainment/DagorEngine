//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/toroidalHelper.h>
#include <generic/dag_staticTab.h>
#include <math/dag_bounds2.h>

// toroidal update is a helper function which splits movement update of texture in integer (texel) space into 4 quads
// it mantains "origin" (center of updated texture) and "main origin" (zero in texel space coordinates). The texture area in texel
// space is [origin-texSize/2, origin+texSize/2) the texture itself should be in wrapping address mode unlike direct access texture, it
// should be addressed with special offset usual adressing is worldSpace.xz/size_of_area_in_meters + offset_in_texels toroidaladressing
// is worldSpace.xz/size_of_area_in_meters + offset_in_texels + offset_in_texture quad while (offset_in_texels + offset_in_texture
// quad) is just one constant, it still makes sense sometimes to keep it in a separate, if one need vignetting or border/clamp
// addressing implementation of worldSpace->texel space addressing is user-defined (usually just one texelSize constant) implementation
// of Callback class is also user-defined class Callback has three functions: start(const IPoint2& origin), end, and renderQuad(IPoint2
// viewport_xy, IPoint2 viewport_wd_ht, IPoint2 texels_xy) renderQuad updates texture in [viewport_xy, viewport_xy+viewport_wd_ht)
// area, with [texels_xy, texels_xy +viewport_wd_ht) texels renderQuad will be called up to 4 times (can [0..4])
//!! it is not guaranteed that origin will be moved  to new origin!!
// this can happen if it is required to call more than 4 times of renderQuad (if origin is intersecting texture border)
// if you need to guarantee that, call function twice, it won't do anything if there is nothing to do.
// sample usage:
//  ToroidalHelper m_data;//helper data
//  float mTexelSize;//world texel size
//  IPoint2 newTexelsOrigin = ipoint2(floor(world_view_pos/mTexelSize));
//  int pixelsUpdated = toroidal_update(newTexelsOrigin, m_data, m_data.texSize/2, cb);
//  Point2 ofs = point2((data.mainOrigin-data.curOrigin)%data.texSize)/data.texSize;
//  ShaderGlobal::set_color4(world_to_hmap_tex_ofsVarId, ofs.x, ofs.y,0,0);
//  Point2 worldSpaceOrigin = point2(m_data.curOrigin)*mTexelSize;
//  float worldArea = m_data.texSize*mTexelSize
//  ShaderGlobal::set_color4(world_to_hmap_ofsVarId, 1.0f/worldArea, 1.0f/worldArea, -worldSpaceOrigin.x/worldArea+0.5f,
//  -worldSpaceOrigin.y/worldArea+0.5); shader code: float2 tc = worldPos.xz*world_to_hmap_ofs.x + world_to_hmap_ofs.zw;// clamp
//  adressing will be saturate(worldPos.xz*world_to_hmap_ofs.x + world_to_hmap_ofs.zw); float val = tex2D(hmap_ofs_tex, float4(tc -
//  world_to_hmap_tex_ofs,0,0) ).x;
//  //border adressing: val = any(abs(tc*2-1)>1) ? 0 : val;
//  float2 vignette = saturate( abs(tc*2-1) * 10 - 9 );
//  val *= saturate( 1.0 - dot( vignette, vignette ) );//vignetting instead of border adressing

// or, if we don't need adressing, we can use optimized version
// ShaderGlobal::set_color4(world_to_hmap_ofsVarId, 1.0f/worldArea, 1.0f/worldArea, -worldSpaceOrigin.x/worldArea+0.5f-ofs.x,
// -worldSpaceOrigin.y/worldArea+0.5-ofs.y); float2 tc = worldPos.xz*world_to_hmap_ofs.x + world_to_hmap_ofs.zw;// that's it! float val
// = tex2D(hmap_ofs_tex, float4(tc,0,0) ).x;

// or, if we want optimized vignette onle we can use
// ShaderGlobal::set_color4(world_to_hmap_ofsVarId, 2.0f/worldArea, 2.0f/worldArea, -2*worldSpaceOrigin.x/worldArea,
// -2*worldSpaceOrigin.y/worldArea); ShaderGlobal::set_color4(world_to_hmap_tex_ofsVarId, 1.0f/worldArea, 1.0f/worldArea,
// -worldSpaceOrigin.x/worldArea+0.5f-ofs.x, -worldSpaceOrigin.y/worldArea+0.5-ofs.y); float2 tc_vignette =
// abs(worldPos.xz*world_to_hmap_ofs.x + world_to_hmap_ofs.zw); float2 tc = worldPos.xz*world_to_hmap_tex_ofs.x +
// world_to_hmap_tex_ofs.zw;// that's it! float val = tex2D(hmap_ofs_tex, float4(tc,0,0) ).x; float2 vignette = saturate( tc_vignette *
// 10 - 9 ); val *= saturate( 1.0 - dot( vignette, vignette ) );//vignetting instead of border adressing

// class Callback has three functions: start(const IPoint2&), end, and renderQuad(IPoint2 viewport_xy, IPoint2 viewport_wd_ht, IPoint2
// texels_xy) renderQuad updates texture in [viewport_xy, viewport_xy+viewport_wd_ht) area, with [texels_xy, texels_xy +viewport_wd_ht)
// texels

struct ToroidalGatherCallback
{
  // in case size or container type is changed, use ToroidalGatherCallback::RegionTab as a container
  typedef StaticTab<ToroidalQuadRegion, 5> RegionTab;
  RegionTab &cb;

  ToroidalGatherCallback(RegionTab &cb_) : cb(cb_) {}
  void start(const IPoint2 &) {}

  void renderQuad(const IPoint2 &lt, const IPoint2 &wd, const IPoint2 &texelsFrom)
  {
    cb.push_back(ToroidalQuadRegion(lt, wd, texelsFrom));
  }
  void end() {}
};

template <class Callback>
inline int toroidal_update(const IPoint2 &new_origin, IPoint2 &cur_origin, IPoint2 &main_origin, int texSize,
  int full_update_texels_threshold, Callback &cb);
inline IPoint2 wrap_texture(const IPoint2 &p, unsigned texSize);
inline IPoint2 transform_point_to_viewport(const IPoint2 &p, const IPoint2 &main_origin, const IPoint2 &cur_origin, unsigned texSize);
inline IPoint2 transform_point_to_viewport(const IPoint2 &p, const ToroidalHelper &helper);

template <class Callback>
inline int toroidal_update(const IPoint2 &new_origin, ToroidalHelper &helper, int full_update_texels_threshold, Callback &cb)
{
  return toroidal_update(new_origin, helper.curOrigin, helper.mainOrigin, helper.texSize, full_update_texels_threshold, cb);
}

inline IPoint2 wrap_texture(const IPoint2 &p, unsigned texSize) { return ((p % texSize) + IPoint2(texSize, texSize)) % texSize; }

inline IPoint2 transform_point_to_viewport(const IPoint2 &p, const IPoint2 &main_origin, const IPoint2 &cur_origin, unsigned texSize)
{
  IPoint2 textureSpace = (p - cur_origin) + IPoint2(texSize >> 1, texSize >> 1);
  G_ASSERT(textureSpace.x >= 0 && textureSpace.x < texSize);
  G_ASSERT(textureSpace.y >= 0 && textureSpace.y < texSize);
  textureSpace += (cur_origin - main_origin) % texSize;
  return wrap_texture(textureSpace, texSize);
}
inline IPoint2 transform_point_to_viewport(const IPoint2 &p, const ToroidalHelper &helper)
{
  return transform_point_to_viewport(p, helper.mainOrigin, helper.curOrigin, helper.texSize);
}
inline IPoint2 transform_viewport_point_to_world(const IPoint2 &p, const ToroidalHelper &helper)
{
  G_ASSERT(p.x >= 0 && p.x < helper.texSize && p.y >= 0 && p.y < helper.texSize);
  IPoint2 curOriginTextureSpace = wrap_texture(p - (helper.curOrigin - helper.mainOrigin), helper.texSize);
  IPoint2 worldCoords = curOriginTextureSpace - IPoint2(helper.texSize >> 1, helper.texSize >> 1) + helper.curOrigin;
  return worldCoords;
}

template <class Callback>
inline int render_toroidal_quad(const IPoint2 &lt, const IPoint2 &wd_, const IPoint2 &new_center, const IPoint2 &main_center,
  int texSize, Callback &cb)
// const Point2 &thlt, const Point2 &thrb)
{
  IPoint2 wd = min(wd_, IPoint2(texSize, texSize) - lt);
  if (wd.x <= 0 || wd.y <= 0)
    return 0;
  IPoint2 newLT = new_center - IPoint2(texSize, texSize) / 2;
  IPoint2 texelsFrom = (lt + ((main_center - new_center) % texSize) + IPoint2(texSize, texSize)) % texSize + newLT;
  cb.renderQuad(lt, wd, texelsFrom);
  return wd.x * wd.y;
}

template <class Callback>
inline int toroidal_update(const IPoint2 &new_origin, IPoint2 &cur_origin, IPoint2 &main_origin, int texSize,
  int full_update_texels_threshold, Callback &cb)
{
  const IPoint2 texelMovement = cur_origin - new_origin;
  IPoint2 updateTexelsBox = min(abs(texelMovement), IPoint2(texSize, texSize));
  int maxMovement = max(updateTexelsBox.x, updateTexelsBox.y);
  if (maxMovement == 0)
    return 0;
  cb.start(new_origin);
  if (maxMovement >= min(texSize, full_update_texels_threshold))
  {
    cur_origin = main_origin = new_origin;
    cb.renderQuad(IPoint2(0, 0), IPoint2(texSize, texSize), IPoint2(new_origin.x - texSize / 2, new_origin.y - texSize / 2));
    cb.end();
    return texSize * texSize;
  }
  IPoint2 oldCenter = wrap_texture(cur_origin - main_origin, texSize);
  IPoint2 newCenter = oldCenter + (new_origin - cur_origin);
  if (newCenter.x < 0)
  {
    if (oldCenter.x > 0)
      newCenter.x = 0;
    else
      newCenter.x += texSize, oldCenter.x = texSize;
  }
  else if (newCenter.x > texSize)
    newCenter.x = texSize;


  if (newCenter.y < 0)
  {
    if (oldCenter.y > 0)
      newCenter.y = 0;
    else
      newCenter.y += texSize, oldCenter.y = texSize;
  }
  else if (newCenter.y > texSize)
    newCenter.y = texSize;
  IPoint2 origin = cur_origin + newCenter - oldCenter;
  ;


  IPoint2 minCenter = min(newCenter, oldCenter);
  IPoint2 maxCenter = max(newCenter, oldCenter);
  IPoint2 viewPortLeft(texSize - maxCenter.x, texSize - maxCenter.y);

  // alignedOrigin = point2(new_origin)*texelSize;
  // Point2 lt = alignedOrigin-Point2(distance, distance), rb = alignedOrigin+Point2(distance, distance);
  int result = 0;
  result += render_toroidal_quad(IPoint2(0, minCenter.y), IPoint2(newCenter.x, updateTexelsBox.y), origin, main_origin, texSize,
    cb); // oldCenter, updateTexelsBox
  result += render_toroidal_quad(IPoint2(minCenter.x, 0), IPoint2(updateTexelsBox.x, minCenter.y), origin, main_origin, texSize,
    cb); // oldCenter, updateTexelsBox

  result += render_toroidal_quad(IPoint2(newCenter.x, minCenter.y), IPoint2(texSize - newCenter.x, updateTexelsBox.y), origin,
    main_origin, texSize, cb);
  result += render_toroidal_quad(IPoint2(minCenter.x, maxCenter.y), IPoint2(updateTexelsBox.x, viewPortLeft.y), origin, main_origin,
    texSize, cb);

  cur_origin = origin;
  cb.end();
  return result;
}

template <class Callback>
inline void toroidal_invalidate_boxes(const ToroidalHelper &helper, float world_texel_size, dag::ConstSpan<BBox2> invalidation_boxes,
  Callback &cb)
{
  IPoint2 ltBorder = helper.curOrigin - IPoint2(helper.texSize, helper.texSize) / 2;
  IPoint2 rbBorder = helper.curOrigin + IPoint2(helper.texSize, helper.texSize) / 2;
  cb.start(helper.curOrigin);
  for (const BBox2 &box : invalidation_boxes)
  {
    IPoint2 lt = ipoint2(floor(box.leftTop() / world_texel_size));
    IPoint2 rb = ipoint2(ceil(box.rightBottom() / world_texel_size));
    lt = max(lt, ltBorder);
    rb = min(rb, rbBorder);
    if (lt.x >= rbBorder.x || lt.y >= rbBorder.y)
      continue;
    IPoint2 ltViewport = transform_point_to_viewport(lt, helper);
    IPoint2 quadSize = rb - lt;
    quadSize = min(quadSize, IPoint2(helper.texSize, helper.texSize) - ltViewport);
    render_toroidal_quad(ltViewport, quadSize, helper.curOrigin, helper.mainOrigin, helper.texSize, cb);
    render_toroidal_quad(IPoint2(0, ltViewport.y), IPoint2(rb.x - lt.x - quadSize.x, quadSize.y), helper.curOrigin, helper.mainOrigin,
      helper.texSize, cb);
    render_toroidal_quad(IPoint2(ltViewport.x, 0), IPoint2(quadSize.x, rb.y - lt.y - quadSize.y), helper.curOrigin, helper.mainOrigin,
      helper.texSize, cb);
    render_toroidal_quad(IPoint2(0, 0), IPoint2(rb - lt - quadSize), helper.curOrigin, helper.mainOrigin, helper.texSize, cb);
  }
  cb.end();
}
