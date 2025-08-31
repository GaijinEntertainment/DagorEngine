// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bsdiff/bsdiff.h>
#include "bsdiffwrap.h"
#include "compression.h"
#include <stdlib.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zstdIo.h>
#include <util/le2be.h>
#include <EASTL/unique_ptr.h>
#include <osApiWrappers/dag_vromfs.h>
#include <../ioSys/dataBlock/blk_shared.h>
#include <util/dag_strUtil.h>

#ifdef BSDIFFWRAP_DIFF_EXECUTABLE
#define TRACE_LOG debug
#else
#define TRACE_LOG(...)
#endif


typedef uint32_t bsdiff_size_t;

static const char *BSDIFF_HEADER = "GAIJIN/BSDIFF";
static const bsdiff_size_t REASONABLE_FILE_SIZE = 128 * 1024 * 1024; // 128 MB

#define BS_DIFF(OP, OS, NP, NS, S, ERR_STATEMENT)                                                          \
  if (int dres = bsdiff((const uint8_t *)(OP), OS, (const uint8_t *)(NP), NS, S))                          \
  {                                                                                                        \
    logwarn("%s(%d): bsdiff failed with %d (old_sz=%d, new_sz=%d)", __FUNCTION__, __LINE__, dres, OS, NS); \
    ERR_STATEMENT;                                                                                         \
  }

#define BS_PATCH(OP, OS, NP, NS, S, ERR_STATEMENT)                                                          \
  if (int pres = bspatch((const uint8_t *)(OP), OS, (uint8_t *)(NP), NS, S))                                \
  {                                                                                                         \
    logwarn("%s(%d): bspatch failed with %d (old_sz=%d, new_sz=%d)", __FUNCTION__, __LINE__, pres, OS, NS); \
    ERR_STATEMENT;                                                                                          \
  }

enum DiffCode
{
  DC_unchanged = 0,
  DC_blk2_diff_zd,
  DC_new_file,
  DC_file_diff,
  DC_shared_nm_diff,

  DC_blk2_diff = DC_blk2_diff_zd | 0x80,
  DC_blk2_diff_z = DC_blk2_diff_zd | 0x40,
};

enum
{
  COMPR_zstd = 'Z',
  COMPR_zstd_blk2_vrom = 0xFF,
};
static constexpr int ZSTD_BLK_CLEVEL = 11;

static const char *diff_code_to_string(DiffCode code)
{
  if (code == DC_unchanged)
    return "unchanged";
  if (code == DC_blk2_diff_zd)
    return "blk2_diff_zd";
  if (code == DC_new_file)
    return "new_file";
  if (code == DC_file_diff)
    return "file_diff";
  if (code == DC_shared_nm_diff)
    return "shared_nm_diff";
  if (code == DC_blk2_diff)
    return "blk2_diff";
  if (code == DC_blk2_diff_z)
    return "blk2_diff_z";
  return "<unknown>";
}

static inline uint32_t ntoh32(uint32_t x)
{
#if _TARGET_CPU_BE
  return x;
#else
  return le2be32(x);
#endif
}

static inline uint32_t hton32(uint32_t x) { return ntoh32(x); }


static void *bsdiff_alloc(size_t size) { return tmpmem->tryAlloc(size); }


static void bsdiff_free(void *mem) { tmpmem->free(mem); }


static int stream_write_cwr(struct bsdiff_stream *stream, const void *buffer, int size)
{
  if (!stream || !stream->opaque || !buffer)
    return -1;
  ((IGenSave *)stream->opaque)->write(buffer, size);
  return 0;
}

static void unpack_blk(IGenSave &cwr, dag::ConstSpan<char> data, ZSTD_DDict_s *ddict)
{
  InPlaceMemLoadCB crd(data.data() + 1, data_size(data) - 1);
  if (data[0] == dblk::BBF_binary_with_shared_nm)
    cwr.write(data.data() + 1, data_size(data) - 1);
  else if (data[0] == dblk::BBF_binary_with_shared_nm_z)
    zstd_stream_decompress_data(cwr, crd, crd.getTargetDataSize());
  else if (data[0] == dblk::BBF_binary_with_shared_nm_zd)
    zstd_stream_decompress_data(cwr, crd, crd.getTargetDataSize(), ddict);
}
static void pack_blk(IGenSave &cwr, dag::ConstSpan<char> data, int fmt, ZSTD_CDict_s *cdict)
{
  InPlaceMemLoadCB crd(data.data(), data_size(data));
  cwr.writeIntP<1>(fmt);
  if (fmt == dblk::BBF_binary_with_shared_nm)
    cwr.write(data.data(), data_size(data));
  else if (fmt == dblk::BBF_binary_with_shared_nm_z)
    zstd_stream_compress_data(cwr, crd, crd.getTargetDataSize(), ZSTD_BLK_CLEVEL);
  else if (fmt == dblk::BBF_binary_with_shared_nm_zd)
    zstd_stream_compress_data_with_dict(cwr, crd, crd.getTargetDataSize(), ZSTD_BLK_CLEVEL, cdict);
}

static void unpack_shared_name_map(IGenSave &cwr, dag::ConstSpan<char> data)
{
  static constexpr int NONPACKED_HASH_SZ = 8 + 32;
  cwr.write(data.data(), NONPACKED_HASH_SZ);
  InPlaceMemLoadCB crd(data.data() + NONPACKED_HASH_SZ, data_size(data) - NONPACKED_HASH_SZ);
  zstd_stream_decompress_data(cwr, crd, crd.getTargetDataSize());
}
static void pack_shared_name_map(IGenSave &cwr, dag::ConstSpan<char> data)
{
  static constexpr int NONPACKED_HASH_SZ = 8 + 32;
  cwr.write(data.data(), NONPACKED_HASH_SZ);
  InPlaceMemLoadCB crd(data.data() + NONPACKED_HASH_SZ, data_size(data) - NONPACKED_HASH_SZ);
  zstd_stream_compress_data(cwr, crd, crd.getTargetDataSize(), ZSTD_BLK_CLEVEL);
}

static inline BsdiffStatus write_one_file_diff(struct bsdiff_stream &stream, dag::ConstSpan<char> file_old,
  dag::ConstSpan<char> file_new, ZSTD_DDict_s *ddict_old, ZSTD_DDict_s *ddict_new, DynamicMemGeneralSaveCB &cwr_old,
  DynamicMemGeneralSaveCB &cwr_new, const char *nm, bool is_shared_nm)
{
  ZstdSaveCB &zcwr = *reinterpret_cast<ZstdSaveCB *>(stream.opaque);

  if (!file_old.data() || (!file_new.size() && file_old.size()))
  {
    zcwr.writeIntP<1>(DC_new_file);
    zcwr.write(file_new.data(), data_size(file_new));
    TRACE_LOG("(%s): %s: new (sz=%d)", diff_code_to_string(DC_new_file), nm, file_new.size());
    return BSDIFF_OK;
  }
  if (file_new.size() == file_old.size() && mem_eq(file_new, file_old.data()))
  {
    zcwr.writeIntP<1>(DC_unchanged);
    return BSDIFF_OK;
  }
  if (str_ends_with_c(nm, ".blk", -1, 4) && file_new.size() && file_old.size())
  {
    unsigned nl = file_new[0];
    unsigned ol = file_old[0];
    if (
      (nl == dblk::BBF_binary_with_shared_nm || nl == dblk::BBF_binary_with_shared_nm_z || nl == dblk::BBF_binary_with_shared_nm_zd) &&
      (ol == dblk::BBF_binary_with_shared_nm || ol == dblk::BBF_binary_with_shared_nm_z ||
        ol == dblk::BBF_binary_with_shared_nm_zd)) // both BLK2 (using shared namemap)
    {
      cwr_old.setsize(0);
      unpack_blk(cwr_old, file_old, ddict_old);
      cwr_new.setsize(0);
      unpack_blk(cwr_new, file_new, ddict_new);

      DiffCode diff_code = (nl == dblk::BBF_binary_with_shared_nm_zd)
                             ? DC_blk2_diff_zd
                             : ((nl == dblk::BBF_binary_with_shared_nm) ? DC_blk2_diff : DC_blk2_diff_z);
      zcwr.writeIntP<1>(diff_code);
      zcwr.writeIntP<3>(cwr_new.size());
      BS_DIFF(cwr_old.data(), cwr_old.size(), cwr_new.data(), cwr_new.size(), &stream, return BSDIFF_INTERNAL_ERROR);
      TRACE_LOG("(%s): %s: blk fmt %d->%d (packed %d->%d, unpacked %d->%d)", diff_code_to_string(diff_code), nm, ol, nl,
        file_old.size(), file_new.size(), cwr_old.size(), cwr_new.size());
      return BSDIFF_OK;
    }
  }
  if (is_shared_nm) // last entry - dblk::SHARED_NAMEMAP_FNAME
  {
    cwr_old.setsize(0);
    unpack_shared_name_map(cwr_old, file_old);
    cwr_new.setsize(0);
    unpack_shared_name_map(cwr_new, file_new);

    zcwr.writeIntP<1>(DC_shared_nm_diff);
    zcwr.writeIntP<3>(cwr_new.size());
    BS_DIFF(cwr_old.data(), cwr_old.size(), cwr_new.data(), cwr_new.size(), &stream, return BSDIFF_INTERNAL_ERROR);
    TRACE_LOG("(%s): %s: shared nameMap (packed %d->%d, unpacked %d->%d)", diff_code_to_string(DC_shared_nm_diff), nm, file_old.size(),
      file_new.size(), cwr_old.size(), cwr_new.size());
    return BSDIFF_OK;
  }

  zcwr.writeIntP<1>(DC_file_diff);
  BS_DIFF(file_old.data(), file_old.size(), file_new.data(), file_new.size(), &stream, return BSDIFF_INTERNAL_ERROR);
  TRACE_LOG("(%s): %s: diff (sz %d->%d)", diff_code_to_string(DC_file_diff), nm, file_old.size(), file_new.size());
  return BSDIFF_OK;
}


#if defined(MIMIC_WRONG_SIGNATURE_OFFSET_DIFF_CREATION)

static int get_vromfs_dump_full_body_size(dag::ConstSpan<char> dump)
{
  const VirtualRomFsDataHdr *hdr = reinterpret_cast<const VirtualRomFsDataHdr *>(dump.data());
  return hdr->fullSz;
}

#endif


BsdiffStatus create_vromfs_bsdiff(dag::ConstSpan<char> old_dump, dag::ConstSpan<char> new_dump, IGenSave &cwr_diff)
{
  if (old_dump.size() > REASONABLE_FILE_SIZE || new_dump.size() > REASONABLE_FILE_SIZE || !old_dump.data() || !new_dump.data())
    return BSDIFF_INTERNAL_ERROR;

  unsigned hdr_sz_old = 0, hdr_sz_new = 0;
  int md5_hash_ofs_new = -1;
  int signature_ofs_new = -1;
  eastl::unique_ptr<VirtualRomFsData, tmpmemDeleter> fs_old, fs_new;
  fs_old.reset(make_non_intrusive_vromfs(old_dump, tmpmem, &hdr_sz_old));
  fs_new.reset(make_non_intrusive_vromfs(new_dump, tmpmem, &hdr_sz_new, &md5_hash_ofs_new, &signature_ofs_new));

  bool blk2_vrom_diff = (fs_new && fs_old && fs_new->files.getNameId(dblk::SHARED_NAMEMAP_FNAME) >= 0 &&
                         fs_old->files.getNameId(dblk::SHARED_NAMEMAP_FNAME) >= 0);

  bsdiff_size_t netOrderSize = hton32(data_size(new_dump));

  cwr_diff.write(BSDIFF_HEADER, (int)strlen(BSDIFF_HEADER));
  cwr_diff.write(&netOrderSize, sizeof(netOrderSize));
  cwr_diff.writeIntP<1>(blk2_vrom_diff ? COMPR_zstd_blk2_vrom : COMPR_zstd);

  ZstdSaveCB zcwr(cwr_diff, 18);
  struct bsdiff_stream stream;
  stream.malloc = bsdiff_alloc;
  stream.free = bsdiff_free;
  stream.write = stream_write_cwr;
  stream.opaque = &zcwr;

  if (blk2_vrom_diff)
  {
    zcwr.writeInt(hdr_sz_new);
    BS_DIFF(old_dump.data(), hdr_sz_old, new_dump.data(), hdr_sz_new, &stream, return BSDIFF_INTERNAL_ERROR);

    DynamicMemGeneralSaveCB cwr_old(tmpmem), cwr_new(tmpmem);
    ZSTD_DDict_s *ddict_old = dblk::get_vromfs_blk_ddict(fs_old.get());
    ZSTD_DDict_s *ddict_new = dblk::get_vromfs_blk_ddict(fs_new.get());
    // do 2 passes - first diff for namemap and ddict, next diff the rest of files
    for (unsigned pass = 0; pass < 2; pass++)
      for (unsigned i = 0; i < fs_new->files.map.size(); i++)
      {
        const char *nm = fs_new->files.map[i];
        bool primary_file = (i + 1 == fs_new->files.map.size() || str_ends_with_c(nm, ".dict", -1, 5));
        if ((pass == 0 && !primary_file) || (pass == 1 && primary_file))
          continue;

        int old_i = fs_old->files.getNameId(nm);
        if (write_one_file_diff(stream,
              make_span<const char>(old_i < 0 ? nullptr : fs_old->data[old_i].data(), old_i < 0 ? 0 : fs_old->data[old_i].size()),
              make_span<const char>(fs_new->data[i].data(), fs_new->data[i].size()), ddict_old, ddict_new, cwr_old, cwr_new, nm,
              i + 1 == fs_new->files.map.size()))
        {
          dblk::release_vromfs_blk_ddict(ddict_old);
          dblk::release_vromfs_blk_ddict(ddict_new);
          return BSDIFF_INTERNAL_ERROR;
        }
      }
    dblk::release_vromfs_blk_ddict(ddict_old);
    dblk::release_vromfs_blk_ddict(ddict_new);
#if defined(MIMIC_WRONG_SIGNATURE_OFFSET_DIFF_CREATION)
    // it's known to be off by the size of headers but we need that for compatibility with old clients
    md5_hash_ofs_new = get_vromfs_dump_full_body_size(new_dump);
    signature_ofs_new = 0;
#endif
    if (md5_hash_ofs_new > 0)
      zcwr.write(new_dump.data() + md5_hash_ofs_new, data_size(new_dump) - md5_hash_ofs_new);
    else if (signature_ofs_new > 0)
      zcwr.write(new_dump.data() + signature_ofs_new, data_size(new_dump) - signature_ofs_new);
  }
  else
  {
#ifdef BSDIFFWRAP_DIFF_EXECUTABLE
    debug("Format: ZSTD BLK 1.0");

    for (unsigned i = 0; i < fs_new->files.map.size(); i++)
    {
      const char *name = fs_new->files.map[i];
      const int oldId = fs_old->files.getNameId(name);
      if (oldId >= 0)
      {
        if (fs_old->data[oldId].size() != fs_new->data[i].size() ||
            ::memcmp(fs_old->data[oldId].data(), fs_new->data[i].data(), fs_new->data[i].size()) != 0)
        {
          debug("(diff): %s: size=%d->%d", name, fs_old->data[i].size(), fs_new->data[i].size());
        }
      }
      else
        debug("(new file): %s: size=%d", name, fs_new->data[i].size());
    }
#endif
    BS_DIFF(old_dump.data(), old_dump.size(), new_dump.data(), new_dump.size(), &stream, return BSDIFF_INTERNAL_ERROR);
  }
  zcwr.finish();
  return BSDIFF_OK;
}

static BsdiffStatus create_bsdiff_deprecated(const char *oldData, int oldSize, const char *newData, int newSize, Tab<char> &diff,
  const char *comprName)
{
  const Compression &compr = Compression::getInstanceByName(comprName);
  if (strcmp(compr.getName(), comprName) != 0)
    return BSDIFF_INVALID_COMPRESSION;
  const char *header = BSDIFF_HEADER;
  const int headerLen = (int)strlen(header);

  if (oldSize < 0 || oldSize > REASONABLE_FILE_SIZE || newSize < 0 || newSize > REASONABLE_FILE_SIZE || !oldData || !newData)
    return BSDIFF_INTERNAL_ERROR;

  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 512 << 10);
  struct bsdiff_stream stream;
  stream.malloc = bsdiff_alloc;
  stream.free = bsdiff_free;
  stream.write = stream_write_cwr;
  stream.opaque = &cwr;

  BS_DIFF(oldData, oldSize, newData, newSize, &stream, return BSDIFF_INTERNAL_ERROR);

  int comprSize = compr.getRequiredCompressionBufferLength(cwr.size());
  Tab<char> comprStorage(tmpmem);
  comprStorage.resize(comprSize);
  const char *comprData = compr.compress(cwr.data(), cwr.size(), comprStorage.data(), comprSize);
  cwr.resize(0); // clears storage

  if (!comprData)
    return BSDIFF_COMPRESSION_ERROR;

  bsdiff_size_t netOrderSize = hton32(newSize);
  diff.resize(headerLen + sizeof(netOrderSize) + 1 + comprSize);
  char *p = diff.data();
  memcpy(p, header, headerLen);
  p = &p[headerLen];
  memcpy(p, &netOrderSize, sizeof(netOrderSize));
  p = &p[sizeof(netOrderSize)];
  p[0] = compr.getId();
  p = &p[1];
  memcpy(p, comprData, comprSize);
  return BSDIFF_OK;
}

BsdiffStatus create_bsdiff(const char *oldData, int oldSize, const char *newData, int newSize, Tab<char> &diff, const char *comprName)
{
  if (comprName && strcmp(comprName, "zstd") != 0)
    return create_bsdiff_deprecated(oldData, oldSize, newData, newSize, diff, comprName);

  MemorySaveCB cwr;
  BsdiffStatus res = create_vromfs_bsdiff(make_span(oldData, oldSize), make_span(newData, newSize), cwr);
  if (res != BSDIFF_OK)
    return res;

  diff.resize(cwr.getSize());
  ConstrainedMemSaveCB tab_cwr(diff.data(), data_size(diff));
  cwr.copyDataTo(tab_cwr);
  return res;
}

struct StreamStruct
{
  const char *data;
  int size;
  int ptr;
};


static int stream_read_crd(const struct bspatch_stream *stream, void *buffer, int length)
{
  if (!stream || !stream->opaque)
    return -1;
  if (length)
    ((IGenLoad *)stream->opaque)->read(buffer, length);
  return 0;
}


unsigned check_bsdiff_header(const char *diff, int diffSize) // returns header size or 0 on error
{
  const char *header = BSDIFF_HEADER;
  int headerLen = (int)strlen(header);
  const char *p = diff;
  bsdiff_size_t newSize;
  const int newSizeLen = sizeof(newSize);
  if (diffSize < headerLen + newSizeLen + 1 || diffSize > REASONABLE_FILE_SIZE)
    return 0;
  if (memcmp(p, header, headerLen) != 0)
    return 0;
  p = &p[headerLen];
  memcpy(&newSize, p, newSizeLen);
  newSize = ntoh32(newSize);
  if (newSize > REASONABLE_FILE_SIZE)
    return 0;
  p = &p[newSizeLen];
  if (uint8_t(p[0]) == COMPR_zstd_blk2_vrom)
    return headerLen;
  const Compression &compr = Compression::getInstanceById(p[0]);
  if (compr.getId() != p[0])
    return 0;
  return headerLen;
}

template <typename T>
class TabPub : public Tab<T>
{
public:
  using Tab<T>::mBeginAndAllocator;
  using Tab<T>::used;
  using Tab<T>::allocated;

  TabPub() : Tab<T>() {}
};

static inline BsdiffStatus apply_one_file_diff(struct bspatch_stream &stream, int diff_code, dag::ConstSpan<char> file_old,
  dag::Span<char> file_new, ZSTD_DDict_s *ddict_old, ZSTD_CDict_s *cdict_new, DynamicMemGeneralSaveCB &cwr_old,
  DynamicMemGeneralSaveCB &cwr_new, Tab<char> &stor_new, const char *nm)
{
  ZstdLoadFromMemCB &zcrd = *reinterpret_cast<ZstdLoadFromMemCB *>(stream.opaque);
  TRACE_LOG("(%x)%s: sz %d->%d", diff_code, nm, file_old.size(), file_new.size());

  if (diff_code == DC_unchanged) // the same
  {
    G_ASSERTF(file_old.size() == file_new.size(), "old_sz=%d new_sz=%d nm=%s", file_old.size(), file_new.size(), nm);

    memcpy(file_new.data(), file_old.data(), file_old.size());
  }
  else if (diff_code == DC_new_file) // new file
    zcrd.read(file_new.data(), data_size(file_new));
  else if (diff_code == DC_blk2_diff_zd || diff_code == DC_blk2_diff || diff_code == DC_blk2_diff_z) // BLK2 diff
  {
    cwr_old.setsize(0);
    unpack_blk(cwr_old, file_old, ddict_old);
    stor_new.resize(zcrd.readIntP<3>());
    BS_PATCH(cwr_old.data(), cwr_old.size(), stor_new.data(), stor_new.size(), &stream, return BSDIFF_CORRUPT_PATCH)

    int nl = dblk::BBF_binary_with_shared_nm_zd;
    if (diff_code != DC_blk2_diff_zd)
      nl = (diff_code == DC_blk2_diff) ? dblk::BBF_binary_with_shared_nm : dblk::BBF_binary_with_shared_nm_z;

    cwr_new.setsize(0);
    pack_blk(cwr_new, stor_new, nl, cdict_new);
    G_ASSERTF(file_new.size() == cwr_new.size(), "file_new.sz=%d, cwr_new.sz=%d, %s", file_new.size(), cwr_new.size(), nm);
    memcpy(file_new.data(), cwr_new.data(), cwr_new.size());
    stor_new.resize(0);
  }
  else if (diff_code == DC_shared_nm_diff) // shared namemap
  {
    cwr_old.setsize(0);
    unpack_shared_name_map(cwr_old, file_old);
    stor_new.resize(zcrd.readIntP<3>());
    BS_PATCH(cwr_old.data(), cwr_old.size(), stor_new.data(), stor_new.size(), &stream, return BSDIFF_CORRUPT_PATCH);

    cwr_new.setsize(0);
    pack_shared_name_map(cwr_new, stor_new);
    G_ASSERTF(file_new.size() == cwr_new.size(), "file_new.sz=%d, cwr_new.sz=%d, %s", file_new.size(), cwr_new.size(), nm);
    memcpy(file_new.data(), cwr_new.data(), cwr_new.size());
    stor_new.resize(0);
  }
  else if (diff_code == DC_file_diff) // bsdiff
  {
    BS_PATCH(file_old.data(), file_old.size(), file_new.data(), file_new.size(), &stream, return BSDIFF_CORRUPT_PATCH);
  }
  else
    return BSDIFF_CORRUPT_PATCH;
  return BSDIFF_OK;
}

BsdiffStatus apply_vromfs_bsdiff(Tab<char> &out_new_dump, dag::ConstSpan<char> old_dump, dag::ConstSpan<char> diff)
{
  if (old_dump.size() > REASONABLE_FILE_SIZE || diff.size() > REASONABLE_FILE_SIZE || !old_dump.data() || !diff.data())
    return BSDIFF_INTERNAL_ERROR;
  unsigned headerLen = check_bsdiff_header(diff.data(), data_size(diff));
  if (!headerLen)
  {
    logwarn("%s: BSDIFF_CORRUPT_PATCH: check_bsdiff_header failed (diffSize=%d)", __FUNCTION__, diff.size());
    return BSDIFF_CORRUPT_PATCH;
  }

  const char *p = diff.data() + headerLen;
  bsdiff_size_t newSize;
  memcpy(&newSize, p, sizeof(newSize));
  newSize = ntoh32(newSize);
  p += sizeof(newSize);

  InPlaceMemLoadCB crd(p + 1, diff.data() + data_size(diff) - p - 1);
  ZstdLoadFromMemCB zcrd;
  eastl::unique_ptr<char, tmpmemDeleter> plainStorage;
  bool blk2_vrom_diff = (uint8_t(*p) == COMPR_zstd_blk2_vrom);

  if (blk2_vrom_diff || *p == COMPR_zstd)
    zcrd.initDecoder(crd.getTargetRomData(), nullptr);
  else
  {
    const Compression &compr = Compression::getInstanceById(p[0]);
    int plainSize = compr.getRequiredDecompressionBufferLength(p + 1, crd.getTargetDataSize());
    if (plainSize < 0 || plainSize > REASONABLE_FILE_SIZE)
    {
      logwarn("%s: BSDIFF_CORRUPT_PATCH: plainSize=%d", __FUNCTION__, plainSize);
      return BSDIFF_CORRUPT_PATCH;
    }
    plainStorage.reset((char *)tmpmem->tryAlloc(plainSize));
    if (!plainStorage)
      return BSDIFF_NOMEM;
    const char *plainData = compr.decompress(p + 1, crd.getTargetDataSize(), plainStorage.get(), plainSize);
    if (!plainData)
    {
      logwarn("%s: BSDIFF_CORRUPT_PATCH: decompress return nullptr (comprSize=%d, plainSize=%d)", __FUNCTION__,
        crd.getTargetDataSize(), plainSize);
      return BSDIFF_CORRUPT_PATCH;
    }
    crd.setSrcMem(plainStorage.get(), plainSize);
  }

  if (newSize > out_new_dump.capacity())
  {
    // somewhat hack - allocate buffer without fatal in aces of OOM
    clear_and_shrink(out_new_dump);
    TabPub<char> &pub = static_cast<TabPub<char> &>(out_new_dump);
    pub.mBeginAndAllocator.first() = (char *)dag::get_allocator(out_new_dump)->tryAlloc(newSize);
    if (!pub.mBeginAndAllocator.first())
      return BSDIFF_NOMEM;
    pub.used() = pub.allocated() = newSize;
  }
  else
    out_new_dump.resize(newSize);

  struct bspatch_stream stream;
  stream.read = stream_read_crd;
  stream.opaque = (blk2_vrom_diff || *p == COMPR_zstd) ? static_cast<IGenLoad *>(&zcrd) : static_cast<IGenLoad *>(&crd);
  auto cleanup = [&zcrd, &out_new_dump](BsdiffStatus ret) {
    zcrd.ceaseReading();
    if (ret != BSDIFF_OK)
      clear_and_shrink(out_new_dump);
    return ret;
  };

  if (blk2_vrom_diff)
  {
    unsigned hdr_sz_new = zcrd.readInt();
    unsigned hdr_sz_old = 0;
    eastl::unique_ptr<VirtualRomFsData, tmpmemDeleter> fs_old;
    eastl::unique_ptr<VirtualRomFsData, tmpmemDeleter> fs_new;
    DynamicMemGeneralSaveCB cwr_old(tmpmem), cwr_new(tmpmem);
    ;
    Tab<char> stor_new;

    fs_old.reset(make_non_intrusive_vromfs(old_dump, tmpmem, &hdr_sz_old));
    if (!fs_old)
      return BSDIFF_INVALID_VROMFS;
    out_new_dump.resize(newSize);
    mem_set_0(out_new_dump);

    BS_PATCH(old_dump.data(), hdr_sz_old, out_new_dump.data(), hdr_sz_new, &stream, return BSDIFF_CORRUPT_PATCH);

    fs_new.reset(make_non_intrusive_vromfs(out_new_dump, tmpmem, nullptr));
    if (!fs_new)
      return BSDIFF_CORRUPT_PATCH;

    ZSTD_DDict_s *ddict_old = dblk::get_vromfs_blk_ddict(fs_old.get());
    ZSTD_CDict_s *cdict_new = nullptr;
    // do 2 passes - first patch for namemap and ddict, next patch the rest of files using ddict and cdict
    for (unsigned pass = 0; pass < 2; pass++)
    {
      for (unsigned i = 0; i < fs_new->files.map.size(); i++)
      {
        const char *nm = fs_new->files.map[i];
        bool primary_file = (i + 1 == fs_new->files.map.size() || str_ends_with_c(nm, ".dict", -1, 5));
        if ((pass == 0 && !primary_file) || (pass == 1 && primary_file))
          continue;

        int diff_code = zcrd.readIntP<1>();
        int old_i = diff_code != DC_new_file ? fs_old->files.getNameId(nm) : -1;
        G_ASSERTF(old_i >= 0 || diff_code == DC_new_file, "old_i=%d nm=%s diff_code=%d", old_i, nm, diff_code);

        if (apply_one_file_diff(stream, diff_code,
              make_span<const char>(old_i < 0 ? nullptr : fs_old->data[old_i].data(), old_i < 0 ? 0 : fs_old->data[old_i].size()),
              make_span((char *)fs_new->data[i].data(), fs_new->data[i].size()), ddict_old, cdict_new, cwr_old, cwr_new, stor_new,
              nm) != BSDIFF_OK)
        {
          zstd_destroy_cdict(cdict_new);
          dblk::release_vromfs_blk_ddict(ddict_old);
          return cleanup(BSDIFF_CORRUPT_PATCH);
        }
      }
      if (pass == 0)
        cdict_new = dblk::create_vromfs_blk_cdict(fs_new.get(), ZSTD_BLK_CLEVEL);
    }
    zstd_destroy_cdict(cdict_new);
    dblk::release_vromfs_blk_ddict(ddict_old);

    cwr_new.setsize(0);
    char buf[512];
    constexpr int MAX_TAIL_SIZE = 65536;
    for (int readSz = zcrd.tryRead(buf, sizeof(buf)); readSz > 0; readSz = zcrd.tryRead(buf, sizeof(buf)))
    {
      if (cwr_new.size() + readSz > MAX_TAIL_SIZE)
        return cleanup(BSDIFF_CORRUPT_PATCH);
      if (cwr_new.tryWrite(buf, readSz) != readSz)
        return cleanup(BSDIFF_NOMEM);
    }

    const int tailSize = cwr_new.size();
    if (tailSize > data_size(out_new_dump))
      return cleanup(BSDIFF_CORRUPT_PATCH);
    char *const tailPtr = out_new_dump.data() + data_size(out_new_dump) - tailSize;
    memcpy(tailPtr, cwr_new.data(), cwr_new.size());
  }
  else
  {
    BS_PATCH(old_dump.data(), old_dump.size(), out_new_dump.data(), out_new_dump.size(), &stream,
      return cleanup(BSDIFF_CORRUPT_PATCH));
  }

  return cleanup(BSDIFF_OK);
}

BsdiffStatus apply_bsdiff(const char *oldData, int oldSize, const char *diff, int diffSize, Tab<char> &newData)
{
  return apply_vromfs_bsdiff(newData, make_span(oldData, oldSize), make_span(diff, diffSize));
}

const char *bsdiff_status_str(BsdiffStatus status)
{
  const char *statusStr = "unknown error";
  switch (status)
  {
    case BSDIFF_OK: statusStr = "ok"; break;
    case BSDIFF_INVALID_COMPRESSION: statusStr = "invalid compression"; break;
    case BSDIFF_INTERNAL_ERROR: statusStr = "internal error"; break;
    case BSDIFF_COMPRESSION_ERROR: statusStr = "compression error"; break;
    case BSDIFF_DECOMPRESSION_ERROR: statusStr = "decompression error"; break;
    case BSDIFF_CORRUPT_PATCH: statusStr = "corrupt patch"; break;
    case BSDIFF_NOMEM: statusStr = "no memory"; break;
    default: statusStr = "unknown error"; break;
  }
  return statusStr;
}
