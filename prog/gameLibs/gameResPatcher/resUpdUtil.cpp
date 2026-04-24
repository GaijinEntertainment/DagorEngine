// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resUpdUtil.h"
#include "resFormats.h"
#if !_TARGET_64BIT
#include "old_dump.h"
#endif
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_memIo.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_string.h>
#include <util/dag_texMetaData.h>
#include <util/dag_finally.h>
#include <debug/dag_debug.h>
#include <memory/dag_framemem.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/string.h>
#include <stdio.h>
#include <stddef.h>

#if _TARGET_PC_LINUX
#include <unistd.h>
#endif //_TARGET_PC_LINUX

int patch_update_game_resources_verbose = 0;
int patch_update_game_resources_file_open_mode = DF_READ;
PatchGameResourcesStrategy patch_update_game_resources_strategy = PatchGameResourcesStrategy::Full;

#if !_TARGET_64BIT
static bool transcode_old_vromfs_dump(Tab<char> &dest, const char *fname)
{
  struct VirtualRomFsDataHdr
  {
    unsigned label, target, fullSz, packedSz;
  };

  VirtualRomFsDataHdr hdr;
  VirtualRomFsData32 *fs = NULL;
  void *buf = NULL;
  file_ptr_t fp = df_open(fname, DF_READ);

  if (!fp || df_read(fp, &hdr, sizeof(hdr)) != sizeof(hdr))
    goto load_fail;

  if (hdr.label != _MAKE4C('VRFS'))
    goto load_fail;

  fs = (VirtualRomFsData32 *)tmpmem->tryAlloc(hdr.fullSz);
  if (!fs)
    goto load_fail;

  if (hdr.packedSz)
  {
    /*
        buf = tmpmem->tryAlloc(hdr.packedSz);
        if (!buf)
          goto load_fail;
        if (df_read(fp, buf, hdr.packedSz) != hdr.packedSz)
          goto load_fail;

        unsigned long sz = hdr.fullSz;
        int err = uncompress((unsigned char*)fs, &sz, (unsigned char*)buf, hdr.packedSz);
        G_ASSERT(err==Z_OK);
        if (err == Z_OK)
          hdr.fullSz = sz;
        else
    */
    goto load_fail;
  }
  else if (df_read(fp, fs, hdr.fullSz) != hdr.fullSz)
    goto load_fail;

  if (buf)
    tmpmem->free(buf);
  df_close(fp);

  fs->files.patchData(fs);
  fs->data.patch(fs);
  for (int i = 0, ie = fs->data.size(); i < ie; i++)
    fs->data[i].patch(fs);

  {
    MemorySaveCB cwr(hdr.fullSz + 8 + 8 + 4 * fs->files.map.size() + 8 * fs->data.size() + (16 << 10));
    cwr.writeInt(_MAKE4C('VRFs'));
    cwr.writeInt(hdr.target);
    cwr.writeInt(hdr.fullSz);
    cwr.writeInt(0);

    int next_ofs = sizeof(VirtualRomFsData) - offsetof(VirtualRomFsData, files);
    cwr.writeInt(next_ofs);
    cwr.writeInt(fs->files.map.size());
    cwr.writeInt(0);
    cwr.writeInt(0);

    next_ofs += sizeof(PatchablePtr<const char>) * fs->files.map.size();
    cwr.writeInt(next_ofs);
    cwr.writeInt(fs->data.size());
    cwr.writeInt(0);
    cwr.writeInt(0);

    int pos0 = cwr.tell();
    write_zeros(cwr, sizeof(PatchablePtr<const char>) * fs->files.map.size());
    write_zeros(cwr, sizeof(PatchableTab<const char>) * fs->data.size());

    for (int i = 0; i < fs->files.map.size(); i++)
    {
      int pos2 = cwr.tell();
      cwr.seekto(pos0);
      cwr.writeInt(pos2 - sizeof(VirtualRomFsDataHdr));
      cwr.seekto(pos2);
      cwr.write(fs->files.map[i], strlen(fs->files.map[i]) + 1);
      pos0 += sizeof(PatchablePtr<const char>);
    }
    for (int i = 0; i < fs->data.size(); i++)
    {
      if (!fs->data[i].size())
        continue;
      int pos2 = cwr.tell();
      cwr.seekto(pos0);
      cwr.writeInt(pos2 - sizeof(VirtualRomFsDataHdr));
      cwr.writeInt(fs->data[i].size());
      cwr.seekto(pos2);
      cwr.write(fs->data[i].data(), data_size(fs->data[i]));
      pos0 += sizeof(PatchableTab<const char>);
    }
    pos0 = cwr.tell();
    cwr.seekto(8);
    cwr.writeInt(pos0 - sizeof(VirtualRomFsDataHdr));
    cwr.seekto(pos0);

    memfree(fs, tmpmem);
    dest.resize(cwr.getSize());
    char *p = dest.data();
    MemoryChainedData *mem = cwr.getMem();
    while (mem)
    {
      if (!mem->used)
        break;
      G_ASSERT(p < dest.data() + dest.size());
      memcpy(p, mem->data, mem->used);
      p += mem->used;
      mem = mem->next;
    }
  }
  return true;

load_fail:
  if (fs)
    memfree(fs, tmpmem);
  if (fp)
    df_close(fp);
  if (buf)
    tmpmem->free(buf);
  return false;
}
#endif

static bool safe_copy_file_to_file(file_ptr_t from, file_ptr_t to, int size)
{
  char buf[32768];
  while (size > 0)
  {
    int len = size > sizeof(buf) ? sizeof(buf) : size;
    size -= len;
    if (df_read(from, buf, len) != len)
      return false;
    if (df_write(to, buf, len) != len)
      return false;
  }
  return true;
}

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

#if !_TARGET_64BIT
static bool load_dxp_old(DxpBinData &dest, IGenLoad &crd, int file_size)
{
  unsigned hdr[4];
  crd.read(hdr, sizeof(hdr));

  if (hdr[0] != _MAKE4C('DxP2') || hdr[1] != 1)
    return false;

  DxpDump32 *tdata = (DxpDump32 *)memalloc(hdr[3], tmpmem);
  crd.read(tdata, hdr[3]);

  dest.oldDump32 = tdata;
  dest.data = (DxpBinData::Dump *)memalloc(sizeof(DxpBinData::Dump), tmpmem);
  memset(dest.data, 0, sizeof(DxpBinData::Dump));

  dest.fn = crd.getTargetName();
  if (dest.fn.size() > 9 && strcmp(dest.fn.end(9), "cache.bin") == 0)
    erase_items(dest.fn, dest.fn.size() - 10, 9);
  dd_simplify_fname_c(dest.fn);
  dest.fn.resize(strlen(dest.fn) + 1);

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

  if (file_size > 0)
  {
    for (int i = 0; i < tdata->texNames.map.size(); i++)
      if (tdata->texRec[i].ofs < hdr[3] || tdata->texRec[i].packedDataSize < 0 ||
          tdata->texRec[i].ofs + tdata->texRec[i].packedDataSize > file_size)
      {
        logwarn("%s: corrupted tex %d <%s>  ofs=%d sz=%d", dest.fn.str(), i, tdata->texNames.map[i].get(), tdata->texRec[i].ofs,
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

  dest.oldMap.resize(tdata->texNames.map.size());
  mem_set_0(dest.oldMap);
  dest.oldRec.resize(tdata->texNames.map.size());
  mem_set_0(dest.oldRec);
  dest.data->texNames.map.init(dest.oldMap.data(), dest.oldMap.size());
  dest.data->texHdr.init(tdata->texHdr.data(), tdata->texHdr.size());
  dest.data->texRec.init(dest.oldRec.data(), dest.oldRec.size());
  for (int i = 0; i < tdata->texNames.map.size(); i++)
  {
    dest.oldMap[i] = tdata->texNames.map[i];
    dest.oldRec[i].ofs = tdata->texRec[i].ofs;
    dest.oldRec[i].packedDataSize = tdata->texRec[i].packedDataSize;
  }
  return true;
}
#endif

static bool load_dxp(DxpBinData &dest, IGenLoad &crd, int file_size, bool old_file)
{
  unsigned hdr[4];
  crd.read(hdr, sizeof(hdr));

#if !_TARGET_64BIT
  if (hdr[0] == _MAKE4C('DxP2') && hdr[1] == 1 && old_file)
  {
    crd.seekrel(-(int)sizeof(hdr));
    return load_dxp_old(dest, crd, file_size);
  }
#else
  G_UNUSED(old_file);
#endif
  if (hdr[0] != _MAKE4C('DxP2') || (hdr[1] != 2 && hdr[1] != 3))
    return false;

  DxpBinData::Dump *tdata = (DxpBinData::Dump *)memalloc(hdr[3], tmpmem);
  crd.read(tdata, hdr[3]);

  dest.data = tdata;
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
  if (hdr[1] == 2)
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


static bool load_packs(const char *pack_list_blk_fname, const char *res_base_dir, const char *save_dir, Tab<GrpBinData> &grp,
  Tab<DxpBinData> &dxp, Tab<String> *grp_save, Tab<String> *dxp_save, bool c, IGameResPatchProgressBar *pbar, bool old_pack)
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
  int skipped_grp = 0, skipped_dxp = 0;

  int nid = blk.getNameId("pack");
  b = blk.getBlockByNameEx("ddsxTexPacks");
  for (int i = 0; i < b->paramCount(); i++)
    if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
    {
      if (pbar && pbar->isCancelled())
        return false;

      const char *fname = b->getStr(i);
      _snprintf(tempBuf, sizeof(tempBuf), "%s%s", buf, fname);
      tempBuf[sizeof(tempBuf) - 1] = 0;

      if (pbar && pbar->ignoreResFile(tempBuf))
      {
        debug("skipped %s", tempBuf);
        skipped_dxp++;
        continue;
      }

      if (c)
        strcat(tempBuf, "cache.bin");

      FullFileLoadCB crd(tempBuf, patch_update_game_resources_file_open_mode);
      if (!crd.fileHandle)
      {
        if (patch_update_game_resources_strategy == PatchGameResourcesStrategy::Full)
          logerr("failed to open %s", tempBuf);
        else if (patch_update_game_resources_verbose)
          debug("skipping old pack (not on disk): %s", tempBuf);
        continue;
      }

      if (load_dxp(dxp.push_back(), crd, c ? -1 : df_length(crd.fileHandle), old_pack))
      {
        if (dxp_save)
          dxp_save->push_back().setStrCat3(save_dir, "/", fname);
      }
      else
        dxp.pop_back();
    }

  b = blk.getBlockByNameEx("gameResPacks");
  for (int i = 0; i < b->paramCount(); i++)
    if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
    {
      if (pbar && pbar->isCancelled())
        return false;

      const char *fname = b->getStr(i);
      _snprintf(tempBuf, sizeof(tempBuf), "%s%s", buf, fname);
      tempBuf[sizeof(tempBuf) - 1] = 0;

      if (pbar && pbar->ignoreResFile(tempBuf))
      {
        debug("skipped %s", tempBuf);
        skipped_grp++;
        continue;
      }

      if (c)
        strcat(tempBuf, "cache.bin");

      FullFileLoadCB crd(tempBuf, patch_update_game_resources_file_open_mode);
      if (!crd.fileHandle)
      {
        if (patch_update_game_resources_strategy == PatchGameResourcesStrategy::Full)
          logerr("failed to open %s", tempBuf);
        else if (patch_update_game_resources_verbose)
          debug("skipping old pack (not on disk): %s", tempBuf);
        continue;
      }

      if (load_grp(grp.push_back(), crd, c ? -1 : df_length(crd.fileHandle)))
      {
        if (grp_save)
          grp_save->push_back().setStrCat3(save_dir, "/", fname);
      }
      else
        grp.pop_back();
    }
  debug("%s: loaded %d grp(s) and %d dxp(s) from %s; skipped %d grp and %d dxp", old_pack ? "OLD" : "NEW", grp.size(), dxp.size(),
    pack_list_blk_fname, skipped_grp, skipped_dxp);
  return true;
}

template <class V, typename T = typename V::value_type>
static int find_the_same_pack(const V &p, const char *fn)
{
  for (int i = 0; i < p.size(); i++)
    if (dd_stricmp(p[i].fn, fn) == 0)
      return i;
  return -1;
}

bool GrpBinData::findRes(dag::Span<GrpBinData> grp, int pair_idx, const char *res_name, int psz, int &old_idx, int &old_rec_idx)
{
  if (pair_idx >= 0)
  {
    grp[pair_idx].buildResMap();
    old_rec_idx = grp[pair_idx].resMap.getStrId(res_name);
    if (old_rec_idx >= 0)
    {
      old_idx = pair_idx;
      if (patch_update_game_resources_verbose && grp[old_idx].resSz[old_rec_idx] != psz)
        debug("res %-40s differs: %7d != %7d, found at %s:%d (%s)", res_name, psz, grp[old_idx].resSz[old_rec_idx],
          grp[old_idx].fn.str(), old_rec_idx, grp[old_idx].data->getName(grp[old_idx].data->resTable[old_rec_idx].resId));
      return grp[old_idx].resSz[old_rec_idx] == psz;
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
      if (patch_update_game_resources_verbose && grp[old_idx].resSz[old_rec_idx] != psz)
        debug("res %-40s differs: %7d != %7d, found at %s:%d (%s)", res_name, psz, grp[old_idx].resSz[old_rec_idx],
          grp[old_idx].fn.str(), old_rec_idx, grp[old_idx].data->getName(grp[old_idx].data->resTable[old_rec_idx].resId));
      debug("moved res: %s  sz=%d", res_name, psz);
      return grp[old_idx].resSz[old_rec_idx] == psz;
    }
  }
  debug("new res: %s  sz=%d", res_name, psz);
  return false;
}
bool DxpBinData::findTex(dag::ConstSpan<DxpBinData> dxp, int pair_idx, const char *tex_name, const ddsx::Header &hdr, int psz,
  int &old_idx, int &old_rec_idx)
{
  int name_len = (int)strlen(tex_name);
  if (pair_idx >= 0)
  {
    old_rec_idx = DxpBinData::resolveNameId(dxp[pair_idx].data->texNames.map, tex_name, name_len);
    if (old_rec_idx >= 0)
    {
      old_idx = pair_idx;
      goto compare_and_return;
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
      debug("moved tex: %s  sz=%d", tex_name, psz);
      goto compare_and_return;
    }
  }
  debug("new tex: %s  sz=%d", tex_name, psz);
  return false;

compare_and_return:
  if (patch_update_game_resources_verbose && dxp[old_idx].data->texRec[old_rec_idx].packedDataSize != psz)
    debug("tex %-40s differs: %7d != %7d, found at %s:%d (%s)", tex_name, psz, dxp[old_idx].data->texRec[old_rec_idx].packedDataSize,
      dxp[old_idx].fn.str(), old_rec_idx, dxp[old_idx].data->texNames.map[old_rec_idx]);

  const ddsx::Header &ohdr = dxp[old_idx].data->texHdr[old_rec_idx];
  bool tex_changed = (hdr.w != ohdr.w || hdr.h != ohdr.h || hdr.d3dFormat != ohdr.d3dFormat);
  if (patch_update_game_resources_verbose && tex_changed)
    debug("tex %-40s differs: %4dx%-4x, %08X != %4dx%-4x, %08X", tex_name, hdr.w, hdr.h, hdr.d3dFormat, ohdr.w, ohdr.h,
      ohdr.d3dFormat);
  return !tex_changed;
}


int patch_update_game_resources_mem(const char *game_dir, const char *cache_dir, const void *vromfs_data, int vromfs_data_sz,
  const char *rel_mount_dir, IGameResPatchProgressBar *pbar, bool dryRun, const char *vrom_name, const char *res_blk_name)
{
  if (pbar)
    pbar->setStage(0);
  debug("patch_update_game_resources%s"
        "\n    root=<%s>"
        "\n    cache=<%s>"
        "\n    new_vfomfs_sz=%d"
        "\n    mount=<%s>"
        "\n    cache_fn=<%s> res_list_fn=<%s>\n",
    dryRun ? " [DRY RUN]" : "", game_dir, cache_dir, vromfs_data_sz, rel_mount_dir, vrom_name, res_blk_name);

  VirtualRomFsData *new_vfs = load_vromfs_dump_from_mem(dag::ConstSpan<char>((const char *)vromfs_data, vromfs_data_sz), tmpmem);
  if (!new_vfs)
  {
    logerr("failed to load new VROMFS ptr=%p sz=%d", vromfs_data, vromfs_data_sz);
    return -1;
  }

  String old_vfs_fn(0, "%s/%s/%s", game_dir, rel_mount_dir, vrom_name);
  VirtualRomFsData *old_vfs = load_vromfs_dump(old_vfs_fn, tmpmem);
#if !_TARGET_64BIT
  Tab<char> old_vfs_cvt;
  if (!old_vfs && transcode_old_vromfs_dump(old_vfs_cvt, old_vfs_fn))
    old_vfs = load_vromfs_dump_from_mem(old_vfs_cvt, tmpmem);
#endif
  if (!old_vfs)
  {
    logwarn("failed to load old VROMFS %s", old_vfs_fn.str());
    memfree(new_vfs, tmpmem);
    return 0;
  }

  Tab<GrpBinData> old_grp(tmpmem), new_grp(tmpmem);
  Tab<DxpBinData> old_dxp(tmpmem), new_dxp(tmpmem);
  Tab<FullFileLoadCB *> crd_cache(tmpmem);
  Tab<String> grpSavePath(tmpmem);
  Tab<String> dxpSavePath(tmpmem);
  Tab<char> buf(tmpmem);
  String stor;
  int64_t new_grp_total = 0, new_grp_reused = 0, new_grp_reused_ident = 0;
  int64_t new_dxp_total = 0, new_dxp_reused = 0, new_dxp_reused_ident = 0;
  int64_t total_sz, done_sz;

  bool useCache = (cache_dir && *cache_dir);
  String mnt(0, "%s/", game_dir);
  String mnt_dir(0, "%s/%s", game_dir, rel_mount_dir);
  String save_dir(0, "%s/%s", useCache ? cache_dir : game_dir, rel_mount_dir);
  String res_list_fn(0, "%s/%s", mnt_dir.str(), res_blk_name);
  const char *eff_mnt_dir = strcmp(rel_mount_dir, ".") == 0 ? nullptr : mnt_dir.c_str();
  String eff_mnt = mnt;
  if (strcmp(rel_mount_dir, "res") != 0)
    eff_mnt.aprintf(0, "%s/", rel_mount_dir);

  add_vromfs(old_vfs, true, eff_mnt);
  if (!load_packs(res_list_fn, eff_mnt_dir, NULL, old_grp, old_dxp, NULL, NULL, false, pbar, true))
  {
    logerr("can't load old packs");
    remove_vromfs(old_vfs);
    memfree(new_vfs, tmpmem);
    memfree(old_vfs, tmpmem);
    return -1;
  }
  remove_vromfs(old_vfs);

  add_vromfs(new_vfs, true, eff_mnt);
  if (!load_packs(res_list_fn, eff_mnt_dir, save_dir.str(), new_grp, new_dxp, &grpSavePath, &dxpSavePath, true, pbar, false))
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

  if (pbar)
    pbar->setStage(1);
  total_sz = 0, done_sz = 0;
  for (int i = 0; i < new_grp.size(); i++)
    total_sz += new_grp[i].fullSz;

  crd_cache.resize(old_grp.size());
  mem_set_0(crd_cache);

  for (int i = 0; i < new_grp.size(); i++)
  {
    if (pbar && pbar->isCancelled())
    {
      clear_all_ptr_items(crd_cache);
      return -1;
    }

    FullFileLoadCB crd(new_grp[i].fn, patch_update_game_resources_file_open_mode);
    String tmp_fn(0, "%s.1", new_grp[i].fn.str());
    // debug("%s %p (%d, %d)", crd.getTargetName(), crd.fileHandle, data_size(new_grp[i].hdrData), new_grp[i].fullSz);
    if (!crd.fileHandle && patch_update_game_resources_strategy == PatchGameResourcesStrategy::PatchOnlyExisting)
    {
      if (patch_update_game_resources_verbose)
        debug("skipping grp %s (not on disk)", new_grp[i].fn.str());

      done_sz += new_grp[i].fullSz;
      if (pbar && total_sz > 0)
        pbar->setPromilleDone(done_sz * 1000 / total_sz);
      continue;
    }
    new_grp_total += new_grp[i].fullSz;
    if (crd.fileHandle)
    {
      buf.resize(data_size(new_grp[i].hdrData));
      if (crd.tryRead(buf.data(), data_size(buf)) != data_size(buf))
      {
        debug("damaged %s", crd.getTargetName());
        crd.close();
      }
      else if (mem_eq(buf, new_grp[i].hdrData.data()))
      {
        debug("skipping identical %s", crd.getTargetName());
        dd_erase(tmp_fn);
        new_grp_reused_ident += new_grp[i].fullSz;
        done_sz += new_grp[i].fullSz;
        if (pbar)
          pbar->setPromilleDone(done_sz * 1000 / total_sz);
        continue;
      }
    }
    crd.close();

    int pair_idx = find_the_same_pack(old_grp, new_grp[i].fn);

    FullFileSaveCB cwr;
    if (!dryRun)
    {
      cwr.open(tmp_fn, DF_WRITE | DF_CREATE);
      if (!cwr.fileHandle)
      {
        logerr("failed to create temp: %s", tmp_fn.str());
        continue;
      }
      cwr.seekto(new_grp[i].fullSz - 4);
      cwr.writeInt(0);
      cwr.seekto(0);

      cwr.write(new_grp[i].hdrData.data(), data_size(new_grp[i].hdrData));
    }
    new_grp_reused += data_size(new_grp[i].hdrData);
    done_sz += data_size(new_grp[i].hdrData);

    for (int j = 0; j < new_grp[i].data->resTable.size(); j++)
    {
      const char *res_name = new_grp[i].data->getName(new_grp[i].data->resTable[j].resId);
      int psz = new_grp[i].resSz[j];

      if (pbar && pbar->isCancelled())
      {
        if (!dryRun)
        {
          cwr.close();
          dd_erase(tmp_fn);
        }
        clear_all_ptr_items(crd_cache);
        return -1;
      }

      int old_idx = -1, old_rec_idx = -1;
      if (GrpBinData::findRes(make_span(old_grp), pair_idx, res_name, psz, old_idx, old_rec_idx))
      {
        if (!crd_cache[old_idx])
        {
          crd_cache[old_idx] = new FullFileLoadCB(old_grp[old_idx].fn);
          if (!crd_cache[old_idx]->fileHandle)
          {
            del_it(crd_cache[old_idx]);
            continue;
          }
          // debug("open: %s", old_grp[old_idx].fn.str());
        }
        bool reuse_ok = true;
        if (!dryRun)
        {
          crd_cache[old_idx]->seekto(old_grp[old_idx].data->resTable[old_rec_idx].offset);
          cwr.seekto(new_grp[i].data->resTable[j].offset);
          reuse_ok = safe_copy_file_to_file(crd_cache[old_idx]->fileHandle, cwr.fileHandle, psz);
        }
        if (reuse_ok)
          new_grp_reused += psz;
      }
      done_sz += psz;
      if (pbar)
        pbar->setPromilleDone(done_sz * 1000 / total_sz);
    }
    if (pair_idx >= 0)
    {
      delete crd_cache[pair_idx];
      erase_items(crd_cache, pair_idx, 1);
      erase_items(old_grp, pair_idx, 1);
    }
    if (!dryRun)
    {
      cwr.close();
      dd_erase(grpSavePath[i].str());
      rename(tmp_fn, grpSavePath[i].str());
      dd_erase(tmp_fn);
      if (useCache)
        dd_erase(new_grp[i].fn.str());
    }
  }

  if (pbar)
    pbar->setStage(2);
  total_sz = 0, done_sz = 0;
  for (int i = 0; i < new_dxp.size(); i++)
    total_sz += new_dxp[i].fullSz;

  clear_all_ptr_items(crd_cache);
  crd_cache.resize(old_dxp.size());
  mem_set_0(crd_cache);
  for (int i = 0; i < new_dxp.size(); i++)
  {
    if (pbar && pbar->isCancelled())
    {
      clear_all_ptr_items(crd_cache);
      return -1;
    }

    FullFileLoadCB crd(new_dxp[i].fn, patch_update_game_resources_file_open_mode);
    String tmp_fn(0, "%s.1", new_dxp[i].fn.str());
    // debug("%s %p (%d, %d)", crd.getTargetName(), crd.fileHandle, data_size(new_dxp[i].hdrData), new_dxp[i].fullSz);
    if (!crd.fileHandle && patch_update_game_resources_strategy == PatchGameResourcesStrategy::PatchOnlyExisting)
    {
      if (patch_update_game_resources_verbose)
        debug("skipping dxp %s (not on disk)", new_dxp[i].fn.str());

      done_sz += new_dxp[i].fullSz;
      if (pbar && total_sz > 0)
        pbar->setPromilleDone(done_sz * 1000 / total_sz);
      continue;
    }
    new_dxp_total += new_dxp[i].fullSz;
    if (crd.fileHandle)
    {
      buf.resize(data_size(new_dxp[i].hdrData));
      if (crd.tryRead(buf.data(), data_size(buf)) != data_size(buf))
      {
        debug("damaged %s", crd.getTargetName());
        crd.close();
      }
      else if (mem_eq(buf, new_dxp[i].hdrData.data()))
      {
        debug("skipping identical %s", crd.getTargetName());
        dd_erase(tmp_fn);
        new_dxp_reused_ident += new_dxp[i].fullSz;
        done_sz += new_dxp[i].fullSz;
        if (pbar)
          pbar->setPromilleDone(done_sz * 1000 / total_sz);
        continue;
      }
    }
    crd.close();

    int pair_idx = find_the_same_pack(old_dxp, new_dxp[i].fn);

    FullFileSaveCB cwr;
    if (!dryRun)
    {
      cwr.open(tmp_fn, DF_WRITE | DF_CREATE);
      if (!cwr.fileHandle)
      {
        logerr("failed to create temp: %s", tmp_fn.str());
        continue;
      }
      cwr.seekto(new_dxp[i].fullSz - 4);
      cwr.writeInt(0);
      cwr.seekto(0);

      cwr.write(new_dxp[i].hdrData.data(), data_size(new_dxp[i].hdrData));
    }
    new_dxp_reused += data_size(new_dxp[i].hdrData);
    done_sz += data_size(new_dxp[i].hdrData);

    for (int j = 0; j < new_dxp[i].data->texNames.map.size(); j++)
    {
      TextureMetaData tmd;
      const char *tex_name = tmd.decode(new_dxp[i].data->texNames.map[j], &stor);
      int ofs = new_dxp[i].data->texRec[j].ofs;
      int psz = new_dxp[i].data->texRec[j].packedDataSize;
      int asz = j + 1 < new_dxp[i].data->texNames.map.size() ? (8 - ((ofs + psz) & 7) & 7) : 0;

      if (pbar && pbar->isCancelled())
      {
        if (!dryRun)
        {
          cwr.close();
          dd_erase(tmp_fn);
        }
        clear_all_ptr_items(crd_cache);
        return -1;
      }

      int old_idx = -1, old_rec_idx = -1;
      if (DxpBinData::findTex(old_dxp, pair_idx, tex_name, new_dxp[i].data->texHdr[j], psz, old_idx, old_rec_idx))
      {
        if (!crd_cache[old_idx])
        {
          crd_cache[old_idx] = new FullFileLoadCB(old_dxp[old_idx].fn);
          if (!crd_cache[old_idx]->fileHandle)
          {
            del_it(crd_cache[old_idx]);
            continue;
          }
          // debug("open: %s", old_dxp[old_idx].fn.str());
        }
        bool reuse_ok = true;
        if (!dryRun)
        {
          int osz = old_dxp[old_idx].data->texRec[old_rec_idx].packedDataSize;
          crd_cache[old_idx]->seekto(old_dxp[old_idx].data->texRec[old_rec_idx].ofs);
          cwr.seekto(ofs);
          reuse_ok = safe_copy_file_to_file(crd_cache[old_idx]->fileHandle, cwr.fileHandle, osz < psz ? osz : psz);
          write_zeros(cwr, osz < psz ? (psz + asz - osz) : asz);
          if (!reuse_ok)
          {
            int oldpos = cwr.tell();
            cwr.seekto(ofs);
            write_zeros(cwr, osz < psz ? osz : psz);
            cwr.seekto(oldpos);
          }
        }
        if (reuse_ok)
          new_dxp_reused += psz + asz;
      }
      done_sz += psz + asz;
      if (pbar)
        pbar->setPromilleDone(done_sz * 1000 / total_sz);
    }
    if (pair_idx >= 0)
    {
      delete crd_cache[pair_idx];
      erase_items(crd_cache, pair_idx, 1);
      erase_items(old_dxp, pair_idx, 1);
    }
    if (!dryRun)
    {
      cwr.close();
      dd_erase(dxpSavePath[i].str());
      rename(tmp_fn, dxpSavePath[i].str());
      dd_erase(tmp_fn);
      if (useCache)
        dd_erase(new_dxp[i].fn.str());
    }
  }

  if (pbar)
    pbar->setStage(3);

  if (!dryRun)
  {
    dd_erase(old_vfs_fn);
    FullFileSaveCB cwr(old_vfs_fn);
    cwr.write(vromfs_data, vromfs_data_sz);
  }
  clear_all_ptr_items(crd_cache);

  debug("patch_update_game_resources results:"
        "\n    GRP (total/reused/ident)  %11lld / %11lld / %11lld"
        "\n    DxP (total/reused/ident)  %11lld / %11lld / %11lld"
        "\n    Reuse    (GRP/DxP/total)  %3lld%% / %3lld%% / %3lld%%"
        "\n    Download (GRP+DxP=total)  %11lld + %11lld = %11lld"
        "\n    %s\n",
    new_grp_total, new_grp_reused, new_grp_reused_ident, new_dxp_total, new_dxp_reused, new_dxp_reused_ident,
    new_grp_total ? (new_grp_reused + new_grp_reused_ident) * 100 / new_grp_total : 100,
    new_dxp_total ? (new_dxp_reused + new_dxp_reused_ident) * 100 / new_dxp_total : 100,
    new_grp_total + new_dxp_total
      ? (new_grp_reused + new_grp_reused_ident + new_dxp_reused + new_dxp_reused_ident) * 100 / (new_grp_total + new_dxp_total)
      : 100,
    new_grp_total - new_grp_reused - new_grp_reused_ident, new_dxp_total - new_dxp_reused - new_dxp_reused_ident,
    new_grp_total - new_grp_reused - new_grp_reused_ident + new_dxp_total - new_dxp_reused - new_dxp_reused_ident,
    dryRun ? "NO DATA CHANGED" : "FILES UPDATED");
  return new_grp_total + new_dxp_total ? int((new_grp_reused + new_grp_reused_ident + new_dxp_reused + new_dxp_reused_ident) * 100 /
                                             (new_grp_total + new_dxp_total))
                                       : 100;
}

int patch_update_game_resources(const char *root_dir, const char *vromfs_fn, const char *rel_mount_dir, IGameResPatchProgressBar *pbar,
  bool dryRun, const char *vrom_name, const char *res_blk_name)
{
  Tab<char> vromfs_data(tmpmem);
  FullFileLoadCB crd(vromfs_fn);

  debug("new_vromfs_fn=%s handle=%d  sz=%d\n", vromfs_fn, crd.fileHandle, df_length(crd.fileHandle));
  if (!crd.fileHandle || df_length(crd.fileHandle) > (128 << 20))
    return -1;
  vromfs_data.resize(df_length(crd.fileHandle));
  crd.read(vromfs_data.data(), data_size(vromfs_data));
  crd.close();

  return patch_update_game_resources_mem(root_dir, NULL, vromfs_data.data(), data_size(vromfs_data), rel_mount_dir, pbar, dryRun,
    vrom_name, res_blk_name);
}

PatchGameResourcesStrategy set_patch_update_game_resources_strategy(PatchGameResourcesStrategy strategy)
{
  PatchGameResourcesStrategy wasStrategy = patch_update_game_resources_strategy;
  patch_update_game_resources_strategy = strategy;

  if (strategy == PatchGameResourcesStrategy::Full)
    patch_update_game_resources_file_open_mode = DF_READ;
  else
    patch_update_game_resources_file_open_mode = DF_READ | DF_IGNORE_MISSING;

  return wasStrategy;
}

static int align16(int v) { return (v + 15) & ~15; }

static void align16(IGenSave &cwr, int pos_offset = 0)
{
  static const char zero[16] = {};

  int rest = ((cwr.tell() + pos_offset) & 15);
  if (rest)
    cwr.write(zero, 16 - rest);
}

int patch_update_vromfs_pack(const char *old_vromfs_pack, const DataBlock &new_vromfs_index)
{
  static const int SHA1_SIZE = 20;
  static const int DATA_HEADER_SIZE = 8 * sizeof(unsigned int) + 16; // pt_names.reserveTab() + pt_data.reserveTab() + writeZeroes(16)
  static const int BIN_DUMP_PTR_SZ = 8;                              // mkbindump::BinDumpSaveCB::PTR_SZ
  static const int BIN_DUMP_TAB_SZ = 16;                             // mkbindump::BinDumpSaveCB::TAB_SZ

  if (!dd_file_exists(old_vromfs_pack))
  {
    debug("old vromfs pack doesn't exist at %s, skipping...", old_vromfs_pack);
    return 0;
  }

  VirtualRomFsPack *oldFsPack = open_vromfs_pack(old_vromfs_pack, tmpmem, DF_READ | DF_IGNORE_MISSING);
  if (!oldFsPack)
  {
    debug("failed to open old vromfs pack from %s\n", old_vromfs_pack);
    return 0;
  }

  const int newFilesCount = new_vromfs_index.paramCount();
  if (oldFsPack->files.nameCount() == newFilesCount)
  {
    bool isNothingChanged = true;
    for (int i = 0; i < newFilesCount; ++i)
    {
      const char *newFilePath = new_vromfs_index.getParamName(i);
      const int newFileSize = new_vromfs_index.getInt(i);

      const char *oldFilePath = oldFsPack->files.map[i].get();
      const int oldFileSize = data_size(oldFsPack->data[i]);

      if ((newFileSize != 0 && newFileSize != oldFileSize) || strcmp(newFilePath, oldFilePath) != 0)
      {
        isNothingChanged = false;
        break;
      }
    }

    if (isNothingChanged)
    {
      close_vromfs_pack(oldFsPack, tmpmem);
      debug("nothing changed in vromfs pack, skipping update\n");
      return 0;
    }
  }

  add_vromfs(oldFsPack);

  int newNamesSize = 0;
  for (int i = 0; i < newFilesCount; ++i)
    newNamesSize += strlen(new_vromfs_index.getParamName(i)) + 1;

  const int newHeaderSize = align16(align16(align16(DATA_HEADER_SIZE + newFilesCount * BIN_DUMP_PTR_SZ) + newNamesSize) +
                                    newFilesCount * (BIN_DUMP_TAB_SZ + SHA1_SIZE));
  const int vromfsHeaderSize = sizeof(VirtualRomFsDataHdr) + sizeof(VirtualRomFsExtHdr);
  const int newFirstFileOffset = vromfsHeaderSize + newHeaderSize;

  String tmpFile(0, "%s.tmp", old_vromfs_pack);

  FullFileSaveCB cwr;
  cwr.open(tmpFile, DF_WRITE | DF_CREATE);

  if (!cwr.fileHandle)
  {
    logwarn("failed to create tmp vromfs pack at %s\n", tmpFile);
    remove_vromfs(oldFsPack);
    close_vromfs_pack(oldFsPack, tmpmem);
    return -1;
  }

  write_zeros(cwr, newFirstFileOffset);

  int resSameFilesCount = 0;
  int resNewFilesCount = 0;
  int resChangedFilesCount = 0;
  int resUniqueFilesCount = 0;

  for (int i = 0; i < newFilesCount; ++i)
  {
    const char *newFilePath = new_vromfs_index.getParamName(i);
    const int newFileSize = new_vromfs_index.getInt(i);

    if (newFileSize == 0)
    {
      if (patch_update_game_resources_verbose)
        debug("duplicate file: %s\n", newFilePath);
      continue;
    }

    ++resUniqueFilesCount;

    if (file_ptr_t fp = df_open(newFilePath, DF_READ | DF_IGNORE_MISSING | DF_VROM_ONLY))
    {
      int oldFileSize = -1;
      if (const void *oldFileData = df_mmap(fp, &oldFileSize))
      {
        if (oldFileSize == newFileSize)
        {
          ++resSameFilesCount;
          cwr.write(oldFileData, oldFileSize);
        }
        else
        {
          if (patch_update_game_resources_verbose)
            debug("size mismatch for %s: old size %d vs new size %d\n", newFilePath, oldFileSize, newFileSize);

          ++resChangedFilesCount;
          write_zeros(cwr, newFileSize);
        }

        df_unmap(oldFileData, oldFileSize);
      }

      df_close(fp);
    }
    else
    {
      ++resNewFilesCount;

      if (patch_update_game_resources_verbose)
        debug("new file: %s\n", newFilePath);

      write_zeros(cwr, newFileSize);
    }

    align16(cwr, -vromfsHeaderSize);
  }

  cwr.close();

  remove_vromfs(oldFsPack);

  close_vromfs_pack(oldFsPack, tmpmem);

  if (!dd_erase(old_vromfs_pack))
  {
    logwarn("failed to remove old vromfs pack: %s", old_vromfs_pack);
    return -1;
  }

  if (!dd_rename(tmpFile, old_vromfs_pack))
  {
    logwarn("failed to replace old vromfs pack with new one: %s", old_vromfs_pack);
    return -1;
  }

  debug("patch_update_vromfs_pack(%s) results: new files=%d, same files=%d, changed files=%d, unique files=%d\n", old_vromfs_pack,
    resNewFilesCount, resSameFilesCount, resChangedFilesCount, resUniqueFilesCount);

  return 1;
}


struct SoundBankData
{
  struct Header
  {
    char riffMagic[4];
    int dataSize;
    char fevMagic[4];
  };


  struct SNDHeader
  {
    char tag[4];
    int dataSize;
  };


  struct SNDData
  {
    SNDHeader hdr;

    int offset;
    int fullSize;
  };


  struct FSB5Header
  {
    char tag[4];
    char version[4];

    int samplesCount;
    int sampleHeaderSize;
    int nameTableSize;
    int audioDataSize;

    char reserved[36];
  };


  struct FSB5Data
  {
    FSB5Header hdr;

    int offset = 0;
    int fullSize = 0;

    int dataBegin = 0;
    int dataEnd = 0;

    eastl::vector<int> sampleOffsets;
    eastl::vector<eastl::string> sampleNames;


    FSB5Data() = default;


    FSB5Data(int snd_offset, IGenLoad &crd, int snd_data_end) { load(snd_offset, crd, snd_data_end); }


    void clear()
    {
      offset = 0;
      fullSize = 0;
      dataBegin = 0;
      dataEnd = 0;
      sampleOffsets.clear();
      sampleNames.clear();
    }


    void load(int snd_offset, IGenLoad &crd, int snd_data_end)
    {
      offset = crd.tell();

      char buffer[256];
      const int bytesRead = crd.tryRead(buffer, sizeof(buffer) - 1);
      buffer[bytesRead] = 0;

      bool found = false;
      for (int i = 0; i < bytesRead - 4; i++)
        if (memcmp(buffer + i, "FSB5", 4) == 0)
        {
          found = true;
          offset += i;
          break;
        }

      if (!found)
      {
        clear();
        return;
      }

      if (offset + (int)sizeof(FSB5Header) > snd_data_end)
      {
        logerr("ERR: not enough data for FSB5 header at offset %d in %s", offset, crd.getTargetName());
        clear();
        return;
      }

      crd.seekto(offset);
      crd.read(&hdr, sizeof(FSB5Header));

      if (
        hdr.samplesCount < 0 || hdr.samplesCount > 65536 || hdr.sampleHeaderSize < 0 || hdr.nameTableSize < 0 || hdr.audioDataSize < 0)
      {
        logerr("ERR: invalid FSB5 header values (samples=%d, sampleHdrSz=%d, nameTblSz=%d, audioDataSz=%d) at offset %d in %s",
          hdr.samplesCount, hdr.sampleHeaderSize, hdr.nameTableSize, hdr.audioDataSize, offset, crd.getTargetName());
        clear();
        return;
      }

      fullSize = sizeof(FSB5Header) + hdr.sampleHeaderSize + hdr.nameTableSize;

      if (offset + fullSize > snd_data_end)
      {
        logerr("ERR: FSB5 data (size=%d) exceeds SND chunk boundary at offset %d in %s", fullSize, offset, crd.getTargetName());
        clear();
        return;
      }

      dataBegin = snd_offset + sizeof(SNDHeader);
      dataEnd = offset + fullSize;

      sampleOffsets.reserve(hdr.samplesCount);
      sampleNames.reserve(hdr.samplesCount);

      const int sampleHeadersOffset = offset + sizeof(FSB5Header);
      const int sampleHeadersEnd = sampleHeadersOffset + hdr.sampleHeaderSize;

      struct FSB5SampleHeader
      {
        uint64_t hasExtra : 1;
        uint64_t freq : 4;
        uint64_t channels : 2;
        uint64_t dataOffset : 28;
        uint64_t numSamples : 29;
      };

      struct FSB5MetadataChunk
      {
        uint32_t hasMore : 1;
        uint32_t size : 24;
        uint32_t type : 7;
      };

      crd.seekto(offset + sizeof(FSB5Header));
      for (int i = 0; i < hdr.samplesCount; i++)
      {
        if (crd.tell() + (int)sizeof(FSB5SampleHeader) > sampleHeadersEnd)
        {
          logerr("ERR: not enough data for FSB5 sample header %d at offset %d in %s", i, crd.tell(), crd.getTargetName());
          clear();
          return;
        }

        FSB5SampleHeader sample;
        crd.read(&sample, sizeof(sample));

        sampleOffsets.push_back(sample.dataOffset * 32); // FSB5 stores offsets in 32-byte units

        for (bool hasExtra = sample.hasExtra; hasExtra && crd.tell() + sizeof(FSB5MetadataChunk) <= sampleHeadersEnd;)
        {
          FSB5MetadataChunk chunk;
          crd.read(&chunk, sizeof(chunk));
          hasExtra = chunk.hasMore;
          crd.seekrel(chunk.size);
        }
      }

      if (hdr.nameTableSize == 0)
        return;

      const int nameTableOffset = offset + sizeof(FSB5Header) + hdr.sampleHeaderSize;
      crd.seekto(nameTableOffset);
      for (int i = 0; i < hdr.samplesCount; i++)
      {
        if (crd.tell() + (int)sizeof(int) > nameTableOffset + hdr.nameTableSize)
        {
          logerr("ERR: not enough data for FSB5 name offset %d in %s", i, crd.getTargetName());
          clear();
          break;
        }

        const int nameOffset = crd.readInt();
        const int restorePos = crd.tell();

        if (nameOffset < 0 || nameTableOffset + nameOffset >= snd_data_end)
        {
          logerr("ERR: invalid FSB5 name offset %d for sample %d at offset %d in %s", nameOffset, i, offset, crd.getTargetName());
          clear();
          break;
        }

        crd.seekto(nameTableOffset + nameOffset);

        int len = 0;
        for (int ch = crd.readIntP<1>(); ch != 0 && len < 4096; ch = crd.readIntP<1>(), ++len)
        {
          if (crd.tell() + 1 >= crd.getTargetDataSize())
          {
            logerr("ERR: not enough data while reading FSB5 sample name for sample %d at offset %d in %s", i, offset,
              crd.getTargetName());
            clear();
            return;
          }
        }

        if (len >= 4096)
        {
          logerr("ERR: invalid sample name length %d for sample %d in FSB5 chunk at offset %d\n", len, i, offset);
          clear();
          break;
        }

        crd.seekrel(-len - 1);

        eastl::vector<char, framemem_allocator> name;
        name.resize(len + 1);
        crd.read(name.data(), len + 1);

        sampleNames.emplace_back(name.data(), name.size() - 1);

        crd.seekto(restorePos);
      }
    }
  };


  Header hdr;

  eastl::vector<SNDData> sndChunks;
  eastl::vector<FSB5Data> fsb5Chunks;


  bool load(IGenLoad &crd)
  {
    crd.seekto(0);

    if (crd.tryRead(&hdr, sizeof(Header)) != sizeof(Header))
    {
      logerr("ERR: failed to read bank file %s\n", crd.getTargetName());
      return false;
    }

    if (memcmp(hdr.riffMagic, "RIFF", 4) != 0 || memcmp(hdr.fevMagic, "FEV ", 4) != 0)
    {
      logerr("ERR: file %s is not a valid bank file (missing RIFF or FEV magic)\n", crd.getTargetName());
      return false;
    }

    if (sizeof(SNDHeader) + hdr.dataSize > crd.getTargetDataSize())
    {
      logerr("ERR: data size in header exceeds file size for bank file %s\n", crd.getTargetName());
      return false;
    }

    crd.seekto(sizeof(Header));

    const int dataEnd = sizeof(SNDHeader) + hdr.dataSize;
    while (crd.tell() + sizeof(SNDHeader) <= dataEnd)
    {
      SNDData snd;
      snd.offset = crd.tell();

      crd.read(&snd.hdr, sizeof(SNDHeader));

      snd.fullSize = sizeof(SNDHeader) + snd.hdr.dataSize;

      if (snd.hdr.dataSize < 0 || crd.tell() + snd.hdr.dataSize > dataEnd)
      {
        logerr("ERR: invalid chunk data size %d at offset %d in bank file %s", snd.hdr.dataSize, snd.offset, crd.getTargetName());
        return false;
      }

      if (memcmp(snd.hdr.tag, "SND ", sizeof(snd.hdr.tag)) == 0)
      {
        sndChunks.push_back(snd);

        const int oldPos = crd.tell();
        fsb5Chunks.emplace_back(snd.offset, crd, snd.offset + snd.fullSize);

        if (fsb5Chunks.back().fullSize == 0)
        {
          logerr("ERR: failed to parse FSB5 data in SND chunk at offset %d in bank file %s", snd.offset, crd.getTargetName());
          return false;
        }

        crd.seekto(oldPos);
      }

      crd.seekrel(snd.hdr.dataSize);
      crd.seekrel(crd.tell() & 1);
    }

    if (sndChunks.empty())
    {
      logerr("ERR: failed to find SND chunk in bank file %s\n", crd.getTargetName());
      return false;
    }

    return true;
  }


  int getHeaderSize() const { return !sndChunks.empty() ? sndChunks[0].offset : 0; }
};


struct SoundIndexData
{
  struct Header
  {
    int indexHeaderSize;
    int uncompressedDataSize;
    int compressedDataSize;
    int newBankSize;
    int blobsCount;
  };

  struct BlobData
  {
    int sndOffset;
    int sndSize;

    int offset;
    int size;
  };

  Header hdr;

  eastl::string bankName;

  eastl::vector<BlobData> blobs;
  eastl::vector<char> data;


  bool load(IGenLoad &crd)
  {
    FINALLY([&] { crd.endBlock(); });

    if (crd.beginTaggedBlock() != _MAKE4C('SNDI'))
    {
      logerr("file is not a valid sound index (missing SNDI block)");
      return false;
    }

    const int indexBegin = crd.tell();

    if (crd.tryRead(&hdr, sizeof(hdr)) != sizeof(hdr))
    {
      logerr("file is not a valid sound index");
      return false;
    }

    for (int i = 0; i < hdr.blobsCount; i++)
    {
      BlobData &blob = blobs.emplace_back();
      if (crd.tryRead(&blob, sizeof(blob)) != sizeof(blob))
      {
        logerr("failed to read blob data for index file");
        return false;
      }
    }

    crd.readString(bankName);

    if (crd.tell() - indexBegin != hdr.indexHeaderSize)
    {
      logerr("index header size mismatch for index file: %d read, %d expected", crd.tell() - indexBegin, hdr.indexHeaderSize);
      return false;
    }

    data.resize(hdr.uncompressedDataSize);

    if (crd.tell() + hdr.compressedDataSize > crd.getTargetDataSize())
    {
      logerr("compressed data size exceeds file size for index file");
      return false;
    }

    ZstdLoadCB zrd(crd, hdr.compressedDataSize);
    if (zrd.tryRead(data.data(), data.size()) != data.size())
    {
      logerr("failed to read compressed data for index file");
      return false;
    }

    return !blobs.empty() && !data.empty();
  }


  int getHeaderSize() const { return !blobs.empty() ? blobs[0].sndOffset : 0; }


  SoundBankData::FSB5Data loadFSB5Data(const BlobData &blob) const
  {
    InPlaceMemLoadCB crd(data.data() + blob.offset, blob.size);
    return SoundBankData::FSB5Data(blob.sndOffset, crd, blob.sndOffset + blob.sndSize);
  }


  void copyBlobToStream(const BlobData &blob, IGenSave &cwr) const
  {
    cwr.seekto(blob.sndOffset);
    cwr.write("SND ", 4);
    cwr.writeInt(blob.sndSize - sizeof(SoundBankData::SNDHeader));

    InPlaceMemLoadCB crd(data.data(), data.size());
    crd.seekto(blob.offset);
    copy_stream_to_stream(crd, cwr, blob.size);
  }
};


bool write_index_for_sound_bank_to_stream(const char *bank_path, IGenSave &index_cwr)
{
  FullFileLoadCB bankCrd(bank_path);
  if (!bankCrd.fileHandle)
  {
    logerr("failed to open bank file %s", bank_path);
    return false;
  }

  SoundBankData bankData;
  if (!bankData.load(bankCrd))
  {
    logerr("failed to parse bank file %s", bank_path);
    return false;
  }

  index_cwr.beginTaggedBlock(_MAKE4C('SNDI'));

  int headerSizePos = index_cwr.tell();
  index_cwr.writeInt(0);

  int dataSizePos = index_cwr.tell();
  index_cwr.writeInt(0);

  int compressedDataSizePos = index_cwr.tell();
  index_cwr.writeInt(0);

  index_cwr.writeInt(bankCrd.getTargetDataSize());
  index_cwr.writeInt(bankData.sndChunks.size());

  int blobOffset = bankData.getHeaderSize();
  for (int i = 0; i < bankData.sndChunks.size(); i++)
  {
    const auto &snd = bankData.sndChunks[i];
    const auto &fsb5 = bankData.fsb5Chunks[i];

    const int blobSize = fsb5.fullSize > 0 ? fsb5.dataEnd - fsb5.dataBegin : 0;

    index_cwr.writeInt(snd.offset);
    index_cwr.writeInt(snd.fullSize);
    index_cwr.writeInt(blobOffset);
    index_cwr.writeInt(blobSize);

    blobOffset += blobSize;
  }

  const char *bankName = dd_get_fname(bank_path);
  index_cwr.writeString(bankName);

  const int indexHeaderSize = index_cwr.tell() - headerSizePos;

  index_cwr.seekto(headerSizePos);
  index_cwr.writeInt(indexHeaderSize);

  index_cwr.seekto(dataSizePos);
  index_cwr.writeInt(blobOffset);

  index_cwr.seekto(headerSizePos + indexHeaderSize);

  const int zstdBeginPos = index_cwr.tell();

  ZstdSaveCB zcwr(index_cwr, 18);

  bankCrd.seekto(0);
  copy_stream_to_stream(bankCrd, zcwr, bankData.getHeaderSize());

  for (const SoundBankData::FSB5Data &fsb5 : bankData.fsb5Chunks)
    if (fsb5.fullSize > 0)
    {
      bankCrd.seekto(fsb5.dataBegin);
      copy_stream_to_stream(bankCrd, zcwr, fsb5.dataEnd - fsb5.dataBegin);
    }

  zcwr.finish();
  zcwr.flush();

  const int zstdEndPos = index_cwr.tell();

  index_cwr.seekto(compressedDataSizePos);
  index_cwr.writeInt(zstdEndPos - zstdBeginPos);

  index_cwr.seekto(zstdEndPos);

  index_cwr.endBlock();

  return true;
}


bool patch_update_sound_bank(const char *index_path, const char *sound_dir)
{
  FullFileLoadCB indexCrd(index_path);
  return patch_update_sound_bank_mem(indexCrd, sound_dir);
}


static void patch_update_sound_bank_with_index(const char *bank_path, const SoundIndexData &index)
{
  debug("patching bank file %s...", bank_path);

  if (!dd_file_exists(bank_path))
  {
    debug("bank file %s does not exist, skipping...", bank_path);
    return;
  }

  FullFileLoadCB oldCrd(bank_path, DF_READ | DF_IGNORE_MISSING);
  if (!oldCrd.fileHandle)
  {
    logwarn("failed to open bank file %s", bank_path);
    return;
  }

  SoundBankData oldBankData;
  if (!oldBankData.load(oldCrd))
  {
    logerr("failed to parse bank file %s", bank_path);
    return;
  }

  if (oldBankData.sndChunks.size() != index.hdr.blobsCount)
  {
    logerr("SND chunk count mismatch between bank file and index file: %d in bank, %d in index", int(oldBankData.sndChunks.size()),
      index.hdr.blobsCount);
    return;
  }

  String tmpFileName(0, "%s.tmp", bank_path);

  FullFileSaveCB newCwr;
  if (!newCwr.open(tmpFileName, DF_WRITE | DF_CREATE))
  {
    logerr("failed to create tmp bank file %s", tmpFileName);
    return;
  }

  oldCrd.seekto(0);
  copy_stream_to_stream(oldCrd, newCwr, min(oldCrd.getTargetDataSize(), int64_t(index.hdr.newBankSize)));

  if (index.hdr.newBankSize > oldCrd.getTargetDataSize())
    write_zeros(newCwr, index.hdr.newBankSize - oldCrd.getTargetDataSize());

  newCwr.seekto(0);
  newCwr.write(index.data.data(), index.getHeaderSize());

  for (int i = 0, sz = index.blobs.size(); i < sz; i++)
  {
    const SoundIndexData::BlobData &blob = index.blobs[i];
    if (blob.size <= 0)
      continue;

    index.copyBlobToStream(blob, newCwr);

    const SoundBankData::FSB5Data &oldFsb5 = oldBankData.fsb5Chunks[i];
    SoundBankData::FSB5Data newFsb5 = index.loadFSB5Data(blob);
    if (newFsb5.fullSize == 0)
    {
      logerr("failed to load FSB5 data for blob %d in index file, skipping audio patching for this blob", i);
      continue;
    }

    const int oldAudioOffset = oldFsb5.offset + oldFsb5.fullSize;
    const int oldAudioSize = oldFsb5.hdr.audioDataSize;

    const int newAudioOffset = blob.sndOffset + sizeof(SoundBankData::SNDHeader) + blob.size;
    const int newAudioSize = blob.sndSize - sizeof(SoundBankData::SNDHeader) - blob.size;

    newCwr.seekto(newAudioOffset);
    write_zeros(newCwr, newAudioSize);

    newCwr.seekto(newAudioOffset);

    const int oldSamplesCount = oldFsb5.hdr.samplesCount;
    const int newSamplesCount = newFsb5.hdr.samplesCount;

    eastl::vector_map<eastl::string, eastl::vector<int>> oldNameToIndices;
    oldNameToIndices.reserve(oldSamplesCount);
    for (int j = 0; j < oldSamplesCount; j++)
      oldNameToIndices[oldFsb5.sampleNames[j]].push_back(j);

    eastl::vector_map<eastl::string, int> oldNameNextDup;
    oldNameNextDup.reserve(newSamplesCount);

    int matchedSamples = 0;
    for (int ni = 0; ni < newSamplesCount; ni++)
    {
      auto it = oldNameToIndices.find(newFsb5.sampleNames[ni]);
      if (it == oldNameToIndices.end())
        continue;

      int &dupIdx = oldNameNextDup[newFsb5.sampleNames[ni]];
      if (dupIdx >= (int)it->second.size())
        continue;

      const int oi = it->second[dupIdx++];

      const int oldSampleOffset = oldFsb5.sampleOffsets[oi];
      const int oldNextSampleOffset = oi + 1 < oldSamplesCount ? oldFsb5.sampleOffsets[oi + 1] : oldAudioSize;

      const int newSampleOffset = newFsb5.sampleOffsets[ni];
      const int newNextSampleOffset = ni + 1 < newSamplesCount ? newFsb5.sampleOffsets[ni + 1] : newAudioSize;

      const int oldSize = oldNextSampleOffset - oldSampleOffset;
      const int newSize = newNextSampleOffset - newSampleOffset;

      if (oldSize == newSize)
      {
        ++matchedSamples;

        oldCrd.seekto(oldAudioOffset + oldSampleOffset);
        newCwr.seekto(newAudioOffset + newSampleOffset);
        copy_stream_to_stream(oldCrd, newCwr, newSize);
      }
    }

    debug("  FSB5 audio: %d/%d matched (%.1f%% reused)", matchedSamples, newFsb5.hdr.samplesCount,
      newSamplesCount > 0 ? 100.f * matchedSamples / newSamplesCount : 100.f);
  }

  oldCrd.close();
  newCwr.close();

  if (!dd_erase(bank_path))
  {
    logerr("failed to delete old bank file %s", bank_path);
    return;
  }

  if (!dd_rename(tmpFileName, bank_path))
  {
    logerr("failed to rename new bank file %s to %s", tmpFileName, bank_path);
    return;
  }

  debug("bank %s updated successfully", bank_path);
}


bool patch_update_sound_bank_mem(IGenLoad &index_crd, const char *sound_dir)
{
  const int indexDataSize = index_crd.getTargetDataSize();
  while (indexDataSize > 0 && index_crd.tell() < indexDataSize)
  {
    SoundIndexData index;
    if (!index.load(index_crd))
    {
      logerr("failed to load index file");
      return false;
    }

    const String bankPath = String(0, "%s/%s", sound_dir, index.bankName.c_str());
    patch_update_sound_bank_with_index(bankPath.c_str(), index);
  }

  return true;
}
