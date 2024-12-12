// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <de3_dxpFactory.h>
#include <assets/asset.h>
#include <assets/assetHlp.h>
#include <oldEditor/de_interface.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <coolConsole/coolConsole.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_createTex.h>
#include <drv/3d/dag_tex3d.h>
#include <generic/dag_smallTab.h>
#include <ioSys/dag_memIo.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <math/dag_adjpow2.h>
#include <util/dag_texMetaData.h>
#include <util/dag_string.h>
#include <3d/dag_texIdSet.h>
#include <3d/ddsxTex.h>
#include <3d/ddsxTexMipOrder.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>

#include "../../../../../engine/lib3d/texMgrData.h"
using texmgr_internal::RMGR;

static String gamePrefix;
namespace texmgr_internal
{
extern bool disable_add_warning;
TexQL max_allowed_q = TQL_high;
} // namespace texmgr_internal

static class DdsxTexPackFactory : public TextureFactory
{
public:
  DdsxTexPackFactory() { defFactory = NULL; }

  static void mark_cur_ql(unsigned idx, TexQL ql)
  {
    RMGR.levDesc[idx] = 0;
    RMGR.setLevDesc(idx, ql, 2);
    RMGR.resQS[idx].setQLev(2);
    RMGR.resQS[idx].setLdLev(ql == TQL_stub ? 1 : 2);
    RMGR.resQS[idx].setMaxQL(ql, ql);
  }
  static auto console() { return EDITORCORE ? &EDITORCORE->getConsole() : nullptr; }

  bool scheduleTexLoading(TEXTUREID id, TexQL ql) override
  {
    if (get_managed_texture_refcount(id) == 0)
      return false;
    if (ql != TQL_stub && forceDiscardTid.has(id))
      return false;
    if (texmgr_internal::max_allowed_q == TQL_stub)
      ql = TQL_stub;
    else
      ql = min(texmgr_internal::max_allowed_q, ql);

    static int tex_atype = DAEDITOR3.getAssetTypeId("tex");
    const char *textureName = get_managed_texture_name(id);
    TextureMetaData tmd;
    String tmd_stor, fpath(tmd.decode(textureName, &tmd_stor));
    erase_items(fpath, fpath.size() - 2, 1);

    if (DagorAsset *a = DAEDITOR3.getAssetByName(fpath, tex_atype))
      if (ql != RMGR.resQS[id.index()].getCurQL())
      {
        if (BaseTexture *bt = acquire_managed_tex(id))
        {
          if (ql != TQL_stub)
            load_tex_data(bt, id, NULL, NULL, a, tmd, ql);
          else
          {
            mark_cur_ql(id.index(), TQL_stub);
            bt->discardTex();
          }
        }
        release_managed_tex(id);
      }
    return true;
  }

  virtual BaseTexture *createTexture(TEXTUREID id)
  {
    G_ASSERT_RETURN(is_main_thread(), nullptr);
    const char *textureName = get_managed_texture_name(id);

    if (!textureName)
      return NULL;

    int idx = id.index();
    TextureMetaData tmd;
    String tmd_stor, fpath(tmd.decode(textureName, &tmd_stor));

    mark_cur_ql(idx, TQL_base);
    if (!isPackTex(textureName))
    {
      if (!isGameTex(textureName))
        return defFactory->createTexture(id);

      debug("read as non-sRGB: %s", textureName);
      return ::create_texture(tmd.encode(gamePrefix + fpath, &tmd_stor), TEXCF_RGB | TEXCF_ABEST, 0, false);
    }
    else
      erase_items(fpath, fpath.size() - 2, 1);

    static int tex_atype = DAEDITOR3.getAssetTypeId("tex");
    DagorAsset *a = DAEDITOR3.getAssetByName(fpath, tex_atype);
    if (!a && dd_get_fname_ext(fpath))
      a = DAEDITOR3.getAssetByName(fpath + ".xxx", tex_atype);
    if (a)
      tmd.read(a->props, a->resolveEffProfileTargetStr("PC", NULL));
    TexQL ql = texmgr_internal::max_allowed_q;
    if (ql != TQL_stub && forceDiscardTid.has(id))
      ql = TQL_thumb;

    if (a && a->props.getBool("convert", false))
    {
      ddsx::Buffer b;
      ddsx::Buffer hq_b;

      if (ql == TQL_thumb)
        if (DagorAsset *tq_a = DAEDITOR3.getAssetByName(fpath + "$tq", tex_atype))
          if (!texconvcache::get_tex_asset_built_ddsx(*tq_a, b, _MAKE4C('PC'), NULL, console()))
            DAEDITOR3.conError("cannot convert <%s> tex asset", tq_a->getName());

      if (!b.ptr && texconvcache::get_tex_asset_built_ddsx(*a, b, _MAKE4C('PC'), NULL, console()))
      {
        if (ql == TQL_uhq)
          if (DagorAsset *uhq_a = DAEDITOR3.getAssetByName(fpath + "$uhq", tex_atype))
            if (!texconvcache::get_tex_asset_built_ddsx(*uhq_a, hq_b, _MAKE4C('PC'), NULL, console()))
              DAEDITOR3.conError("cannot convert <%s> tex asset", uhq_a->getName());

        if (!hq_b.ptr && ql >= TQL_high)
          if (DagorAsset *hq_a = DAEDITOR3.getAssetByName(fpath + "$hq", tex_atype))
            if (!texconvcache::get_tex_asset_built_ddsx(*hq_a, hq_b, _MAKE4C('PC'), NULL, console()))
              DAEDITOR3.conError("cannot convert <%s> tex asset", hq_a->getName());

        if (b.ptr && hq_b.ptr)
        {
          ddsx::Header &hq_hdr = *(ddsx::Header *)hq_b.ptr;
          if (hq_hdr.levels)
          {
            ddsx::Header hdr = hq_hdr;
            if (hdr.lQmip < hdr.levels)
              hdr.lQmip = hdr.levels;
            hdr.levels += ((ddsx::Header *)b.ptr)->levels;
            hdr.flags &= ~hdr.FLG_HQ_PART;

            BaseTexture *bt =
              d3d::alloc_ddsx_tex(hdr, TEXCF_LOADONCE, 0, 0, a->getName(), tql::getEffStubIdx(tmd.stubTexIdx, tql::get_tex_type(hdr)));
            if (load_tex_data(bt, id, &b, &hq_b, a, tmd, ql))
              applyTmd(bt, tmd);
            else
              del_d3dres(bt);
            return bt;
          }
          hq_b.free();
        }
      }

      if (b.ptr && !hq_b.ptr)
      {
        auto &hdr = *(ddsx::Header *)b.ptr;
        if (!hdr.d3dFormat && !hdr.memSz)
        {
          b.free();
          return nullptr;
        }

        if (!(hdr.flags & hdr.FLG_HQ_PART) && hdr.hqPartLevels)
        {
          hdr.mQmip = hdr.mQmip > hdr.hqPartLevels ? (hdr.mQmip - hdr.hqPartLevels) : 0;
          hdr.lQmip = hdr.lQmip > hdr.hqPartLevels ? (hdr.lQmip - hdr.hqPartLevels) : 0;
          hdr.hqPartLevels = 0;
        }

        BaseTexture *bt =
          d3d::alloc_ddsx_tex(hdr, TEXCF_LOADONCE, 0, 0, a->getName(), tql::getEffStubIdx(tmd.stubTexIdx, tql::get_tex_type(hdr)));
        if (load_tex_data(bt, id, &b, NULL, a, tmd, ql))
          applyTmd(bt, tmd);
        else
          del_d3dres(bt);
        return bt;
      }

      DAEDITOR3.conError("cannot convert <%s> tex asset", a->getName());
      return NULL;
    }

    float gamma = 1.0f;
    if (a)
    {
      fpath = a->getTargetFilePath();
      gamma = a->getProfileTargetProps(_MAKE4C('PC'), NULL).getReal("gamma", a->props.getReal("gamma", 2.2));
    }

    int flags = TEXCF_RGB | TEXCF_ABEST;
    if (gamma > 1)
      flags |= TEXCF_SRGBREAD;

    bool is_dds = dd_get_fname_ext(fpath) && stricmp(dd_get_fname_ext(fpath), ".dds") == 0;
    if (is_dds && !d3d::is_stub_driver())
    {
      if (BaseTexture *bt = loadViaDdsx(id, fpath, gamma, tmd, ql, textureName))
        return bt;
      else
        DAEDITOR3.conError("failed to convert <%s> to DDSx on-fly", fpath);
    }
    else if (BaseTexture *bt = ::create_texture(tmd.encode(fpath, &tmd_stor), flags, 0, false))
      return bt;
    else
      DAEDITOR3.conError("failed to load <%s>", fpath);
    return NULL;
  }

  virtual void releaseTexture(BaseTexture *texture, TEXTUREID id)
  {
    const char *textureName = get_managed_texture_name(id);
    if (!textureName)
      return;

    if (!isPackTex(textureName) && !isGameTex(textureName))
      return defFactory->releaseTexture(texture, id);

    if (texture)
      texture->destroy();
  }

  static void unpackDdsxBuf(ddsx::Buffer &b)
  {
    ddsx::Header &hdr = *(ddsx::Header *)b.ptr;
    InPlaceMemLoadCB fcrd((char *)b.ptr + sizeof(hdr), hdr.packedSz);
    if (hdr.isCompressionZSTD())
    {
      ZstdLoadCB crd(fcrd, hdr.packedSz);
      void *p = memalloc(b.len = sizeof(hdr) + hdr.memSz, tmpmem);
      hdr.flags &= ~hdr.FLG_COMPR_MASK;
      hdr.packedSz = 0;
      memcpy(p, &hdr, sizeof(hdr));
      crd.read((char *)p + sizeof(hdr), hdr.memSz);
      memfree(b.ptr, tmpmem);
      b.ptr = p;
      crd.ceaseReading();
    }
    else if (hdr.isCompressionOODLE())
    {
      OodleLoadCB crd(fcrd, hdr.packedSz, hdr.memSz);
      void *p = memalloc(b.len = sizeof(hdr) + hdr.memSz, tmpmem);
      hdr.flags &= ~hdr.FLG_COMPR_MASK;
      hdr.packedSz = 0;
      memcpy(p, &hdr, sizeof(hdr));
      crd.read((char *)p + sizeof(hdr), hdr.memSz);
      memfree(b.ptr, tmpmem);
      b.ptr = p;
      crd.ceaseReading();
    }
    else if (hdr.isCompressionZLIB())
    {
      ZlibLoadCB crd(fcrd, hdr.packedSz);
      void *p = memalloc(b.len = sizeof(hdr) + hdr.memSz, tmpmem);
      hdr.flags &= ~hdr.FLG_COMPR_MASK;
      hdr.packedSz = 0;
      memcpy(p, &hdr, sizeof(hdr));
      crd.read((char *)p + sizeof(hdr), hdr.memSz);
      memfree(b.ptr, tmpmem);
      b.ptr = p;
      crd.ceaseReading();
    }
    else if (hdr.isCompression7ZIP())
    {
      LzmaLoadCB crd(fcrd, hdr.packedSz);
      void *p = memalloc(b.len = sizeof(hdr) + hdr.memSz, tmpmem);
      hdr.flags &= ~hdr.FLG_COMPR_MASK;
      hdr.packedSz = 0;
      memcpy(p, &hdr, sizeof(hdr));
      crd.read((char *)p + sizeof(hdr), hdr.memSz);
      memfree(b.ptr, tmpmem);
      b.ptr = p;
      crd.ceaseReading();
    }
  }

  virtual bool getTextureDDSx(TEXTUREID id, Tab<char> &out_ddsx)
  {
    G_ASSERT_RETURN(is_main_thread(), false);
    const char *textureName = get_managed_texture_name(id);
    if (!textureName)
      return false;

    if (!isPackTex(textureName))
      return false;

    TextureMetaData tmd;
    String tmd_stor, fpath(tmd.decode(textureName, &tmd_stor));
    erase_items(fpath, fpath.size() - 2, 1);

    static int tex_atype = DAEDITOR3.getAssetTypeId("tex");
    DagorAsset *a = DAEDITOR3.getAssetByName(fpath, tex_atype);
    if (a)
      tmd.read(a->props, a->resolveEffProfileTargetStr("PC", NULL));
    if (a && a->props.getBool("convert", false))
    {
      ddsx::Buffer b;
      const int hdr_sz = sizeof(ddsx::Header);
      if (texconvcache::get_tex_asset_built_ddsx(*a, b, _MAKE4C('PC'), NULL, console()))
      {
        unpackDdsxBuf(b);
        ddsx_forward_mips_inplace(*(ddsx::Header *)b.ptr, (char *)b.ptr + hdr_sz, b.len - hdr_sz);
        if (DagorAsset *hq_a = DAEDITOR3.getAssetByName(fpath + "$hq", tex_atype))
        {
          ddsx::Buffer hq_b;
          if (texconvcache::get_tex_asset_built_ddsx(*hq_a, hq_b, _MAKE4C('PC'), NULL, console()))
          {
            unpackDdsxBuf(hq_b);
            ddsx::Header &hq_hdr = *(ddsx::Header *)hq_b.ptr;
            ddsx_forward_mips_inplace(hq_hdr, (char *)hq_b.ptr + hdr_sz, hq_b.len - hdr_sz);
            if (hq_hdr.levels)
            {
              ddsx::Header hdr = hq_hdr;
              if (hdr.lQmip < hdr.levels)
                hdr.lQmip = hdr.levels;
              hdr.levels += ((ddsx::Header *)b.ptr)->levels;
              hdr.flags &= ~hdr.FLG_HQ_PART;

              if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER))
              {
                out_ddsx.resize(hq_b.len);
                mem_copy_from(out_ddsx, hq_b.ptr);
                memcpy(out_ddsx.data(), &hdr, sizeof(hdr));
              }
              else
              {
                out_ddsx.resize(hq_b.len + b.len - hdr_sz);
                memcpy(out_ddsx.data(), hq_b.ptr, hq_b.len);
                memcpy(out_ddsx.data() + hq_b.len, (char *)b.ptr + hdr_sz, b.len - hdr_sz);
                memcpy(out_ddsx.data(), &hdr, sizeof(hdr));
              }
              b.free();
              hq_b.free();
              return true;
            }
            hq_b.free();
          }
          else
            DAEDITOR3.conError("cannot convert <%s> tex asset", hq_a->getName());
        }
        out_ddsx.resize(b.len);
        mem_copy_from(out_ddsx, b.ptr);
        b.free();
        return true;
      }
    }
    return false;
  }
  static bool isPackTex(const char *textureName)
  {
    const char *plast = strchr(textureName, '*');
    return plast && (plast[1] == '?' || plast[1] == '\0');
  }
  static bool isGameTex(const char *textureName) { return !gamePrefix.empty() && strnicmp(textureName, "tex/", 4) == 0; }


  static bool load_tex_data(BaseTexture *_bt, TEXTUREID tid, ddsx::Buffer *bq, ddsx::Buffer *hq, DagorAsset *a, TextureMetaData &tmd,
    TexQL ql, const char *tex_fn = NULL)
  {
    static int tex_atype = DAEDITOR3.getAssetTypeId("tex");
    if (!_bt)
    {
      if (hq)
        hq->free();
      if (bq)
        bq->free();
      mark_cur_ql(tid.index(), TQL_stub);
      return false;
    }
    if (ql == TQL_stub)
    {
      if (hq)
        hq->free();
      if (bq)
        bq->free();
      mark_cur_ql(tid.index(), ql);
      return true;
    }
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

    ddsx::Buffer l_bq, l_hq;
    if (ql == TQL_thumb && a && !bq)
      if (DagorAsset *tq_a = DAEDITOR3.getAssetByName(String(0, "%s$tq", a->getName()), tex_atype))
        a = tq_a;
    if (a && a->props.getBool("convert", false))
    {
      if (!bq)
      {
        bq = &l_bq;
        if (!texconvcache::get_tex_asset_built_ddsx(*a, *bq, _MAKE4C('PC'), NULL, console()))
        {
          DAEDITOR3.conError("cannot convert <%s> tex asset", a->getName());
          if (hq)
            hq->free();
          return false;
        }
      }

      ddsx::Header &bq_hdr = *(ddsx::Header *)bq->ptr;
      int bq_lev = get_log2i(max(bq_hdr.w, bq_hdr.h));
      bool hold_sysmem = (bq_hdr.flags & bq_hdr.FLG_HOLD_SYSMEM_COPY) != 0;
      DagorAsset *hq_a = ql > TQL_high ? DAEDITOR3.getAssetByName(String(0, "%s$uhq", a->getName()), tex_atype) : NULL;
      if (!hq_a && ql > TQL_base)
        hq_a = DAEDITOR3.getAssetByName(String(0, "%s$hq", a->getName()), tex_atype);
      if (hq_a)
      {
        if (!hq)
        {
          hq = &l_hq;
          if (!texconvcache::get_tex_asset_built_ddsx(*hq_a, *hq, _MAKE4C('PC'), NULL, console()))
          {
            DAEDITOR3.conError("cannot convert <%s> tex asset", hq_a->getName());
            bq->free();
            return false;
          }
        }

        ddsx::Header &hq_hdr = *(ddsx::Header *)hq->ptr;
        if (hq_hdr.levels)
        {
          BaseTexture *bt = _bt->makeTmpTexResCopy(hq_hdr.w, hq_hdr.h, hq_hdr.depth, hq_hdr.levels + bq_hdr.levels);
          int target_lev = get_log2i(max(hq_hdr.w, hq_hdr.h));
          hold_sysmem = (hq_hdr.flags & hq_hdr.FLG_HOLD_SYSMEM_COPY) != 0;
          if (hold_sysmem)
            RMGR.incBaseTexRc(tid);

          InPlaceMemLoadCB crd(bq->ptr, bq->len);
          crd.seekrel(sizeof(ddsx::Header));
          bt->allocateTex();
          if (d3d_load_ddsx_tex_contents(bt, tid, bt_ref.id, bq_hdr, crd, 0, target_lev - bq_lev, 1) != TexLoadRes::OK)
            DAEDITOR3.conError("error loading tex %s", a->getName());
          bq->free();

          InPlaceMemLoadCB crd_hq(hq->ptr, hq->len);
          crd_hq.seekrel(sizeof(ddsx::Header));
          if (d3d_load_ddsx_tex_contents(bt, tid, bt_ref.id, hq_hdr, crd_hq, 0, 0, bq_lev) != TexLoadRes::OK)
            DAEDITOR3.conError("error loading tex %s", hq_a->getName());
          hq->free();

          if (hold_sysmem)
            RMGR.decBaseTexRc(tid);
          _bt->replaceTexResObject(bt);
          mark_cur_ql(tid.index(), ql);
          return true;
        }
        hq->free();
      }
      else if (hq)
        hq->free();

      BaseTexture *bt = _bt->makeTmpTexResCopy(bq_hdr.w, bq_hdr.h, bq_hdr.depth, bq_hdr.levels);
      if (hold_sysmem)
        RMGR.incBaseTexRc(tid);

      InPlaceMemLoadCB crd(bq->ptr, bq->len);
      crd.seekrel(sizeof(ddsx::Header));
      bt->allocateTex();
      if (d3d_load_ddsx_tex_contents(bt, tid, bt_ref.id, bq_hdr, crd, 0, 0, 1) != TexLoadRes::OK)
        DAEDITOR3.conError("error loading tex %s", a->getName());
      bq->free();

      if (hold_sysmem)
        RMGR.decBaseTexRc(tid);
      _bt->replaceTexResObject(bt);
      mark_cur_ql(tid.index(), ql);
      return true;
    }

    if (hq)
      hq->free();
    if (!bq)
    {
      G_ASSERT(a);
      if (!a)
        return false;
      if (a->getFileNameId() < 0)
        return false;
      String dds_path(a->getTargetFilePath());

      bq = &l_bq;
      file_ptr_t fp = df_open(dds_path, DF_READ);
      SmallTab<char, TmpmemAlloc> dds;
      if (!fp)
      {
        DAEDITOR3.conError("can't read ERR: cannot open <%s>", dds_path);
        return false;
      }
      clear_and_resize(dds, df_length(fp));
      df_read(fp, dds.data(), data_size(dds));
      df_close(fp);

      ddsx::ConvertParams cp;
      cp.packSzThres = 256 << 20;
      cp.addrU = tmd.d3dTexAddr(tmd.addrU);
      cp.addrV = tmd.d3dTexAddr(tmd.addrV);
      cp.hQMip = tmd.hqMip;
      cp.mQMip = tmd.mqMip;
      cp.lQMip = tmd.lqMip;
      cp.imgGamma = a->getProfileTargetProps(_MAKE4C('PC'), NULL).getReal("gamma", a->props.getReal("gamma", 2.2));

      if (!ddsx::convert_dds(_MAKE4C('PC'), *bq, dds.data(), data_size(dds), cp))
      {
        DAEDITOR3.conError("can't convert texture <%s>: %s", dds_path, ddsx::get_last_error_text());
        return false;
      }
    }

    ddsx::Header &bq_hdr = *(ddsx::Header *)bq->ptr;
    BaseTexture *bt = _bt->makeTmpTexResCopy(bq_hdr.w, bq_hdr.h, bq_hdr.depth, bq_hdr.levels);
    InPlaceMemLoadCB crd(bq->ptr, bq->len);
    crd.seekrel(sizeof(ddsx::Header));
    bt->allocateTex();
    if (d3d_load_ddsx_tex_contents(bt, tid, BAD_TEXTUREID, *(ddsx::Header *)bq->ptr, crd, 0, 0, 1) != TexLoadRes::OK)
      DAEDITOR3.conError("error loading tex %s", a ? a->getName() : tex_fn);
    bq->free();
    _bt->replaceTexResObject(bt);
    mark_cur_ql(tid.index(), ql);
    return true;
  }

  static BaseTexture *loadViaDdsx(TEXTUREID tid, const char *dds_path, float gamma, TextureMetaData &tmd, TexQL ql,
    const char *tex_name)
  {
    file_ptr_t fp = df_open(dds_path, DF_READ);
    SmallTab<char, TmpmemAlloc> dds;
    if (!fp)
    {
      DAEDITOR3.conError("can't read ERR: cannot open <%s>", dds_path);
      return NULL;
    }
    clear_and_resize(dds, df_length(fp));
    df_read(fp, dds.data(), data_size(dds));
    df_close(fp);

    ddsx::ConvertParams cp;
    cp.canPack = false;
    cp.packSzThres = 256 << 20;
    cp.addrU = tmd.d3dTexAddr(tmd.addrU);
    cp.addrV = tmd.d3dTexAddr(tmd.addrV);
    cp.hQMip = tmd.hqMip;
    cp.mQMip = tmd.mqMip;
    cp.lQMip = tmd.lqMip;
    cp.imgGamma = gamma;

    ddsx::Buffer buf;
    if (!ddsx::convert_dds(_MAKE4C('PC'), buf, dds.data(), data_size(dds), cp))
    {
      DAEDITOR3.conError("can't convert texture <%s>: %s", dds_path, ddsx::get_last_error_text());
      return NULL;
    }

    InPlaceMemLoadCB crd(buf.ptr, buf.len);
    auto &hdr = *((ddsx::Header *)buf.ptr);
    BaseTexture *bt =
      d3d::alloc_ddsx_tex(hdr, TEXCF_LOADONCE, 0, 0, tex_name, tql::getEffStubIdx(tmd.stubTexIdx, tql::get_tex_type(hdr)));
    if (load_tex_data(bt, tid, &buf, NULL, NULL, tmd, ql, dds_path))
      applyTmd(bt, tmd);
    else
      del_d3dres(bt);
    buf.free();
    return bt;
  }
  static void applyTmd(BaseTexture *t, const TextureMetaData &tmd)
  {
    t->texaddru(tmd.d3dTexAddr(tmd.addrU));
    t->texaddrv(tmd.d3dTexAddr(tmd.addrV));
    if (t->restype() == RES3D_VOLTEX)
      reinterpret_cast<VolTexture *>(t)->texaddrw(tmd.d3dTexAddr(tmd.addrW));
    t->setAnisotropy(tmd.calcAnisotropy(::dgs_tex_anisotropy));
    if (tmd.needSetBorder())
      t->texbordercolor(tmd.borderCol);
    if (tmd.lodBias)
      t->texlod(tmd.lodBias / 1000.0f);
    if (tmd.needSetTexFilter())
      t->texfilter(tmd.d3dTexFilter());
    if (tmd.needSetMipFilter())
      t->texfilter(tmd.d3dMipFilter());
  }

  TextureFactory *defFactory;
  TextureIdSet forceDiscardTid;
} ddsx_tex_pack_factory;

void init_dxp_factory_service()
{
  if (::get_default_tex_factory() == &ddsx_tex_pack_factory)
    return;

  texmgr_internal::disable_add_warning = true;
  ddsx_tex_pack_factory.defFactory = ::get_default_tex_factory();
  ::set_default_tex_factory(&ddsx_tex_pack_factory);

  if (DAGORED2)
    gamePrefix.printf(260, "%s/", DAGORED2->getGameDir());
}

void term_dxp_factory_service() {}

void dxp_factory_force_discard(const TextureIdSet &tid)
{
  Tab<TEXTUREID> tid_restore;
  for (TEXTUREID id : ddsx_tex_pack_factory.forceDiscardTid)
    if (get_managed_texture_refcount(id) > 0 && !tid.has(id))
      tid_restore.push_back(id);

  ddsx_tex_pack_factory.forceDiscardTid = tid;
  for (TEXTUREID id : tid)
  {
    if (get_managed_texture_refcount(id) > 0)
    {
      if (BaseTexture *t = acquire_managed_tex(id))
      {
        DdsxTexPackFactory::mark_cur_ql(id.index(), TQL_stub);
        t->discardTex();
      }
      release_managed_tex(id);
    }
  }
  for (TEXTUREID id : tid_restore)
    ddsx_tex_pack_factory.scheduleTexLoading(id, texmgr_internal::max_allowed_q);
}

void dxp_factory_force_discard(bool all)
{
  TextureIdSet tid;
  if (!all)
    return dxp_factory_force_discard(tid);

  for (TEXTUREID id = first_managed_texture(1); id != BAD_TEXTUREID; id = next_managed_texture(id, 1))
    tid.add(id);
  dxp_factory_force_discard(tid);
}

void dxp_factory_after_reset()
{
  TextureIdSet tid;
  tid = ddsx_tex_pack_factory.forceDiscardTid;
  dxp_factory_force_discard(true);
  dxp_factory_force_discard(tid);
}

void dxp_factory_reload_tex(TEXTUREID tid, TexQL ql)
{
  DdsxTexPackFactory::mark_cur_ql(tid.index(), TQL_stub);
  ddsx_tex_pack_factory.scheduleTexLoading(tid, ql);
}
