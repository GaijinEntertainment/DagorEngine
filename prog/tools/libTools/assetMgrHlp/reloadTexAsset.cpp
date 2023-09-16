#include <assets/assetHlp.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetMsgPipe.h>
#include <image/dag_texPixel.h>
#include <image/dag_loadImage.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_texMgr.h>
#include <3d/ddsxTex.h>
#include <libTools/util/strUtil.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <ioSys/dag_memIo.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_texMetaData.h>

#include "../../../engine2/lib3d/texMgrData.h"
using texmgr_internal::RMGR;


bool reload_changed_texture_asset(const DagorAsset &a)
{
  static String texName;
  static String d3dErrTex;
  texName.printf(64, "%s*", a.getName());

  TEXTUREID texAssetId = get_managed_texture_id(texName);
  TEXTUREID texId = get_managed_texture_id(a.getSrcFilePath());

  if ((texAssetId == BAD_TEXTUREID) && (texId == BAD_TEXTUREID))
    return false;

  discard_unused_managed_texture(texAssetId);
  discard_unused_managed_texture(texId);

  BaseTexture *bAssetTex =
    ((texAssetId != BAD_TEXTUREID) && (get_managed_texture_refcount(texAssetId) > 0)) ? acquire_managed_tex(texAssetId) : NULL;

  BaseTexture *bTex = ((texId != BAD_TEXTUREID) && (get_managed_texture_refcount(texId) > 0)) ? acquire_managed_tex(texId) : NULL;

  Texture *atex = (bAssetTex && (bAssetTex->restype() == RES3D_TEX)) ? (Texture *)bAssetTex : NULL;
  Texture *tex = (bTex && (bTex->restype() == RES3D_TEX)) ? (Texture *)bTex : NULL;

  if (!atex && !tex)
  {
  err_exit:
    if (bAssetTex)
      release_managed_tex(texAssetId);
    if (bTex)
      release_managed_tex(texId);
    return false;
  }

  TextureInfo tinfo;
  if (atex)
    atex->getinfo(tinfo);
  else
    tex->getinfo(tinfo); //-V522
  TextureMetaData tmd;
  tmd.read(a.props, a.resolveEffProfileTargetStr("PC", NULL));
  struct AutoTexIdHolder
  {
    TEXTUREID id;

    AutoTexIdHolder(TextureMetaData &tmd) : id(BAD_TEXTUREID)
    {
      if (tmd.baseTexName.empty())
        return;
      id = get_managed_texture_id(String(64, "%s*", tmd.baseTexName.str()));
      if (id == BAD_TEXTUREID)
        return;
      RMGR.incBaseTexRc(id);
      acquire_managed_tex(id);
    }
    ~AutoTexIdHolder()
    {
      if (id == BAD_TEXTUREID)
        return;
      release_managed_tex(id);
      RMGR.decBaseTexRc(id);
    }
  };
  AutoTexIdHolder bt_ref(tmd);

  int target_lev = get_log2i(max(tinfo.w, tinfo.h));
  const char *ext = ::get_file_ext(a.getSrcFileName());
  if (a.props.getBool("convert", false))
  {
    ddsx::Buffer b;
    if (texconvcache::get_tex_asset_built_ddsx((DagorAsset &)a, b, _MAKE4C('PC'), NULL, NULL))
    {
      ddsx::Header &bq_hdr = *(ddsx::Header *)b.ptr;
      int bq_lev = get_log2i(max(bq_hdr.w, bq_hdr.h));
      if (DagorAsset *hq_a = a.getMgr().findAsset(String(0, "%s$hq", a.getName()), a.getType()))
      {
        Texture *bt = atex ? atex : tex;
        ddsx::Buffer hq_b;
        if (texconvcache::get_tex_asset_built_ddsx(*hq_a, hq_b, _MAKE4C('PC'), NULL, NULL))
        {
          ddsx::Header &hq_hdr = *(ddsx::Header *)hq_b.ptr;
          int hq_lev = get_log2i(max(hq_hdr.w, hq_hdr.h));
          if (hq_hdr.levels)
          {
            ddsx::Header hdr = hq_hdr;
            hdr.levels += ((ddsx::Header *)b.ptr)->levels;

            InPlaceMemLoadCB crd(b.ptr, b.len);
            crd.seekrel(sizeof(ddsx::Header));
            if (!d3d_load_ddsx_tex_contents(bt, texAssetId, bt_ref.id, bq_hdr, crd, 0, target_lev - bq_lev, 1))
              a.getMgr().getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::ERROR, String(0, "error loading tex %s", a.getName()),
                NULL, a.getSrcFilePath());
            b.free();

            InPlaceMemLoadCB crd_hq(hq_b.ptr, hq_b.len);
            crd_hq.seekrel(sizeof(ddsx::Header));
            if (!d3d_load_ddsx_tex_contents(bt, texAssetId, bt_ref.id, hq_hdr, crd_hq, 0, 0, bq_lev))
              a.getMgr().getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::ERROR, String(0, "error loading tex %s", hq_a->getName()),
                NULL, a.getSrcFilePath());
            hq_b.free();

            goto normal_exit;
          }
          hq_b.free();
        }
        else
          a.getMgr().getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::ERROR,
            String(0, "cannot convert <%s> tex asset", hq_a->getName()), NULL, a.getSrcFilePath());
      }
      InPlaceMemLoadCB crd(b.ptr, b.len);

      crd.seekto(sizeof(ddsx::Header));
      bool loadErr = atex && !d3d_load_ddsx_tex_contents(atex, texAssetId, bt_ref.id, bq_hdr, crd, 0, target_lev - bq_lev, 1);
      if (loadErr)
        d3dErrTex = d3d::get_last_error();
      crd.seekto(sizeof(ddsx::Header));
      if (tex && !d3d_load_ddsx_tex_contents(tex, texId, bt_ref.id, bq_hdr, crd, 0, target_lev - bq_lev, 1))
      {
        loadErr = true;
        d3dErrTex = d3d::get_last_error();
      }

      b.free();

      if (loadErr)
      {
        a.getMgr().getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::ERROR,
          String(512, "cant reload DDSx tex data: %s", d3dErrTex.str()), NULL, a.getSrcFilePath());
        goto err_exit;
      }
    }
    else
    {
      a.getMgr().getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::ERROR, String(128, "cannot convert <%s> tex asset", a.getName()),
        NULL, NULL);
      goto err_exit;
    }
  }
  else if (stricmp(ext, "dds") == 0)
  {
    a.getMgr().getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::ERROR, String(512, "cant reload DDS tex data: %s", d3dErrTex.str()),
      NULL, a.getSrcFilePath());
    goto err_exit;
  }
  else
  {
    TexImage32 *img = load_image(a.getSrcFilePath(), midmem);
    if (!img)
      goto err_exit;

    if ((img->w != tinfo.w) || (img->h != tinfo.h))
    {
      a.getMgr().getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::WARNING,
        String(512, "cant reload tex data: size %dx%d -> %dx%d", tinfo.w, tinfo.h, img->w, img->h), NULL, a.getSrcFilePath());
      goto err_exit;
    }

    bool loadErr = false;
    int stride;
    uint8_t *p;

    if (atex)
    {
      if (atex->lockimg((void **)&p, stride, 0, TEXLOCK_WRITE))
      {
        memcpy(p, img->getPixels(), img->w * img->h * 4);
        atex->unlockimg();
      }
      else
        loadErr = true;
    }

    if (tex)
    {
      if (tex->lockimg((void **)&p, stride, 0, TEXLOCK_WRITE))
      {
        memcpy(p, img->getPixels(), img->w * img->h * 4);
        tex->unlockimg();
      }
      else
        loadErr = true;
    }

    if (loadErr)
    {
      a.getMgr().getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::ERROR, "cant reload tex data", NULL, a.getSrcFilePath());
      goto err_exit;
    }
  }

normal_exit:
  if (Texture *bt = atex ? atex : tex)
  {
    bt->texaddru(tmd.d3dTexAddr(tmd.addrU));
    bt->texaddrv(tmd.d3dTexAddr(tmd.addrV));
    if (bt->restype() == RES3D_VOLTEX)
      reinterpret_cast<VolTexture *>(bt)->texaddrw(tmd.d3dTexAddr(tmd.addrW));
    bt->setAnisotropy(tmd.calcAnisotropy(::dgs_tex_anisotropy));
    if (tmd.needSetBorder())
      bt->texbordercolor(tmd.borderCol);
    if (tmd.lodBias)
      bt->texlod(tmd.lodBias / 1000.0f);
    if (tmd.needSetTexFilter())
      bt->texfilter(tmd.d3dTexFilter());
    if (tmd.needSetMipFilter())
      bt->texfilter(tmd.d3dMipFilter());
  }

  if (bAssetTex)
    release_managed_tex(texAssetId);
  if (bTex)
    release_managed_tex(texId);

  a.getMgr().getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::NOTE, String(256, "texture '%s' reloaded", a.getName()), NULL, NULL);

  return true;
}
