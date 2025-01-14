//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_roNameMap.h>
#include <osApiWrappers/dag_rwLock.h>
#include <generic/dag_tabFwd.h>
#include <supp/dag_define_KRNLIMP.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>

class IMemAlloc;
class String;

struct VirtualRomFsData;
struct VirtualRomFsPack;
struct VirtualRomFsSingleFile;

//! callback to be used to obtain file data for files mentioned in backed vromfs
typedef bool (*vromfs_user_get_file_data_t)(const char *rel_fn, const char *mount_path, String &out_fn, int &out_base_ofs,
  bool sync_wait_ready, bool dont_load, const char *src_fn, void *arg);


struct VirtualRomFsDataHdr
{
  unsigned label;
  unsigned target;
  unsigned fullSz;
  unsigned hw32;

  unsigned packedSz() const { return hw32 & 0x3FFFFFFU; }
  bool zstdPacked() const { return (hw32 & 0x40000000U) != 0; }
  bool signedContents() const { return (hw32 & 0x80000000U) != 0; }
};

enum EVirtualRomFsExtFlags : uint16_t
{
  EVFSEF_SIGN_PLAIN_DATA = 0x0001,
  EVFSEF_HAVE_FILE_ATTRIBUTES = 0x0002
};

enum EVirtualRomFsFileAttributes : uint8_t
{
  EVFSFA_TYPE_PLAIN = 0x00,
  EVFSFA_TYPE_BLK = 0x01,
  EVFSFA_TYPE_SHARED_NM = 0x02,
  EVFSFA_TYPE_MASK = 0x03
};

struct VirtualRomFsFileAttributes
{
  EVirtualRomFsFileAttributes *attrData = nullptr;
  uint8_t version = 0;
};

KRNLIMP EVirtualRomFsFileAttributes guess_vromfs_file_type(const char *fileName, const dag::ConstSpan<const char> &fileData);

struct VirtualRomFsExtHdr
{
  uint16_t size = 0, flags = 0;
  uint32_t version = 0;
};

struct VirtualRomFsDataBase
{
  VirtualRomFsFileAttributes attr;
  int64_t mtime = -1;
  uint32_t version = 0;
  uint16_t flags = 0;
  bool firstPriority = true;
};

struct VirtualRomFsData : public VirtualRomFsDataBase
{
  RoNameMap files;
  PatchableTab<PatchableTab<const char>> data;
};

struct VirtualRomFsPack : public VirtualRomFsData
{
  struct BackedData
  {
    static constexpr int C_SHA1_RECSZ = 20;
    const char *contentSHA1BasePtr = NULL;
    VirtualRomFsPack *cache = NULL;
    vromfs_user_get_file_data_t getData = NULL;
    void *getDataArg = NULL;
    VirtualRomFsSingleFile *resolvedFilesLinkedList = NULL;
  };

  int hdrSz;
  int _resv;
  PATCHABLE_DATA64(void *, ptr);

  bool isValid() const { return (void *)files.map.data() >= (void *)(this + 1) && ptr; }
  const char *getFilePath() const { return isValid() && _resv == 0 ? ((const char *)&files) + hdrSz : NULL; }
  BackedData *getBackedData() { return isValid() && _resv == 'BCKD' ? (BackedData *)ptr : NULL; }


  //! helper generally for internal usage, returns resolved pack pointer and changes inout_entry to new entry in that pack
  //! when sync_wait_ready and entry cannot be resolved immediately. NULL is returned
  static VirtualRomFsPack *(*resolve_backed_entry)(VirtualRomFsData *fs, int &inout_entry, bool sync_wait_ready, bool dont_load,
    const char *mnt);
};

struct VirtualRomFsSingleFile : public VirtualRomFsPack
{
  VirtualRomFsSingleFile *nextPtr;
  IMemAlloc *mem;

  KRNLIMP void release();

  //! creates VirtualRomFsSingleFile as link to real file; rel_fn must be specified if pack is registered with add_vromfs()
  //! pointer maybe used as VirtualRomFsPack
  static KRNLIMP VirtualRomFsSingleFile *make_file_link(IMemAlloc *mem, const char *dest_fn, int base_ofs, int data_len,
    const char *rel_fn = NULL);

  //! creates VirtualRomFsSingleFile as memory data (optionally copied)
  //! pointer maybe used as VirtualRomFsPack
  static KRNLIMP VirtualRomFsSingleFile *make_mem_data(IMemAlloc *mem, void *data, int data_len, const char *rel_fn,
    bool copy_data = true);
};

struct VromfsDumpSections
{
  dag::ConstSpan<uint8_t> header;
  dag::ConstSpan<uint8_t> headerExt;
  dag::ConstSpan<uint8_t> body;
  dag::ConstSpan<uint8_t> md5;
  dag::ConstSpan<uint8_t> signature;
  bool isPacked = false;
};

KRNLIMP bool dissect_vromfs_dump(dag::ConstSpan<char> dump, VromfsDumpSections &result);

struct VromfsDumpBodySections
{
  dag::ConstSpan<uint8_t> tables;
  dag::ConstSpan<uint8_t> sha1Hashes;
  dag::ConstSpan<uint8_t> attributes;
  dag::ConstSpan<uint8_t> data;
  int filesCount = 0;
};

KRNLIMP bool dissect_vromfs_dump_body(const VromfsDumpSections &sections, VromfsDumpBodySections &result);

KRNLIMP bool init_vromfs_file_attr(VirtualRomFsFileAttributes &fsAttr, const VromfsDumpBodySections &bodySections);

//! callback interface that is used for signature verification (both if signature exist or not),
//! if its check() method returns false load is considered to be failed
//! data is fed into checker by calling append() method one or multiple times
//! the allowAbsentSignature() method should return true if vroms without signatures are considered valid
class VromfsSignatureChecker
{
public:
  virtual ~VromfsSignatureChecker() = default;
  virtual bool append(const void *buf, int len) = 0;
  virtual bool check(const void *sigbuf, int siglen) = 0;
  virtual bool allowAbsentSignature() const { return false; }
};
typedef eastl::unique_ptr<VromfsSignatureChecker> VromfsSignatureCheckerPtr;
typedef VromfsSignatureCheckerPtr (*signature_checker_factory_cb)();

KRNLIMP bool check_vromfs_dump_signature(VromfsSignatureChecker &checker, const VromfsDumpSections &vfsdump,
  const dag::ConstSpan<uint8_t> *to_verify = NULL);

//! loads vromfs dump from file into memory (to be released with mem->free(fs))
KRNLIMP VirtualRomFsData *load_vromfs_dump(const char *fname, IMemAlloc *mem, signature_checker_factory_cb checker_cb = NULL,
  const dag::ConstSpan<uint8_t> *to_verify = NULL, int file_flags = 0);

//! loads vromfs dump from cryped file into memory (to be released with mem->free(fs))
KRNLIMP VirtualRomFsData *load_crypted_vromfs_dump(const char *fname, IMemAlloc *mem);

#if _TARGET_ANDROID
//! finds Android asset and copies its data to storage
KRNLIMP bool android_get_asset_data(const char *asset_name, Tab<char> &out_storage);

//! loads vromfs dump from Android asset into memory (to be released with mem->free(fs))
KRNLIMP VirtualRomFsData *load_vromfs_from_asset(const char *asset_name, IMemAlloc *mem);
#endif

//! loads vromfs dump from memory into memory (to be released with mem->free(fs))
KRNLIMP VirtualRomFsData *load_vromfs_dump_from_mem(dag::ConstSpan<char> data, IMemAlloc *mem);

//! allocates VirtualRomFsData for headers only and resolves vromfs data from unpacked dump without changing it
//! returns vromfs object to access dump or nullptr on error;
//! optionally returns size of header part of dump and offset of signature (if any) in dump
KRNLIMP VirtualRomFsData *make_non_intrusive_vromfs(dag::ConstSpan<char> dump, IMemAlloc *mem, unsigned *out_hdr_sz = nullptr,
  int *out_signature_ofs = nullptr);
KRNLIMP VirtualRomFsData *make_non_intrusive_vromfs(const VromfsDumpSections &sections, const VromfsDumpBodySections &body_sections,
  IMemAlloc *mem, unsigned *out_hdr_sz = nullptr);

//! opens vromfs dump from file into memory (to be released with close_vromfs_pack)
KRNLIMP VirtualRomFsPack *open_vromfs_pack(const char *fname, IMemAlloc *mem, int file_flags = 0);
//! opens vromfs dump from file into memory (to be released with close_vromfs_pack)
//! vromfs is backed by optional cache_vrom and user-specific callback to retrieve file (e.g. from CDN)
KRNLIMP VirtualRomFsPack *open_backed_vromfs_pack(const char *fname, IMemAlloc *mem, VirtualRomFsPack *cache_vrom,
  vromfs_user_get_file_data_t get_file_data, void *gfd_arg, int file_flags = 0);
//! replaces cache-vrom for backed vrom-pack with new one and returns old one (cache_vrom=NULL is allowed, it means we don't use cache)
KRNLIMP VirtualRomFsPack *backed_vromfs_replace_cache(VirtualRomFsPack *fs, VirtualRomFsPack *cache_vrom);
//! prefetches all files mentioned in vromfs (useful to create download queue for files that are not backed by cache_vrom)
KRNLIMP void backed_vromfs_prefetch_all_files(VirtualRomFsPack *fs);
//! closes files referenced by vromfs dump and releases allocated memory
KRNLIMP void close_vromfs_pack(VirtualRomFsPack *fs, IMemAlloc *mem);

//! prefetches file in backed vromfs
KRNLIMP void backed_vromfs_prefetch_file(const char *fn, bool fn_is_prefix = false);
//! return availability for file in backed vromfs
KRNLIMP bool backed_vromfs_is_file_prefetched(const char *fn, bool fn_is_prefix = false);

//! returns MD5 digest for vromfs data
KRNLIMP bool get_vromfs_dump_digest(const char *fname, unsigned char *out_md5_digest);

//! returns version for vromfs data (only new VRFx format), or 0 if not available
KRNLIMP uint32_t get_vromfs_dump_version(const char *fname);

//! adds vromfs to list of active file systems
//! mount_path must be persistent string (pointer is stored for later use), it will be simplified and strlwr'ed
KRNLIMP void add_vromfs(VirtualRomFsData *fs, bool insert_first = false, char *mount_path = 0);
//! removes vromfs from list of active file systems; returns mount path (to be freed when needed)
KRNLIMP char *remove_vromfs(VirtualRomFsData *fs);
//! replaces vromfs by index (mount path remains the same); returns previously mounted vromfs for that index
KRNLIMP VirtualRomFsData *replace_vromfs(int idx, VirtualRomFsData *fs);

//! returns pointer to mount path for given vromfs
KRNLIMP char *get_vromfs_mount_path(VirtualRomFsData *fs);
//! sets mount path for gived vromfs; mount_path must be persistent string, it will be simplified and strlwr'ed; returns previous mount
//! point
KRNLIMP char *set_vromfs_mount_path(VirtualRomFsData *fs, char *mount_path);

//! delete all vromfs in file systems list, file system list is made empty after that
KRNLIMP void delete_all_vromfs();

class VromReadHandle
{
public:
  using value_type = char; // for make_span
  using data_type = dag::ConstSpan<value_type>;

  static KRNLIMP ReadWriteLock lock;

  VromReadHandle() {}
  VromReadHandle(const data_type &data) : wrapped(data)
  {
    // no need to lock if there is no data
    if (wrapped.data())
      lock.lockRead();
  }
  ~VromReadHandle()
  {
    if (wrapped.data())
      lock.unlockRead();
  }

  VromReadHandle(VromReadHandle &&rhs)
  {
    wrapped = eastl::move(rhs.wrapped);
    rhs.wrapped = data_type(nullptr, 0);
  }

  VromReadHandle &operator=(VromReadHandle &&rhs)
  {
    wrapped = eastl::move(rhs.wrapped);
    rhs.wrapped = data_type(nullptr, 0);
    return *this;
  }

  VromReadHandle(const VromReadHandle &) = delete;
  VromReadHandle &operator=(const VromReadHandle &) = delete;

  const value_type *data() const { return wrapped.data(); }
  uint32_t size() const { return wrapped.size(); }

  explicit operator data_type() { return wrapped; }
  explicit operator data_type() const { return wrapped; }

private:
  data_type wrapped;
};

static inline uint32_t data_size(const VromReadHandle &handle) { return data_size(make_span_const(handle)); }

//! finds data entry in active filesystems
KRNLIMP VromReadHandle vromfs_get_file_data(const char *fname, VirtualRomFsData **out_vrom = NULL);

//! get all available vromfs entries. not thread-safe!
KRNLIMP dag::Span<VirtualRomFsData *> vromfs_get_entries_unsafe();

//! execute callback for each available vromfs entry
template <typename F>
static inline void iterate_vroms(F callback)
{
  VromReadHandle::lock.lockRead();
  dag::Span<VirtualRomFsData *> vroms = vromfs_get_entries_unsafe();
  for (size_t i = 0; i < vroms.size(); ++i)
    if (!callback(vroms[i], i))
      break;
  VromReadHandle::lock.unlockRead();
}

//! checks whether file exists in one of active vromfs
inline bool vromfs_check_file_exists(const char *fname)
{
  VirtualRomFsData *p;
  return vromfs_get_file_data(fname, &p).data();
}

//! priority of VROMFS over regular files;
//! 'true' by default (AND REQUIRED TO BE TRUE FOR PERFORMANCE REASONS),
//! can be changed to 'false' in runtime for development purposes
extern KRNLIMP bool vromfs_first_priority;

#include <supp/dag_undef_KRNLIMP.h>
