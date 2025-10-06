// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resFormats.h"
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zstdIo.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_strUtil.h>
#include <util/dag_texMetaData.h>
#include <util/dag_fastIntList.h>
#include <supp/dag_zstdObfuscate.h>
#include <libTools/util/fileUtils.h>
#include <libTools/util/genericCache.h>
#include <libTools/util/strUtil.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <stddef.h>
#include <time.h>

int game_resources_diff_verbose = 0;
bool game_resources_diff_strict_tex = false;

template <class V, typename T = typename V::value_type>
static bool check_slice(const V &s, void *b, int sz)
{
  if (!s.data() && !s.size())
    return true;
  return (void *)s.data() >= b && (void *)(s.data() + s.size()) <= (char *)b + sz;
}
static bool check_str(const char *s, void *b, int sz, int max_strlen)
{
  if (s < b || s > (char *)b + sz)
    return false;
  for (const char *se = s + max_strlen; s < se && s < (char *)b + sz; s++)
    if (*s == 0)
      return true;
  return false;
}

static bool corrupted(GrpBinData &dest, int ln)
{
  logerr("file is corrupted: %s (%d)", dest.fn.str(), ln);
  memfree(dest.data, tmpmem);
  dest.data = NULL;
  return false;
}
static bool corrupted(DxpBinData &dest, int ln)
{
  logerr("file is corrupted: %s (%d)", dest.fn.str(), ln);
  memfree(dest.data, tmpmem);
  dest.data = NULL;
  return false;
}

static bool load_grp(GrpBinData &dest, IGenLoad &crd, int file_size)
{
  gamerespackbin::GrpHeader hdr;

  crd.read(&hdr, sizeof(hdr));

  if (hdr.label != _MAKE4C('GRP2') && hdr.label != _MAKE4C('GRP3'))
    return false;
  if (file_size > 0 && hdr.restFileSize + sizeof(hdr) != file_size)
    return false;

  gamerespackbin::GrpData *gdata = gdata = (gamerespackbin::GrpData *)memalloc(hdr.fullDataSize, tmpmem);
  crd.read(gdata, hdr.fullDataSize);

  dest.verLabel = hdr.label;
  dest.data = gdata;
  dest.fullSz = hdr.restFileSize + sizeof(hdr);
  clear_and_resize(dest.hdrData, (hdr.fullDataSize + sizeof(hdr) + 15) & ~15);
  mem_set_0(dest.hdrData);

  dest.fn = crd.getTargetName();
  if (dest.fn.size() > 9 && strcmp(dest.fn.end(9), "cache.bin") == 0)
    erase_items(dest.fn, dest.fn.size() - 10, 9);
  dd_simplify_fname_c(dest.fn);
  dest.fn.resize(strlen(dest.fn) + 1);

  memcpy(dest.hdrData.data(), &hdr, sizeof(hdr));
  memcpy(dest.hdrData.data() + sizeof(hdr), gdata, hdr.fullDataSize);

  void *base = gdata->dumpBase();
  gdata->nameMap.patch(base);
  if (!check_slice(gdata->nameMap, gdata, hdr.fullDataSize))
    return corrupted(dest, __LINE__);
  for (int i = 0; i < gdata->nameMap.size(); i++)
    if (!check_str(gdata->getName(i), gdata, hdr.fullDataSize, 2048))
      return corrupted(dest, __LINE__);
  gdata->resTable.patch(base);
  if (!check_slice(gdata->resTable, gdata, hdr.fullDataSize))
    return corrupted(dest, __LINE__);
  for (int i = 0; i < gdata->resTable.size(); i++)
    if (short(gdata->resTable[i].resId) < -1 || short(gdata->resTable[i].resId) >= gdata->nameMap.size())
      return corrupted(dest, __LINE__);

  gdata->resData.patch(base);
  if (!check_slice(gdata->resData, gdata, hdr.fullDataSize))
    return corrupted(dest, __LINE__);
  if (hdr.label == _MAKE4C('GRP2'))
    gamerespackbin::ResData::cvtRecInplaceVer2to3(gdata->resData.data(), gdata->resData.size());
  for (int i = 0; i < gdata->resData.size(); i++)
  {
    if (short(gdata->resData[i].resId) < -1 || short(gdata->resData[i].resId) >= gdata->nameMap.size())
      return corrupted(dest, __LINE__);
    gdata->resData[i].refResIdPtr.patchNonNull(base);
    if (!check_slice(make_span_const(gdata->resData[i].refResIdPtr.get(), gdata->resData[i].refResIdCnt), gdata, hdr.fullDataSize))
      return corrupted(dest, __LINE__);
  }

  dest.resSz.resize(gdata->resTable.size());
  for (int i = 0, ie = gdata->resTable.size(); i < ie; i++)
  {
    dest.resSz[i] = (i + 1 < ie ? gdata->resTable[i + 1].offset : hdr.restFileSize + sizeof(hdr)) - gdata->resTable[i].offset;
    if (gdata->resTable[i].offset < hdr.fullDataSize || gdata->resTable[i].offset > dest.fullSz || dest.resSz[i] < 0 ||
        gdata->resTable[i].offset + dest.resSz[i] > dest.fullSz)
    {
      logwarn("%s: corrupted res %d <%s>  ofs=%d sz=%d", dest.fn.str(), i, gdata->getName(gdata->resTable[i].resId),
        gdata->resTable[i].offset, dest.resSz[i]);
      dest.resSz[i] = gdata->resTable[i].offset = 0;
      // return corrupted(dest, __LINE__);
    }
  }

  return true;
}

static bool load_dxp(DxpBinData &dest, IGenLoad &crd, int file_size, bool old_file)
{
  unsigned hdr[4];
  crd.read(hdr, sizeof(hdr));

  if (hdr[0] != _MAKE4C('DxP2') || (hdr[1] != 2 && hdr[1] != 3))
    return false;

  DxpBinData::Dump *tdata = (DxpBinData::Dump *)memalloc(hdr[3], tmpmem);
  crd.read(tdata, hdr[3]);

  dest.data = tdata;
  dest.ver = hdr[1];
  clear_and_resize(dest.hdrData, hdr[3] + sizeof(hdr));

  dest.fn = crd.getTargetName();
  if (dest.fn.size() > 9 && strcmp(dest.fn.end(9), "cache.bin") == 0)
    erase_items(dest.fn, dest.fn.size() - 10, 9);
  dd_simplify_fname_c(dest.fn);
  dest.fn.resize(strlen(dest.fn) + 1);

  memcpy(dest.hdrData.data(), &hdr, sizeof(hdr));
  memcpy(dest.hdrData.data() + sizeof(hdr), tdata, hdr[3]);

  tdata->texNames.patchData(tdata);
  if (!check_slice(tdata->texNames.map, tdata, hdr[3]))
    return corrupted(dest, __LINE__);
  for (int i = 0; i < tdata->texNames.map.size(); i++)
    if (!check_str(tdata->texNames.map[i], tdata, hdr[3], 2048))
      return corrupted(dest, __LINE__);

  tdata->texHdr.patch(tdata);
  if (!check_slice(tdata->texHdr, tdata, hdr[3]))
    return corrupted(dest, __LINE__);
  tdata->texRec.patch(tdata);
  if (!check_slice(tdata->texRec, tdata, hdr[3]))
    return corrupted(dest, __LINE__);
  if (dest.ver == 2)
    if (!DxpBinData::cvtRecInplaceVer2to3(make_span(tdata->texRec), hdr[3] - (uintptr_t(tdata->texRec.cbegin()) - uintptr_t(tdata))))
      return corrupted(dest, __LINE__);

  if (file_size > 0)
  {
    for (int i = 0; i < tdata->texNames.map.size(); i++)
      if (tdata->texRec[i].ofs < hdr[3] || tdata->texRec[i].packedDataSize < 0 ||
          tdata->texRec[i].ofs + tdata->texRec[i].packedDataSize > file_size)
      {
        logwarn("%s: corrupted tex %d <%s>  ofs=%d sz=%d", dest.fn.str(), i, tdata->texNames.map[i], tdata->texRec[i].ofs,
          tdata->texRec[i].packedDataSize);
        tdata->texRec[i].ofs = tdata->texRec[i].packedDataSize = 0;
        // return corrupted(dest, __LINE__);
      }
    dest.fullSz = file_size;
  }
  else
  {
    dest.fullSz = 0;
    for (int i = 0; i < tdata->texNames.map.size(); i++)
      if (tdata->texRec[i].ofs + tdata->texRec[i].packedDataSize > dest.fullSz)
        dest.fullSz = tdata->texRec[i].ofs + tdata->texRec[i].packedDataSize;
  }
  return true;
}


static bool load_packs(const char *pack_list_blk_fname, const char *res_base_dir, Tab<GrpBinData> &grp, Tab<DxpBinData> &dxp, bool c,
  bool old_pack)
{
  DataBlock blk;
  const DataBlock *b;
  if (!blk.load(pack_list_blk_fname))
  {
    logerr("cannot load res pack list: %s", pack_list_blk_fname);
    return false;
  }

  char buf[512];
  if (res_base_dir)
    strcpy(buf, res_base_dir);
  else
    dd_get_fname_location(buf, pack_list_blk_fname);

  dd_simplify_fname_c(buf);
  if (!buf[0])
    strcpy(buf, "./");
  dd_append_slash_c(buf);

  char tempBuf[DAGOR_MAX_PATH];

  const DataBlock &bHash = *blk.getBlockByNameEx("hash_md5");
  int nid = blk.getNameId("pack");
  b = blk.getBlockByNameEx("ddsxTexPacks");
  for (int i = 0; i < b->paramCount(); i++)
    if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
    {
      _snprintf(tempBuf, sizeof(tempBuf), "%s%s", buf, b->getStr(i));
      tempBuf[sizeof(tempBuf) - 1] = 0;
      if (c)
        strcat(tempBuf, "cache.bin");

      FullFileLoadCB crd(tempBuf);
      if (!crd.fileHandle)
      {
        logerr("failed to open %s", tempBuf);
        continue;
      }
      if (!load_dxp(dxp.push_back(), crd, c ? -1 : df_length(crd.fileHandle), old_pack))
        dxp.pop_back();
      else
        dxp.back().hashMD5 = bHash.getStr(b->getStr(i), NULL);
    }

  b = blk.getBlockByNameEx("gameResPacks");
  for (int i = 0; i < b->paramCount(); i++)
    if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
    {
      _snprintf(tempBuf, sizeof(tempBuf), "%s%s", buf, b->getStr(i));
      tempBuf[sizeof(tempBuf) - 1] = 0;
      if (c)
        strcat(tempBuf, "cache.bin");

      FullFileLoadCB crd(tempBuf);
      if (!crd.fileHandle)
      {
        logerr("failed to open %s", tempBuf);
        continue;
      }
      if (!load_grp(grp.push_back(), crd, c ? -1 : df_length(crd.fileHandle)))
        grp.pop_back();
      else
        grp.back().hashMD5 = bHash.getStr(b->getStr(i), NULL);
    }
  debug("loaded %d grp(s) and %d dxp(s) from %s", grp.size(), dxp.size(), pack_list_blk_fname);
  return true;
}

template <class V, typename T = typename V::value_type>
static int find_the_same_pack(const V &p, const char *fn, int where_prefix_len)
{
  for (int i = 0; i < p.size(); i++)
    if (dd_stricmp(p[i].fn + where_prefix_len, fn) == 0)
      return i;
  return -1;
}

bool GrpBinData::findRes(dag::Span<GrpBinData> grp, int pair_idx, const char *res_name, int &old_idx, int &old_rec_idx)
{
  if (pair_idx >= 0)
  {
    grp[pair_idx].buildResMap();
    old_rec_idx = grp[pair_idx].resMap.getStrId(res_name);
    if (old_rec_idx >= 0)
    {
      old_idx = pair_idx;
      return true;
    }
  }
  for (int i = 0; i < grp.size(); i++)
  {
    if (i == pair_idx)
      continue;
    grp[i].buildResMap();
    old_rec_idx = grp[i].resMap.getStrId(res_name);
    if (old_rec_idx >= 0)
    {
      old_idx = i;
      return true;
    }
  }
  return false;
}
bool DxpBinData::findTex(dag::ConstSpan<DxpBinData> dxp, int pair_idx, const char *tex_name, const ddsx::Header &hdr, int &old_idx,
  int &old_rec_idx)
{
  int name_len = (int)strlen(tex_name);
  if (pair_idx >= 0)
  {
    old_rec_idx = DxpBinData::resolveNameId(dxp[pair_idx].data->texNames.map, tex_name, name_len);
    if (old_rec_idx >= 0)
    {
      old_idx = pair_idx;
      return true;
    }
  }
  for (int i = 0; i < dxp.size(); i++)
  {
    if (i == pair_idx)
      continue;
    old_rec_idx = DxpBinData::resolveNameId(dxp[i].data->texNames.map, tex_name, name_len);
    if (old_rec_idx >= 0)
    {
      old_idx = i;
      return true;
    }
  }
  return false;
}

static bool stream_equal_0(IGenLoad &crd, int sz)
{
  static const int BUF_SZ = 16;
  char buf[BUF_SZ];
  char buf0[BUF_SZ];
  memset(buf0, 0, sizeof(buf0));
  while (sz > 0)
  {
    int len = sz >= BUF_SZ ? BUF_SZ : sz;
    crd.read(buf, len);
    if (memcmp(buf, buf0, len) != 0)
      return false;
    sz -= len;
  }
  return true;
}
static bool stream_equal(IGenLoad &crd1, IGenLoad &crd2, int sz)
{
  static const int BUF_SZ = 4096;
  char buf1[BUF_SZ];
  char buf2[BUF_SZ];
  while (sz > 0)
  {
    int len = sz >= BUF_SZ ? BUF_SZ : sz;
    crd1.read(buf1, len);
    crd2.read(buf2, len);
    if (memcmp(buf1, buf2, len) != 0)
      return false;
    sz -= len;
  }
  return true;
}
static bool file_equal(const char *fn1, const char *fn2, const char *f1_md5, const char *f2_md5)
{
  if (!f1_md5[0])
    f1_md5 = NULL;
  if (!f2_md5[0])
    f2_md5 = NULL;
  if (f1_md5 && f2_md5)
    return stricmp(f1_md5, f2_md5) == 0;
  String stor;
  if (f1_md5)
  {
    unsigned char hash[GenericBuildCache::HASH_SZ];
    if (GenericBuildCache::getFileHash(fn2, hash))
      return stricmp(f1_md5, data_to_str_hex(stor, hash, sizeof(hash))) == 0;
  }
  else if (f2_md5)
  {
    unsigned char hash[GenericBuildCache::HASH_SZ];
    if (GenericBuildCache::getFileHash(fn1, hash))
      return stricmp(f2_md5, data_to_str_hex(stor, hash, sizeof(hash))) == 0;
  }


  FullFileLoadCB crd1(fn1);
  if (!crd1.fileHandle)
    return false;
  FullFileLoadCB crd2(fn2);
  if (!crd2.fileHandle)
    return false;

  int len = df_length(crd1.fileHandle);
  if (len != df_length(crd2.fileHandle))
    return false;
  return stream_equal(crd1, crd2, len);
}

static bool make_gameres_desc_diff(const char *desc_fn, const char *mnt_dir, const char *new_mnt_dir, const char *patch_dest_dir,
  const char *rel_mount_dir, int64_t &total_changed)
{
  String stor;
  String old_desc_bin(0, "%s/%s", mnt_dir, desc_fn);
  DataBlock old_desc_blk;
  if (dd_file_exists(old_desc_bin))
    old_desc_blk.load(old_desc_bin);
  String new_desc_bin(0, "%s/%s", new_mnt_dir, desc_fn);
  DataBlock new_desc_blk;
  if (dd_file_exists(new_desc_bin))
    new_desc_blk.load(new_desc_bin);

  // build *Desc.bin diff
  DynamicMemGeneralSaveCB cwr_new(tmpmem, 0, 4 << 10), cwr_old(tmpmem, 0, 4 << 10);
  for (int i = new_desc_blk.blockCount() - 1, old_i; i >= 0; i--)
    if ((old_i = old_desc_blk.findBlock(new_desc_blk.getBlock(i)->getBlockName())) >= 0)
    {
      cwr_new.seekto(0);
      cwr_new.resize(0);
      cwr_old.seekto(0);
      cwr_old.resize(0);
      new_desc_blk.getBlock(i)->saveToTextStream(cwr_new);
      old_desc_blk.getBlock(old_i)->saveToTextStream(cwr_old);

      if (cwr_new.size() == cwr_old.size() && memcmp(cwr_new.data(), cwr_old.data(), cwr_new.size()) == 0)
      {
        new_desc_blk.removeBlock(i);
        old_desc_blk.removeBlock(old_i);
      }
    }
  for (int i = old_desc_blk.blockCount() - 1; i >= 0; i--)
    if (new_desc_blk.findBlock(old_desc_blk.getBlock(i)->getBlockName()) < 0)
      new_desc_blk.addNewBlock(old_desc_blk.getBlock(i)->getBlockName());

  // write *Desc.bin diff
  FullFileSaveCB fcwr(String(0, "%s/%s/%s", patch_dest_dir, rel_mount_dir, desc_fn));
  if (!fcwr.fileHandle)
  {
    logerr("cannot write %s", fcwr.getTargetName());
    return false;
  }
  if (new_desc_blk.blockCount())
  {
    unsigned char hash[GenericBuildCache::HASH_SZ];
    if (GenericBuildCache::getFileHash(old_desc_bin, hash))
      new_desc_blk.setStr("base_md5", data_to_str_hex(stor, hash, sizeof(hash)));

    dblk::pack_to_stream(new_desc_blk, fcwr);
    total_changed += fcwr.tell();
  }
  return true;
}

int64_t make_game_resources_diff(const char *base_root_dir, const char *new_root_dir, const char *patch_dest_dir, bool dryRun,
  const char *vrom_name, const char *res_blk_name, const char *rel_mount_dir, int &out_diff_files)
{
  int t0 = time(NULL);
  debug("patch_update_game_resources%s"
        "\n    base_root=<%s>"
        "\n     new_root=<%s>"
        "\n    mount=<%s>"
        "\n    cache_fn=<%s> res_list_fn=<%s>\n",
    dryRun ? " [DRY RUN]" : "", base_root_dir, new_root_dir, rel_mount_dir, vrom_name, res_blk_name);

  String eff_rel_mount_dir(rel_mount_dir);
  bool is_root_mount = strcmp(rel_mount_dir, ".") == 0 && dd_get_fname(res_blk_name) != res_blk_name;
  if (is_root_mount)
  {
    char dirbuf[DAGOR_MAX_PATH];
    dd_get_fname_location(dirbuf, res_blk_name);
    eff_rel_mount_dir = dirbuf;
    remove_trailing_string(eff_rel_mount_dir, "/");
  }

  String new_vfs_fn(0, "%s/%s/%s", new_root_dir, rel_mount_dir, vrom_name);
  VirtualRomFsData *new_vfs = load_vromfs_dump(new_vfs_fn, tmpmem);
  if (!new_vfs)
  {
    logerr("failed to load new VROMFS %s", new_vfs_fn);
    return -1;
  }

  String old_vfs_fn(0, "%s/%s/%s", base_root_dir, rel_mount_dir, vrom_name);
  VirtualRomFsData *old_vfs = load_vromfs_dump(old_vfs_fn, tmpmem);
  if (!old_vfs)
  {
    logwarn("failed to load old VROMFS %s", old_vfs_fn.str());
    memfree(new_vfs, tmpmem);
    return 0;
  }

  Tab<GrpBinData> old_grp(tmpmem), new_grp(tmpmem);
  Tab<DxpBinData> old_dxp(tmpmem), new_dxp(tmpmem);
  Tab<FullFileLoadCB *> crd_cache(tmpmem);
  Tab<char> buf(tmpmem);
  String stor;
  String stor2, stor3;

  int64_t new_grp_total = 0, new_grp_changed = 0;
  int64_t new_dxp_total = 0, new_dxp_changed = 0;
  int64_t total_changed = 0;
  int new_grp_changed_files = 0, new_dxp_changed_files = 0;

  String mnt(0, "%s/", base_root_dir);
  String mnt_dir(0, "%s/%s", base_root_dir, eff_rel_mount_dir);
  String res_list_fn(0, "%s/%s/%s", base_root_dir, rel_mount_dir, res_blk_name);
  String eff_mnt = mnt;
  if (strcmp(rel_mount_dir, "res") != 0 && !is_root_mount)
    eff_mnt.aprintf(0, "%s/", rel_mount_dir);

  add_vromfs(old_vfs, true, eff_mnt);
  if (!load_packs(res_list_fn, mnt_dir, old_grp, old_dxp, false, true))
  {
    logerr("can't load old packs");
    remove_vromfs(old_vfs);
    memfree(new_vfs, tmpmem);
    memfree(old_vfs, tmpmem);
    return -1;
  }
  remove_vromfs(old_vfs);

  String new_mnt_dir(0, "%s/%s", new_root_dir, eff_rel_mount_dir);
  String new_res_list_fn(0, "%s/%s/%s", new_root_dir, rel_mount_dir, res_blk_name);
  String new_eff_mnt(0, "%s/", new_root_dir);
  if (strcmp(rel_mount_dir, "res") != 0 && !is_root_mount)
    new_eff_mnt.aprintf(0, "%s/", rel_mount_dir);

  add_vromfs(new_vfs, true, new_eff_mnt);
  if (!load_packs(new_res_list_fn, new_mnt_dir, new_grp, new_dxp, false, false))
  {
    logerr("can't load new pack headers");
    remove_vromfs(new_vfs);
    memfree(new_vfs, tmpmem);
    memfree(old_vfs, tmpmem);
    return -1;
  }
  remove_vromfs(new_vfs);

  memfree(new_vfs, tmpmem);
  memfree(old_vfs, tmpmem);

  crd_cache.resize(old_grp.size());
  mem_set_0(crd_cache);

  for (int i = 0; i < new_grp.size(); i++)
  {
    new_grp_total += new_grp[i].fullSz;
    int pair_idx = find_the_same_pack(old_grp, new_grp[i].fn + strlen(new_root_dir), strlen(base_root_dir));
    if (pair_idx >= 0 && file_equal(new_grp[i].fn, old_grp[pair_idx].fn, new_grp[i].hashMD5, old_grp[pair_idx].hashMD5))
    {
      if (game_resources_diff_verbose)
        debug("skipping identical GRP %s and %s", new_grp[i].fn, old_grp[pair_idx].fn);
      continue;
    }

    Tab<SimpleString> old_refs, new_refs;
    for (int j = 0; j < new_grp[i].data->resTable.size(); j++)
    {
      FullFileLoadCB crd(new_grp[i].fn);
      const char *res_name = new_grp[i].data->getName(new_grp[i].data->resTable[j].resId);
      int nres_ofs = new_grp[i].data->resTable[j].offset, nres_sz = new_grp[i].resSz[j];

      int old_idx = -1, old_rec_idx = -1;
      if (GrpBinData::findRes(make_span(old_grp), pair_idx, res_name, old_idx, old_rec_idx))
      {
        if (!crd_cache[old_idx])
        {
          crd_cache[old_idx] = new FullFileLoadCB(old_grp[old_idx].fn);
          if (!crd_cache[old_idx]->fileHandle)
          {
            del_it(crd_cache[old_idx]);
            continue;
          }
        }

        new_grp[i].getResRefs(new_refs, new_grp[i].data->resTable[j].resId);
        old_grp[old_idx].getResRefs(old_refs, old_grp[old_idx].data->resTable[old_rec_idx].resId);
        if (new_refs.size() != old_refs.size())
        {
          debug("  res differs <%s> refs %d -> %d (%s)", res_name, old_refs.size(), new_refs.size(), new_grp[i].fn);
          goto add_changed;
        }
        for (int k = 0; k < new_refs.size(); k++)
          if (strcmp(new_refs[k], old_refs[k]) != 0)
          {
            debug("  res differs <%s> ref[%d] \"%s\" -> \"%s\" (%s)", res_name, k, old_refs[k], new_refs[k], new_grp[i].fn);
            goto add_changed;
          }

        int ores_ofs = old_grp[old_idx].data->resTable[old_rec_idx].offset, ores_sz = old_grp[old_idx].resSz[old_rec_idx];
        if (ores_sz == nres_sz)
        {
          crd_cache[old_idx]->seekto(ores_ofs);
          crd.seekto(nres_ofs);
          if (stream_equal(*crd_cache[old_idx], crd, nres_sz))
          {
            if (game_resources_diff_verbose)
              debug("skipping identical res=%s (%s:%d -> %s:%d)", res_name, old_grp[old_idx].fn, old_rec_idx, new_grp[i].fn, j);
            continue;
          }
        }
        else if (abs(ores_sz - nres_sz) < 16)
        {
          bool trailing_zeroes = false;
          if (ores_sz > nres_sz)
          {
            crd_cache[old_idx]->seekto(ores_ofs + nres_sz);
            trailing_zeroes = stream_equal_0(*crd_cache[old_idx], ores_sz - nres_sz);
          }
          else
          {
            crd.seekto(nres_ofs + ores_sz);
            trailing_zeroes = stream_equal_0(crd, nres_sz - ores_sz);
          }
          if (trailing_zeroes)
          {
            crd_cache[old_idx]->seekto(ores_ofs);
            crd.seekto(nres_ofs);
            if (stream_equal(*crd_cache[old_idx], crd, nres_sz < ores_sz ? nres_sz : ores_sz))
            {
              if (game_resources_diff_verbose)
                debug("skipping identical res=%s (%s:%d -> %s:%d) sz=%d->%d", res_name, old_grp[old_idx].fn, old_rec_idx,
                  new_grp[i].fn, j, ores_sz, nres_sz);
              continue;
            }
          }
        }
        debug("  res differs <%s> sz=%d->%d  %s:%d", res_name, ores_sz, nres_sz, new_grp[i].fn, j);
      }
      else
        debug("  res   added <%s> sz=%d  %s:%d", res_name, nres_sz, new_grp[i].fn, j);
    add_changed:
      new_grp[i].changedIdx.push_back(j);
      new_grp_changed += nres_sz;
    }
    if (new_grp[i].changedIdx.size())
      new_grp_changed_files++;
  }
  if (!dryRun && new_grp_changed_files)
  {
    for (int i = 0; i < new_grp.size(); i++)
      if (new_grp[i].changedIdx.size())
      {
        mkbindump::BinDumpSaveCB cwr(8 << 20, _MAKE4C('PC'), false);
        new_grp[i].buildPatch(cwr);
        total_changed += cwr.getSize();

        String fn(0, "%s/%s", patch_dest_dir, new_grp[i].fn + strlen(new_root_dir));
        dd_mkpath(fn);
        FullFileSaveCB fcwr(fn);
        cwr.copyDataTo(fcwr);
      }
  }
  clear_all_ptr_items(crd_cache);


  crd_cache.resize(old_dxp.size());
  mem_set_0(crd_cache);
  for (int i = 0; i < new_dxp.size(); i++)
  {
    new_dxp_total += new_dxp[i].fullSz;
    int pair_idx = find_the_same_pack(old_dxp, new_dxp[i].fn + strlen(new_root_dir), strlen(base_root_dir));
    if (pair_idx >= 0 && file_equal(new_dxp[i].fn, old_dxp[pair_idx].fn, new_dxp[i].hashMD5, old_dxp[pair_idx].hashMD5))
    {
      if (game_resources_diff_verbose)
        debug("skipping identical DxP %s and %s", new_dxp[i].fn, old_dxp[pair_idx].fn);
      continue;
    }

    for (int j = 0; j < new_dxp[i].data->texNames.map.size(); j++)
    {
      FullFileLoadCB crd(new_dxp[i].fn);
      TextureMetaData tmd;
      const ddsx::Header &nhdr = new_dxp[i].data->texHdr[j];
      const char *tex_name = tmd.decode(new_dxp[i].data->texNames.map[j], &stor);
      int ntex_ofs = new_dxp[i].data->texRec[j].ofs, ntex_sz = new_dxp[i].data->texRec[j].packedDataSize;

      int old_idx = -1, old_rec_idx = -1;
      if (DxpBinData::findTex(old_dxp, pair_idx, tex_name, nhdr, old_idx, old_rec_idx))
      {
        if (!crd_cache[old_idx])
        {
          crd_cache[old_idx] = new FullFileLoadCB(old_dxp[old_idx].fn);
          if (!crd_cache[old_idx]->fileHandle)
          {
            del_it(crd_cache[old_idx]);
            continue;
          }
        }

        const ddsx::Header &ohdr = old_dxp[old_idx].data->texHdr[old_rec_idx];
        int otex_ofs = old_dxp[old_idx].data->texRec[old_rec_idx].ofs,
            otex_sz = old_dxp[old_idx].data->texRec[old_rec_idx].packedDataSize;

        TextureMetaData otmd;
        otmd.decodeData(old_dxp[old_idx].data->texNames.map[old_rec_idx]);
        if (memcmp(&ohdr, &nhdr, sizeof(ohdr)) != 0)
          ; // changed
        else if (game_resources_diff_strict_tex && stricmp(otmd.encode(tex_name, &stor2), tmd.encode(tex_name, &stor3)) != 0)
        {
          // changed tex meta data!
          if (game_resources_diff_verbose)
            debug("  tex metadata changed: %s -> %s", stor2, stor3);
        }
        else if (stricmp(otmd.baseTexName, tmd.baseTexName) != 0)
        {
          // changed basetex ref!
          if (game_resources_diff_verbose)
            debug("  tex basetex changed: %s -> %s", otmd.baseTexName, tmd.baseTexName);
        }
        else if (otex_sz == ntex_sz)
        {
          crd_cache[old_idx]->seekto(otex_ofs);
          crd.seekto(ntex_ofs);
          if (stream_equal(*crd_cache[old_idx], crd, ntex_sz))
          {
            if (game_resources_diff_verbose)
              debug("skipping identical tex=%s (%s:%d -> %s:%d)", tex_name, old_dxp[old_idx].fn, old_rec_idx, new_dxp[i].fn, j);
            continue;
          }
        }
        else if (abs(otex_sz - ntex_sz) < 8)
        {
          bool trailing_zeroes = false;
          if (otex_sz > ntex_sz)
          {
            crd_cache[old_idx]->seekto(otex_ofs + ntex_sz);
            trailing_zeroes = stream_equal_0(*crd_cache[old_idx], otex_sz - ntex_sz);
          }
          else
          {
            crd.seekto(ntex_ofs + otex_sz);
            trailing_zeroes = stream_equal_0(crd, ntex_sz - otex_sz);
          }
          if (trailing_zeroes)
          {
            crd_cache[old_idx]->seekto(otex_ofs);
            crd.seekto(ntex_ofs);
            if (stream_equal(*crd_cache[old_idx], crd, ntex_sz < otex_sz ? ntex_sz : otex_sz))
            {
              if (game_resources_diff_verbose)
                debug("skipping identical tex=%s (%s:%d -> %s:%d) sz=%d->%d", tex_name, old_dxp[old_idx].fn, old_rec_idx,
                  new_dxp[i].fn, j, otex_sz, ntex_sz);
              continue;
            }
          }
        }
        debug("  tex differs <%s> sz=%d->%d  %s:%d", tex_name, otex_sz, ntex_sz, new_dxp[i].fn, j);
      }
      else
        debug("  tex   added <%s> sz=%d  %s:%d", tex_name, ntex_sz, new_dxp[i].fn, j);
      new_dxp[i].changedIdx.push_back(j);
      new_dxp_changed += ntex_sz;
    }
    if (new_dxp[i].changedIdx.size())
      new_dxp_changed_files++;
  }
  if (!dryRun && new_dxp_changed_files)
  {
    for (int i = 0; i < new_dxp.size(); i++)
      if (new_dxp[i].changedIdx.size())
      {
        mkbindump::BinDumpSaveCB cwr(8 << 20, _MAKE4C('PC'), false);
        new_dxp[i].buildPatch(cwr);
        total_changed += cwr.getSize();

        String fn(0, "%s/%s", patch_dest_dir, new_dxp[i].fn + strlen(new_root_dir));
        dd_mkpath(fn);
        FullFileSaveCB fcwr(fn);
        cwr.copyDataTo(fcwr);
      }
  }
  clear_all_ptr_items(crd_cache);

  if (!dryRun && new_grp_changed_files + new_dxp_changed_files)
  {
    String tmpStr;
    DataBlock resList_blk;
    int prefix_len = new_mnt_dir.length() + 1;

    {
      DataBlock *b;
      b = resList_blk.addNewBlock("gameResPacks");
      for (int i = 0; i < new_grp.size(); i++)
        if (new_grp[i].changedIdx.size())
          b->addStr("pack", new_grp[i].fn + prefix_len);

      b = resList_blk.addNewBlock("ddsxTexPacks");
      for (int i = 0; i < new_dxp.size(); i++)
        if (new_dxp[i].changedIdx.size())
          b->addStr("pack", new_dxp[i].fn + prefix_len);

      unsigned char hash[GenericBuildCache::HASH_SZ];
      if (GenericBuildCache::getFileHash(old_vfs_fn, hash))
        resList_blk.setStr("base_grpHdr_md5", data_to_str_hex(tmpStr, hash, sizeof(hash)));

      if (!make_gameres_desc_diff("riDesc.bin", mnt_dir, new_mnt_dir, patch_dest_dir, eff_rel_mount_dir, total_changed))
        return -1;
      if (!make_gameres_desc_diff("dynModelDesc.bin", mnt_dir, new_mnt_dir, patch_dest_dir, eff_rel_mount_dir, total_changed))
        return -1;
    }

    FastNameMap names;
    FastIntList grpNids;
    mkbindump::BinDumpSaveCB cwr(128 << 10, _MAKE4C('PC'), false);

    mkbindump::PatchTabRef pt_names, pt_data;
    pt_names.reserveTab(cwr);
    pt_data.reserveTab(cwr);

    for (int i = 0; i < new_grp.size(); i++)
      if (new_grp[i].changedIdx.size())
      {
        tmpStr = new_grp[i].fn + prefix_len;
        simplify_fname(tmpStr);
        dd_strlwr(tmpStr);
        int id = names.addNameId(tmpStr);
        G_ASSERT(id == names.nameCount() - 1);
        grpNids.addInt(id);
      }
    for (int i = 0; i < new_dxp.size(); i++)
      if (new_dxp[i].changedIdx.size())
      {
        tmpStr = new_dxp[i].fn + prefix_len;
        simplify_fname(tmpStr);
        dd_strlwr(tmpStr);
        int id = names.addNameId(tmpStr);
        G_ASSERT(id == names.nameCount() - 1);
      }
    int reslist_id = names.addNameId(String(res_blk_name).toLower());
    pt_names.reserveData(cwr, names.nameCount(), cwr.PTR_SZ);

    Tab<int> sorted_ids;
    gather_ids_in_lexical_order(sorted_ids, names);

    cwr.align16();
    int i = 0;
    iterate_names_in_order(names, sorted_ids, [&](int id, const char *name) {
      cwr.writeInt32eAt(cwr.tell(), pt_names.resvDataPos + i * cwr.PTR_SZ);
      if (id != reslist_id)
      {
        if (is_root_mount)
          tmpStr.printf(260, "%s/%scache.bin", eff_rel_mount_dir, name);
        else
          tmpStr.printf(260, "%scache.bin", name);
      }
      else
        tmpStr = name;
      simplify_fname(tmpStr);
      cwr.writeRaw(tmpStr, (int)strlen(tmpStr) + 1);
      i++;
    });

    cwr.align16();
    pt_data.reserveData(cwr, names.nameCount(), cwr.TAB_SZ);
    i = 0;
    iterate_names_in_order(names, sorted_ids, [&](int id, const char *name) {
      if (id == reslist_id)
      {
        cwr.align16();
        int p_start = cwr.tell();
        resList_blk.saveToStream(cwr.getRawWriter());
        int p_end = cwr.tell();
        cwr.writeInt32eAt(p_start, pt_data.resvDataPos + i * cwr.TAB_SZ);
        cwr.writeInt32eAt(p_end - p_start, pt_data.resvDataPos + i * cwr.TAB_SZ + 4);
      }
      i++;
    });
    bool failed = false;
    i = 0;
    iterate_names_in_order(names, sorted_ids, [&](int id, const char *name) {
      if (id == reslist_id)
      {
        i++;
        return;
      }
      cwr.align16();
      int p_start = cwr.tell();
      if (grpNids.hasInt(id))
      {
        if (!GrpBinData::writeGrpHdr(cwr, String(260, "%s/%s/%s", patch_dest_dir, eff_rel_mount_dir, name)))
        {
          debug("\nERR: failed write GRP hdr for %s/%s/%s", patch_dest_dir, rel_mount_dir, name);
          failed = true;
          return;
        }
      }
      else
      {
        if (!DxpBinData::writeDxpHdr(cwr, String(260, "%s/%s/%s", patch_dest_dir, eff_rel_mount_dir, name)))
        {
          debug("\nERR: failed write DxP hdr for %s/%s/%s", patch_dest_dir, rel_mount_dir, name);
          failed = true;
          return;
        }
      }
      int p_end = cwr.tell();
      cwr.writeInt32eAt(p_start, pt_data.resvDataPos + i * cwr.TAB_SZ);
      cwr.writeInt32eAt(p_end - p_start, pt_data.resvDataPos + i * cwr.TAB_SZ + 4);
      i++;
    });
    if (failed)
      return false;
    pt_names.finishTab(cwr);
    pt_data.finishTab(cwr);

    // finally, write file on disk
    String fname(0, "%s/%s/%s", patch_dest_dir, rel_mount_dir, vrom_name);
    dd_mkpath(fname);
    FullFileSaveCB fcwr(fname);
    if (!fcwr.fileHandle)
    {
      debug("\nERR: failed write %s", fname);
      return false;
    }

    fcwr.writeInt(_MAKE4C('VRFs'));
    fcwr.writeInt(cwr.getTarget());
    fcwr.writeInt(cwr.getSize());

    MemoryLoadCB crd(cwr.getMem(), false);
    DynamicMemGeneralSaveCB mcwr(tmpmem, cwr.getSize() + 4096);

    int packed_sz = zstd_compress_data(mcwr, crd, cwr.getSize(), 16 << 10);
    OBFUSCATE_ZSTD_DATA(mcwr.data(), packed_sz);

    unsigned hw32 = packed_sz | 0x40000000;
    fcwr.writeInt(hw32);
    fcwr.write(mcwr.data(), packed_sz);
    total_changed += fcwr.tell();
  }

  debug("patch_update_game_resources results:"
        "\n    GRP (total/changed/share)  %11lld (%3d) / %11lld (%3d) / %3d%%"
        "\n    DxP (total/changed/share)  %11lld (%3d) / %11lld (%3d) / %3d%%"
        "\n    Changed   (GRP+DxP=total)  %11lld (%3d) + %11lld (%3d) = %11lld (%3d)  diskSz=%11lld"
        "\n    %s (took %d sec)\n",
    new_grp_total, new_grp.size(), new_grp_changed, new_grp_changed_files,
    new_grp_total > 0 ? 100 * new_grp_changed / new_grp_total : 0, new_dxp_total, new_dxp.size(), new_dxp_changed,
    new_dxp_changed_files, new_dxp_total > 0 ? 100 * new_dxp_changed / new_dxp_total : 0, new_grp_changed, new_grp_changed_files,
    new_dxp_changed, new_dxp_changed_files, new_grp_changed + new_dxp_changed, new_grp_changed_files + new_dxp_changed_files,
    total_changed, dryRun ? "NO DATA CHANGED" : "FILES UPDATED", time(NULL) - t0);

  out_diff_files = new_grp_changed_files + new_dxp_changed_files;
  return new_grp_changed + new_dxp_changed;
}

bool GrpBinData::buildPatch(mkbindump::BinDumpSaveCB &cwr)
{
  if (!changedIdx.size())
    return false;
  if (changedIdx.size() == data->resTable.size())
  {
    // all resources changed, copy whole file
    FullFileLoadCB crd(fn);
    copy_stream_to_stream(crd, cwr.getRawWriter(), df_length(crd.fileHandle));
    return true;
  }

  using namespace mkbindump;
  OAHashNameMap<true> nm;
  PatchTabRef nm_pt, rrt_pt, rd_pt;
  SharedStorage<uint16_t> refIds;
  Tab<Ref> refs(tmpmem);
  Tab<uint16_t> tmpRefs(tmpmem);
  Tab<int> nameMapOfs(tmpmem);
  Tab<char> strData(tmpmem);
  Tab<int> rrdHdrOfs(tmpmem);
  Tab<int> rdToChangedMap(tmpmem);
  int desc_sz = 0, data_sz = 0;

  rdToChangedMap.resize(data->resData.size());
  mem_set_ff(rdToChangedMap);
  for (int i = 0; i < data->resData.size(); ++i)
    for (int j = 0; j < changedIdx.size(); ++j)
      if (data->resTable[changedIdx[j]].resId == data->resData[i].resId)
      {
        rdToChangedMap[i] = j;
        break;
      }

  for (int i = 0; i < changedIdx.size(); ++i)
    nm.addNameId(data->getName(data->resTable[changedIdx[i]].resId));

  refs.resize(data->resData.size());
  for (int i = 0; i < data->resData.size(); ++i)
  {
    if (rdToChangedMap[i] < 0)
      continue;

    nm.addNameId(data->getName(data->resData[i].resId));
    nm.addNameId(data->getName(data->resData[i].resId));

    tmpRefs = make_span_const(data->resData[i].refResIdPtr.get(), data->resData[i].refResIdCnt);
    for (int j = 0; j < tmpRefs.size(); ++j)
      if (tmpRefs[j] != 0xFFFF)
        tmpRefs[j] = nm.addNameId(data->getName(tmpRefs[j]));
    refIds.getRef(refs[i], tmpRefs.data(), tmpRefs.size());
  }

  nameMapOfs.resize(nm.nameCount());
  for (int i = 0; i < nameMapOfs.size(); i++)
    nameMapOfs[i] = append_items(strData, strlen(nm.getName(i)) + 1, nm.getName(i));

  cwr.writeFourCC(verLabel);
  cwr.writeInt32e(0);
  cwr.writeInt32e(0);
  cwr.beginBlock();

  // prepare binary dump header (root data)
  nm_pt.reserveTab(cwr);  //  PatchableTab<int> nameMap;
  rrt_pt.reserveTab(cwr); //  PatchableTab<ResEntry> resTable;
  rd_pt.reserveTab(cwr);  //  PatchableTab<ResData> resData;

  // write name map
  for (int i = 0; i < nameMapOfs.size(); i++)
    nameMapOfs[i] += cwr.tell();
  cwr.writeTabDataRaw(strData);

  cwr.align16();
  nm_pt.startData(cwr, nameMapOfs.size());
  cwr.writeTabData32ex(nameMapOfs);
  nm_pt.finishTab(cwr);

  // write real-res table
  rrdHdrOfs.resize(changedIdx.size());
  rrt_pt.startData(cwr, changedIdx.size());
  for (int i = 0; i < changedIdx.size(); ++i)
  {
    // ResEntry:  uint32_t classId; uint32_t offset; uint16_t resId, _resv;
    cwr.writeInt32e(data->resTable[changedIdx[i]].classId);

    rrdHdrOfs[i] = cwr.tell();
    cwr.writeInt32e(0);

    cwr.writeInt16e(nm.getNameId(data->getName(data->resTable[changedIdx[i]].resId)));
    cwr.writeInt16e(0);
  }
  rrt_pt.finishTab(cwr);

  cwr.align16();
  rd_pt.reserveData(cwr, changedIdx.size(), 8 + (verLabel == _MAKE4C('GRP2') ? cwr.TAB_SZ : 8));
  rd_pt.finishTab(cwr);
  desc_sz = cwr.tell() - 16;

  cwr.writeStorage16e(refIds);
  data_sz = cwr.tell() - 16;
  cwr.align16();
  int data_pos = cwr.tell();

  cwr.seekto(rd_pt.resvDataPos);
  for (int i = 0; i < data->resData.size(); ++i)
  {
    if (rdToChangedMap[i] < 0)
      continue;

    refIds.refIndexToOffset(refs[i]);

    // ResDataV2:  uint32_t classId; uint16_t resId, _resv; PatchableTab<uint16_t> refResId;
    // ResDataV3:  uint32_t classId; uint16_t resId, refResIdCnt; PatchablePtr<uint16_t> refResIdPtr;
    cwr.writeInt32e(data->resData[i].classId);
    cwr.writeInt16e(nm.getNameId(data->getName(data->resData[i].resId)));
    if (verLabel == _MAKE4C('GRP2'))
    {
      cwr.writeInt16e(nm.getNameId(data->getName(data->resData[i].resId)));
      cwr.writeRef(refs[i]);
    }
    else
    {
      cwr.writeInt16e(refs[i].count);
      cwr.writeInt64e(refs[i].start);
    }
  }
  cwr.seekto(data_pos);

  // write real-res data
  FullFileLoadCB crd(fn);
  for (int i = 0; i < changedIdx.size(); ++i)
  {
    cwr.align16();
    int dataOfs = cwr.tell();

    crd.seekto(data->resTable[changedIdx[i]].offset);
    copy_stream_to_stream(crd, cwr.getRawWriter(), resSz[changedIdx[i]]);
    cwr.writeInt32eAt(dataOfs, rrdHdrOfs[i]);
  }

  cwr.writeInt32eAt(desc_sz, 4);
  cwr.writeInt32eAt(data_sz, 8);
  cwr.endBlock();

  return true;
}
bool GrpBinData::writeGrpHdr(mkbindump::BinDumpSaveCB &cwr, const char *grp_fname)
{
  FullFileLoadCB crd(grp_fname);
  if (!crd.fileHandle)
    return false;

  gamerespackbin::GrpHeader ghdr;
  crd.read(&ghdr, sizeof(ghdr));
  if (ghdr.label != _MAKE4C('GRP2') && ghdr.label != _MAKE4C('GRP3'))
    return false;

  if (ghdr.restFileSize + sizeof(ghdr) != df_length(crd.fileHandle))
    return false;

  cwr.writeRaw(&ghdr, sizeof(ghdr));
  copy_stream_to_stream(crd, cwr.getRawWriter(), ghdr.fullDataSize);
  return true;
}

bool DxpBinData::buildPatch(mkbindump::BinDumpSaveCB &cwr)
{
  if (!changedIdx.size())
    return false;
  if (changedIdx.size() == data->texNames.map.size())
  {
    // all textures changed, copy whole file
    FullFileLoadCB crd(fn);
    copy_stream_to_stream(crd, cwr.getRawWriter(), df_length(crd.fileHandle));
    return true;
  }

  SmallTab<Rec, TmpmemAlloc> rec;
  mkbindump::StrCollector texName;
  for (int i = 0; i < changedIdx.size(); i++)
    texName.addString(data->texNames.map[changedIdx[i]]);

  clear_and_resize(rec, changedIdx.size());
  mem_set_0(rec);

  mkbindump::RoNameMapBuilder b;
  mkbindump::PatchTabRef pt_ddsx, pt_rec;

  b.prepare(texName);

  cwr.writeFourCC(_MAKE4C('DxP2'));
  cwr.writeInt32e(ver);
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
  pt_ddsx.startData(cwr, changedIdx.size());
  for (int i = 0; i < changedIdx.size(); i++)
    cwr.writeRaw(&data->texHdr[changedIdx[i]], sizeof(ddsx::Header));
  pt_ddsx.finishTab(cwr);

  pt_rec.reserveData(cwr, changedIdx.size(), ver == 3 ? sizeof(Rec) : sizeof(Rec) + 12);
  pt_rec.finishTab(cwr);
  cwr.popOrigin();
  cwr.endBlock();

  FullFileLoadCB crd(fn);
  for (int i = 0; i < changedIdx.size(); i++)
  {
    cwr.align8();
    rec[i].ofs = cwr.tell();
    rec[i].texId = ver == 3 ? 0 : -1;
    rec[i].packedDataSize = data->texRec[changedIdx[i]].packedDataSize;
    crd.seekto(data->texRec[changedIdx[i]].ofs);
    copy_stream_to_stream(crd, cwr.getRawWriter(), rec[i].packedDataSize);
  }

  int wrpos = cwr.tell();
  cwr.seekto(pt_rec.resvDataPos + 16);
  if (ver == 3)
    cwr.writeTabData32ex(rec);
  else // if (ver == 2)
    for (auto &r : rec)
    {
      cwr.writeInt64e(0);
      cwr.write32ex(&r, sizeof(r));
      cwr.writeInt32e(0);
    }
  cwr.seekto(wrpos);

  return true;
}
bool DxpBinData::writeDxpHdr(mkbindump::BinDumpSaveCB &cwr, const char *dxp_fname)
{
  FullFileLoadCB crd(dxp_fname);
  if (!crd.fileHandle)
    return false;

  unsigned hdr[4];
  crd.read(hdr, sizeof(hdr));
  cwr.writeRaw(hdr, sizeof(hdr));
  copy_stream_to_stream(crd, cwr.getRawWriter(), hdr[3]);
  return true;
}
