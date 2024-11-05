// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blk_shared.h"
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <hash/mum_hash.h>
#include <ioSys/dag_memIo.h>
#include "blk_iterate.h"
#include <util/le2be.h>
#include <ioSys/dag_roDataBlock.h>
#include "blk_comments_def.h"
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>
#include <stdio.h> // sprintf

#define BLK_MAX_MEMORY_FOR_NAMES (256 << 20) // 256 MB

static void writeIndent(IGenSave &cb, int n)
{
  static const char *space8 = "        ";
  for (; n >= 8; n -= 8)
    cb.write(space8, 8);
  if (n > 0)
    cb.write(space8, n);
}

static void writeString(IGenSave &cb, const char *s)
{
  if (!s || !*s)
    return;
  int l = i_strlen(s);
  cb.write(s, l);
}

static void writeStringValue(IGenSave &cb, const char *s)
{
  if (!s)
    s = "";

  const char *quot = "\"";
  int quot_len = 1;
  {
    // choose quotation type to minimize escaping
    bool has_ln = false, has_quot = false, has_tick = false;
    for (const char *p = s; *p; ++p)
    {
      if (*p == '\n' || *p == '\r')
        has_ln = true;
      else if (*p == '\"')
        has_quot = true;
      else if (*p == '\'')
        has_tick = true;
      else
        continue;
      if (has_ln && has_quot && has_tick)
        break;
    }
    if (has_ln && !has_quot)
      quot_len = 4, quot = "\"\"\"\n\"\"\"";
    else if (has_ln && !has_tick)
      quot_len = 4, quot = "'''\n'''";
    else if (has_ln)
      quot_len = 4, quot = "\"\"\"\n\"\"\"";
    else if (has_quot && !has_tick)
      quot = "'";
  }

  cb.write(quot, quot_len);

  for (; *s; ++s)
  {
    char c = *s;
    if (c == '~')
      cb.write("~~", 2);
    else if (c == quot[0] && (quot_len == 1 || s[1] == c))
      cb.write(c == '\"' ? "~\"" : "~\'", 2);
    else if (c == '\r' && quot_len == 1)
      cb.write("~r", 2);
    else if (c == '\n' && quot_len == 1)
      cb.write("~n", 2);
    else if (c == '\t')
      cb.write("~t", 2);
    else
      cb.write(&c, 1);
  }

  cb.write(quot + quot_len - 1, quot_len);
}

template <bool print_with_limits>
bool DataBlock::writeText(IGenSave &cb, int level, int *max_lines, int max_levels) const
{
  const char *eol = (level >= 0 && !print_with_limits) ? "\r\n" : "\n";
  const uint32_t eolLen = (level >= 0 && !print_with_limits) ? 2 : 1;

#define CHECK_LINES_LIMIT()  \
  if (print_with_limits)     \
    if (--max_lines[0] == 0) \
  return false

  auto oldParams = getParamsImpl();
  int skipped_comments = 0;
  bool skip_next_indent = false;
  for (uint32_t i = 0, e = paramCount(); i < e; ++i, ++oldParams)
  {
    const Param &p = *oldParams;

    bool isCIdent = true;
    const char *keyName = getName(p.nameId);
    if (!keyName)
      keyName = "@null";
    if (!*keyName)
      isCIdent = false;
    for (const char *c = keyName; *c && isCIdent; ++c)
      isCIdent = dblk::is_ident_char(*c);

    if (DAGOR_LIKELY(!DataBlock::parseCommentsAsParams) && p.type == TYPE_STRING && CHECK_COMMENT_PREFIX(keyName))
    {
      skipped_comments++;
      continue;
    }

    if (level > 0)
    {
      if (!skip_next_indent)
        writeIndent(cb, level * 2);
      else
        skip_next_indent = false;
    }
    if (p.type == TYPE_STRING && keyName[0] == '@')
    {
      if (strcmp(keyName, "@include") == 0)
      {
        cb.write("include ", 8);
        writeStringValue(cb, getStr(i));
        cb.write(eol, eolLen);
        CHECK_LINES_LIMIT();
        continue;
      }
      else if (DAGOR_UNLIKELY(CHECK_COMMENT_PREFIX(keyName)))
      {
        if (IS_COMMENT_C(keyName))
        {
          cb.write("/*", 2);
          writeString(cb, getStr(i));
          cb.write("*/", 2);
          cb.write(eol, eolLen);
        }
        if (IS_COMMENT_CPP(keyName))
        {
          cb.write("//", 2);
          writeString(cb, getStr(i));
          cb.write(eol, eolLen);
        }
        CHECK_LINES_LIMIT();
        continue;
      }
    }
    (isCIdent ? writeString : writeStringValue)(cb, keyName);
    writeString(cb, ":");
    writeString(cb, dblk::resolve_short_type(p.type));
    writeString(cb, "=");
    if (p.type == TYPE_STRING)
    {
      writeStringValue(cb, getStr(i)); // getParamString<string_t>(p.v)
    }
    else
    {
      const char *paramData = getParamData(p);
      const int *cdi = (const int *)paramData;
      const int64_t *cdi64 = (const int64_t *)paramData;
      const float *cd = (const float *)paramData;
      char buf[256];
      switch (p.type)
      {
        case TYPE_BOOL: snprintf(buf, sizeof(buf), *paramData ? "yes" : "no"); break;
        case TYPE_INT: snprintf(buf, sizeof(buf), "%d", cdi[0]); break;
        case TYPE_REAL: snprintf(buf, sizeof(buf), "%g", cd[0]); break;
        case TYPE_POINT2: snprintf(buf, sizeof(buf), "%g, %g", cd[0], cd[1]); break;
        case TYPE_POINT3: snprintf(buf, sizeof(buf), "%g, %g, %g", cd[0], cd[1], cd[2]); break;
        case TYPE_POINT4: snprintf(buf, sizeof(buf), "%g, %g, %g, %g", cd[0], cd[1], cd[2], cd[3]); break;
        case TYPE_IPOINT2: snprintf(buf, sizeof(buf), "%d, %d", cdi[0], cdi[1]); break;
        case TYPE_IPOINT3: snprintf(buf, sizeof(buf), "%d, %d, %d", cdi[0], cdi[1], cdi[2]); break;
        case TYPE_E3DCOLOR:
        {
          E3DCOLOR c = getE3dcolor(i);
          snprintf(buf, sizeof(buf), "%d, %d, %d, %d", c.r, c.g, c.b, c.a);
          break;
        }
        case TYPE_MATRIX:
          snprintf(buf, sizeof(buf), "[[%g, %g, %g] [%g, %g, %g] [%g, %g, %g] [%g, %g, %g]]", cd[0 * 3 + 0], cd[0 * 3 + 1],
            cd[0 * 3 + 2], cd[1 * 3 + 0], cd[1 * 3 + 1], cd[1 * 3 + 2], cd[2 * 3 + 0], cd[2 * 3 + 1], cd[2 * 3 + 2], cd[3 * 3 + 0],
            cd[3 * 3 + 1], cd[3 * 3 + 2]);
          break;
        case TYPE_INT64: snprintf(buf, sizeof(buf), "%lld", (long long int)*cdi64); break;
        default: G_ASSERT(0);
      }
      writeString(cb, buf);
    }
    if (level < 0 && paramCount() == 1 && blockCount() == 0)
      cb.write(";", 1);
    else
    {
      if (DAGOR_UNLIKELY(DataBlock::parseCommentsAsParams) && i + 1 < paramCount())
      {
        const char *keyName = getName((oldParams + 1)->nameId);
        if (CHECK_COMMENT_PREFIX(keyName) && IS_COMMENT_POST(keyName))
        {
          cb.write(" ", 1);
          skip_next_indent = true;
          continue;
        }
      }
      cb.write(eol, eolLen);
      CHECK_LINES_LIMIT();
    }
  }

  bool params_blocks_sep = false;
  for (uint32_t i = 0, e = blockCount(); i < e; ++i)
  {
    const DataBlock &b = *getBlock(i);
    bool isCIdent = true;
    const char *blkName = b.getName(b.getNameId());
    if (!blkName)
      blkName = "@null";
    if (DAGOR_UNLIKELY(CHECK_COMMENT_PREFIX(blkName)))
    {
      if (DAGOR_LIKELY(!DataBlock::parseCommentsAsParams))
        continue;
      if (level > 0)
      {
        if (!params_blocks_sep && paramCount() > skipped_comments)
        {
          params_blocks_sep = true;
          cb.write(eol, eolLen);
          CHECK_LINES_LIMIT();
        }
        writeIndent(cb, level * 2);
      }

      if (IS_COMMENT_PRE_C(blkName))
      {
        cb.write("/*", 2);
        writeString(cb, b.getStr(0));
        cb.write("*/", 2);
        cb.write(eol, eolLen);
      }
      else if (IS_COMMENT_PRE_CPP(blkName))
      {
        cb.write("//", 2);
        writeString(cb, b.getStr(0));
        cb.write(eol, eolLen);
      }
      CHECK_LINES_LIMIT();
      continue;
    }

    if (level >= 0 && !params_blocks_sep && paramCount() > skipped_comments)
    {
      params_blocks_sep = true;
      cb.write(eol, eolLen);
      CHECK_LINES_LIMIT();
    }
    if (level > 0)
      writeIndent(cb, level * 2);

    if (!*blkName)
      isCIdent = false;
    for (const char *c = blkName; *c && isCIdent; ++c)
      isCIdent = dblk::is_ident_char(*c);
    (isCIdent ? writeString : writeStringValue)(cb, blkName);
    if (b.isEmpty())
    {
      cb.write("{}", 2);
      cb.write(eol, eolLen);
      CHECK_LINES_LIMIT();
      continue;
    }

    cb.write("{", 1);
    if (print_with_limits && max_levels == 1)
    {
      cb.write(" ... }", 6);
      cb.write(eol, eolLen);
      CHECK_LINES_LIMIT();
      continue;
    }
    if (!(level < 0 && b.paramCount() == 1 && b.blockCount() == 0))
    {
      cb.write(eol, eolLen);
      CHECK_LINES_LIMIT();
    }

    if (
      !b.writeText<print_with_limits>(cb, level >= 0 ? level + 1 : level, max_lines, print_with_limits ? max_levels - 1 : max_levels))
      return false;
    if (level > 0)
      writeIndent(cb, level * 2);
    cb.write("}", 1);
    cb.write(eol, eolLen);
    CHECK_LINES_LIMIT();

    if (i != e - 1 && level >= 0)
    {
      cb.write(eol, eolLen);
      CHECK_LINES_LIMIT();
    }
  }
#undef CHECK_LINES_LIMIT
  return true;
}

bool DataBlock::saveToTextStreamCompact(IGenSave &cwr) const
{
  DAGOR_TRY { writeText<false>(cwr, -1, nullptr, 0); }
  DAGOR_CATCH(const IGenSave::SaveException &) { return false; }
  return true;
}

bool DataBlock::saveToTextStream(IGenSave &cwr) const
{
  DAGOR_TRY { writeText<false>(cwr, 0, nullptr, 0); }
  DAGOR_CATCH(const IGenSave::SaveException &) { return false; }
  return true;
}

template <typename T, typename C>
static void writeCompressedUnsignedGeneric(T v, C &cb)
{
  G_STATIC_ASSERT(eastl::is_unsigned<T>::value);
  for (uint32_t i = 0; i < sizeof(v) + 1; ++i)
  {
    uint8_t byte = uint8_t(v) | (v >= (1 << 7) ? (1 << 7) : 0);
    cb.write(&byte, 1);
    v >>= 7;
    if (!v)
      break;
  }
}

template <typename T, typename C>
static void writeCompressedSignedGeneric(T v, C &cb)
{
  G_STATIC_ASSERT(eastl::is_signed<T>::value);
  T vu = (v << 1) ^ (v >> (sizeof(T) * CHAR_BIT - 1));
  writeCompressedUnsignedGeneric(typename eastl::make_unsigned<T>::type(vu), cb);
}

template <typename T, typename C>
static bool readCompressedUnsignedGeneric(T &v, C &cb)
{
  G_STATIC_ASSERT(eastl::is_unsigned<T>::value);
  v = 0;
  for (uint32_t i = 0; i < sizeof(v) + 1; ++i)
  {
    uint8_t byte = 0;
    cb.read(&byte, 1);
    v |= T(byte & ~(1 << 7)) << (i * 7);
    if ((byte & (1 << 7)) == 0)
      break;
  }
  return true;
}

template <typename T, typename C>
static bool readCompressedSignedGeneric(T &v, C &cb)
{
  G_STATIC_ASSERT(eastl::is_signed<T>::value);
  typename eastl::make_unsigned<T>::type vu;
  if (!readCompressedUnsignedGeneric(vu, cb))
    return false;
  v = static_cast<T>((vu >> 1) ^ (-(vu & 1)));
  return true;
}

struct ComplexDataContainer
{
  SmallTab<char> data;
  struct DataId
  {
    uint32_t ofs, sz;
  };
  SmallTab<DataId> keys;
  typedef uint32_t hash_t;
  HashedKeyMap<hash_t, uint32_t> hashToKeyId;
  static inline hash_t hash(const char *s, size_t len)
  {
    hash_t ret = (hash_t)wyhash(s, len, 0);
    return ret ? ret : 0x80000000;
  };
  void alignTo(size_t al)
  {
    al -= 1;
    if ((data.size() & al) != 0)
    {
      const uint32_t add = uint32_t(((data.size() + al) & (~al)) - data.size());
      memset(data.insert_default(data.end(), add), 0, add);
    }
  }
  uint32_t addDataId(const char *val, uint32_t len, hash_t hash)
  {
    int it = hashToKeyId.findOr(hash, -1,
      [&, this](uint32_t id) { return keys[id].sz == len && memcmp(data.data() + keys[id].ofs, val, len) == 0; });
    if (it != -1)
      return keys[it].ofs;
    const uint32_t ofs = data.size();
    memcpy(data.insert_default(data.end(), len), val, len);
    const uint32_t id = keys.size();
    keys.emplace_back(DataId{ofs, len});
    hashToKeyId.emplace(hash, id);
    return ofs;
  }
  uint32_t addDataId(const char *val, uint32_t len) { return addDataId(val, len, hash(val, len)); }
};

bool dblk::write_names_base(IGenSave &cwr, const DBNameMapBase &names, uint64_t *names_hash)
{
  DAGOR_TRY
  {
    // fixme: todo: allow skipping name Ids (holes)
    uint32_t totalNames = names.nameCount();
    writeCompressedUnsignedGeneric(totalNames, cwr);
    if (names_hash)
      *names_hash = 0;
    if (totalNames)
    {
      uint32_t totalNamesSize = 0;
      for (uint32_t i = 0, e = names.nameCount(); i < e; ++i)
        totalNamesSize += (uint32_t)strlen(names.getName(i)) + 1; //-V575
      writeCompressedUnsignedGeneric(totalNamesSize, cwr);
      eastl::unique_ptr<char[]> fullNames;
      if (names_hash)
        fullNames = eastl::make_unique<char[]>(totalNamesSize);
      char *namesForHash = fullNames.get();
      // uint64_t calc_hash = totalNames;
      for (uint32_t i = 0, e = names.nameCount(); i < e; ++i)
      {
        const char *name = names.getName(i);
        const uint32_t nameLen = (uint32_t)strlen(name) + 1; //-V575
        cwr.write(name, nameLen);
        if (names_hash)
        {
          memcpy(namesForHash, name, nameLen);
          namesForHash += nameLen;
        }
      }
      if (names_hash)
      {
        *names_hash = mum_hash(fullNames.get(), totalNamesSize, names.nameCount());
        if (!*names_hash)
          *names_hash = 1;
      }
    }
  }
  DAGOR_CATCH(const IGenSave::SaveException &) { return false; }
  return true;
}
bool dblk::write_names(IGenSave &cwr, const DBNameMap &names, uint64_t *names_hash)
{
  return write_names_base(cwr, names, names_hash);
}

bool dblk::read_names_base(IGenLoad &cr, DBNameMapBase &names, uint64_t *names_hash)
{
  DAGOR_TRY
  {
    // fixme: todo: allow skipping name Ids (holes)
    G_UNUSED(names_hash);
    uint32_t totalNamesSize = 0, totalNames = names.nameCount();
    names.reset();
    readCompressedUnsignedGeneric(totalNames, cr);
    if (names_hash)
      *names_hash = 0;
    if (totalNames)
    {
      readCompressedUnsignedGeneric(totalNamesSize, cr);
      if (!totalNamesSize || totalNamesSize > BLK_MAX_MEMORY_FOR_NAMES)
        return false;
      auto page = names.allocator.addPageUnsafe(totalNamesSize + 1);
      cr.read(page->data.get(), totalNamesSize);
      page->data.get()[totalNamesSize] = 0;
      if (names_hash)
      {
        *names_hash = mum_hash(page->data.get(), totalNamesSize, totalNames);
        if (!*names_hash)
          *names_hash = 1;
      }
      names.strings.resize(totalNames);
      names.hashToStringId.reserve(totalNames);
      eastl::swap(page->used, page->left); // page is empty
      names.allocator.page_shift = get_bigger_log2(totalNamesSize);
      names.allocator.start_page_shift = names.allocator.page_shift;
      names.allocator.max_page_shift = max(names.allocator.page_shift, names.allocator.max_page_shift);

      const char *name = page->data.get();
      const char *endOfNamesPtr = name + totalNamesSize;
      for (uint32_t i = 0; i < totalNames; ++i)
      {
        if (name > endOfNamesPtr)
          return false;

        const size_t len = strlen(name);
        DBNameMap::hash_t hash = DBNameMap::string_hash(name, len);
        int current = names.hashToStringId.findOr(hash, -1);
        if (current != -1)
        {
          if (memcmp(name, names.getStringDataUnsafe(current), len) != 0)
          {
            names.hasCollisions() = 1;
            current = -1;
          }
          else
          {
            G_ASSERTF(0, "duplicate string in namemap found <%s> <%s>!", name, names.getStringDataUnsafe(current));
            names.strings[i] = current;
          }
        }
        if (current == -1)
        {
          names.strings[i] = (uint32_t)(name - page->data.get());
          names.hashToStringId.emplace(eastl::move(hash), eastl::move(i));
        }
        name += len + 1;
      }
    }
    names.shrink_to_fit();
  }

  DAGOR_CATCH(const IGenSave::SaveException &) { return false; }
  return true;
}
bool dblk::read_names(IGenLoad &cr, DBNameMap &names, uint64_t *names_hash) { return read_names_base(cr, names, names_hash); }

struct DataBlockInfo
{
  uint32_t nameId;
  uint16_t paramsCount, blocksCount;
  uint32_t fBlock;
};

static void write_packed(const DataBlockInfo &d, IGenSave &cwr)
{
  writeCompressedUnsignedGeneric(d.nameId, cwr); // only nameId == -1 is allowed, so nameId +1 is positive
  writeCompressedUnsignedGeneric(d.paramsCount, cwr);
  writeCompressedUnsignedGeneric(d.blocksCount, cwr);
  if (d.blocksCount)
    writeCompressedUnsignedGeneric(d.fBlock, cwr);
}
static void read_packed(DataBlockInfo &d, IGenLoad &cr)
{
  readCompressedUnsignedGeneric(d.nameId, cr);
  readCompressedUnsignedGeneric(d.paramsCount, cr);
  readCompressedUnsignedGeneric(d.blocksCount, cr);
  if (d.blocksCount)
    readCompressedUnsignedGeneric(d.fBlock, cr);
  else
    d.fBlock = 0;
}

void DataBlock::saveToBinStreamWithoutNames(const DataBlockShared &names, IGenSave &cwr) const
{
  const bool differentNameMap = (void *)&names != (void *)shared;
  uint32_t blocks_count = 0;
  uint32_t totalParamsCount = 0;

  // uint16_t maxParams = 0, maxBlocks = 0;
  dblk::iterate_blocks(*this, [&](auto &d) {
    ++blocks_count;
    totalParamsCount += d.paramCount();
  });

  static constexpr const unsigned MAX_BLK_ON_STACK = 64;
  dag::RelocatableFixedVector<const DataBlock *, MAX_BLK_ON_STACK, true> bfsOrderBlocks(blocks_count);
  const DataBlock **bfsBlocks = bfsOrderBlocks.data();
  auto bfs_block_iter = [&](auto &d) { *(bfsBlocks++) = &d; };
  if (blocks_count <= MAX_BLK_ON_STACK)
  {
    FRAMEMEM_REGION;
    iterate_blocks_bfs<decltype(bfs_block_iter), framemem_allocator>(*this, bfs_block_iter);
  }
  else
    iterate_blocks_bfs(*this, bfs_block_iter);
  ComplexDataContainer complex;
  dag::RelocatableFixedVector<DataBlockInfo, MAX_BLK_ON_STACK, true> dataBlocks(blocks_count);
  dag::RelocatableFixedVector<DataBlock::Param, MAX_BLK_ON_STACK, true> newParams(totalParamsCount);
  DataBlock::Param *__restrict cParam = newParams.data();
  uint32_t currentBlocks = 1;
  for (auto dbP : bfsOrderBlocks) // first, intern all strings
  {
    auto &d = *dbP;
    auto oldParams = d.getParamsImpl();
    for (uint32_t i = 0, e = d.paramCount(); i < e; ++i, ++oldParams, ++cParam)
    {
      *cParam = *oldParams;
      if (differentNameMap)
      {
        // can be optimized if nameId is in same shared namemap
        const char *name = getName(cParam->nameId);
        G_ASSERT(name);
        cParam->nameId = name ? names.getNameId(name) : 0;
      }
      if (cParam->type == DataBlock::TYPE_STRING)
      {
        if (!(cParam->v & IS_NAMEMAP_ID) || differentNameMap) // otherwise already interned
        {
          const char *paramData = d.getParamData(*oldParams);
          const int name_id = names.getNameId(paramData); // try to intern
          if (name_id >= 0)
            cParam->v = name_id | IS_NAMEMAP_ID;
          else
            cParam->v = complex.addDataId(paramData, (uint32_t)strlen(paramData) + 1);
        }
      }
    }
  }
  bool aligned = false;
  DataBlockInfo *__restrict cBlock = dataBlocks.data();
  cParam = newParams.data();
  for (auto dbP : bfsOrderBlocks)
  {
    auto &d = *dbP;
    *cBlock = DataBlockInfo{
      uint32_t(d.nameIdAndFlags & NAME_ID_MASK), d.paramsCount, d.blocksCount, d.blocksCount ? currentBlocks : (uint32_t)0};
    if (differentNameMap && cBlock->nameId != 0)
      cBlock->nameId = names.getNameId(getName(cBlock->nameId - 1)) + 1;
    currentBlocks += d.blocksCount;
    auto oldParams = d.getParamsImpl();
    // eastl::stable_sort(cParam, cParam+d.paramCount(), [](auto &a, auto &b) {return a.nameId<b.nameId;});
    for (uint32_t i = 0, e = d.paramCount(); i < e; ++i, ++oldParams, ++cParam)
    {
      const uint32_t sz = dblk::get_type_size(cParam->type);
      if (sz > d.INPLACE_PARAM_SIZE && cParam->type != DataBlock::TYPE_STRING)
      {
        if (!aligned)
        {
          complex.alignTo(4);
          aligned = true;
        }
        cParam->v = complex.addDataId(d.getParamData(*oldParams), sz);
      }
    }
    ++cBlock;
  }

  // const uint32_t totalSize = newParams.size()*sizeof(DataBlock::Param) + complex.data.size();
  writeCompressedUnsignedGeneric(blocks_count, cwr);
  writeCompressedUnsignedGeneric(newParams.size(), cwr);
  writeCompressedUnsignedGeneric(complex.data.size(), cwr);
  cwr.write(complex.data.data(), complex.data.size());
  cwr.write(newParams.data(), newParams.size() * sizeof(DataBlock::Param));
  for (auto &d : dataBlocks)
    write_packed(d, cwr);
}

bool DataBlock::loadFromBinDump(IGenLoad &cr, const DBNameMap *ro)
{
  unsigned blkFlags = shared ? shared->blkFlags : 0;
  deleteShared();

  shared = NULL;
  DBNameMapBase rw;
  uint64_t hash;
  if (!dblk::read_names_base(cr, rw, &hash))
    return false;
  uint32_t roDataBlocks = 0, paramsCnt = 0, complexDataSize = 0;
  readCompressedUnsignedGeneric(roDataBlocks, cr);
  readCompressedUnsignedGeneric(paramsCnt, cr);
  readCompressedUnsignedGeneric(complexDataSize, cr);
  uint32_t blocksStartsAt = complexDataSize + paramsCnt * sizeof(DataBlock::Param);
  size_t sharedSize = roDataBlocks * sizeof(DataBlock) + blocksStartsAt + sizeof(DataBlockShared);
  shared = new (memalloc(sharedSize, tmpmem), _NEW_INPLACE) DataBlockShared;
  nameIdAndFlags |= IS_TOPMOST;
  shared->roDataBlocks = roDataBlocks;
  shared->blocksStartsAt = blocksStartsAt;
  shared->ro = ro;
  if (shared->ro)
    shared->ro->addRef();
  shared->blkFlags = blkFlags;
  eastl::swap(shared->rw, rw);

  cr.read(shared->get(0), complexDataSize);
  cr.read(shared->get(complexDataSize), paramsCnt * sizeof(DataBlock::Param));
  uint32_t cOfs = complexDataSize;
  DataBlock *blData = shared->getROBlockUnsafe(0);
  for (uint32_t i = 0, e = shared->roDataBlocks; i < e; ++i, ++blData)
  {
    DataBlockInfo d;
    read_packed(d, cr);
    new (blData, _NEW_INPLACE) DataBlock(shared, d.nameId - 1, d.paramsCount, d.blocksCount, d.fBlock, cOfs);
    cOfs += d.paramsCount * sizeof(DataBlock::Param);
  }
  blData = shared->getROBlockUnsafe(0);
  nameIdAndFlags = blData->nameIdAndFlags | IS_TOPMOST;
  paramsCount = blData->paramsCount;
  blocksCount = blData->blocksCount;
  firstBlockId = blData->firstBlockId;
  ofs = blData->ofs;
  return true;
}

static void calc_namemap_diff(DBNameMapBase &diff, const DBNameMapBase &ro, const DataBlockShared &names)
{
  diff.reset();
  for (uint32_t i = 0, e = names.nameCount(); i != e; ++i)
  {
    const char *name = names.getName(i);
    const size_t nameLen = strlen(name); //-V575
    int nid = ro.getNameId(name, nameLen);
    if (nid < 0)
      diff.addNameId(name, nameLen);
  }
}
static void fill_namemap(const DataBlock &d, DBNameMapBase &rw)
{
  dblk::iterate_blocks(d, [&rw](auto &db) {
    if (const char *nm = db.getBlockName())
      rw.addNameId(nm);
    dblk::iterate_params(db, [&](int, int pnid, int) { rw.addNameId(db.getName(pnid)); });
  });
}

bool dblk::add_name_to_name_map(DBNameMap &nm, const char *s)
{
  int names = nm.nameCount();
  return nm.addNameId(s) == names; // new added
}

void DataBlock::appendNamemapToSharedNamemap(DBNameMap &to, const DBNameMap *skip) const
{
  if (shared->ro && shared->ro != skip)
    for (uint32_t i = 0, e = shared->ro->nameCount(); i != e; ++i)
      to.addNameId(shared->ro->getName(i));
  for (uint32_t i = 0, e = shared->rw.nameCount(); i != e; ++i)
    to.addNameId(shared->rw.getName(i));
}

bool DataBlock::saveDumpToBinStream(IGenSave &cwr, const DBNameMap *ro) const
{
  DAGOR_TRY
  {
    DataBlockShared diff;
    const DataBlockShared *dbShared = shared;
    if (ro != shared->ro)
    {
      dbShared = &diff;
      diff.ro = ro;
      if (ro)
        calc_namemap_diff(diff.rw, *ro, *shared);
      else
        fill_namemap(*this, diff.rw);
    }
    // todo:if we write with a shared namemap (ro!=NULL), we should write an identifier (hash of ro content)
    dblk::write_names_base(cwr, dbShared->rw, NULL);
    saveToBinStreamWithoutNames(*dbShared, cwr);
  }
  DAGOR_CATCH(const IGenSave::SaveException &) { return false; }

  return true;
}

bool DataBlock::saveToStream(IGenSave &cwr) const
{
  cwr.writeIntP<1>(dblk::BBF_full_binary_in_stream);
  return saveDumpToBinStream(cwr, NULL);
}


void DataBlock::setFrom(const RoDataBlock &from)
{
  G_ASSERT(this != &emptyBlock);
  clearData();

  setParamsFrom(from);
  for (uint32_t i = 0, ie = from.blockCount(); i < ie; ++i)
    addNewBlock(from.getBlock(i)->getBlockName())->setFrom(*from.getBlock(i));
}

void DataBlock::setParamsFrom(const RoDataBlock &blk)
{
  G_ASSERT(this != &emptyBlock);
  clearParams();

  for (uint32_t i = 0, ie = blk.paramCount(); i < ie; ++i)
  {
    const char *name = blk.getParamName(i);

    switch (blk.getParamType(i))
    {
      case TYPE_STRING: addStr(name, blk.getStr(i)); break;
      case TYPE_INT: addInt(name, blk.getInt(i)); break;
      case TYPE_REAL: addReal(name, blk.getReal(i)); break;
      case TYPE_POINT2: addPoint2(name, blk.getPoint2(i)); break;
      case TYPE_POINT3: addPoint3(name, blk.getPoint3(i)); break;
      case TYPE_POINT4: addPoint4(name, blk.getPoint4(i)); break;
      case TYPE_IPOINT2: addIPoint2(name, blk.getIPoint2(i)); break;
      case TYPE_IPOINT3: addIPoint3(name, blk.getIPoint3(i)); break;
      case TYPE_BOOL: addBool(name, blk.getBool(i)); break;
      case TYPE_E3DCOLOR: addE3dcolor(name, blk.getE3dcolor(i)); break;
      case TYPE_MATRIX: addTm(name, blk.getTm(i)); break;
      case TYPE_INT64: addInt64(name, blk.getInt64(i)); break;
      default: G_ASSERT(0);
    }
  }
}


#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_fileIo.h>
namespace dblk
{
static bool save_to_text_file_ex(const DataBlock &blk, const char *filename, bool compact);
struct OpenDataBlock : public DataBlock
{
  friend ReadFlags get_flags(const DataBlock &blk);
  friend void set_flag(DataBlock &blk, dblk::ReadFlags flg_to_add);
  friend void clr_flag(DataBlock &blk, dblk::ReadFlags flg_to_clr);
  friend bool load(DataBlock &blk, const char *fname, dblk::ReadFlags flg, DataBlock::IFileNotify *fnotify);
  friend bool load_text(DataBlock &blk, dag::ConstSpan<char> text, dblk::ReadFlags flg, const char *fname,
    DataBlock::IFileNotify *fnotify);
  friend bool load_from_stream(DataBlock &blk, IGenLoad &crd, dblk::ReadFlags flg, const char *fname, DataBlock::IFileNotify *fnotify,
    unsigned hint_size);
  friend bool save_to_text_file_ex(const DataBlock &blk, const char *filename, bool compact);
  friend bool print_to_text_stream_limited(const DataBlock &blk, IGenSave &cwr, int max_ln, int max_lev, int init_lev);
};
} // namespace dblk

static bool dblk::save_to_text_file_ex(const DataBlock &blk, const char *filename, bool compact)
{
  file_ptr_t h = ::df_open(filename, DF_WRITE | DF_CREATE);
  if (!h)
    return false;

#if _TARGET_C3




#else
  LFileGeneralSaveCB cb(h);
#endif

  DAGOR_TRY
  {
    static_cast<const dblk::OpenDataBlock &>(blk).writeText<false>(cb, compact ? -1 : 0, nullptr, 0);
#if _TARGET_C3

#endif
  }
  DAGOR_CATCH(const IGenSave::SaveException &)
  {
    ::df_close(h);
    return false;
  }

  ::df_close(h);
  return true;
}

bool dblk::save_to_text_file(const DataBlock &blk, const char *filename) { return dblk::save_to_text_file_ex(blk, filename, false); }

bool dblk::save_to_text_file_compact(const DataBlock &blk, const char *filename)
{
  return dblk::save_to_text_file_ex(blk, filename, true);
}

bool dblk::print_to_text_stream_limited(const DataBlock &blk, IGenSave &cwr, int max_ln, int max_lev, int init_lev)
{
  DAGOR_TRY
  {
    if (max_ln < 0)
      max_ln = 0x7FFFFFFF;
    if (!static_cast<const dblk::OpenDataBlock &>(blk).writeText<true>(cwr, init_lev, &max_ln, max_lev))
    {
      cwr.write("...\n", 4);
      return false;
    }
  }
  DAGOR_CATCH(const IGenSave::SaveException &) { return false; }
  return true;
}

bool dblk::save_to_binary_file(const DataBlock &blk, const char *filename)
{
  FullFileSaveCB cwr(filename);
  if (!cwr.fileHandle)
    return false;

  DAGOR_TRY { blk.saveToStream(cwr); }
  DAGOR_CATCH(const IGenSave::SaveException &) { return false; }

  return true;
}

#define PARSE_FLAGS_PROLOGUE()                                                     \
  auto *shared = static_cast<OpenDataBlock &>(blk).shared;                         \
  bool prev_robust = shared->blkRobustLoad(), prev_bin = shared->blkBinOnlyLoad(); \
  bool prev_allow_ss = DataBlock::allowSimpleString;                               \
  if (flg & ReadFlag::ROBUST)                                                      \
    shared->setBlkRobustLoad(1), shared->setBlkRobustOps(1);                       \
  if (flg & ReadFlag::BINARY_ONLY)                                                 \
    shared->setBlkBinOnlyLoad(1);                                                  \
  if (flg & ReadFlag::ALLOW_SS)                                                    \
  DataBlock::allowSimpleString = true

#define RESTORE_FLAGS_EPILOGUE()                                                                                      \
  shared = static_cast<OpenDataBlock &>(blk).shared;                                                                  \
  if (flg & ReadFlag::RESTORE_FLAGS)                                                                                  \
    shared->setBlkRobustLoad(prev_robust), shared->setBlkRobustOps(prev_robust), shared->setBlkBinOnlyLoad(prev_bin); \
  if (flg & ReadFlag::ALLOW_SS)                                                                                       \
  DataBlock::allowSimpleString = prev_allow_ss

bool dblk::load(DataBlock &blk, const char *fname, dblk::ReadFlags flg, DataBlock::IFileNotify *fnotify)
{
  PARSE_FLAGS_PROLOGUE();
  bool res = static_cast<dblk::OpenDataBlock &>(blk).load(fname, fnotify);
  RESTORE_FLAGS_EPILOGUE();
  return res;
}

bool dblk::load_text(DataBlock &blk, dag::ConstSpan<char> text, dblk::ReadFlags flg, const char *fname,
  DataBlock::IFileNotify *fnotify)
{
  PARSE_FLAGS_PROLOGUE();
  bool res = static_cast<dblk::OpenDataBlock &>(blk).loadText(text.data(), data_size(text), fname, fnotify);
  RESTORE_FLAGS_EPILOGUE();
  return res;
}

bool dblk::load_from_stream(DataBlock &blk, IGenLoad &crd, dblk::ReadFlags flg, const char *fname, DataBlock::IFileNotify *fnotify,
  unsigned hint_size)
{
  PARSE_FLAGS_PROLOGUE();
  bool res = static_cast<dblk::OpenDataBlock &>(blk).loadFromStream(crd, fname, fnotify, hint_size);
  RESTORE_FLAGS_EPILOGUE();
  return res;
}
#undef PARSE_FLAGS_PROLOGUE
#undef RESTORE_FLAGS_EPILOGUE

dblk::ReadFlags dblk::get_flags(const DataBlock &blk)
{
  auto &shared = *static_cast<const OpenDataBlock &>(blk).shared;
  ReadFlags f;
  if (shared.blkRobustLoad())
    f |= ReadFlag::ROBUST;
  if (shared.blkBinOnlyLoad())
    f |= ReadFlag::BINARY_ONLY;
  return f;
}
void dblk::set_flag(DataBlock &blk, dblk::ReadFlags flg_to_add)
{
  auto &shared = *static_cast<const OpenDataBlock &>(blk).shared;
  if (flg_to_add & ReadFlag::ROBUST)
    shared.setBlkRobustLoad(true), shared.setBlkRobustOps(true);
  if (flg_to_add & ReadFlag::BINARY_ONLY)
    shared.setBlkBinOnlyLoad(true);
}
void dblk::clr_flag(DataBlock &blk, dblk::ReadFlags flg_to_clr)
{
  auto &shared = *static_cast<const OpenDataBlock &>(blk).shared;
  if (flg_to_clr & ReadFlag::ROBUST)
    shared.setBlkRobustLoad(false), shared.setBlkRobustOps(false);
  if (flg_to_clr & ReadFlag::BINARY_ONLY)
    shared.setBlkBinOnlyLoad(false);
}
DBNameMap *dblk::create_db_names() { return new DBNameMap; }
void dblk::destroy_db_names(DBNameMap *nm) { delete nm; }
int dblk::db_names_count(DBNameMap *nm) { return nm ? nm->nameCount() : 0; }

#define EXPORT_PULL dll_pull_iosys_datablock_serialize
#include <supp/exportPull.h>
