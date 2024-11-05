// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_startupTex.h>
#include <3d/dag_createTex.h>
#include <image/dag_loadImage.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <3d/dag_texMgr.h>
#include <image/dag_texPixel.h>
#include <util/dag_texMetaData.h>
#include <util/fnameMap.h>
#include <util/dag_string.h>
#include <util/dag_strUtil.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_log.h>
#include <math/dag_adjpow2.h>
#include <util/dag_hash.h>

static inline void tex_premul_alpha(TexImage32 *im)
{
  for (TexPixel32 *p = im->getPixels(), *pe = p + im->w * im->h; p < pe; p++)
    if (p->a == 0)
      p->r = p->g = p->b = 0;
    else if (p->a != 255)
      p->r = (unsigned(p->r) * p->a + 127) / 255, p->g = (unsigned(p->g) * p->a + 127) / 255,
      p->b = (unsigned(p->b) * p->a + 127) / 255;
}

static void set_img_reloadable_callback(BaseTexture *t, const char *fn, const char *fn_ext, bool premul_a)
{
  struct TexReload : public IAllocImg
  {
    Texture *tex;
    int tw, th;
    const char *fname;

    TexReload(Texture *t, int w, int h, const char *fn) : tex(t), tw(w), th(h), fname(fn) {}

    virtual void *allocImg32(int w, int h, TexPixel32 **out_data, int *out_stride)
    {
      if (w == tw && h == th)
      {
        if (tex->lockimgEx(out_data, *out_stride, 0, TEXLOCK_WRITE | TEXLOCK_DELSYSMEMCOPY) && *out_data)
          return tex;
        logerr("cannot lock texture %dx%d to load image from %s", w, h, fname);
        return NULL;
      }
      logerr("cannot reload texture %dx%d != %dx%d from %s", tw, th, w, h, fname);
      return NULL;
    }
    virtual void finishImg32Fill(void *img, TexPixel32 *) { ((Texture *)img)->unlockimg(); }
    virtual void freeImg32(void *) {}
  };

  struct ReloadImageFile : public BaseTexture::IReloadData
  {
    const char *fn = NULL;
    const char *fn_ext = NULL;
    bool premulAlpha = false;

    static bool is_simple_fname(const char *fn)
    {
      for (int i = 0; i < 8; i++)
        if (fn[i] == ':')
        {
          if (i > 2 && strncmp(fn + i + 1, "//", 2) == 0)
            return false; // URL-like fname
        }
        else if (!fn[i])
          break;

      if (const char *suff = dd_get_fname_ext(fn))
        return strchr(suff, ':') == NULL; // has meta-data suffix after extension
      else
        return false; // no extension
      return true;    // simple filename
    }

    virtual void reloadD3dRes(BaseTexture *bt)
    {
      if (is_simple_fname(fn) && !dd_file_exists(fn))
      {
        logerr("cannot restore texture, file is missing: %s", fn);
        return;
      }

      Texture *t = (Texture *)bt;
      TextureInfo ti;
      t->getinfo(ti);

      TexReload a(t, ti.w, ti.h, fn);
      if (!premulAlpha && load_image2(fn, a, fn_ext))
        return;

      TexImage32 *im = load_image(fn, tmpmem, NULL, fn_ext);
      if (!im)
        return;

      if (premulAlpha)
        tex_premul_alpha(im);

      if (im->w == ti.w && im->h == ti.h)
      {
        TexPixel32 *dest, *src = (TexPixel32 *)(im + 1);
        int stride;
        if (t->lockimgEx(&dest, stride, 0, TEXLOCK_WRITE | TEXLOCK_DELSYSMEMCOPY) && dest)
        {
          for (int i = 0; i < ti.h; i++, dest += stride / sizeof(*dest), src += ti.w)
            memcpy(dest, src, ti.w * sizeof(*dest));
          t->unlockimg();
        }
      }
      memfree(im, tmpmem);
    }
    virtual void destroySelf() { delete this; }
  };

  if (!t)
    return;
  ReloadImageFile *rld = new ReloadImageFile;
  if (t->setReloadCallback(rld))
  {
    rld->fn = dagor_fname_map_add_fn(fn);
    rld->fn_ext = fn_ext ? dagor_fname_map_add_fn(fn_ext) : NULL;
    rld->premulAlpha = premul_a;
  }
  else
    delete rld;
}


class LoadableCreateTexFactory : public ICreateTexFactory
{
public:
  virtual BaseTexture *createTex(const char *fn, int flg, int /*levels*/, const char *fn_ext, const TextureMetaData &tmd)
  {
    int levels = 1; // since noone else can autogenerate mips...
    if (flg & TEXCF_DYNAMIC)
      flg &= ~TEXCF_LOADONCE;

    char tmptexname[64];
    const char *texName = fn;
    static const char b64pref[] = "b64://";
    if (DAGOR_UNLIKELY(*(const int *)fn == _MAKE4C('b64:')))
    {
      G_ASSERT(strncmp(fn, b64pref, sizeof(b64pref) - 1) == 0);
      const char *hb = fn + sizeof(b64pref) - 1;
      const char *pd = strrchr(hb, '.');
      size_t hl = pd ? (pd - hb) : strlen(hb);
      snprintf(tmptexname, sizeof(tmptexname), "%s<%#x>%s", b64pref, mem_hash_fnv1(hb, hl), pd ? pd : "");
      texName = tmptexname;
    };

    struct TexAlloc : public IAllocImg
    {
      TexAlloc(int f, int l, const char *fn) : flg(f), levels(l), fname(fn) {}

      virtual void *allocImg32(int w, int h, TexPixel32 **out_data, int *out_stride)
      {
        int f = flg;
        Texture *t;
        {
          d3d::LoadingAutoLock gpuLock;
          t = d3d::create_tex(NULL, w, h, f, levels, fname);
        }
        if (!t)
          logerr("cannot create texture %dx%d to load image from %s", w, h, fname);
        else if (!t->lockimgEx(out_data, *out_stride, 0, TEXLOCK_WRITE | TEXLOCK_DELSYSMEMCOPY) || !*out_data)
        {
          logerr("cannot lock texture %dx%d to load image from %s", w, h, fname);
          destroy_d3dres(t);
          return NULL;
        }
        return t;
      }
      virtual void finishImg32Fill(void *img, TexPixel32 *) { ((Texture *)img)->unlockimg(); }
      virtual void freeImg32(void *img) { destroy_d3dres((Texture *)img); }

      int flg, levels;
      const char *fname;
    } a(flg | TEXFMT_A8R8G8B8 | TEXCF_LINEAR_LAYOUT | TEXCF_LOADONCE, levels, texName);

    // try load image directly to texture...
    if ((tmd.flags & tmd.FLG_NONPOW2) && levels)
      a.levels = 1;
    Texture *t = (tmd.flags & tmd.FLG_NONPOW2) && !(tmd.flags & tmd.FLG_PREMUL_A) ? (Texture *)load_image2(fn, a, fn_ext) : NULL;
    TexImage32 *im;

    if (t)
      goto setup_tex_props;

    // fallback to old (wasty) method
    im = load_image(fn, tmpmem, NULL, fn_ext);
    if (!im)
      return NULL;

    flg |= TEXCF_RGB | TEXCF_ABEST | TEXCF_LOADONCE;
    if (flg & TEXCF_DYNAMIC)
      flg &= ~TEXCF_LOADONCE;

    if (tmd.flags & tmd.FLG_PREMUL_A)
      tex_premul_alpha(im);

    {
      d3d::LoadingAutoLock gpuLock;
      t = d3d::create_tex(im, 0, 0, flg, levels, texName);
    }
    memfree(im, tmpmem);
    if (!t)
      return NULL;

  setup_tex_props:
    apply_gen_tex_props(t, tmd);
    TEXTUREID tid = get_managed_texture_id(fn);
    set_texture_separate_sampler(tid, get_sampler_info(tmd));
    if (!(flg & TEXCF_SYSTEXCOPY))
      set_img_reloadable_callback(t, fn, fn_ext, (tmd.flags & tmd.FLG_PREMUL_A) ? true : false);
    return t;
  }
};

static LoadableCreateTexFactory loadable_create_tex_factory;

void register_loadable_tex_create_factory() { add_create_tex_factory(&loadable_create_tex_factory); }

struct AnyVromfsTexCreateFactory : public ICreateTexFactory
{
  virtual BaseTexture *createTex(const char *fn, int flg, int levels, const char *fn_ext, const TextureMetaData &tmd)
  {
    if (fn_ext)
      return NULL;

    if (!vromfs_first_priority)
    {
      for (int i = 0; i < useExt.size(); i++)
      {
        String new_fn(0, "%s%s", fn, useExt[i]);
        if (dd_file_exists(new_fn))
          return create_texture_via_factories(new_fn, flg, levels, useExt[i], tmd, this);
      }
      return NULL;
    }

    for (int i = 0; i < useExt.size(); i++)
    {
      String new_fn(0, "%s%s", fn, useExt[i]);
      if (::vromfs_check_file_exists(new_fn))
        return create_texture_via_factories(new_fn, flg, levels, useExt[i], tmd, this);
    }

    return NULL;
  }

  int init(const char *try_order) { return parse_string_list(useExt, try_order, "."); }

  Tab<String> useExt;
};

static AnyVromfsTexCreateFactory any_tex_create_factory;

void register_any_vromfs_tex_create_factory(const char *try_order)
{
  if (any_tex_create_factory.init(try_order))
    add_create_tex_factory(&any_tex_create_factory);
}
