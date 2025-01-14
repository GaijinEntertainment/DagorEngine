// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_DRIVER 1

#include <generic/dag_tab.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_commands.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug.h>
#include <ioSys/dag_genIo.h>
#include <image/dag_texPixel.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_renderTarget.h>

#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <util/dag_string.h>
#include <util/dag_watchdog.h>

#include "render.h"
#include "textureloader.h"
#include "drv_log_defs.h"

using namespace drv3d_metal;

struct TrackDriver3dRenderTarget : public Driver3dRenderTarget
{
  bool changed, srgb_bb;

  bool isEqual(const Driver3dRenderTarget &a) const
  {
    if ((used&TOTAL_MASK) != (a.used&TOTAL_MASK))
    {
      return false;
    }

    if (used&DEPTH && depth.tex != a.depth.tex || depth.face != a.depth.face || depth.level != a.depth.level)
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
  }
};

TrackDriver3dRenderTarget nextRtState, currentRtState;

drv3d_metal::Render::Viewport vp;

namespace d3d
{
struct RenderPass
{
  struct Subpass
  {
    struct Attachment
    {
      uint32_t index = ~0u;
      uint32_t resolve_index = ~0u;
      MTLLoadAction load_action = MTLLoadActionDontCare;
      MTLStoreAction store_action = MTLStoreActionDontCare;
    };
    struct PassInput
    {
      uint32_t dst_slot = 0;
      uint32_t src_slot = 0;
    };
    eastl::vector<Attachment> colors;
    Attachment depth_stencil;
    eastl::vector<PassInput> inputs;
  };

  eastl::vector<RenderPassTarget> textures;
  eastl::vector<Subpass> subpasses;
  String name;
  RenderPassArea area;

  static RenderPass *active;
  static uint32_t active_subpass;

  static void bind();
};
}

static inline bool is_depth_format(unsigned int f)
{
  f &= TEXFMT_MASK;
  return f >= TEXFMT_FIRST_DEPTH && f <= TEXFMT_LAST_DEPTH;
}

bool setRtState(TrackDriver3dRenderTarget &oldrt, TrackDriver3dRenderTarget &rt)
{
  bool isBackBufferColor = rt.isBackBufferColor();
  if (isBackBufferColor && (!oldrt.isBackBufferColor() ||
    (rt.used&Driver3dRenderTarget::COLOR_MASK) != (oldrt.used&Driver3dRenderTarget::COLOR_MASK)))
    render.restoreRT();

  for (uint32_t used_left = (rt.used&Driver3dRenderTarget::COLOR_MASK) >> ((uint32_t)isBackBufferColor),
      old_used_left = (oldrt.used&Driver3dRenderTarget::COLOR_MASK) >> ((uint32_t)isBackBufferColor),
      ri = (uint32_t)isBackBufferColor;
      used_left || old_used_left;
      used_left >>= 1, old_used_left >>= 1, ri++)
  {
    if (!(used_left & 1))
    {
      //if ((old_used_left & 1) && ri)
      {
        render.setRT(ri, drv3d_metal::RenderAttachment {} );
      }

      continue;
    }

    RenderAttachment attach =
    {
      .texture = (drv3d_metal::Texture*)rt.color[ri].tex,
      .level = rt.color[ri].level,
      .layer = rt.color[ri].face
    };
    if (attach.texture && attach.texture->memoryless)
    {
      attach.load_action = MTLLoadActionDontCare;
      if ((attach.texture->cflg & TEXCF_TRANSIENT) || attach.texture->samples == 1)
        attach.store_action = MTLStoreActionDontCare;
      else
      {
        attach.store_action = MTLStoreActionMultisampleResolve;
        attach.resolve_target = attach.texture;
      }

    }
    render.setRT(ri, attach);
  }

  if (!rt.isDepthUsed())// && oldrt.isDepthUsed())
  {
    render.setDepth(drv3d_metal::RenderAttachment { .store_action = MTLStoreActionDontCare });
  }
  else
  if (rt.isDepthUsed())
  {
    RenderAttachment attach =
    {
      .texture = (drv3d_metal::Texture*)rt.depth.tex,
      .store_action = rt.used&Driver3dRenderTarget::DEPTH_READONLY ? MTLStoreActionDontCare : MTLStoreActionStore,
      .layer = rt.depth.face
    };
    if (attach.texture && attach.texture->memoryless)
    {
      attach.load_action = MTLLoadActionDontCare;
      if ((attach.texture->cflg & TEXCF_TRANSIENT) || attach.texture->samples == 1)
        attach.store_action = MTLStoreActionDontCare;
      else
      {
        attach.store_action = MTLStoreActionMultisampleResolve;
        attach.resolve_target = attach.texture;
      }

    }
    render.setDepth(attach);

    if (!rt.isColorUsed())
    {
      //NULL RT
    }
  }

  return true;
}

bool dirty_render_target()
{
  if (d3d::RenderPass::active)
    return false;

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
    D3D_ERROR("can't set_render_target with depth texture %s, use set_depth instead", tex->getTexName());
    return false;
  }

  if (!(tex->cflg&TEXCF_RTARGET))
  {
    D3D_ERROR("can't set_render_target with non rtarget texture %s", tex->getTexName());
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
  nextRtState.removeDepth();
  nextRtState.changed = true;

  vp.used = false;

  return true;
}

bool d3d::set_depth(::Texture *tex, DepthAccess access)
{
  set_depth_impl(tex, 0, access == DepthAccess::SampledRO);

  return true;
}

bool d3d::set_depth(BaseTexture *tex, int face, DepthAccess access)
{
  set_depth_impl(tex, face, access == DepthAccess::SampledRO);
  return true;
}

bool d3d::set_render_target(int rt_index, BaseTexture* rt, uint8_t level)
{
  return set_render_target_impl(rt_index, rt, level, 0);
}

bool d3d::set_render_target(int rt_index, BaseTexture* rt, int fc, uint8_t level)
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

bool d3d::get_render_target_size(int &w, int &h, BaseTexture *rt_tex, uint8_t level)
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

  G_ASSERT(level < tex->mipLevels);

  w = max(1, tex->width >> level);
  h = max(1, tex->height >> level);

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

  drv3d_metal::Texture* mtl_dsc = render.backbuffer;

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

  drv3d_metal::Texture* src = rs->isBackBufferColor() ? render.backbuffer : (drv3d_metal::Texture*)rs->color[0].tex;
  G_ASSERT(src);
  render.copyTex(src, (drv3d_metal::Texture*)to_tex);

  return false;
}

namespace d3d
{

RenderAttachment toRenderAttachment(const RenderPass::Subpass::Attachment &attach, eastl::vector<RenderPassTarget> targets)
{
  const RenderPassTarget &target = targets[attach.index];
  return
  {
    .texture = (drv3d_metal::Texture *)target.resource.tex,
    .resolve_target = attach.resolve_index != ~0u ? (drv3d_metal::Texture *)targets[attach.resolve_index].resource.tex : nullptr,
    .load_action = attach.load_action,
    .store_action = attach.store_action,
    .level = target.resource.mip_level,
    .layer = target.resource.layer,
    .clear_value[0] = target.clearValue.asFloat[0],
    .clear_value[1] = target.clearValue.asFloat[1],
    .clear_value[2] = target.clearValue.asFloat[2],
    .clear_value[3] = target.clearValue.asFloat[3]
  };
}

void RenderPass::bind()
{
  using namespace drv3d_metal;

  G_ASSERT(active);
  G_ASSERT(active_subpass < active->subpasses.size());

  Subpass &pass = active->subpasses[active_subpass];

  for (int index = 0; index < Program::MAX_SIMRT; ++index)
  {
    render.setRT(index, {});
    nextRtState.removeColor(index);
  }
  nextRtState.removeDepth();
  render.setDepth({});

  int samples = -1, layers = -1;
  for (auto &color : pass.colors)
  {
    auto attach = toRenderAttachment(color, active->textures);
    render.setRT(color.index, attach);
    nextRtState.setColor(color.index, attach.resolve_target ? attach.resolve_target : attach.texture, attach.level, attach.layer);
  }

  if (pass.depth_stencil.index != ~0u)
  {
    auto attach = toRenderAttachment(pass.depth_stencil, active->textures);
    render.setDepth(attach);
    nextRtState.setDepth(attach.resolve_target ? attach.resolve_target : attach.texture, attach.layer, attach.store_action == MTLStoreActionDontCare);
  }

  for (auto &tex : pass.inputs)
  {
    G_ASSERT(tex.src_slot < active->textures.size());
    const RenderPassTarget &target = active->textures[tex.src_slot];
    render.setTexture(STAGE_PS, tex.dst_slot, (drv3d_metal::Texture *)target.resource.tex, false, 0, false);
  }

  render.setViewport(active->area.left, active->area.top, active->area.width, active->area.height, active->area.minZ, active->area.maxZ);
}

RenderPass *RenderPass::active = nullptr;
uint32_t RenderPass::active_subpass = 0;

static uint32_t decodeSubpassAction(RenderPass::Subpass &pass, uint32_t action, const RenderPassBind &bind)
{
  switch (action & RP_TA_SUBPASS_MASK)
  {
    case RP_TA_SUBPASS_READ:
      break;
    case RP_TA_SUBPASS_WRITE:
      break;
    case RP_TA_SUBPASS_RESOLVE:
      break;
    case RP_TA_SUBPASS_KEEP:
      break;
    default:
      G_ASSERTF(0, "Unknown subpass mask action %d for subpass %d", action, bind.subpass);
  }
  return bind.target;
}

static MTLLoadAction decodeLoadAction(RenderPass::Subpass &pass, uint32_t load_action, const RenderPassBind &bind)
{
  switch (load_action)
  {
    case RP_TA_NONE:
      return MTLLoadActionLoad;
    case RP_TA_LOAD_READ:
      return MTLLoadActionLoad;
    case RP_TA_LOAD_CLEAR:
      return MTLLoadActionClear;
    case RP_TA_LOAD_NO_CARE:
      return MTLLoadActionDontCare;
    default:
      G_ASSERTF(0, "Unknown subpass load action %d for subpass %d", load_action, bind.subpass);
  }
  return MTLLoadActionDontCare;
}

static MTLStoreAction decodeStoreAction(RenderPass::Subpass &pass, uint32_t store_action, const RenderPassBind &bind)
{
  switch (store_action)
  {
    case RP_TA_NONE:
      return MTLStoreActionStore;
    case RP_TA_STORE_WRITE:
      return MTLStoreActionStore;
    case RP_TA_STORE_NONE:
    case RP_TA_STORE_NO_CARE:
      return MTLStoreActionDontCare;
    default:
      G_ASSERTF(0, "Unknown subpass store action %d for subpass %d", store_action, bind.subpass);
  }
  return MTLStoreActionDontCare;
}

RenderPass *create_render_pass(const RenderPassDesc &rp_desc)
{
  RenderPass* rp = new RenderPass;

  int subpass_count = -1;
  for (uint32_t i = 0; i < rp_desc.bindCount; ++i)
    subpass_count = std::max(subpass_count, rp_desc.binds[i].subpass);
  G_ASSERTF(subpass_count >= 0, "Render pass %s doesn't have any passes", rp_desc.debugName ? rp_desc.debugName : "(unknown)");

  rp->name = rp_desc.debugName ? rp_desc.debugName : "unknown rp";
  rp->textures.resize(rp_desc.targetCount);
  rp->subpasses.resize(subpass_count + 1);

  G_ASSERT(rp_desc.targetCount <= Program::MAX_SIMRT);
  RenderPass::Subpass::Attachment attachments[Program::MAX_SIMRT + 1];
  for (int i = 0; i < Program::MAX_SIMRT + 1; ++i)
  {
    attachments[i].load_action = MTLLoadActionDontCare;
    attachments[i].index = ~0u;
  }

  for (uint32_t i = 0; i < rp_desc.bindCount; ++i)
  {
    const RenderPassBind &bind = rp_desc.binds[i];
    if (bind.subpass == RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END && !render.has_image_blocks)
      continue;
    if (bind.action == RP_TA_NONE)
      continue;

    uint32_t subpass_action = bind.action & RP_TA_SUBPASS_MASK;
    uint32_t load_action = bind.action & RP_TA_LOAD_MASK;
    uint32_t store_action = bind.action & RP_TA_STORE_MASK;

    RenderPass::Subpass &pass = rp->subpasses[render.has_image_blocks ? 0 : bind.subpass];
    if (!render.has_image_blocks)
    {
      RenderPass::Subpass::Attachment attach;
      attach.index = bind.target;
      attach.load_action = decodeLoadAction(pass, load_action, bind);
      attach.store_action = decodeStoreAction(pass, store_action, bind);
      if (subpass_action == RP_TA_SUBPASS_WRITE)
        attach.store_action = MTLStoreActionStore;

      if (subpass_action == RP_TA_SUBPASS_READ)
      {
        RenderPass::Subpass::PassInput input;
        input.dst_slot = bind.slot + rp_desc.subpassBindingOffset;
        input.src_slot = bind.target;

        if (bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
          pass.depth_stencil = attach;
        else
          pass.inputs.push_back(input);
      }
      else if (subpass_action == RP_TA_SUBPASS_RESOLVE)
      {
        auto &attach = bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL ? pass.depth_stencil : pass.colors[bind.slot];
        G_ASSERT(attach.index != ~0u);
        G_ASSERT(bind.target != ~0u);
        attach.store_action = MTLStoreActionMultisampleResolve;
        attach.resolve_index = bind.target;
      }
      else if (bind.slot != RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
        pass.colors.push_back(attach);
      else
        pass.depth_stencil = attach;
    }
    else
    {
      G_ASSERT(subpass_action != RP_TA_SUBPASS_RESOLVE && "Not implemented for image blocks");
      if ((bind.action & (RP_TA_LOAD_MASK | RP_TA_STORE_MASK)) == 0)
        continue;

      int index = bind.slot != RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL ? bind.target : Program::MAX_SIMRT;
      if (bind.action & RP_TA_LOAD_MASK)
        attachments[index].load_action = decodeLoadAction(pass, load_action, bind);
      if (bind.action & RP_TA_STORE_MASK)
        attachments[index].store_action = decodeStoreAction(pass, store_action, bind);

      G_ASSERT(attachments[index].index == ~0u || attachments[index].index == bind.target);
      attachments[index].index = bind.target;
    }
  }

  if (render.has_image_blocks)
  {
    RenderPass::Subpass &pass = rp->subpasses[0];
    for (int i = 0; i < Program::MAX_SIMRT; ++i)
      if (attachments[i].index != ~0u)
        pass.colors.push_back(attachments[i]);
    pass.depth_stencil = attachments[Program::MAX_SIMRT];
  }

  return rp;
}

void delete_render_pass(RenderPass *rp)
{
  G_ASSERT(rp);
  delete rp;
}

void begin_render_pass(RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets)
{
  G_ASSERT(rp);
  G_ASSERT(RenderPass::active == nullptr);
  RenderPass::active = rp;

  for (size_t i = 0; i < rp->textures.size(); ++i)
    rp->textures[i] = targets[i];
  rp->area = area;

  G_ASSERT(rp->subpasses.empty() == false);
  RenderPass::bind();

  render.setRenderPass(true);
}

void next_subpass()
{
  G_ASSERT(RenderPass::active);
  G_ASSERT(RenderPass::active_subpass + 1 < RenderPass::active->subpasses.size());
  RenderPass::active_subpass++;
  if (render.has_image_blocks == false)
    RenderPass::bind();
  else
    render.setViewport(RenderPass::active->area.left, RenderPass::active->area.top, RenderPass::active->area.width, RenderPass::active->area.height, RenderPass::active->area.minZ, RenderPass::active->area.maxZ);
}

void end_render_pass()
{
  G_ASSERT(RenderPass::active);
  RenderPass::active = nullptr;
  RenderPass::active_subpass = 0;

  render.setRenderPass(false);

  nextRtState.changed = false;
  currentRtState = nextRtState;
}

#if DAGOR_DBGLEVEL > 0
void allow_render_pass_target_load()
{
}
#endif

}
