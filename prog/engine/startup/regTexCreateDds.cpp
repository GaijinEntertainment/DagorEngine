// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_startupTex.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_createTex.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <3d/ddsxTex.h>
#include <3d/dag_texPackMgr2.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_fastSeqRead.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_texMetaData.h>
#include <util/le2be.h>
#include <util/fnameMap.h>
#include <debug/dag_debug.h>
#include "../lib3d/texMgrData.h"
#include <math/dag_adjpow2.h>

static BaseTexture *load_ddsx(const char *fn, int flg, int levels, const TextureMetaData &tmd, bool ignore_missing = false)
{
  BaseTexture *t = NULL;

  TEXTUREID bt_id = BAD_TEXTUREID;
  if (!tmd.baseTexName.empty())
  {
    bt_id = get_managed_texture_id(tmd.baseTexName);
    if (bt_id == BAD_TEXTUREID)
      bt_id = add_managed_texture(tmd.baseTexName);
  }

  if (::vromfs_check_file_exists(fn))
  {
    FullFileLoadCB crd(fn);
    if (crd.fileHandle)
      t = d3d::create_ddsx_tex(crd, flg, dgs_tex_quality, levels, fn);
  }
  else
  {
    FullFileLoadCB crd(fn, DF_READ | (ignore_missing ? DF_IGNORE_MISSING : 0));
    if (crd.fileHandle)
    {
      using texmgr_internal::RMGR;
      TEXTUREID tid = get_managed_texture_id(fn);
      bool streamable_tex = tmd.stubTexIdx != 0xF;
      if (tmd.stubTexIdx == 0) // only explicit Ux is allowed to assign stubTex index here
      {
        const char *meta = get_managed_texture_name(tid);
        meta = meta ? strchr(meta, '?') : meta;
        if (!meta || (!strstr(meta, "U0") && !strstr(meta, "u0")))
          streamable_tex = false;
      }
      if (tid != BAD_TEXTUREID && (bt_id != BAD_TEXTUREID || df_length(crd.fileHandle) > (128 << 10) || streamable_tex))
      {
        ddsx::Header hdr;
        crd.read(&hdr, sizeof(hdr));

        int idx = tid.index();
        if (streamable_tex)
        {
          auto &desc = RMGR.texDesc[idx];
          unsigned base_lev = max(get_log2i(max(max(hdr.w, hdr.h), hdr.depth)), 1u);
          RMGR.setLevDesc(idx, TQL_base, base_lev);
          desc.packRecIdx[TQL_base].pack = 0x7FFF;
          desc.packRecIdx[TQL_base].rec = dagor_fname_map_get_fn_id(fn, true);
          if (base_lev > 9 && hdr.levels > 1) // use virtual split into base/high QL
          {
            RMGR.setLevDesc(idx, TQL_base, max(9u, base_lev - hdr.levels + 1));
            RMGR.setLevDesc(idx, TQL_high, base_lev);
            desc.packRecIdx[TQL_high].pack = desc.packRecIdx[TQL_base].pack;
            desc.packRecIdx[TQL_high].rec = desc.packRecIdx[TQL_base].rec;
          }
          desc.dim.w = hdr.w;
          desc.dim.h = hdr.h;
          desc.dim.d = hdr.depth;
          desc.dim.l = hdr.levels;
          desc.dim.maxLev = base_lev;
          desc.dim.stubIdx = tql::getEffStubIdx(tmd.stubTexIdx, tql::get_tex_type(hdr));

          auto &resQS = RMGR.resQS[idx];
          RMGR.setupQLev(idx, ::dgs_tex_quality, hdr);
          if (RMGR.resQS[idx].getQLev() > 9 && hdr.w > 128 && hdr.h > 128)
            RMGR.setMaxLev(idx, RMGR.resQS[idx].getQLev() - tql::dyn_qlev_decrease);
          else
            RMGR.setMaxLev(idx, RMGR.resQS[idx].getQLev());
          resQS.setLdLev(1);
          resQS.setMaxQL(RMGR.calcMaxQL(idx), RMGR.calcCurQL(idx, resQS.getLdLev()));
          resQS.setMaxReqLev(texmgr_internal::texq_load_on_demand ? 1 : resQS.getQLev());
        }
        RMGR.pairedBaseTexId[idx] = bt_id;

        t = d3d::alloc_ddsx_tex(hdr, flg, dgs_tex_quality, levels, fn, streamable_tex ? RMGR.texDesc[idx].dim.stubIdx : -1);
        if (streamable_tex)
          t->setTID(tid);
        if (!streamable_tex || !texmgr_internal::texq_load_on_demand)
          ddsx::enqueue_ddsx_load(t, tid, fn, dgs_tex_quality, bt_id);
      }
      else
        t = d3d::create_ddsx_tex(crd, flg, dgs_tex_quality, levels, fn);
    }
  }

  TEXTUREID tid = get_managed_texture_id(fn);
  if (t)
  {
    set_texture_separate_sampler(tid, get_sampler_info(tmd, false));
  }
  set_ddsx_reloadable_callback(t, tid, fn, 0, bt_id);
  return t;
}

class DdsxCreateTexFactory : public ICreateTexFactory
{
public:
  BaseTexture *createTex(const char *fn, int flg, int levels, const char *fn_ext, const TextureMetaData &tmd) override
  {
    BaseTexture *t = NULL;
    if (!fn_ext)
    {
      const char *ddsx_ext = ".ddsx";
      char fn_ddsx[DAGOR_MAX_PATH];
      strncpy(fn_ddsx, fn, sizeof(fn_ddsx) - strlen(ddsx_ext) - 1);
      fn_ddsx[sizeof(fn_ddsx) - strlen(ddsx_ext)] = 0;
      strcat(fn_ddsx, ddsx_ext);
      return load_ddsx(fn_ddsx, flg, levels, tmd, true);
    }

    else if (dd_stricmp(fn_ext, ".ddsx") == 0)
      return load_ddsx(fn, flg, levels, tmd);
    else if (fn_ext == dd_get_fname_ext(fn) && fn_ext > fn + 3 && dd_stricmp(fn_ext - 3, ".ta.bin") == 0)
    {
      FullFileLoadCB crd(fn);
      if (crd.readInt() != _MAKE4C('TA.'))
        return t;
      crd.seekrel(readInt32(crd, true));
      int ofs = crd.tell();
      t = d3d::create_ddsx_tex(crd, flg, dgs_tex_quality, levels, fn);
      TEXTUREID tid = get_managed_texture_id(fn);
      if (t)
      {
        set_texture_separate_sampler(tid, get_sampler_info(tmd, false));
      }
      set_ddsx_reloadable_callback(t, tid, fn, ofs, BAD_TEXTUREID);
    }
    return t;
  }
};

static DdsxCreateTexFactory ddsx_create_tex_factory;

void register_dds_tex_create_factory()
{
  int queue_sz = 256;
  if (::dgs_get_settings())
    queue_sz = ::dgs_get_settings()->getInt("ddsxLoadQueueSize", queue_sz);
  ddsx::init_ddsx_load_queue(queue_sz);
  add_create_tex_factory(&ddsx_create_tex_factory);
}

void set_ddsx_reloadable_callback(BaseTexture *t, TEXTUREID tid, const char *fn, int ofs, TEXTUREID bt_ref)
{
  struct ReloadDdsxFile : public BaseTexture::IReloadData
  {
    const char *fn;
    int ofs;
    TEXTUREID tid, btRef;

    ReloadDdsxFile(TEXTUREID _tid, int _ofs, TEXTUREID bt_ref) : fn(NULL), ofs(_ofs), tid(_tid), btRef(bt_ref) {}
    virtual void reloadD3dRes(BaseTexture *t)
    {
      using texmgr_internal::RMGR;
      FullFileLoadCB crd(fn, DF_READ | DF_IGNORE_MISSING);
      if (!crd.fileHandle)
      {
        logerr("cannot restore texture, file is missing: %s", fn);
        return;
      }

      ddsx::Header hdr;
      crd.seekto(ofs);
      crd.read(&hdr, sizeof(hdr));
      if (btRef != BAD_TEXTUREID)
      {
        if (!RMGR.hasTexBaseData(btRef))
          ddsx::process_ddsx_load_queue();
        if (!RMGR.hasTexBaseData(btRef))
        {
          logerr("failed to reload <%s:0x%x> due to missing sysMemCopy in basetex <%s> (bt refc=%d)", fn, ofs,
            get_managed_texture_name(btRef), get_managed_texture_refcount(btRef));
          return;
        }
      }
      d3d_load_ddsx_tex_contents(t, tid, btRef, hdr, crd, dgs_tex_quality);
    }
    virtual void destroySelf() { delete this; }
  };

  if (!t)
    return;
  if (t->getTID() != BAD_TEXTUREID) // don't set reload callback for streamable textures
    return;
  ReloadDdsxFile *rld = new ReloadDdsxFile(tid, ofs, bt_ref);
  if (t->setReloadCallback(rld))
    rld->fn = dagor_fname_map_add_fn(fn);
  else
    delete rld;
}
