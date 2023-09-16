#pragma once

#include <ioSys/dag_dataBlock.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_simpleString.h>
#include <osApiWrappers/dag_atomic.h>

#define DATABLOCK_USES_FIXED_BLOCK_ALLOCATOR 1
static constexpr uint32_t IS_NAMEMAP_ID = 0x80000000;
static inline bool is_string_id_in_namemap(uint32_t id) { return id & IS_NAMEMAP_ID; };
static inline uint32_t namemap_id_from_string_id(uint32_t id) { return id & ~IS_NAMEMAP_ID; };

struct DBNameMapBase : public OAHashNameMap<false>
{};
struct DBNameMap : public DBNameMapBase
{
  void addRef() const { interlocked_increment(usageRefCount); }
  int delRef() const { return interlocked_decrement(usageRefCount); }
  int getUsageRefs() const { return interlocked_acquire_load(usageRefCount); }

private:
  mutable volatile int usageRefCount = 0;
};

template <class To, class From>
static __forceinline To memcpy_cast(const From &f)
{
  To ret;
  memcpy(&ret, &f, sizeof(To));
  return ret;
}

template <class To>
static __forceinline To memcpy_cast(const char *f)
{
  To ret;
  memcpy(&ret, f, sizeof(To));
  return ret;
}

struct DataBlockOwned // not thread safe
{
  SmallTab<char> data;

public:
  uint32_t allocate(uint32_t sz)
  {
    uint32_t at = data.size();
    data.resize(at + sz);
    return at;
  }

  char *getUnsafe(uint32_t at) { return data.data() + at; }
  const char *getUnsafe(uint32_t at) const { return data.data() + at; }
  char *get(uint32_t at)
  {
    G_ASSERTF(data.size() > at, "%d sz at = %d", data.size(), at);
    return getUnsafe(at);
  }
  const char *get(uint32_t at) const
  {
    G_ASSERTF(data.size() > at, "%d sz at = %d", data.size(), at);
    return getUnsafe(at);
  }

  char *insertAt(uint32_t at, uint32_t n)
  {
    G_ASSERTF(data.size() >= at, "sz %d at %d n %d", data.size(), at, n);
    return data.insert_default(data.begin() + at, n);
  }
  char *insertAt(uint32_t at, uint32_t n, const char *v)
  {
    G_ASSERTF(data.size() >= at, "sz %d at %d n %d", data.size(), at, n);
    // that's faster then SmallTab::insert
    auto ret = data.insert_default(data.begin() + at, n);
    memcpy(ret, v, n);
    return ret;
  }
};

struct DataBlockShared
{
  const char *getName(uint32_t id) const
  {
    int roc = !ro ? 0 : ro->nameCount();
    return id < roc ? ro->getName(id) : rw.getName((int)id - roc); //-V1004
  }
  uint32_t nameCount() const { return rw.nameCount() + (!ro ? 0 : ro->nameCount()); }
  bool nameExists(uint32_t id) const { return id < nameCount(); }

  int getNameId(const char *name, size_t name_len) const
  {
    auto hash = DBNameMap::string_hash(name, name_len);
    const int roc = !ro ? 0 : ro->nameCount();
    if (rw.nameCount())
    {
      int id = rw.getNameId(name, name_len, hash);
      if (id >= 0)
        return id + roc;
    }
    return roc ? ro->getNameId(name, name_len, hash) : -1; //-V1004
  }
  int getNameId(const char *name) const { return getNameId(name, strlen(name)); }

  int addNameId(const char *name, size_t name_len)
  {
    auto hash = DBNameMap::string_hash(name, name_len);
    const int roc = !ro ? 0 : ro->nameCount();
    int id = roc ? ro->getNameId(name, name_len, hash) : -1; //-V1004
    if (id >= 0)
      return id;
    return rw.addNameId(name, name_len, hash) + roc;
  }
  int addNameId(const char *name) { return addNameId(name, strlen(name)); }

  DataBlock *getBlock(uint32_t i)
  {
    G_ASSERT(i < roDataBlocks);
    return getROBlockUnsafe(i);
  }
  const DataBlock *getBlock(uint32_t i) const
  {
    G_ASSERT(i < roDataBlocks);
    return getROBlockUnsafe(i);
  }

  const DataBlock *getROBlockUnsafe(uint32_t i) const { return ((const DataBlock *)getUnsafe(blocksStartsAt)) + i; }
  DataBlock *getROBlockUnsafe(uint32_t i) { return ((DataBlock *)getUnsafe(blocksStartsAt)) + i; }

  char *getUnsafe(uint32_t at) { return getDataUnsafe() + at; }
  const char *getUnsafe(uint32_t at) const { return getDataUnsafe() + at; }
  char *get(uint32_t at)
  {
    G_ASSERT(at < roDataSize());
    return getData() + at;
  }
  const char *get(uint32_t at) const
  {
    G_ASSERT(at < roDataSize());
    return getData() + at;
  }

  bool isROBlock(const DataBlock *db) const { return getROBlockUnsafe(0) <= db && db - getROBlockUnsafe(0) < roDataBlocks; }
  uint32_t roDataSize() const { return blocksStartsAt + roDataBlocks * sizeof(DataBlock) + sizeof(*this); }

  // protected:
  DBNameMapBase rw;
  const DBNameMap *ro = nullptr;
  uint32_t roDataBlocks = 0;
  uint32_t blocksStartsAt = 0;

  SimpleString srcFilename; // src filename
  void setSrc(const char *src) { srcFilename = src; }
  const char *getSrc() const { return !srcFilename.empty() ? srcFilename.str() : nullptr; }

  enum
  {
    F_ROBUST_LD = 1u << 0,
    F_ROBUST_OPS = 1u << 1,
    F_BINONLY_LD = 1u << 2,
    F_VALID = 1u << 3
  };
  unsigned blkFlags = F_VALID; // BLK property flags

  unsigned blkRobustLoad() const { return blkFlags & F_ROBUST_LD; }
  unsigned blkRobustOps() const { return blkFlags & F_ROBUST_OPS; }
  unsigned blkBinOnlyLoad() const { return blkFlags & F_BINONLY_LD; }
  unsigned blkValid() const { return blkFlags & F_VALID; }
  void setBlkFlag(unsigned f, bool v) { v ? blkFlags |= f : blkFlags &= ~f; }
  void setBlkRobustLoad(bool v) { setBlkFlag(F_ROBUST_LD, v); }
  void setBlkRobustOps(bool v) { setBlkFlag(F_ROBUST_OPS, v); }
  void setBlkBinOnlyLoad(bool v) { setBlkFlag(F_BINONLY_LD, v); }
  void setBlkValid(bool v) { setBlkFlag(F_VALID, v); }

#if DATABLOCK_USES_FIXED_BLOCK_ALLOCATOR
  FixedBlockAllocator blocksAllocator = {sizeof(DataBlock), 1};
  FixedBlockAllocator dataAllocator = {sizeof(DataBlockOwned), 1};

  void *allocateBlock() { return blocksAllocator.allocateOneBlock(); }
  void deallocateBlock(void *p) { blocksAllocator.freeOneBlock(p); }
  void *allocateData() { return dataAllocator.allocateOneBlock(); }
  void deallocateData(void *p) { dataAllocator.freeOneBlock(p); }
#else
  void *allocateBlock() { return memalloc(sizeof(DataBlock), midmem); }
  void deallocateBlock(void *p) { memfree_anywhere(p); }
  void *allocateData() { return memalloc(sizeof(DataBlockOwned), midmem); }
  void deallocateData(void *p) { memfree_anywhere(p); }
#endif

  void shrink_to_fit() { rw.shrink_to_fit(); }

  // instead of having additional indirection, we just allocate the 'rest' of DataBlockShared for data (as it is of known size)
  char *getDataUnsafe() { return ((char *)this + sizeof(DataBlockShared)); }
  const char *getDataUnsafe() const { return ((const char *)this + sizeof(DataBlockShared)); }

  char *getData()
  {
    G_ASSERT(roDataBlocks);
    return getDataUnsafe();
  }
  const char *getData() const
  {
    G_ASSERT(roDataBlocks);
    return getDataUnsafe();
  }

  friend class DataBlock;
};

__forceinline void DataBlock::insertNewParamRaw(uint32_t at, uint32_t name_id, uint32_t type, size_t type_sz, const char *nd)
{
  G_ASSERT(type != TYPE_STRING);
  G_ASSERT(isOwned());
  // const uint32_t pcnt = paramCount();
  // uint32_t paramsEnd = pcnt*sizeof(Param);
  // uint32_t paramsVal = paramsVal + pcnt*sizeof(uint32_t);
  Param p;
  p.nameId = name_id;
  p.type = type;
  if (type_sz <= INPLACE_PARAM_SIZE)
  {
    G_STATIC_ASSERT(INPLACE_PARAM_SIZE == sizeof(Param::v));
    p.v = 0;
    memcpy(&p.v, nd, type_sz);
    insertAt(at * sizeof(Param), sizeof(Param), (char *)&p);
    paramsCount++;
  }
  else
  {
    p.v = complexParamsSize();
    insertAt(at * sizeof(Param), sizeof(Param), (char *)&p);
    paramsCount++;
    insertAt(getUsedSize() + p.v, (uint32_t)type_sz, nd);
  }
}

template <bool rw>
DataBlock::Param &DataBlock::getParam(uint32_t i)
{
  G_ASSERT(i < paramCount());
  return getParams<rw>()[i];
}

template <bool rw>
const DataBlock::Param &DataBlock::getParam(uint32_t i) const
{
  G_ASSERT(i < paramCount());
  return getCParams<rw>()[i];
}

inline DataBlock::Param *DataBlock::getParamsImpl()
{
  return paramCount() ? (isOwned() ? getParams<true>() : getParams<false>()) : nullptr;
}
inline const DataBlock::Param *DataBlock::getParamsImpl() const
{
  return paramCount() ? (isOwned() ? getParams<true>() : getParams<false>()) : nullptr;
}

namespace dblk
{
enum FormatHeaderByte
{
  BBF_full_binary_in_stream = '\1',    // complete BLK with private namemap in binary stream follows to the end of the file
  BBF_full_binary_in_stream_z = '\2',  // 3 bytes (compressed data size) and then complete BLK with private namemap in ZSTD compressed
                                       // binary stream follows
  BBF_binary_with_shared_nm = '\3',    // BLK (using shared namemap) in binary stream follows to the end of the file
  BBF_binary_with_shared_nm_z = '\4',  // BLK (using shared namemap) in ZSTD compressed binary stream follows to the end of the file
  BBF_binary_with_shared_nm_zd = '\5', // BLK (using shared namemap) in ZSTD compressed (with dict) binary stream follows to the end of
                                       // the file
};

bool check_shared_name_map_valid(const VirtualRomFsData *fs, const char **out_err_desc);
bool add_name_to_name_map(DBNameMap &nm, const char *s);

bool read_names_base(IGenLoad &cr, DBNameMapBase &names, uint64_t *names_hash);
bool write_names_base(IGenSave &cwr, const DBNameMapBase &names, uint64_t *names_hash);

} // namespace dblk
