// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blk_shared.h"
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <debug/dag_debug.h>

#ifndef DATABLOCK_INTERCHANGEABLE_INT_INT64
#define DATABLOCK_INTERCHANGEABLE_INT_INT64 1
#endif

#ifndef DATABLOCK_ALIGNMENT
// our platforms supports unaligned reads and writes!
#define DATABLOCK_ALIGNMENT 1 // otherwise define as 4
#endif

void DataBlock::createData()
{
  G_ASSERT(!data);
  if (shared)
    data = new (shared->allocateData(), _NEW_INPLACE) DataBlockOwned;
}

bool DataBlock::topMost() const { return shared && (nameIdAndFlags & IS_TOPMOST); }

uint32_t DataBlock::complexParamsSize() const
{
  G_ASSERT(isOwned());
  return data->data.size() - ((isBlocksOwned() ? blocksCount * sizeof(block_id_t) : 0) + paramsCount * sizeof(Param));
}

DataBlock::DataBlock(IMemAlloc *) : shared(new DataBlockShared) { nameIdAndFlags |= IS_TOPMOST; }
DataBlock::DataBlock(const char *filename, IMemAlloc *a) : DataBlock(a) { load(filename); }
DataBlock::DataBlock(const DataBlock &b) : DataBlock()
{
  if (!b.hasNoNameId())
    changeBlockName(b.getBlockName());
  setFrom(&b);
}

DataBlock::DataBlock(DataBlockShared *s, const char *nm) : shared(s)
{
  createData();
  nameIdAndFlags |= (addNameId(nm) + 1);
}
DataBlock::DataBlock(DataBlockShared *s, int nid, uint16_t pcnt, uint16_t bcnt, uint32_t fBlock, uint32_t ofs_) :
  shared(s), nameIdAndFlags(nid + 1), paramsCount(pcnt), blocksCount(bcnt), firstBlockId(fBlock), ofs(ofs_)
{}

DataBlock::DataBlock(DataBlock &&a)
{
  if (a.topMost())
  {
    eastl::swap(shared, a.shared);
    eastl::swap(nameIdAndFlags, a.nameIdAndFlags);
    eastl::swap(paramsCount, a.paramsCount);
    eastl::swap(blocksCount, a.blocksCount);
    eastl::swap(firstBlockId, a.firstBlockId);
    eastl::swap(ofs, a.ofs);
    eastl::swap(data, a.data);
  }
  else
  {
    shared = new DataBlockShared;
    nameIdAndFlags |= IS_TOPMOST;
    setFrom(&a);
  }
}

DataBlock &DataBlock::operator=(DataBlock &&a)
{
  if (a.topMost())
  {
    eastl::swap(shared, a.shared);
    eastl::swap(nameIdAndFlags, a.nameIdAndFlags);
    eastl::swap(paramsCount, a.paramsCount);
    eastl::swap(blocksCount, a.blocksCount);
    eastl::swap(firstBlockId, a.firstBlockId);
    eastl::swap(ofs, a.ofs);
    eastl::swap(data, a.data);
  }
  else
  {
    reset();
    setFrom(&a);
  }
  return *this;
}


DataBlock::~DataBlock()
{
  if (!shared && !data)
    return;
  for (uint32_t i = 0, e = blockCount(); i < e; ++i)
  {
    auto db = getBlock(i);
    db->~DataBlock();
    if (!shared->isROBlock(db))
      shared->deallocateBlock(db);
#if DAGOR_DBGLEVEL > 0
    else
      memset(db, 0x7F, sizeof(DataBlock));
#endif
  }
  deleteShared();
}

void DataBlock::deleteShared()
{
  if (data)
  {
    G_ASSERT(shared);
    data->~DataBlockOwned();
    shared->deallocateData(data);
    data = NULL;
  }
  if (topMost())
  {
    if (shared->ro)
      shared->ro->delRef();
    del_it(shared);
  }
}

const DataBlock DataBlock::emptyBlock((IMemAlloc *)nullptr);

const DataBlock::Param *DataBlock::getParamsPtr() const { return getParamsImpl(); }
const DataBlock *const *DataBlock::getBlockRWPtr() const
{
  return isBlocksOwned() && blocksCount ? &const_cast<DataBlock *>(this)->getRW<block_id_t>(blocksOffset()) : nullptr;
}
const DataBlock *DataBlock::getBlockROPtr() const
{
  return !isBlocksOwned() && blocksCount ? shared->getROBlockUnsafe(firstBlock()) : nullptr;
}

int DataBlock::getParamType(uint32_t i) const
{
  return i < paramCount() ? (isOwned() ? getParam<true>(i).type : getParam<false>(i).type) : -1;
}

int DataBlock::getParamNameId(uint32_t i) const
{
  return i < paramCount() ? (isOwned() ? getParam<true>(i).nameId : getParam<false>(i).nameId) : -1;
}

char *DataBlock::insertAt(uint32_t at, uint32_t n)
{
  G_ASSERT(data);
  return data->insertAt(at, n);
}
char *DataBlock::insertAt(uint32_t at, uint32_t n, const char *v)
{
  G_ASSERT(data);
  return data->insertAt(at, n, v);
}

const char *DataBlock::getParamData(const Param &p) const
{
  const bool rw = isOwned();
  const uint32_t &pv = getParamV(p);
  if (p.type == TYPE_STRING)
    return rw ? getParamString<string_t, true>(pv) : getParamString<string_t, false>(pv);
  return dblk::get_type_size(p.type) <= INPLACE_PARAM_SIZE ? (const char *)(&pv) : (rw ? rwDataAt(getUsedSize() + pv) : roDataAt(pv));
}

template <class T, bool rw>
T DataBlock::getByNameId(int paramNameId, const T &def) const
{
  const int id = findParam<rw>(paramNameId);
  if (id < 0)
    return def;
  return get<T, rw>(id, def);
}

template <class T, bool rw>
__forceinline T DataBlock::get(uint32_t param_idx, const T &def) const
{
  const Param &p = getParam<rw>(param_idx);
  const uint32_t &pv = getParamV(p);
  if (DAGOR_UNLIKELY(p.type != TypeOf<T>::type))
  {
#if DATABLOCK_INTERCHANGEABLE_INT_INT64
    // this is hack to allow getInt with int64_t parameter
    //  I am not sure if it is ever needed, and it's messy
    if (TypeOf<T>::type == TYPE_INT && p.type == TYPE_INT64)
    {
      int64_t v = memcpy_cast<int64_t>(rw ? rwDataAt(getUsedSize() + pv) : roDataAt(pv));
      if (DAGOR_LIKELY(v == int32_t(v)))
        return memcpy_cast<T, int32_t>(v);
    }
    else if (TypeOf<T>::type == TYPE_INT64 && p.type == TYPE_INT)
      return memcpy_cast<T, int64_t>(pv);
#if DAGOR_DBGLEVEL > 0
    issue_error_bad_type_get(getNameId(), p.nameId, TypeOf<T>::type, p.type);
#endif
#endif
    return def;
  }
  if (TypeOf<T>::type == TYPE_STRING) // const expression
    return getParamString<T, rw>(pv);
  if (sizeof(T) <= INPLACE_PARAM_SIZE)
    return memcpy_cast<T, uint32_t>(pv);
  return memcpy_cast<T>(rw ? rwDataAt(getUsedSize() + pv) : roDataAt(pv));
}

template <class T, bool rw>
__forceinline T DataBlock::get(uint32_t param_idx) const
{
  return get<T, rw>(param_idx, T());
}

template <class T>
int DataBlock::insertParamAt(uint32_t at, uint32_t name_id, const T &v)
{
  // insert at the end, memmove params data, update blocks ofs
  G_ASSERT(isOwned());
  insertNewParamRaw(at, name_id, TypeOf<T>::type, sizeof(T), (const char *)&v);
  return at;
}

template <>
int DataBlock::insertParamAt(uint32_t at, uint32_t name_id, const string_t &v)
{
  // insert at the end, memmove params data, update blocks ofs
  G_ASSERT(isOwned());
  Param p;
  p.nameId = name_id;
  p.type = TypeOf<string_t>::type;
  p.v = insertNewString(v ? v : "", v ? strlen(v) : 0);
  insertAt(at * sizeof(Param), sizeof(Param), (char *)&p);
  paramsCount++;
  return at;
}
template <class T, bool rw>
T DataBlock::getParamString(int pv) const
{
  if (is_string_id_in_namemap(pv)) //
    return return_string<T>(shared->getName(namemap_id_from_string_id(pv)));
  return return_string<T>(rw ? shared->rw.allocator.getDataRawUnsafe(pv) : roDataAt(pv));
}

uint32_t DataBlock::allocateNewString(string_t v, size_t vlen) { return shared->rw.allocator.addDataRaw(v, vlen + 1); }

uint32_t DataBlock::insertNewString(string_t v, size_t vlen)
{
  int id = shared->getNameId(v, vlen);
  if (id >= 0)
    return id | IS_NAMEMAP_ID;
  return allocateNewString(v, vlen);
}

template <class T>
bool DataBlock::set(uint32_t param_idx, const T &v)
{
  G_ASSERT(isOwned());
  Param &p = getParam<true>(param_idx);
  if (DAGOR_UNLIKELY(p.type != TypeOf<T>::type))
  {
    issue_deprecated_type_change(p.nameId, TypeOf<T>::type, p.type);
    return false;
  }
  uint32_t &pv = getParamV(p);
  if (sizeof(T) <= INPLACE_PARAM_SIZE)
    memcpy(&pv, &v, sizeof(T));
  else
    memcpy(rwDataAt(getUsedSize() + pv), &v, sizeof(T));
  return true;
}

#if DATABLOCK_INTERCHANGEABLE_INT_INT64
template <>
bool DataBlock::set(uint32_t param_idx, const int &v)
{
  G_ASSERT(isOwned());
  Param &p = getParam<true>(param_idx);
  if (DAGOR_LIKELY(p.type == TYPE_INT))
    p.v = v;
  else if (DAGOR_LIKELY(p.type == TYPE_INT64))
    getRW<int64_t>(getUsedSize() + getParamV(p)) = v;
  else
  {
    issue_deprecated_type_change(p.nameId, TYPE_INT, p.type);
    return false;
  }
  return true;
}

template <>
bool DataBlock::set(uint32_t param_idx, const int64_t &v)
{
  G_ASSERT(isOwned());
  Param &p = getParam<true>(param_idx);
  if (DAGOR_LIKELY(p.type == TYPE_INT64))
    getRW<int64_t>(getUsedSize() + getParamV(p)) = v;
  else if (DAGOR_LIKELY(p.type == TYPE_INT))
  {
    if (DAGOR_LIKELY(int(v) == v))
      p.v = v;
    else
    {
      uint32_t at = isBlocksOwned() ? blocksOffset() : data->data.size();
      p.type = TYPE_INT64;
      p.v = at - getUsedSize();
      data->insertAt(at, sizeof(v), (const char *)&v);
    }
  }
  else
  {
    issue_deprecated_type_change(p.nameId, TYPE_INT64, p.type);
    return false;
  }
  return true;
}
#endif // DATABLOCK_INTERCHANGEABLE_INT_INT64

template <>
bool DataBlock::set(uint32_t param_idx, const string_t &v)
{
  G_ASSERT(isOwned());
  Param &p = getParam<true>(param_idx);
  if (p.type != TypeOf<string_t>::type)
    return false;
  // use only owned data after that
  uint32_t &pv = getParamV(p);
  const size_t vlen = v ? strlen(v) : 0;
  const char *curStr = getParamString<string_t, true>(pv);
  // strings are unaligned with that. All our platforms supports unaligned read
  if (is_string_id_in_namemap(pv) || vlen > strlen(curStr)) // if string id is namemap id or string cant fit 'inplace'
  {
    // add new string
    // memset(curStr, 0, slen);//fill with zeroes. Not needed, just in case we will save the whole memory.
    pv = insertNewString(v ? v : "", vlen);
  }
  else
  {
    // replace existing string. String can only shrink.
    memcpy(const_cast<char *>(curStr), v ? v : "", (vlen + 1));
  }
  return true;
}

template <class T, bool check_name_id>
int DataBlock::setByNameId(int paramNameId, const T &val)
{
  G_ASSERT(isOwned());
  const int id = findParam<true>(paramNameId);
  if (id < 0)
    return addByNameId<T, check_name_id>(paramNameId, val);
  set<T>(id, val);
  return id;
}

template <class T, bool check_name_id>
int DataBlock::addByNameId(int name_id, const T &val)
{
  if (check_name_id && !shared->nameExists(name_id))
    return -1;

  if (!shared->blkRobustOps())
  {
    const int pid = findParam<true>(name_id);
    if (pid >= 0 && getParam<true>(pid).type != TypeOf<T>::type)
      issue_error_bad_type(name_id, TypeOf<T>::type, getParam<true>(pid).type);
  }
  return insertParamAt<T>(paramCount(), uint32_t(name_id), val);
}

#define TYPE_FUNCTION_3(CppType, CRefType, ApiName)                        \
  CppType DataBlock::get##ApiName(int pidx) const                          \
  {                                                                        \
    return uint32_t(pidx) < paramCount() ? get<CppType>(pidx) : CppType(); \
  }                                                                        \
  CppType DataBlock::get##ApiName(const char *name, CRefType def) const    \
  {                                                                        \
    return getByName<CppType>(name, def);                                  \
  }                                                                        \
  CppType DataBlock::get##ApiName(const char *name) const                  \
  {                                                                        \
    return getByName<CppType>(name);                                       \
  }                                                                        \
  CppType DataBlock::get##ApiName##ByNameId(int pnid, CRefType def) const  \
  {                                                                        \
    return getByNameId<CppType>(pnid, def);                                \
  }                                                                        \
  bool DataBlock::set##ApiName(int pidx, CRefType v)                       \
  {                                                                        \
    if (uint32_t(pidx) >= paramCount())                                    \
      return false;                                                        \
    toOwned();                                                             \
    return set<CppType>(pidx, v);                                          \
  }                                                                        \
  int DataBlock::set##ApiName##ByNameId(int nid, CRefType v)               \
  {                                                                        \
    toOwned();                                                             \
    return setByNameId<CppType, true>(nid, v);                             \
  }                                                                        \
  int DataBlock::set##ApiName(const char *name, CRefType v)                \
  {                                                                        \
    toOwned();                                                             \
    return setByName<CppType>(name, v);                                    \
  }                                                                        \
  int DataBlock::add##ApiName(const char *name, CRefType v)                \
  {                                                                        \
    toOwned();                                                             \
    return addByName<CppType>(name, v);                                    \
  }                                                                        \
  int DataBlock::addNew##ApiName##ByNameId(int nid, CRefType v)            \
  {                                                                        \
    toOwned();                                                             \
    return addByNameId<CppType, true>(nid, v);                             \
  }

#define TYPE_FUNCTION(CppType, ApiName)    TYPE_FUNCTION_3(CppType, CppType, ApiName)
#define TYPE_FUNCTION_CR(CppType, ApiName) TYPE_FUNCTION_3(CppType, const CppType &, ApiName)
TYPE_FUNCTION(DataBlock::string_t, Str)
TYPE_FUNCTION(int, Int)
TYPE_FUNCTION(E3DCOLOR, E3dcolor)
TYPE_FUNCTION(int64_t, Int64)
TYPE_FUNCTION(float, Real)
TYPE_FUNCTION(bool, Bool)
TYPE_FUNCTION_CR(Point2, Point2)
TYPE_FUNCTION_CR(Point3, Point3)
TYPE_FUNCTION_CR(Point4, Point4)
TYPE_FUNCTION_CR(IPoint2, IPoint2)
TYPE_FUNCTION_CR(IPoint3, IPoint3)
TYPE_FUNCTION_CR(TMatrix, Tm)
#undef TYPE_FUNCTION
#undef TYPE_FUNCTION_CR
#undef TYPE_FUNCTION_3

int DataBlock::findBlockRO(int nid, int after) const
{
  const uint32_t nameIdIncreased = nid + 1;
  for (int fb = firstBlock(), i = fb + eastl::max((int)0, after + 1), e = fb + blockCount(); i < e; ++i)
  {
    auto db = shared->getROBlockUnsafe(i);
    if (db->getNameIdIncreased() == nameIdIncreased)
      return i - fb;
  }
  return -1;
}

uint32_t DataBlock::getUsedSize() const
{
  G_ASSERT(isOwned());
  return paramCount() * sizeof(Param);
}

uint32_t DataBlock::blocksOffset() const
{
  G_ASSERT(isBlocksOwned());
  return data->data.size() - blocksCount * sizeof(block_id_t);
}

int DataBlock::findBlockRW(int nid, int after) const
{
  after = eastl::max((int)0, after + 1);
  const block_id_t *blockId = ((const block_id_t *)rwDataAt(blocksOffset())) + after;
  const uint32_t nameIdIncreased = nid + 1;
  for (uint32_t i = after, e = blockCount(); i < e; ++i, ++blockId)
    if ((*blockId)->getNameIdIncreased() == nameIdIncreased)
      return i;
  return -1;
}

int DataBlock::findBlock(int nid, int after) const { return isBlocksOwned() ? findBlockRW(nid, after) : findBlockRO(nid, after); }

int DataBlock::findBlockRev(int name_id, int start_before) const
{
  if (start_before <= 0 || !blocksCount)
    return -1;
  if (start_before > blocksCount)
    start_before = blocksCount;
  const uint32_t nameIdIncreased = name_id + 1;
  if (const DataBlock *b = getBlockROPtr())
  {
    for (const DataBlock *__restrict i = b + start_before - 1, *__restrict e = b - 1; i != e; --i)
      if (i->getNameIdIncreased() == nameIdIncreased)
        return i - b;
  }
  else if (const DataBlock *const *b = getBlockRWPtr())
  {
    for (const DataBlock *const *__restrict i = b + start_before - 1, *const *__restrict e = b - 1; i != e; --i)
      if ((*i)->getNameIdIncreased() == nameIdIncreased)
        return i - b;
  }
  return -1;
}

int DataBlock::blockCountById(int nid) const
{
  if (!blocksCount)
    return 0;
  uint32_t res = 0;
  const uint32_t nameIdIncreased = nid + 1;
  if (const DataBlock *__restrict b = getBlockROPtr())
  {
    for (const DataBlock *__restrict be = b + blockCount(); b < be; b++)
      if (b->getNameIdIncreased() == nameIdIncreased)
        res++;
  }
  else if (const DataBlock *const *__restrict b = getBlockRWPtr())
  {
    for (const DataBlock *const *__restrict be = b + blockCount(); b < be; b++)
      if ((*b)->getNameIdIncreased() == nameIdIncreased)
        res++;
  }
  return res;
}

void DataBlock::changeBlockName(const char *name)
{
  G_ASSERT(this != &emptyBlock);
  G_ASSERT(name && *name);
  if (name && *name)
    nameIdAndFlags = (addNameId(name) + 1) | (nameIdAndFlags & IS_TOPMOST);
}

DataBlock *DataBlock::getBlockByName(int name_id, int start_after, bool expect_single)
{
#if DAGOR_DBGLEVEL > 0
  if (expect_single && singleBlockChecking)
    for (uint32_t i = 0, cnt = 0, e = blockCount(); i < e && cnt < 2; ++i)
      if (getBlock(i)->getNameId() == name_id && ++cnt > 1)
        logerr("BLK: more than one block '%s' in <%s>", getBlock(i)->getBlockName(), resolveFilename());
#else
  (void)(expect_single);
#endif
  return isBlocksOwned() ? getBlockRW(findBlockRW(name_id, start_after)) : getBlockRO(findBlockRO(name_id, start_after));
}

const DataBlock *DataBlock::getBlockByName(int name_id, int start_after, bool expect_single) const
{
  return const_cast<const DataBlock *>(const_cast<DataBlock *>(this)->getBlockByName(name_id, start_after, expect_single));
}

DataBlock *DataBlock::addBlock(const char *name)
{
  G_ASSERT(this != &emptyBlock);
  DataBlock *blk = getBlockByName(getNameId(name));
  if (blk)
    return blk;
  return addNewBlock(name);
}

DataBlock *DataBlock::addNewBlock(const char *name)
{
  G_ASSERT(this != &emptyBlock);
  convertToBlocksOwned();
  const uint32_t bCnt = blockCount();
  constexpr auto maxBlocksCount = eastl::numeric_limits<decltype(blocksCount)>::max();
  G_ASSERTF_RETURN(bCnt < maxBlocksCount, getBlock(bCnt - 1),
    "DataBlock::addNewBlock(%s) blockCount=%d reaches limit of %d max blocks", name, bCnt, maxBlocksCount);
  auto nb = new (shared->allocateBlock(), _NEW_INPLACE) DataBlock(shared, name);
  insertAt(data->data.size(), sizeof(block_id_t), (const char *)&nb);
  blocksCount++;
  return nb;
}

DataBlock *DataBlock::addNewBlock(const DataBlock *blk, const char *as_name)
{
  if (!blk)
    return NULL;
  G_ASSERT(this != &emptyBlock);

  DataBlock *newBlk = addNewBlock(as_name ? as_name : blk->getBlockName());
  G_ASSERT(newBlk);

  newBlk->setParamsFrom(blk);

  // add sub-blocks
  for (uint32_t i = 0, ie = blk->blockCount(); i < ie; ++i)
    newBlk->addNewBlock(blk->getBlock(i));
  return newBlk;
}


void DataBlock::setFrom(const DataBlock *from, const char *src_fname)
{
  if (from == this)
    return;
  G_ASSERT(this != &emptyBlock);

  clearData();
  if (!from)
    return;

  if (src_fname)
  {
    G_ASSERT(topMost() && "setFrom() may set src filename only to root BLK");
    shared->setSrc(src_fname);
  }

  setParamsFrom(from);
  for (uint32_t i = 0, ie = from->blockCount(); i < ie; ++i)
    addNewBlock(from->getBlock(i));
}

void DataBlock::addParamsTo(DataBlock &dest) const
{
  if (paramCount())
    dest.toOwned();
  auto oldParams = getParamsImpl();
  for (uint32_t i = 0, e = paramCount(); i < e; ++i, ++oldParams)
  {
    const Param &p = *oldParams;
    int name_id = dest.addNameId(getName(p.nameId));
    const char *pd = getParamData(p);
    if (!dest.shared->blkRobustOps())
    {
      const int pid = dest.findParam<true>(name_id);
      if (pid >= 0 && dest.getParam<true>(pid).type != p.type)
        dest.issue_error_bad_type(name_id, p.type, dest.getParam<true>(pid).type);
    }
    if (p.type == TYPE_STRING)
      dest.insertParamAt<string_t>(dest.paramCount(), name_id, pd);
    else
      dest.insertNewParamRaw(dest.paramCount(), name_id, p.type, dblk::get_type_size(p.type), pd);
  }
}
void DataBlock::appendParamsFrom(const DataBlock *blk) { blk->addParamsTo(*this); }

DataBlock *DataBlock::getBlockRW(uint32_t i)
{
  G_ASSERT(isBlocksOwned());
  if (i < blockCount())
    return getRW<block_id_t>(blocksOffset() + i * sizeof(block_id_t));
  return NULL;
}

DataBlock *DataBlock::getBlockRO(uint32_t i)
{
  G_ASSERT(!isBlocksOwned());
  if (i < blockCount())
    return shared->getBlock(firstBlock() + i);
  return NULL;
}

DataBlock *DataBlock::getBlock(uint32_t i) { return isBlocksOwned() ? getBlockRW(i) : getBlockRO(i); }

const DataBlock *DataBlock::getBlock(uint32_t i) const
{
  return const_cast<const DataBlock *>(const_cast<DataBlock *>(this)->getBlock(i));
}

const char *DataBlock::roDataAt(uint32_t at) const { return shared->getUnsafe(at); }
const char *DataBlock::rwDataAt(uint32_t at) const
{
  G_ASSERT(data);
  return data->getUnsafe(at);
}
char *DataBlock::rwDataAt(uint32_t at)
{
  G_ASSERT(data);
  return data->getUnsafe(at);
}

int DataBlock::addNameId(const char *name) { return name && shared ? shared->addNameId(name) : -1; }
int DataBlock::getNameId(const char *name) const { return name && shared ? shared->getNameId(name) : -1; }
const char *DataBlock::getName(int name_id) const { return name_id >= 0 && shared ? shared->getName(name_id) : nullptr; }

void DataBlock::convertToOwned()
{
  G_ASSERT(!isOwned());
  if (!data)
    createData();
  if (paramCount())
  {
    auto oldParams = getParams<false>();
    const uint32_t paramsSize = sizeof(Param) * paramCount();
    uint32_t complexParams = 0;
    insertAt(0, paramsSize);
    for (uint32_t i = 0, e = paramCount(); i < e; ++i, ++oldParams)
    {
      Param *cParam = (Param *)rwDataAt(i * sizeof(Param));
      *cParam = *oldParams;
      const uint32_t sz = dblk::get_type_size(cParam->type);
      if (sz > INPLACE_PARAM_SIZE)
      {
        if (cParam->type == TYPE_STRING)
        {
          if (!is_string_id_in_namemap(cParam->v)) // otherwise it is interned already
          {
            const char *paramData = getParamData(*oldParams);
            cParam->v = allocateNewString(paramData, strlen(paramData)); // intern string
          }
        }
        else
        {
          const char *paramData = getParamData(*oldParams);
          cParam->v = complexParams;
          insertAt(paramsSize + complexParams, sz, paramData);
          complexParams += sz;
        }
      }
    }
  }
  ofs = IS_OWNED;
}

void DataBlock::convertToBlocksOwned()
{
  if (isBlocksOwned())
    return;
  if (!data)
    createData();
  // convert all blocks
  block_id_t *blocks = (block_id_t *)insertAt(data->data.size(), blockCount() * sizeof(block_id_t));
  for (uint32_t i = firstBlock(), e = i + blockCount(); i < e; ++i, ++blocks)
    *blocks = shared->getROBlockUnsafe(i);
  firstBlockId = IS_OWNED;
}

struct DbUtils
{
  __forceinline static int find(int name_id, const DataBlock::Param *__restrict s, const DataBlock::Param *__restrict e)
  {
    for (const DataBlock::Param *__restrict i = s; i != e; ++i)
      if (i->nameId == name_id)
        return i - s;
    return -1;
  }

  __forceinline static int find(int name_id, int start_after, const DataBlock::Param *__restrict params,
    const DataBlock::Param *__restrict e)
  {
    start_after = max(start_after + 1, int(0));
    const DataBlock::Param *__restrict s = params + start_after;
    for (; s < e; ++s)
      if (s->nameId == name_id)
        return s - params;
    return -1;
  }
};

template <bool rw>
int DataBlock::findParam(int name_id) const
{
  if (!paramCount() || name_id < 0)
    return -1;
  const Param *s = getParams<rw>(), *e = s + paramCount();
  return DbUtils::find(name_id, s, e);
}
int DataBlock::findParam(int name_id) const { return isOwned() ? findParam<true>(name_id) : findParam<false>(name_id); }

int DataBlock::findParamRev(int name_id, int start_before) const
{
  if (start_before <= 0 || !paramsCount)
    return -1;
  const Param *ptr = getParamsImpl();
  if (start_before > paramsCount)
    start_before = paramsCount;
  for (const DataBlock::Param *__restrict i = ptr + start_before - 1, *__restrict e = ptr - 1; i != e; --i)
    if (i->nameId == name_id)
      return i - ptr;
  return -1;
}

int DataBlock::paramCountById(int nid) const
{
  uint32_t res = 0;
  for (const Param *s = getParamsImpl(), *e = s + paramCount(); s < e; s++)
    if (s->nameId == nid)
      res++;
  return res;
}

void DataBlock::changeParamName(uint32_t i, const char *name)
{
  if (i < paramCount())
    (isOwned() ? getParam<true>(i) : getParam<false>(i)).nameId = addNameId(name);
}

template <bool rw>
int DataBlock::findParam(int name_id, int start_after) const
{
  if (!paramCount() || name_id < 0)
    return -1;
  const Param *s = getParams<rw>(), *e = s + paramCount();
  return DbUtils::find(name_id, start_after, s, e);
}

int DataBlock::findParam(int name_id, int start_after) const
{
  return isOwned() ? findParam<true>(name_id, start_after) : findParam<false>(name_id, start_after);
}

void DataBlock::shrink()
{
  // compress
  if (!data)
    return;
  if (isOwned())
  {
    decltype(data->data) newData;
    newData.reserve(data->data.size());
    if (isBlocksOwned())
    {
      const uint32_t oldBlocksOffset = blocksOffset();
      newData.resize(sizeof(block_id_t) * blocksCount);
      memcpy(newData.data(), data->data.data() + oldBlocksOffset, newData.size());
    }
    const Param *oldParams = isOwned() ? getCParams<true>() : getCParams<false>();
    // uint32_t newComplexParamsCapacity = 0;
    uint16_t newParamsCount = 0;
    // todo: optimize it:make resize
    for (int i = 0, e = paramCount(); i < e; ++i, ++oldParams)
    {
      const Param p = *oldParams;
      const char *paramData = getParamData(p);
      eastl::swap(newData, data->data);
      eastl::swap(newParamsCount, paramsCount);

      if (p.type == TYPE_STRING)
        insertParamAt<string_t>(i, p.nameId, paramData);
      else
        insertNewParamRaw(i, p.nameId, p.type, dblk::get_type_size(p.type), paramData);
      // debug("pid=%d used=%d (%d), params = %d", i, getUsedSize(), data->data.size(), paramCount());
      eastl::swap(newData, data->data);
      eastl::swap(newParamsCount, paramsCount);
    }
    eastl::swap(newData, data->data);
    eastl::swap(newParamsCount, paramsCount);
    // debug("2 getUsedSize()=%d blocks = %d", getUsedSize(), blocks);
    data->data.shrink_to_fit();
  }
  for (int i = 0, e = blockCount(); i < e; ++i)
    getBlock(i)->shrink();
  if (topMost())
    shared->shrink_to_fit();
}

size_t DataBlock::memUsed_() const
{
  size_t sz = (bool(data) ? data->data.capacity() + sizeof(DataBlockOwned) + sizeof(DataBlock) : 0);
  for (int i = 0, e = blockCount(); i < e; ++i)
    if (auto d = getBlock(i))
      sz += d->memUsed_();
  return sz;
}

size_t DataBlock::memUsed() const { return memUsed_() + (shared ? shared->roDataSize() + sizeof(DataBlockShared) : 0); }

bool DataBlock::removeBlock(uint32_t idx)
{
  if (idx >= blockCount())
    return false;
  convertToBlocksOwned(); // we have to convert, as pointers can not change
  block_id_t *db = ((block_id_t *)rwDataAt(blocksOffset())) + idx;

  (*db)->~DataBlock();
  if (!shared->isROBlock(*db))
    shared->deallocateBlock(*db);
#if DAGOR_DBGLEVEL > 0
  else
    memset(*db, 0x7E, sizeof(DataBlock));
#endif
  memmove(db, db + 1, (blocksCount - 1 - idx) * sizeof(block_id_t));
  data->data.resize(data->data.size() - sizeof(block_id_t));
  blocksCount--;
  return true;
}

bool DataBlock::removeBlock(const char *name)
{
  if (!blockCount())
    return false;
  int nid = getNameId(name);
  if (nid < 0)
    return false;
  int bidx = findBlockRev(nid, blockCount());
  if (bidx >= 0)
  {
    do
    {
      removeBlock(bidx);
    } while ((bidx = findBlockRev(nid, bidx)) >= 0);
    return true;
  }
  return false;
}

bool DataBlock::swapBlocks(uint32_t i1, uint32_t i2)
{
  if (i1 >= blockCount() || i2 >= blockCount())
    return false;
  if (i1 == i2)
    return true;

  convertToBlocksOwned();

  block_id_t *b1 = ((block_id_t *)rwDataAt(blocksOffset())) + i1;
  block_id_t *b2 = ((block_id_t *)rwDataAt(blocksOffset())) + i2;
  eastl::swap(*b1, *b2);

  return true;
}

bool DataBlock::removeParam(uint32_t idx)
{
  if (uint32_t(idx) >= (uint32_t)paramCount())
    return false;
  Param *p = getParamsImpl();
  p += idx;
  if (isOwned())
  {
    memmove(p, p + 1, data->data.data() + data->data.size() - (const char *)(p + 1));
    data->data.resize(data->data.size() - sizeof(Param));
  }
  else
    memmove(p, p + 1, (paramsCount - 1 - idx) * sizeof(Param));
  paramsCount--;
  return true;
}

bool DataBlock::removeParam(const char *name)
{
  if (!paramCount())
    return false;
  int nid = getNameId(name);
  if (nid < 0)
    return false;
  int pidx = findParamRev(nid, paramCount());
  if (pidx >= 0)
  {
    do
    {
      removeParam(pidx);
    } while ((pidx = findParamRev(nid, pidx)) >= 0);
    return true;
  }
  return false;
}

bool DataBlock::swapParams(uint32_t i1, uint32_t i2)
{
  if (i1 >= paramCount() || i2 >= paramCount())
    return false;
  if (i1 == i2)
    return true;

  toOwned();

  DataBlock::Param *p1 = getParams<true>() + i1;
  DataBlock::Param *p2 = getParams<true>() + i2;
  eastl::swap(*p1, *p2);

  return true;
}

void DataBlock::clearData()
{
  for (uint32_t i = 0, e = blockCount(); i < e; ++i)
  {
    auto db = getBlock(i);
    db->~DataBlock();
    if (!shared->isROBlock(db))
      shared->deallocateBlock(db);
  }
  if (data)
    data->data.clear();
  paramsCount = blocksCount = 0;
  firstBlockId = 0;
  ofs = 0; // RO param data starts here.
}

void DataBlock::clearParams()
{
  if (isOwned())
  {
    if (isBlocksOwned())
      data->data.erase(data->data.data(), data->data.data() + blocksOffset());
    else
      data->data.clear();
  }
  else
    ofs = 0;
  paramsCount = 0;
}

void DataBlock::reset()
{
  G_ASSERTF_RETURN(topMost() || hasNoNameId(), , "trying to reset child datablock: nameId=%d topMost=%d", getNameId(), topMost());
  nameIdAndFlags &= ~NAME_ID_MASK;
  if (topMost())
    shared->rw.reset();
  clearData();
}

void DataBlock::resetAndReleaseRoNameMap()
{
  reset();
  if (shared && shared->ro)
  {
    shared->ro->delRef();
    shared->ro = nullptr;
  }
}

void DataBlock::setSharedNameMapAndClearData(DataBlock *owner_of_shared_namemap)
{
  G_ASSERT_RETURN(hasNoNameId() && "setSharedNameMapAndClearData() must be called for root datablock", );
  reset();

  if (owner_of_shared_namemap)
  {
    deleteShared();
    shared = owner_of_shared_namemap->shared;
    nameIdAndFlags &= ~IS_TOPMOST;
  }
  else if (!topMost())
  {
    shared = new DataBlockShared;
    nameIdAndFlags |= IS_TOPMOST;
  }
}

bool DataBlock::operator==(const DataBlock &rhs) const
{
  if (&rhs == this)
    return true;
  if (paramCount() != rhs.paramCount() || blockCount() != rhs.blockCount())
    return false;
  // todo: comparison of blocks can be optimized if we know they have same shared, or at least shared->ro
  const char *my_name = getName(getNameId()), *rhs_name = rhs.getName(rhs.getNameId());
  if (!!my_name != !!rhs_name || (my_name && strcmp(my_name, rhs_name) != 0)) //-V575
    return false;

  auto param1 = getParamsImpl();
  auto param2 = rhs.getParamsImpl();
  for (uint32_t i = 0, e = paramCount(); i < e; ++i, ++param1, ++param2)
  {
    if (param1->type != param2->type || strcmp(getName(param1->nameId), rhs.getName(param2->nameId)) != 0) //-V575
      return false;

    bool eq = false;
    if (param1->type == TYPE_STRING)
      eq = (strcmp(getParamData(*param1), rhs.getParamData(*param2)) == 0);
    else
      eq = memcmp(getParamData(*param1), rhs.getParamData(*param2), dblk::get_type_size(param1->type)) == 0;

    if (!eq)
      return false;
  }

  for (uint32_t i = 0, ie = blockCount(); i < ie; ++i)
    if (*getBlock(i) != *rhs.getBlock(i))
      return false;

  return true;
}

#define INSTANCIATE_GET_BY_NAME_ID(CLS)                                                   \
  template CLS DataBlock::getByNameId<CLS, false>(int paramNameId, const CLS &def) const; \
  template CLS DataBlock::getByNameId<CLS, true>(int paramNameId, const CLS &def) const
INSTANCIATE_GET_BY_NAME_ID(int);
INSTANCIATE_GET_BY_NAME_ID(bool);
INSTANCIATE_GET_BY_NAME_ID(int64_t);
INSTANCIATE_GET_BY_NAME_ID(E3DCOLOR);
INSTANCIATE_GET_BY_NAME_ID(float);
INSTANCIATE_GET_BY_NAME_ID(DataBlock::string_t);
INSTANCIATE_GET_BY_NAME_ID(Point2);
INSTANCIATE_GET_BY_NAME_ID(Point3);
INSTANCIATE_GET_BY_NAME_ID(Point4);
INSTANCIATE_GET_BY_NAME_ID(IPoint2);
INSTANCIATE_GET_BY_NAME_ID(IPoint3);
INSTANCIATE_GET_BY_NAME_ID(TMatrix);
#undef INSTANCIATE_GET_BY_NAME_ID

#define EXPORT_PULL dll_pull_iosys_datablock_core
#include <supp/exportPull.h>
