// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blk_shared.h"
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_strUtil.h>

namespace vromfsinternal
{
extern void (*on_vromfs_unmounted)(VirtualRomFsData *fs);
}

const char *dblk::SHARED_NAMEMAP_FNAME = "\xff?nm\0";

namespace
{
enum
{
  SNM_HSZ = 8,
  DICT_HSZ = 32,
  SNM_RAW_HDR_SZ = SNM_HSZ + DICT_HSZ,
  DEFAULT_COMPRESSION_LEVEL = 18,
  MIN_SIZE_TO_COMPRESS_UNCONDITIONALLY = 128
};

struct SharedNameMapRec
{
  DBNameMap *nm = nullptr;
  uint64_t nmHash = 0;
};
struct BlkDDictRec
{
  ZSTD_DDict_s *dict = nullptr;
  char dictHash[DICT_HSZ] = {0};
  const VirtualRomFsData *fs = nullptr;
  int rc = 0;
};
static Tab<SharedNameMapRec> sharedNameMaps;
static Tab<BlkDDictRec> blkDDict;
static WinCritSec ccGlobGuard;
} // namespace

static dag::ConstSpan<char> get_name_map_vrom_data(const VirtualRomFsData *fs)
{
  int last_fidx = fs ? fs->files.map.size() - 1 : -1;
  if (last_fidx >= 0 && strcmp(fs->files.map[last_fidx], dblk::SHARED_NAMEMAP_FNAME) == 0) //-V1004
    return fs->data[last_fidx];
  return {};
}

bool dblk::check_shared_name_map_valid(const VirtualRomFsData *fs, const char **out_err_desc)
{
  if (get_name_map_vrom_data(fs).data())
    return true;
  if (!fs)
    *out_err_desc = "binary format with shared namemap (\\3, \\4, \\5) may be read from vromfs only";
  else
    *out_err_desc = "missing shared namemap for binary format (\\3, \\4, \\5)";
  // if (fs)
  //   logerr("%s %s, fs->files=%d", fs->files.map[0], dblk::SHARED_NAMEMAP_FNAME, fs->files.map.size());
  return false;
}

DBNameMap *dblk::get_vromfs_shared_name_map(const VirtualRomFsData *fs)
{
  dag::ConstSpan<char> nm_data_in_vrom = get_name_map_vrom_data(fs);

  if (!nm_data_in_vrom.data() || nm_data_in_vrom.size() < SNM_RAW_HDR_SZ)
    return nullptr;

  WinAutoLock lock(ccGlobGuard);
  uint64_t hash = *(uint64_t *)nm_data_in_vrom.data();
  for (SharedNameMapRec &r : sharedNameMaps)
    if (r.nmHash == hash)
    {
      r.nm->addRef();
      return r.nm;
    }

  SharedNameMapRec r;
  r.nm = dblk::create_db_names();
  r.nmHash = hash;
  ZstdLoadFromMemCB zcrd(make_span_const(nm_data_in_vrom).subspan(SNM_RAW_HDR_SZ));
  if (!dblk::read_names(zcrd, *r.nm, nullptr))
  {
    dblk::destroy_db_names(r.nm);
    return nullptr;
  }

  r.nm->addRef();
  sharedNameMaps.push_back(r);
  // debug("get_shared_name_map(%llx)=%p", r.nmHash, r.nm);
  return r.nm;
}
int dblk::release_vromfs_shared_name_map(DBNameMap *nm) { return nm ? nm->delRef() : -1; }
int dblk::discard_unused_shared_name_maps()
{
  WinAutoLock lock(ccGlobGuard);
  unsigned released_cnt = 0;
  for (SharedNameMapRec &r : sharedNameMaps)
    if (!r.nm->getUsageRefs())
    {
      dblk::destroy_db_names(r.nm);
      r.nm = nullptr;
      released_cnt++;
    }
  if (!released_cnt)
    return sharedNameMaps.size();
  for (int i = sharedNameMaps.size() - 1; i >= 0; i--)
    if (!sharedNameMaps[i].nm)
      erase_items(sharedNameMaps, i, 1);

  if (!sharedNameMaps.size())
    clear_and_shrink(sharedNameMaps);
  return sharedNameMaps.size();
}

static void blk_ddict_on_vromfs_unmounted(VirtualRomFsData *fs)
{
  WinAutoLock lock(ccGlobGuard);
  for (BlkDDictRec &r : blkDDict)
    if (fs == r.fs)
    {
      G_ASSERTF(r.rc == 0, "vromfs=%p is unmounted while dict=%p rc=%d > 0", r.fs, r.dict, r.rc);
      // debug("destroy ddict=%p rc=%d on vromfs=%p removal", r.dict, r.rc, r.fs);
      zstd_destroy_ddict(r.dict);
      r.dict = nullptr;
      erase_items(blkDDict, &r - blkDDict.data(), 1);
      return;
    }
}

dag::ConstSpan<char> dblk::get_vromfs_dict_hash(dag::ConstSpan<char> shared_nm_data)
{
  if (!shared_nm_data.data() || shared_nm_data.size() < SNM_RAW_HDR_SZ)
    return make_span_const((const char *)nullptr, 0);
  return make_span_const(shared_nm_data.data() + SNM_HSZ, DICT_HSZ);
}

ZSTD_DDict_s *dblk::get_vromfs_blk_ddict(const VirtualRomFsData *fs)
{
  dag::ConstSpan<char> nm_data_in_vrom = get_name_map_vrom_data(fs);

  if (!nm_data_in_vrom.data() || nm_data_in_vrom.size() < SNM_RAW_HDR_SZ)
    return nullptr;

  WinAutoLock lock(ccGlobGuard);
  for (BlkDDictRec &r : blkDDict)
    if (memcmp(r.dictHash, nm_data_in_vrom.data() + SNM_HSZ, DICT_HSZ) == 0)
    {
      r.rc++;
      return r.dict;
    }

  BlkDDictRec r;
  memcpy(r.dictHash, nm_data_in_vrom.data() + SNM_HSZ, DICT_HSZ);

  char dict_name_buf[DICT_HSZ * 2 + 8];
  data_to_str_hex_buf(dict_name_buf, sizeof(dict_name_buf), r.dictHash, sizeof(r.dictHash));
  strcat(dict_name_buf, ".dict");

  dag::ConstSpan<char> dict_data;
  VromReadHandle readHandle;
  int idx = fs->files.getNameId(dict_name_buf);
  if (idx >= 0)
    dict_data = fs->data[idx], r.fs = fs;
  else
  {
    readHandle = vromfs_get_file_data(dict_name_buf, (VirtualRomFsData **)&r.fs);
    dict_data = readHandle;
  }
  if (!r.fs)
    return nullptr;

  r.dict = zstd_create_ddict(dag::ConstSpan<char>(dict_data.data(), data_size(dict_data)), true);
  r.rc++;
  blkDDict.push_back(r);
  // debug("get_blk_ddict(%s)=%p", dict_name_buf, r.dict);
  if (vromfsinternal::on_vromfs_unmounted != blk_ddict_on_vromfs_unmounted)
    vromfsinternal::on_vromfs_unmounted = blk_ddict_on_vromfs_unmounted;
  return r.dict;
}
int dblk::release_vromfs_blk_ddict(ZSTD_DDict_s *ddict)
{
  if (!ddict)
    return -1;
  WinAutoLock lock(ccGlobGuard);
  for (BlkDDictRec &r : blkDDict)
    if (r.dict == ddict)
      return --r.rc;
  return -1;
}
int dblk::discard_unused_blk_ddict()
{
  WinAutoLock lock(ccGlobGuard);
  unsigned released_cnt = 0;
  for (BlkDDictRec &r : blkDDict)
    if (!r.rc)
    {
      zstd_destroy_ddict(r.dict);
      r.dict = nullptr;
      released_cnt++;
    }
  if (!released_cnt)
    return blkDDict.size();
  for (int i = blkDDict.size() - 1; i >= 0; i--)
    if (!blkDDict[i].dict)
      erase_items(blkDDict, i, 1);

  if (!blkDDict.size())
    clear_and_shrink(blkDDict);
  return blkDDict.size();
}

ZSTD_CDict_s *dblk::create_vromfs_blk_cdict(const VirtualRomFsData *fs, int compr_level)
{
  dag::ConstSpan<char> nm_data_in_vrom = get_name_map_vrom_data(fs);

  if (!nm_data_in_vrom.data() || nm_data_in_vrom.size() < SNM_RAW_HDR_SZ)
    return nullptr;

  char dict_name_buf[DICT_HSZ * 2 + 8];
  data_to_str_hex_buf(dict_name_buf, sizeof(dict_name_buf), nm_data_in_vrom.data() + SNM_HSZ, DICT_HSZ);
  strcat(dict_name_buf, ".dict");
  int idx = fs->files.getNameId(dict_name_buf);
  if (idx < 0)
    return nullptr;

  return zstd_create_cdict(dag::ConstSpan<char>(fs->data[idx].data(), data_size(fs->data[idx])), compr_level);
}

void dblk::pack_shared_nm_dump_to_stream(IGenSave &cwr, IGenLoad &crd, int sz, int compr_level, const ZSTD_CDict_s *cdict)
{
  if (sz >= MIN_SIZE_TO_COMPRESS_UNCONDITIONALLY)
  {
    if (cdict)
    {
      cwr.writeIntP<1>(dblk::BBF_binary_with_shared_nm_zd);
      zstd_stream_compress_data_with_dict(cwr, crd, sz, compr_level, cdict);
    }
    else
    {
      cwr.writeIntP<1>(dblk::BBF_binary_with_shared_nm_z);
      zstd_stream_compress_data(cwr, crd, sz, compr_level);
    }
    return;
  }

  MemorySaveCB mcwr(256);
  const int posOnStart = crd.tell();
  if (cdict)
    zstd_stream_compress_data_with_dict(mcwr, crd, sz, compr_level, cdict);
  else
    zstd_stream_compress_data(mcwr, crd, sz, compr_level);

  const int plainSize = crd.tell() - posOnStart; // can't use sz, it can be negative to autodetect size
  if (mcwr.getSize() > plainSize)
  {
    cwr.writeIntP<1>(dblk::BBF_binary_with_shared_nm);
    crd.seekto(posOnStart);
    copy_stream_to_stream(crd, cwr, plainSize);
    return;
  }

  cwr.writeIntP<1>(cdict ? dblk::BBF_binary_with_shared_nm_zd : dblk::BBF_binary_with_shared_nm_z);
  mcwr.copyDataTo(cwr);
}

void dblk::pack_shared_nm_dump_to_stream(IGenSave &cwr, IGenLoad &crd, int sz, const ZSTD_CDict_s *cdict)
{
  dblk::pack_shared_nm_dump_to_stream(cwr, crd, sz, DEFAULT_COMPRESSION_LEVEL, cdict);
}

bool DataBlock::saveBinDumpWithSharedNamemap(IGenSave &cwr, const DBNameMap *shared_nm, bool pack, const ZSTD_CDict_s *dict) const
{
  if (!pack)
  {
    cwr.writeIntP<1>(dblk::BBF_binary_with_shared_nm);
    return saveDumpToBinStream(cwr, shared_nm);
  }

  MemorySaveCB mcwr(128 << 10);
  if (!saveDumpToBinStream(mcwr, shared_nm))
    return false;
  MemoryLoadCB mcrd(mcwr.getMem(), false);
  dblk::pack_shared_nm_dump_to_stream(cwr, mcrd, mcrd.getTargetDataSize(), dict);
  return true;
}

bool DataBlock::loadBinDumpWithSharedNamemap(IGenLoad &crd, const DBNameMap *shared_nm, const ZSTD_DDict_s *dict)
{
  unsigned label = crd.readIntP<1>();

  if (label == dblk::BBF_full_binary_in_stream)
    return loadFromBinDump(crd, nullptr);
  if (label == dblk::BBF_binary_with_shared_nm)
    return loadFromBinDump(crd, shared_nm);

  MemorySaveCB mcwr(16 << 10);
  if (label == dblk::BBF_binary_with_shared_nm_z)
    zstd_stream_decompress_data(mcwr, crd, crd.getTargetDataSize() - crd.tell());
  else if (label == dblk::BBF_binary_with_shared_nm_zd)
    zstd_stream_decompress_data(mcwr, crd, crd.getTargetDataSize() - crd.tell(), dict);
  else
    return false;
  MemoryLoadCB mcrd(mcwr.takeMem(), true);
  return loadFromBinDump(mcrd, shared_nm);
}

void dblk::pack_to_stream(const DataBlock &blk, IGenSave &cwr, int approx_sz)
{
  MemorySaveCB mcwr(approx_sz);
  blk.saveToStream(mcwr);

  int pos = cwr.tell();
  cwr.writeInt(0);

  MemoryLoadCB mcrd(mcwr.getMem(), false);
  zstd_stream_compress_data(cwr, mcrd, mcrd.getTargetDataSize(), DEFAULT_COMPRESSION_LEVEL);
  unsigned sz = cwr.tell() - pos - 4;

  cwr.seekto(pos);
  cwr.writeInt(dblk::BBF_full_binary_in_stream_z | (sz << 8));
  cwr.seekto(pos + 4 + sz);
}

bool dblk::pack_to_binary_file(const DataBlock &blk, const char *filename, int approx_sz)
{
  FullFileSaveCB cwr(filename);
  if (!cwr.fileHandle)
    return false;

  DAGOR_TRY { dblk::pack_to_stream(blk, cwr, approx_sz); }
  DAGOR_CATCH(const IGenSave::SaveException &) { return false; }
  return true;
}

#define EXPORT_PULL dll_pull_iosys_datablock_zstd
#include <supp/exportPull.h>
