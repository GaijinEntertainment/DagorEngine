#include "resUpdUtil.h"
#include "resFormats.h"
#if !_TARGET_64BIT
#include "old_dump.h"
#endif
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_ioUtils.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_string.h>
#include <util/dag_texMetaData.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <stddef.h>

#if _TARGET_PC_LINUX
#include <unistd.h>
#endif //_TARGET_PC_LINUX

int patch_update_game_resources_verbose = 0;

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

      FullFileLoadCB crd(tempBuf);
      if (!crd.fileHandle)
      {
        logerr("failed to open %s", tempBuf);
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

      FullFileLoadCB crd(tempBuf);
      if (!crd.fileHandle)
      {
        logerr("failed to open %s", tempBuf);
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
  int name_len = i_strlen(tex_name);
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

    FullFileLoadCB crd(new_grp[i].fn);
    String tmp_fn(0, "%s.1", new_grp[i].fn.str());
    // debug("%s %p (%d, %d)", crd.getTargetName(), crd.fileHandle, data_size(new_grp[i].hdrData), new_grp[i].fullSz);
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

    FullFileLoadCB crd(new_dxp[i].fn);
    String tmp_fn(0, "%s.1", new_dxp[i].fn.str());
    // debug("%s %p (%d, %d)", crd.getTargetName(), crd.fileHandle, data_size(new_dxp[i].hdrData), new_dxp[i].fullSz);
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
