
#define INSIDE_DRIVER 1

#include <generic/dag_tab.h>
#include <3d/dag_drv3dCmd.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug.h>
#include <ioSys/dag_genIo.h>
#include <image/dag_texPixel.h>

#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <util/dag_string.h>
#include <util/dag_watchdog.h>

#include "render.h"
#include "textureloader.h"

using namespace drv3d_metal;

Tab<drv3d_metal::Texture*> tmp_depth;

struct TrackDriver3dRenderTarget : public Driver3dRenderTarget
{
  bool changed, unmarked_aa_rts, srgb_bb;
  bool aa_textures_in_states;

  bool isEqual(const Driver3dRenderTarget &a) const
  {
    if ((used&TOTAL_MASK) != (a.used&TOTAL_MASK))
    {
      return false;
    }

    if (used&DEPTH && depth.tex != a.depth.tex)
    {
      return false;
    }

    if (used&DEPTH && ((used&DEPTH_READONLY) != (a.used&DEPTH_READONLY)))
    {
      return false;
    }

    uint32_t used_left = used&COLOR_MASK;

    if (!used_left)
    {
      return true;
    }

    if (isBackBufferColor() != a.isBackBufferColor())
    {
      return false;
    }

    for (uint32_t ri = 0; used_left; used_left >>= 1, ri++)
    {
      if (used_left & 1)
      {
        if (color[ri].tex != a.color[ri].tex || color[ri].level != a.color[ri].level || color[ri].face != a.color[ri].face)
        {
          return false;
        }
      }
    }

    return true;
  }

  bool isEqual(const TrackDriver3dRenderTarget &a) const
  {
    if (!isEqual((const Driver3dRenderTarget &)a))
    {
      return false;
    }

    if (isBackBufferColor() && a.srgb_bb != srgb_bb)
    {
      return false;
    }

    return true;
  }

  Driver3dRenderTarget & operator = (const Driver3dRenderTarget &a)
  {
    changed = true;
    return Driver3dRenderTarget::operator =(a);
  }

  TrackDriver3dRenderTarget & operator = (TrackDriver3dRenderTarget &a)
  {
    changed = true;
    (*this) = (const Driver3dRenderTarget &)a;
    srgb_bb = a.srgb_bb;
    return *this;
  }

  TrackDriver3dRenderTarget()
  {
    changed = true;
    srgb_bb = false;
    aa_textures_in_states = false;
    unmarked_aa_rts = true;
  }
};

TrackDriver3dRenderTarget nextRtState, currentRtState;

drv3d_metal::Render::Viewport vp;

static inline bool is_depth_format(unsigned int f)
{
  f &= TEXFMT_MASK;
  return f >= TEXFMT_FIRST_DEPTH && f <= TEXFMT_LAST_DEPTH;
}

bool setRtState(TrackDriver3dRenderTarget &oldrt, TrackDriver3dRenderTarget &rt)
{
  if (!(rt.used&Driver3dRenderTarget::TOTAL_MASK))
  {
    return false;
  }

  if (rt.isBackBufferColor() && (!oldrt.isBackBufferColor()/* || oldrt.srgb_bb != rt.srgb_bb*/))
  {
    render.restoreRT();

    for (uint32_t old_used_left = (oldrt.used&Driver3dRenderTarget::COLOR_MASK) >> 1, ri = 1; old_used_left; old_used_left >>= 1, ri++)
    {
      render.setRT(ri, NULL, 0, 0);
    }
  }
  else
  if (!rt.isBackBufferColor())
  {
    for (uint32_t used_left = rt.used&Driver3dRenderTarget::COLOR_MASK, old_used_left = oldrt.used&Driver3dRenderTarget::COLOR_MASK, ri = 0
        ; used_left || old_used_left
        ; used_left >>= 1, old_used_left >>= 1, ri++)
    {
      if (!(used_left & 1))
      {
        //if ((old_used_left & 1) && ri)
        {
          render.setRT(ri, NULL, 0, 0);
        }

        continue;
      }

      render.setRT(ri, (drv3d_metal::Texture*)rt.color[ri].tex, rt.color[ri].level, rt.color[ri].face);
    }
  }

  if (!rt.isDepthUsed())// && oldrt.isDepthUsed())
  {
    render.setDepth(NULL, 0, true);
  }
  else
  if (rt.isDepthUsed())
  {
    if (rt.isBackBufferDepth())
    {
      if (rt.isBackBufferColor())
      {
        render.restoreDepth();
      }
      else
      {
        drv3d_metal::Texture* depth = NULL;

        drv3d_metal::Texture* ri = nullptr;
        for (uint32_t i = 0; i < 8; ++i)
          if (rt.color[i].tex)
            ri = (drv3d_metal::Texture*)rt.color[i].tex;

        for (int i = 0; i < tmp_depth.size() && ri; i++)
        {
            if (ri->getWidth() == tmp_depth[i]->getWidth() &&
                ri->getHeight() == tmp_depth[i]->getHeight())
            {
                depth = tmp_depth[i];
                break;
            }
        }

        if (!depth && ri)
        {
            depth = new drv3d_metal::Texture(ri->getWidth(), ri->getHeight(), 1, 1, RES3D_TEX,
                                             TEXCF_UNORDERED, TEXFMT_DEPTH24, "depth_text_temp", true, true);
            tmp_depth.push_back(depth);
        }

        render.setDepth(depth, 0, false);
      }
    }
    else
    {
      render.setDepth((drv3d_metal::Texture*)rt.depth.tex, rt.depth.face, rt.used&Driver3dRenderTarget::DEPTH_READONLY);

      if (!rt.isColorUsed())
      {
        //NULL RT
      }
    }
  }

  return true;
}

bool dirty_render_target()
{
  if (!nextRtState.changed || nextRtState.isEqual(currentRtState))
  {
    if (vp.used)
    {
      render.setViewport(vp.x, vp.y, vp.w, vp.h, vp.minz, vp.maxz);
    }

    nextRtState.changed = false;

    return true;
  }

  bool ret = setRtState(currentRtState, nextRtState);
  currentRtState = nextRtState;
  nextRtState.changed = false;

  if (vp.used)
  {
    render.setViewport(vp.x, vp.y, vp.w, vp.h, vp.minz, vp.maxz);
  }

  return ret;
}

void set_depth_impl(::BaseTexture *tex, int face, bool readonly)
{
  nextRtState.changed = true;

  if (!tex)
  {
    nextRtState.removeDepth();
  }
  else
  {
    drv3d_metal::Texture *depth = (drv3d_metal::Texture*)tex;
    G_ASSERTF(is_depth_format(depth->cflg), "%s", depth->getResName());
    nextRtState.setDepth(depth, face, readonly);
  }
}

bool set_render_target_impl(int rt_index, ::BaseTexture* rt, int level, int layer)
{
  nextRtState.changed = true;
  vp.used = false;

  if (!rt)
  {
    nextRtState.removeColor(rt_index);
    return true;
  }

  if (!rt_index)
  {
    nextRtState.removeDepth();
  }

  drv3d_metal::Texture*tex = (drv3d_metal::Texture*)rt;

  if (is_depth_format(tex->cflg))
  {
    logerr("can't set_render_target with depth texture %s, use set_depth instead", tex->getTexName());
    return false;
  }

  if (!(tex->cflg&TEXCF_RTARGET))
  {
    logerr("can't set_render_target with non rtarget texture %s", tex->getTexName());
    return false;
  }

  nextRtState.setColor(rt_index, rt, level, layer);

  return true;
}

bool d3d::clearview(int what, E3DCOLOR c, float z, uint32_t stencil)
{
  dirty_render_target();
  render.clearView(what, c, z, stencil);

  return true;
}

bool d3d::set_render_target()
{
  nextRtState.setBackbufColor();
  nextRtState.setBackbufDepth();
  nextRtState.changed = true;

  vp.used = false;

  return true;
}

bool d3d::set_depth(::Texture *tex, bool readonly)
{
  set_depth_impl(tex, 0, readonly);

  return true;
}

bool d3d::set_depth(BaseTexture *tex, int face, bool readonly)
{
  set_depth_impl(tex, face, readonly);
  return true;
}

bool d3d::set_backbuf_depth()
{
  nextRtState.changed = true;
  nextRtState.setBackbufDepth();

  return true;
}

bool d3d::set_render_target(int rt_index, BaseTexture* rt, int level)
{
  return set_render_target_impl(rt_index, rt, level, 0);
}

bool d3d::set_render_target(int rt_index, BaseTexture* rt, int fc, int level)
{
  return set_render_target_impl(rt_index, rt, level, fc);
}

void d3d::get_render_target(Driver3dRenderTarget& out_rt)
{
  out_rt = nextRtState;
}

bool d3d::set_render_target(const Driver3dRenderTarget& rt)
{
  nextRtState = rt;
  nextRtState.changed = true;

  vp.used = false;

  return true;
}

bool d3d::get_target_size(int &w, int &h)
{
  TrackDriver3dRenderTarget* rt = &currentRtState;

  if (nextRtState.changed)
  {
    rt = &nextRtState;
  }

  if (rt->isBackBufferColor())
  {
    d3d::get_screen_size(w, h);
    return true;
  }
  else
  if (rt->isColorUsed(0))
  {
    return d3d::get_render_target_size(w, h, rt->color[0].tex, rt->color[0].level);
  }
  else
  if (rt->isDepthUsed() && rt->depth.tex)
  {
    return d3d::get_render_target_size(w, h, rt->depth.tex, rt->depth.level);
  }
  else
  {
    d3d::get_render_target_size(w, h, NULL, 0);
  }

  return false;
}

bool d3d::get_render_target_size(int &w, int &h, BaseTexture *rt_tex, int lev)
{
  w = h = 0;

  if (rt_tex == NULL)
  {
    w = render.scr_wd;
    h = render.scr_ht;

    return true;
  }

  drv3d_metal::Texture* tex = (drv3d_metal::Texture*)rt_tex;

  if (tex == NULL || !(tex->cflg & TEXCF_RTARGET))
  {
    return false;
  }

  G_ASSERT(lev < tex->mipLevels);

  w = max(1, tex->width >> lev);
  h = max(1, tex->height >> lev);

  return true;
}

bool d3d::setview(int x, int y, int w, int h, float minz, float maxz)
{
  vp.used = true;

  vp.x = x;
  vp.y = y;

  vp.w = w;
  vp.h = h;

  vp.minz = minz;
  vp.maxz = maxz;

  return true;
}

bool d3d::setviews(dag::ConstSpan<Viewport> viewports)
{
  G_UNUSED(viewports);
  G_ASSERTF(false, "Not implemented");
  return false;
}

bool d3d::getview(int &x, int &y, int &w, int &h, float &minz, float &maxz)
{
  if (vp.used)
  {
    x = vp.x;
    y = vp.y;

    w = vp.w;
    h = vp.h;

    minz = vp.minz;
    maxz = vp.maxz;

    return true;
  }

  x = 0;
  y = 0;

  minz = 0.0f;
  maxz = 1.0f;

  TrackDriver3dRenderTarget* rs = &currentRtState;

  if (nextRtState.changed)
  {
    rs = &nextRtState;
  }

  if (rs->isBackBufferColor())
  {
    w = render.scr_wd;
    h = render.scr_ht;
  }
  else
  {
    if (rs->color[0].tex)
    {
      w = ((drv3d_metal::Texture*)rs->color[0].tex)->getWidth();
      h = ((drv3d_metal::Texture*)rs->color[0].tex)->getHeight();
    }
    else if (rs->depth.tex)
    {
      w = ((drv3d_metal::Texture*)rs->depth.tex)->getWidth();
      h = ((drv3d_metal::Texture*)rs->depth.tex)->getHeight();
    }
    else
    {
      w = render.scr_wd;
      h = render.scr_ht;
    }
  }

  return true;
}

bool d3d::setscissor(int x, int y, int w, int h)
{
  render.setScissor(x,y,w,h);
  return true;
}

bool d3d::setscissors(dag::ConstSpan<ScissorRect> scissorRects)
{
  G_UNUSED(scissorRects);
  G_ASSERTF(false, "Not implemented");
  return false;
}

float d3d::get_screen_aspect_ratio()
{
  return (float)render.scr_wd / (float)render.scr_ht;
}

bool d3d::stretch_rect(BaseTexture *src, BaseTexture *dst, RectInt *rsrc, RectInt *rdst)
{
  if (!src)
  {
    return false;
  }

  drv3d_metal::Texture* mtl_dsc = (drv3d_metal::Texture*)-1;

  if (dst)
  {
    mtl_dsc = (drv3d_metal::Texture*)dst;
  }

  render.copyTex((drv3d_metal::Texture*)src, mtl_dsc, rsrc, rdst);
  return true;
}

bool d3d::copy_from_current_render_target(BaseTexture* to_tex)
{
  TrackDriver3dRenderTarget* rs = nextRtState.changed ? &nextRtState : &currentRtState;

  drv3d_metal::Texture* src = rs->isBackBufferColor() ? (drv3d_metal::Texture*)-1 : (drv3d_metal::Texture*)rs->color[0].tex;
  G_ASSERT(src);
  render.copyTex(src, (drv3d_metal::Texture*)to_tex);

  return false;
}
