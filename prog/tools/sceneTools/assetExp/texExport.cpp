// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daBuild.h"
#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/util/conLogWriter.h>
#include <libTools/util/progressInd.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>
#include <libTools/util/binDumpReader.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetExporter.h>
#include <assets/assetExpCache.h>
#include <assets/assetHlp.h>
#include <3d/ddsxTex.h>
#include <ioSys/dag_fileIo.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_globalMutex.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_stdint.h>
#include <util/dag_string.h>
#include <util/dag_roNameMap.h>
#include <util/dag_texMetaData.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>

static const int DDSX_EXP_VER = 2;
static const char *tq_split_fn = "assets/.tex-tq-split.blk";
static const int FORCETQ_no = 0;
int dabuild_dxp_write_ver = 3;

static void load_eff_tmd(TextureMetaData &tmd, const DagorAsset *a, int tq_nsid, const char *target_str, const char *profile)
{
  if ((tq_nsid >= 0 && a->getNameSpaceId() == tq_nsid))
  {
    const char *tq_name = a->getName();
    const char *suff = strstr(tq_name, "$tq");
    G_ASSERTF_RETURN(suff, , "a->getName()=%s", tq_name);
    String texname(0, "%.*s", suff - tq_name, tq_name);
    const DagorAsset *a_base = a->getMgr().findAsset(texname, a->getMgr().getTexAssetTypeId());
    G_ASSERTF(a_base, "%s -> %s", tq_name, texname);
    tmd.read(a_base->props, a_base->resolveEffProfileTargetStr(target_str, profile));
  }
  else
    tmd.read(a->props, a->resolveEffProfileTargetStr(target_str, profile));
}
static bool should_encode_tmd(const DagorAsset *a, int tq_nsid)
{
  if (tq_nsid >= 0 && a->getNameSpaceId() == tq_nsid)
    return a->props.getInt("forceTQ", FORCETQ_no) != FORCETQ_no;
  const char *tname = a->getName();
  if (strchr(tname, '$'))
    return false;
  if (const DagorAsset *tq_a = a->getMgr().findAsset(String(0, "%s$tq", tname), a->getMgr().getTexAssetTypeId()))
    return tq_a->props.getInt("forceTQ", FORCETQ_no) == FORCETQ_no;
  return true;
}
static bool should_skip_writing_tex_contents(const DagorAsset *a, const ddsx::Buffer &b, unsigned tc, const char *profile)
{
  if (b.len <= sizeof(ddsx::Header))
    return false;
  const char *tname = a->getName();
  if (strchr(tname, '$'))
    return false;
  if (DagorAsset *tq_a = a->getMgr().findAsset(String(0, "%s$tq", tname), a->getMgr().getTexAssetTypeId()))
  {
    int force_tq = tq_a->props.getInt("forceTQ", FORCETQ_no);
    if (force_tq == FORCETQ_no)
      return false;
    const ddsx::Header &h = *(ddsx::Header *)b.ptr;
    if (h.memSz <= tq_a->props.getInt("thumbnailMaxDataSize", 0))
      return true;
    // if (force_tq == FORCETQ_no)
    //   return false;
    ddsx::Buffer tq_b;
    if (!texconvcache::get_tex_asset_built_ddsx(*tq_a, tq_b, tc, profile, nullptr))
      return false;
    if (memcmp(tq_b.ptr, b.ptr, sizeof(ddsx::Header)) == 0)
      return true;
    return false;
  }
  return false;
}

class DDSxTexPack2Serv
{
  struct Rec
  {
    int texId, ofs, packedDataSize;
  };

public:
  IGenLoad *crd = nullptr;
  int ver = 2;
  //-- dump start
  RoNameMap texNames;
  PatchableTab<ddsx::Header> hdr;
  PatchableTab<Rec> rec;
  //-- dump end; no members allowed after this point

public:
  static DDSxTexPack2Serv *make(BinDumpReader &crd)
  {
    if (crd.readFourCC() != _MAKE4C('DxP2'))
      return NULL;
    int ver = crd.readInt32e();
    if (ver != 2 && ver != 3)
      return NULL;
    int tex_num = crd.readInt32e();

    crd.beginBlock();
    int dump_sz = crd.getBlockRest();
    DDSxTexPack2Serv *pack = new (memalloc(offsetof(DDSxTexPack2Serv, texNames) + dump_sz, tmpmem), _NEW_INPLACE) DDSxTexPack2Serv;
    pack->ver = ver;
    void *dump_ptr = &pack->texNames;
    crd.getRawReader().read(dump_ptr, dump_sz);
    crd.endBlock();

    crd.convert32(dump_ptr, sizeof(*pack) - offsetof(DDSxTexPack2Serv, texNames));

    PatchableTab<PatchablePtr<const char>> map;
    memcpy(&map, &pack->texNames.map, sizeof(map)); //-V780
    map.patch(dump_ptr);
    crd.convert32(map.data(), data_size(map));

    pack->texNames.patchData(dump_ptr);

    pack->hdr.patch(dump_ptr);
    pack->rec.patch(dump_ptr);
    if (ver == 3)
      crd.convert32(pack->rec.data(), data_size(pack->rec));
    else // if (ver == 2)
    {
      struct RecV2
      {
        int64_t texPtr;
        int texId, ofs, packedDataSize, _resv;
      };
      const RecV2 *src = reinterpret_cast<RecV2 *>(pack->rec.data());
      for (auto *dst = pack->rec.data(), *dst_e = pack->rec.end(); dst < dst_e; dst++, src++)
        dst->texId = 0, dst->ofs = src->ofs, dst->packedDataSize = src->packedDataSize;
    }

    pack->crd = &crd.getRawReader();
    return pack;
  }

  int getFormatVer() const { return ver; }
  int getTexRecId(const char *name) const { return texNames.getNameId(name); }

  bool getDDSx(const char *name, ddsx::Buffer &b)
  {
    int id = texNames.getNameId(name);
    if (id < 0)
      return false;
    if (rec[id].packedDataSize < 0)
      return false;

    b.len = rec[id].packedDataSize + sizeof(ddsx::Header);
    b.ptr = memalloc(b.len, tmpmem);
    memcpy(b.ptr, &hdr[id], sizeof(ddsx::Header));

    crd->seekto(rec[id].ofs);
    crd->read((char *)b.ptr + sizeof(ddsx::Header), rec[id].packedDataSize);
    return true;
  }
  void removeDDSx(const char *name)
  {
    int namelen = (int)strlen(name);
    for (int i = 0; i < texNames.map.size(); i++)
      if (strnicmp(texNames.map[i], name, namelen) == 0 && texNames.map[i][namelen] == '*')
      {
        debug("removed changed %s: %s", name, texNames.map[i]);
        rec[i].packedDataSize = -1;
        return;
      }
  }
  bool hasDDSx(const char *name)
  {
    int namelen = (int)strlen(name);
    for (int i = 0; i < texNames.map.size(); i++)
      if (strnicmp(texNames.map[i], name, namelen) == 0 && texNames.map[i][namelen] == '*')
        return true;
    return false;
  }
};

class Pack2TexGatherHelper
{
  Tab<String> texPath;
  Tab<DagorAsset *> texAsset;
  struct PackRec
  {
    int texId, ofs, packedDataSize;
  };

public:
  mkbindump::StrCollector texName;

public:
  Pack2TexGatherHelper() : texPath(tmpmem), texAsset(tmpmem) {}

  bool needsPacking(DDSxTexPack2Serv &prev_pack)
  {
    if (prev_pack.getFormatVer() != dabuild_dxp_write_ver)
      return true;
    auto status = iterate_names_breakable(texName.getMap(), [&](int, const char *name) {
      int id = prev_pack.getTexRecId(name);
      if (id == -1 || prev_pack.rec[id].packedDataSize < 0)
        return IterateStatus::StopOk;
      return IterateStatus::Continue;
    });
    if (status == IterateStatus::StopOk)
      return true;
    for (int i = 0; i < prev_pack.texNames.map.size(); i++)
      if (texName.getMap().getNameId(prev_pack.texNames.map[i]) == -1)
        return true;
    return false;
  }

  bool saveTextures(DDSxTexPack2Serv *prev_pack, mkbindump::BinDumpSaveCB &cwr, IDagorAssetExporter *exp, const char *target_str,
    const char *profile, ILogWriter &log, IGenericProgressIndicator &pbar, const char *pack_fname, void *active_mutex)
  {
    SmallTab<char, TmpmemAlloc> buf;
    SmallTab<ddsx::Buffer, TmpmemAlloc> ddsx_buf;
    SmallTab<PackRec, TmpmemAlloc> rec;
    SmallTab<int, TmpmemAlloc> ordmap;
    SmallTab<const char *, TmpmemAlloc> ordname;

    pbar.setActionDesc(String(256, "%sConverting textures to DDSx: %s...", dabuild_progress_prefix_text, pack_fname));
    pbar.setDone(0);
    pbar.setTotal(texName.strCount());

    clear_and_resize(ordmap, texName.strCount());
    mem_set_ff(ordmap);
    clear_and_resize(ordname, texName.strCount());
    mem_set_0(ordname);
    clear_and_resize(ddsx_buf, texName.strCount());
    mem_set_0(ddsx_buf);
    clear_and_resize(rec, texName.strCount());
    mem_set_0(rec);

    int build_errors = 0;
    int ord_idx = 0;
    Tab<int> sorted_ids;
    gather_ids_in_lexical_order(sorted_ids, texName.getMap());
    auto status = iterate_names_in_order_breakable(texName.getMap(), sorted_ids, [&](int id, const char *name) {
      ordmap[id] = ord_idx++;
      ordname[id] = name;
      const char *tex_path = texPath[id];
      DagorAsset *a = texAsset[id];
      TextureMetaData tmd;
      tmd.read(a->props, a->resolveEffProfileTargetStr(target_str, profile));
      if (!dagor_target_code_supports_tex_diff(cwr.getTarget()))
        tmd.baseTexName = NULL;

      ddsx::Buffer &b = ddsx_buf[ordmap[id]];

      if (!dd_file_exist(tex_path))
      {
        log.addMessage(ILogWriter::ERROR, "Can't open image: %s", tex_path);
        build_errors++;
        if (!dabuild_stop_on_first_error)
          return IterateStatus::Continue;
        return IterateStatus::StopFail;
      }

      if (dabuild_strip_d3d_res)
      {
        b.len = sizeof(ddsx::Header);
        b.ptr = memalloc(b.len, tmpmem);
        memset(b.ptr, 0, b.len);

        ddsx::Header &h = *(ddsx::Header *)b.ptr;
        h.label = _MAKE4C('DDSx');
        return IterateStatus::Continue;
      }

      if (prev_pack && prev_pack->getDDSx(name, b))
      {
        debug("%s reused from prev pack", name);
        pbar.incDone();
        return IterateStatus::Continue;
      }

      struct AutoMutexUnlock
      {
        void *amutex;
        AutoMutexUnlock(void *m) : amutex(m)
        {
          if (amutex)
            global_mutex_leave(amutex);
        }
        ~AutoMutexUnlock()
        {
          if (amutex)
            global_mutex_enter(amutex);
        }
      };
      AutoMutexUnlock mutexRelock(active_mutex);
      FATAL_CONTEXT_AUTO_SCOPE(a->getNameTypified());

      int len;
      if (exp && a->props.getBool("convert", false))
      {
        if (!texconvcache::get_tex_asset_built_ddsx(*a, b, cwr.getTarget(), profile, &log))
        {
          log.addMessage(ILogWriter::ERROR, "Can't export tex asset: %s", a->getName());
          build_errors++;
          if (dabuild_stop_on_first_error)
            return IterateStatus::StopFail;
        }
        pbar.incDone();
        return IterateStatus::Continue;
      }

      ddsx::ConvertParams cp;
      cp.packSzThres = 16 << 10;
      cp.addrU = tmd.d3dTexAddr(tmd.addrU);
      cp.addrV = tmd.d3dTexAddr(tmd.addrV);
      cp.hQMip = tmd.hqMip;
      cp.mQMip = tmd.mqMip;
      cp.lQMip = tmd.lqMip;
      cp.optimize = (tmd.flags & tmd.FLG_OPTIMIZE) ? true : false;

#define GET_PROP(TYPE, PROP, DEF) props.get##TYPE(PROP, &props != &a->props ? a->props.get##TYPE(PROP, DEF) : DEF)
      const DataBlock &props = a->getProfileTargetProps(cwr.getTarget(), cwr.getProfile());
      cp.imgGamma = GET_PROP(Real, "gamma", 2.2);
      cp.mipOrdRev = GET_PROP(Bool, "mipOrdRev", cp.mipOrdRev);
      cp.splitHigh = GET_PROP(Bool, "splitHigh", false);
      cp.splitAt = GET_PROP(Int, "splitAt", 0);
      if (!GET_PROP(Bool, "splitAtOverride", false))
        cp.splitAt = a->props.getBlockByNameEx(target_str)->getInt("splitAtSize", cp.splitAt);

      if (!tmd.baseTexName.empty())
        cp.needBaseTex = true;
      if (const char *paired = GET_PROP(Str, "pairedToTex", NULL))
        if (dagor_target_code_supports_tex_diff(cwr.getTarget()) && *paired)
        {
          int nid = a->props.getNameId("pairedToTex");
          for (int i = 0; i < props.paramCount(); i++)
            if (props.getParamNameId(i) == nid && props.getParamType(i) == DataBlock::TYPE_STRING)
              if (a->getMgr().findAsset(props.getStr(i), a->getType()))
              {
                cp.needSysMemCopy = true;
                break;
              }
          if (!cp.needSysMemCopy && &a->props != &props)
            for (int i = 0; i < a->props.paramCount(); i++)
              if (a->props.getParamNameId(i) == nid && a->props.getParamType(i) == DataBlock::TYPE_STRING)
                if (a->getMgr().findAsset(a->props.getStr(i), a->getType()))
                {
                  cp.needSysMemCopy = true;
                  break;
                }
        }
#undef GET_PROP

      if (!texconvcache::convert_dds(b, tex_path, a, cwr.getTarget(), cp))
      {
        log.addMessage(ILogWriter::ERROR, "Can't export image: %s, err=%s", tex_path, ddsx::get_last_error_text());
        build_errors++;
        if (dabuild_stop_on_first_error)
          return IterateStatus::StopFail;
      }
      pbar.incDone();
      return IterateStatus::Continue;
    });
    if (status != IterateStatus::StopDoneAll || build_errors)
      return false;
    iterate_names_in_order(texName.getMap(), sorted_ids, [&](int id, const char *name) {
      DagorAsset *a = texAsset[id];
      ddsx::Buffer &b = ddsx_buf[ordmap[id]];
      if (should_skip_writing_tex_contents(a, b, cwr.getTarget(), profile))
      {
        b.len = sizeof(ddsx::Header);
        memset(b.ptr, 0, b.len);
        ddsx::Header &h = *(ddsx::Header *)b.ptr;
        h.label = _MAKE4C('DDSx');
      }
    });

    if (dabuild_build_tex_separate)
    {
      pbar.setActionDesc(String(256, "Writing ddsx tex: %s/", pack_fname));
      pbar.setDone(0);
      pbar.setTotal(texName.strCount());
      int tex_changed = 0;
      for (int j = 0; j < ordmap.size(); j++)
      {
        int i = ordmap[j];
        const char *enc_tex_nm = ordname[j];
        int enc_tex_nm_len = i_strlen(enc_tex_nm);
        if (const char *s = strchr(enc_tex_nm, '*'))
          enc_tex_nm_len = s - enc_tex_nm;

        String fn(0, "%s/%.*s.ddsx", pack_fname, enc_tex_nm_len, enc_tex_nm);
        if (!cmp_data_eq(ddsx_buf[i].ptr, ddsx_buf[i].len, fn))
        {
          stat_tex_sz_diff += ddsx_buf[i].len - get_file_sz(fn);
          FullFileSaveCB fcwr(fn);
          fcwr.write(ddsx_buf[i].ptr, ddsx_buf[i].len);
          tex_changed++;
        }
        ddsx_buf[i].free();
        pbar.incDone();
      }
      if (!tex_changed)
        stat_tex_unchanged++;
      return true;
    }

    pbar.setActionDesc(String(256, "Writing ddsxTex pack: %s...", pack_fname));
    pbar.setDone(0);
    pbar.setTotal(texName.strCount() + 2);

    mkbindump::RoNameMapBuilder b;
    mkbindump::PatchTabRef pt_ddsx, pt_rec;

    b.prepare(texName);

    cwr.writeFourCC(_MAKE4C('DxP2'));
    cwr.writeInt32e(dabuild_dxp_write_ver);
    cwr.writeInt32e(texName.strCount());

    cwr.beginBlock();
    cwr.setOrigin();

    b.writeHdr(cwr);
    pt_ddsx.reserveTab(cwr);
    pt_rec.reserveTab(cwr);
    cwr.writePtr64e(0);

    texName.writeStrings(cwr);
    cwr.align8();
    b.writeMap(cwr);

    cwr.align32();
    pt_ddsx.startData(cwr, ddsx_buf.size());
    unsigned ddsx_sz = 0;
    for (int i = 0; i < ddsx_buf.size(); i++)
    {
      cwr.writeRaw(ddsx_buf[i].ptr, sizeof(ddsx::Header));
      ddsx_sz += ddsx_buf[i].len;
    }
    pt_ddsx.finishTab(cwr);

    pt_rec.reserveData(cwr, ddsx_buf.size(), dabuild_dxp_write_ver == 3 ? sizeof(PackRec) : sizeof(PackRec) + 12);
    pt_rec.finishTab(cwr);
    cwr.popOrigin();
    cwr.endBlock();
    pbar.incDone();

    int hdr_sz = cwr.tell(), ddsx_data_start = (hdr_sz + 0xFFFF) & ~0xFFFF; // align on 64K boundary for PS4 patching
    if (ddsx_buf.size() > 1 && (ddsx_data_start - hdr_sz < 1024 || ddsx_sz > (128 << 10)))
      cwr.writeZeroes(ddsx_data_start - hdr_sz);
    else
      cwr.align32();

    for (int j = 0; j < ordmap.size(); j++)
    {
      int i = ordmap[j];
      cwr.align8();
      rec[i].ofs = cwr.tell();
      rec[i].texId = dabuild_dxp_write_ver == 3 ? 0 : -1;
      rec[i].packedDataSize = ddsx_buf[i].len - sizeof(ddsx::Header);
      cwr.writeRaw((char *)ddsx_buf[i].ptr + sizeof(ddsx::Header), rec[i].packedDataSize);
      ddsx_buf[i].free();
      pbar.incDone();
    }

    int wrpos = cwr.tell();
    cwr.seekto(pt_rec.resvDataPos + 16);
    if (dabuild_dxp_write_ver == 3)
      cwr.writeTabData32ex(rec);
    else
      for (auto &r : rec)
      {
        cwr.writeInt64e(0);
        cwr.write32ex(&r, sizeof(r));
        cwr.writeInt32e(0);
      }

    cwr.seekto(wrpos);
    pbar.incDone();

    return true;
  }

  bool addTexture(char *fpath, DagorAsset *a, ILogWriter &log, const char *target_str, const char *profile, unsigned tc, int tq_nsid)
  {
    TextureMetaData tmd;
    load_eff_tmd(tmd, a, tq_nsid, target_str, profile);
    if (!dagor_target_code_supports_tex_diff(tc))
      tmd.baseTexName = NULL;

    // normalize quality mips - hqMip will be removed during ddsx conversion
    tmd.mqMip -= tmd.hqMip;
    tmd.lqMip -= tmd.hqMip;
    tmd.hqMip = 0;

    G_ASSERTF(tmd.isValid(), "Invalid texture metadata for <%s> (%s)", a->getName(), fpath);

    String tname(0, "%s*", a->getName());
    String tname_enc_lwr(tname);
    if (should_encode_tmd(a, tq_nsid))
      tname_enc_lwr = tmd.encode(tname);
    dd_strlwr(tname_enc_lwr);

    texName.addString(tname_enc_lwr);

    if (texName.strCount() == texPath.size())
      return true;
    G_ASSERT(texName.strCount() == texPath.size() + 1);

    texPath.push_back() = fpath;
    texAsset.push_back(a);

    if (!dd_file_exist(fpath))
    {
      log.addMessage(log.ERROR, "missing tex: %s (%s)", fpath, tname_enc_lwr.str());
      return false;
    }

    // debug("%s -> %s", a->getName(), fpath);
    return true;
  }
};


bool buildDdsxTexPack(mkbindump::BinDumpSaveCB &cwr, dag::ConstSpan<DagorAsset *> assets, AssetExportCache &c4, const char *pack_fname,
  ILogWriter &log, IGenericProgressIndicator &pbar, bool &up_to_date)
{
  if (dabuild_skip_any_build)
    return up_to_date = dd_file_exist(pack_fname);

  char target_str[5];
  strcpy(target_str, mkbindump::get_target_str(cwr.getTarget()));

  up_to_date = false;

  IDagorAssetExporter *exp = assets[0]->getMgr().getAssetExporter(assets[0]->getType());
  int gameres_ver = exp ? exp->getGameResVersion() : 0;
  int tq_nsid = assets[0]->getMgr().getAssetNameSpaceId("texTQ");
  Tab<SimpleString> a_files(tmpmem);

  FullFileLoadCB prev_fcrd(pack_fname);
  BinDumpReader prev_crd(&prev_fcrd, cwr.getTarget(), cwr.WRITE_BE);
  DDSxTexPack2Serv *prev_pack = NULL;

  if (!dabuild_force_dxp_rebuild && prev_fcrd.fileHandle && df_length(prev_fcrd.fileHandle) >= 16 &&
      !c4.checkAssetExpVerChanged(assets[0]->getType(), gameres_ver, DDSX_EXP_VER))
    prev_pack = DDSxTexPack2Serv::make(prev_crd);

  if (prev_pack && c4.checkTargetFileChanged(pack_fname))
    del_it(prev_pack);

  if (dabuild_build_tex_separate)
  {
    del_it(prev_pack);
    prev_fcrd.close();
    dd_erase(pack_fname);
    dd_mkdir(pack_fname);
  }
  else if (!prev_fcrd.fileHandle)
  {
    FullFileSaveCB fcwr(pack_fname);
    if (!fcwr.fileHandle)
    {
      log.addMessage(log.ERROR, "Can't write file %s", pack_fname);
      return false;
    }
    fcwr.close();
    dd_erase(pack_fname);
  }
  if (!prev_pack)
    c4.reset();
  if (dd_file_exist(tq_split_fn))
    if (c4.checkFileChanged(tq_split_fn))
    {
      c4.reset();
      c4.checkFileChanged(tq_split_fn);
    }
  c4.setAssetExpVer(assets[0]->getType(), gameres_ver, DDSX_EXP_VER);

  Pack2TexGatherHelper trh;
  String texname;
  bool errors = false;
  for (int i = 0; i < assets.size(); i++)
  {
    texname = assets[i]->getTargetFilePath();
    bool changed = false;
    if (c4.checkFileChanged(texname))
      changed = true;
    if (c4.checkDataBlockChanged(assets[i]->getNameTypified(), assets[i]->props))
      changed = true;

    if (exp)
    {
      exp->gatherSrcDataFiles(*assets[i], a_files);
      int cnt = a_files.size();
      for (int j = 0; j < cnt; j++)
        if (c4.checkFileChanged(a_files[j]))
          changed = true;
    }

    if (prev_pack && changed)
      prev_pack->removeDDSx(assets[i]->getName());

    if (!trh.addTexture(texname, assets[i], log, target_str, cwr.getProfile(), cwr.getTarget(), tq_nsid))
      errors = true;
  }
  if (errors)
  {
    del_it(prev_pack);
    return false;
  }

  if (prev_pack && !trh.needsPacking(*prev_pack))
  {
    log.addMessage(log.NOTE, "skip up-to-date %s", pack_fname);
    del_it(prev_pack);
    up_to_date = true;
    return true;
  }

  struct IntensiveDiskIoMutex
  {
    void *m;
    IntensiveDiskIoMutex() { m = global_mutex_create("dabuild-write-tex-pack"); }
    ~IntensiveDiskIoMutex() { global_mutex_leave_destroy(m, "dabuild-write-tex-pack"); }
  };
  IntensiveDiskIoMutex ioMutex;

  // write textures (each in own block)
  if (!trh.saveTextures(prev_pack, cwr, exp, target_str, cwr.getProfile(), log, pbar, pack_fname, ioMutex.m))
  {
    del_it(prev_pack);
    return false;
  }

  if (dabuild_build_tex_separate)
  {
    log.addMessage(log.NOTE, "built %s/, %d textures\n", pack_fname, trh.texName.strCount());
    stat_tex_built++;
    return true;
  }

  prev_fcrd.close();
  if (cmp_data_eq(cwr, pack_fname))
  {
    log.addMessage(log.NOTE, "built %s: %d Kb, %d textures; binary remains the same\n", pack_fname,
      dag_get_file_size(pack_fname) >> 10, trh.texName.strCount());
    del_it(prev_pack);
    stat_tex_unchanged++;
    c4.setTargetFileHash(pack_fname);
    return true;
  }

  stat_tex_sz_diff -= get_file_sz(pack_fname);
  stat_tex_sz_diff += cwr.getSize();
  stat_changed_tex_total_sz += cwr.getSize();

  if (dabuild_dry_run)
  {
    log.addMessage(log.NOTE, "built(dry) %s: %d Kb, %d textures\n", pack_fname, dag_get_file_size(pack_fname) >> 10,
      trh.texName.strCount());
    del_it(prev_pack);
    return true;
  }

  ::dd_mkpath(pack_fname);
  FullFileSaveCB fcwr(pack_fname);
  if (!fcwr.fileHandle)
  {
    pbar.destroy();
    log.addMessage(log.ERROR, "Can't write file %s", pack_fname);
    del_it(prev_pack);
    dd_erase(pack_fname);
    return false;
  }
  DAGOR_TRY { cwr.copyDataTo(fcwr); }
  DAGOR_CATCH(const IGenSave::SaveException &)
  {
    log.addMessage(ILogWriter::ERROR, "Error writing DXP file %s", pack_fname);
    dd_erase(pack_fname);
    del_it(prev_pack);
    return false;
  }
  fcwr.close();
  c4.setTargetFileHash(pack_fname);

  log.addMessage(log.NOTE, "built %s: %d Kb, %d textures\n", pack_fname, dag_get_file_size(pack_fname) >> 10, trh.texName.strCount());
  del_it(prev_pack);
  stat_tex_built++;
  return true;
}

bool checkDdsxTexPackUpToDate(unsigned tc, const char *profile, bool be, dag::ConstSpan<DagorAsset *> assets, AssetExportCache &c4,
  const char *pack_fname, int ch_bit)
{
  if (dabuild_build_tex_separate)
    return true;
  char target_str[5];
  strcpy(target_str, mkbindump::get_target_str(tc));

  FullFileLoadCB prev_fcrd(pack_fname);
  BinDumpReader prev_crd(&prev_fcrd, tc, be);
  DDSxTexPack2Serv *prev_pack = NULL;
  IDagorAssetExporter *exp = assets[0]->getMgr().getAssetExporter(assets[0]->getType());
  int gameres_ver = exp ? exp->getGameResVersion() : 0;
  int tq_nsid = assets[0]->getMgr().getAssetNameSpaceId("texTQ");
  Tab<SimpleString> a_files(tmpmem);
  mkbindump::StrCollector texName;

  if (!dabuild_force_dxp_rebuild && prev_fcrd.fileHandle && (df_length(prev_fcrd.fileHandle) >= 16) &&
      !c4.checkAssetExpVerChanged(assets[0]->getType(), gameres_ver, DDSX_EXP_VER) && !c4.checkTargetFileChanged(pack_fname))
    prev_pack = DDSxTexPack2Serv::make(prev_crd);

  if (dd_file_exist(tq_split_fn))
    if (c4.checkFileChanged(tq_split_fn))
      del_it(prev_pack);

  String texname;
  bool changed = false;
  if (prev_pack && prev_pack->getFormatVer() != dabuild_dxp_write_ver)
    changed = true;
  for (int i = 0; i < assets.size(); i++)
  {
    {
      TextureMetaData tmd;
      load_eff_tmd(tmd, assets[i], tq_nsid, target_str, profile);
      if (!dagor_target_code_supports_tex_diff(tc))
        tmd.baseTexName = NULL;

      // normalize quality mips - hqMip will be removed during ddsx conversion
      tmd.mqMip -= tmd.hqMip;
      tmd.lqMip -= tmd.hqMip;
      tmd.hqMip = 0;

      String tname(0, "%s*", assets[i]->getName());
      String tname_enc_lwr(tname);
      if (should_encode_tmd(assets[i], tq_nsid))
        tname_enc_lwr = tmd.encode(tname);
      dd_strlwr(tname_enc_lwr);
      texName.addString(tname_enc_lwr);
    }

    if (!prev_pack)
      goto cur_changed;
    if (!prev_pack->hasDDSx(assets[i]->getName()))
      goto cur_changed;
    if (c4.checkDataBlockChanged(assets[i]->getNameTypified(), assets[i]->props))
      goto cur_changed;

    texname = assets[i]->getTargetFilePath();

    if (c4.checkFileChanged(texname))
      goto cur_changed;

    if (exp)
    {
      exp->gatherSrcDataFiles(*assets[i], a_files);
      // A lot of assets add the asset itself to the list of src data files.
      // checkFileChanged is quite slow and we checked this asset already above.
      // Removing this asset from gather list is faster then calling checkFileChanged one extra time
      if (a_files.size() > 0 && strcmp(a_files[0].c_str(), texname.c_str()) == 0)
        a_files.erase(a_files.begin());
      int cnt = a_files.size();
      for (int j = 0; j < cnt; j++)
        if (c4.checkFileChanged(a_files[j]))
          goto cur_changed;
    }

    assets[i]->setUserFlags(ch_bit);
    continue;

  cur_changed:
    assets[i]->clrUserFlags(ch_bit);
    changed = true;
  }

  if (!changed && prev_pack)
    for (int i = 0; i < prev_pack->texNames.map.size(); i++)
      if (texName.getMap().getNameId(prev_pack->texNames.map[i]) == -1)
      {
        changed = true;
        break;
      }
  prev_fcrd.close();
  del_it(prev_pack);

  return changed;
}
