//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// this one is following FastNameMap API, but is better in all regards
// use that, do not use FastMameMap
// ignore_case makes things much slower, as it forcing to use FNV1 hash + table access

#include <util/dag_stringTableAllocator.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_sort.h>
#include <debug/dag_assert.h>
#include <util/dag_hashedKeyMap.h>
#include <util/dag_nameHashers.h>

template <bool ignore_case, typename Hasher = DefaultOAHasher<ignore_case>>
struct OAHashNameMap
{
  typedef uint32_t hash_t;
  HashedKeyMap<hash_t, uint32_t> hashToStringId;
  SmallTab<uint32_t> strings;
  StringTableAllocator allocator;
  uint8_t noCollisions() const { return allocator.padding; }
  uint8_t &noCollisions() { return allocator.padding; }
  uint32_t addString(const char *name, size_t name_len)
  {
    const uint32_t len = (uint32_t)name_len + 1; // term zero, or one symbol after len. todo: fixme, so we don't memcpy from it
    const uint32_t stringId = (uint32_t)strings.size();
    const uint32_t at = allocator.addDataRaw(name, len);
    const_cast<char *>(allocator.head.data.get() + allocator.get_page_ofs(at))[name_len] = 0; // ensure term zero
    strings.push_back(at);
    return stringId;
  }
  uint32_t addString(const char *name) { return addString(name, strlen(name)); }
  static inline hash_t string_hash(const char *s, size_t len) { return Hasher::hash_data(s, len); }
  static inline hash_t string_hash(const char *s) { return string_hash(s, strlen(s)); }
  const char *getStringDataUnsafe(uint32_t id, uint32_t &max_len) const { return allocator.getDataRawUnsafe(strings[id], max_len); }
  const char *getStringDataUnsafe(uint32_t id) const { return allocator.getDataRawUnsafe(strings[id]); }
  inline bool string_equal(const char *name, size_t name_len, const uint32_t id) const
  {
    G_FAST_ASSERT(name != nullptr);
    if (ignore_case)
    {
      const char *str = getStringDataUnsafe(id);
      const unsigned char *to_lower_lut = dd_local_cmp_lwtab;
      for (; name_len; ++name, ++str, --name_len)
        if (to_lower_lut[uint8_t(*name)] != to_lower_lut[uint8_t(*str)])
          return false;
      return true;
    }
    uint32_t max_len; //-V779
    const char *str = getStringDataUnsafe(id, max_len);
    // memcmp is faster but we can only call it, when there is enough data in str
    return (max_len >= name_len ? memcmp(str, name, name_len) : strncmp(str, name, name_len)) == 0; // we actually probably don't ever
                                                                                                    // collide
  }
  int getNameId(const char *name, size_t name_len, const hash_t hash) const
  {
    if (DAGOR_LIKELY(noCollisions()))
    {
      const int it = hashToStringId.findOr(hash, -1);
      if (it == -1 || !string_equal(name, name_len, it))
        return -1;
      return it;
    }
    // check hash collision
    return hashToStringId.findOr(hash, -1, [this, name, name_len](uint32_t id) { return string_equal(name, name_len, id); });
  }
  int getNameId(const char *name, size_t name_len) const { return getNameId(name, name_len, string_hash(name, name_len)); }
  int getNameId(const char *name) const { return name ? getNameId(name, strlen(name)) : -1; }
  int addNameId(const char *name, size_t name_len, hash_t hash)
  {
    int it = -1;
    if (DAGOR_LIKELY(noCollisions()))
    {
      it = hashToStringId.findOr(hash, -1);
      if (DAGOR_UNLIKELY(it != -1 && !string_equal(name, name_len, it)))
      {
        noCollisions() = 0;
        it = -1;
      }
    }
    else
      it = hashToStringId.findOr(hash, -1, [this, name, name_len](uint32_t id) { return string_equal(name, name_len, id); });
    if (it == -1)
    {
      uint32_t id = addString(name, name_len);
      hashToStringId.emplace(hash, eastl::move(id));
      return id;
    }
    return it;
  }
  int addNameId(const char *name, size_t name_len) { return addNameId(name, name_len, string_hash(name, name_len)); }
  int addNameId(const char *name) { return name ? addNameId(name, strlen(name)) : -1; } //-V1071
  void erase(uint32_t id)
  {
    if (id >= strings.size())
      return;
    const char *name = getStringDataUnsafe(id);
    size_t name_len = strlen(name);
    hash_t hash = string_hash(name, name_len);
    if (DAGOR_LIKELY(noCollisions()))
      hashToStringId.erase(hash);
    else
      hashToStringId.erase(hash, [this, name, name_len](uint32_t id) { return string_equal(name, name_len, id); });
    for (auto &i : hashToStringId)
      if (i.val() > id)
        i.val()--;
    strings.erase(strings.begin() + id);
  }

  int nameCount() const { return (int)strings.size(); }
  const char *getName(int name_id) const
  {
    if (uint32_t(name_id) >= strings.size())
      return NULL;
    return getStringDataUnsafe(name_id);
  }
  size_t totalAllocated() const { return allocator.totalAllocated(); }
  size_t totalUsed() const { return allocator.totalUsed(); }

  void combine_pages()
  {
    // todo: implement me, find optimal start_page size
    return;
  }

  void shrink_to_fit()
  {
    hashToStringId.shrink_to_fit();
    strings.shrink_to_fit();
    // combine_pages();//while this can be potentially benefitial for mem usage, due to reminders in pages, in Enlisted it saves just
    // 7kb of memory, and requires one big chunk of memory (1.3mb)
    allocator.shrink_to_fit();
  }
  void reset(bool erase_only = true)
  {
    hashToStringId.clear();
    allocator.clear();
    strings.clear();
    if (!erase_only)
      shrink_to_fit();
  }
  void clear() { reset(false); }
  void reserve(int n)
  {
    hashToStringId.reserve(n);
    strings.reserve(n);
  }
};

struct FixedCapacityOAHashNameMap
{
  typedef uint32_t hash_t;
  // strings_size is used only for validation. 'zeroed' means that keys array is already zeroed
  bool fillNames(uint32_t names_cnt, uint32_t strings_size, bool zeroed)
  {
    if (hashToStringId.bucket_count() <= names_cnt)
      return false;
    if (!zeroed || hashToStringId.size())
      memset(hashToStringId.keys, 0, hashToStringId.bucket_count() * sizeof(hash_t));
    hashToStringId.used = 0;
    uint32_t cOfs = 0;
    for (uint32_t i = 0; i < names_cnt; ++i)
    {
      const size_t len = strlen(stringsData + cOfs);
      if (len + cOfs > strings_size)
        return false;
      stringsOfs[i] = cOfs;
      hashToStringId.emplace(string_hash(stringsData + cOfs, len), eastl::move(i));
      cOfs += (uint32_t)len;
    }
    stringsDataSize = strings_size;
    return hashToStringId.used == names_cnt;
  }
  // init FixedCapacityOAHashNameMap from dump. Dunp should be just all names (zero terminated) + enough memory to fit hash table
  // enough memory is just 4*strings count + max(4, bigger_pow_of_two(ctrings_count*4/3))*8
  bool init(char *data, uint32_t strings_size, uint32_t cnt, uint32_t total_size, bool zeroed = false)
  {
    const uint32_t stringsIdOfs = ((uintptr_t(data) + strings_size + 3) & ~uintptr_t(3)) - uintptr_t(data);
    const uint32_t buckets = oa_hashmap_util::max(4, get_bigger_pow2(cnt * 4 / 3));
    if (total_size < stringsIdOfs + cnt * sizeof(uint32_t) + buckets * (sizeof(uint32_t) + sizeof(hash_t)))
      return false;
    stringsData = data;
    stringsOfs = (uint32_t *)(data + stringsIdOfs);
    hashToStringId.keys = stringsOfs + cnt;
    hashToStringId.vals = hashToStringId.keys + buckets;
    hashToStringId.mask = buckets - 1;
    hashToStringId.used = 0;
    return fillNames(cnt, strings_size, zeroed);
  }
  char *stringsData;
  uint32_t *stringsOfs;
  FixedCapacityHashedKeyMap<hash_t, uint32_t> hashToStringId;
  uint32_t stringsDataSize;
  static inline hash_t string_hash(const char *s, size_t len)
  {
    hash_t ret = (hash_t)wyhash(s, len, 1);
    return ret ? ret : 1;
  }
  static inline hash_t string_hash(const char *s) { return string_hash(s, strlen(s)); }
  const char *getStringDataUnsafe(uint32_t id) const { return stringsData + stringsOfs[id]; }
  inline bool string_equal(const char *name, size_t name_len, uint32_t id) const
  {
    const uint32_t ofs = stringsOfs[id];
    const char *str = stringsData + ofs;
    return ((ofs + name_len <= stringsDataSize) ? memcmp(str, name, name_len) : strcmp(str, name)) == 0;
  }
  int getNameId(const char *name, size_t name_len, const hash_t hash) const
  {
    return hashToStringId.findOr(hash, -1, [this, name, name_len](uint32_t id) { return string_equal(name, name_len, id); });
  }
  int getNameId(const char *name, size_t name_len) const { return getNameId(name, name_len, string_hash(name, name_len)); }
  int getNameId(const char *name) const { return getNameId(name, strlen(name)); }

  uint32_t nameCount() const { return hashToStringId.size(); }
  const char *getName(uint32_t name_id) const
  {
    if (name_id >= hashToStringId.size())
      return NULL;
    return getStringDataUnsafe(name_id);
  }
};

using NameMap = OAHashNameMap<false>;
using NameMapCI = OAHashNameMap<true>;
using FastNameMap = OAHashNameMap<false>;
typedef FastNameMap FastNameMapEx;

#include <supp/dag_iterateStatus.h>

template <class T, bool CI>
static void gather_ids_in_lexical_order(T &out_sorted_stor, const OAHashNameMap<CI, DefaultOAHasher<CI>> &nm)
{
  out_sorted_stor.resize(nm.nameCount());
  for (int i = 0; i < out_sorted_stor.size(); i++)
    out_sorted_stor[i] = i;
  stlsort::sort(out_sorted_stor.begin(), out_sorted_stor.end(), [&nm](const int a, const int b) {
    return (CI ? dd_stricmp(nm.getName(a), nm.getName(b)) : strcmp(nm.getName(a), nm.getName(b))) < 0; //-V575
  });
}

template <typename CB, typename NM>
static inline uint32_t iterate_names_in_id_order(const NM &nm, CB cb /*(int id, const char *name)*/)
{
  const uint32_t count = nm.nameCount();
  for (int id = 0; id < count; id++)
    cb(id, nm.getStringDataUnsafe(id));
  return count;
}
template <typename CB, typename TAB, typename NM>
static inline void iterate_names_in_order(const NM &nm, const TAB &ids, CB cb /*(int id, const char *name)*/)
{
  for (int id : ids)
    cb(id, nm.getName(id));
}
template <typename CB, typename TAB, typename NM>
static inline IterateStatus iterate_names_in_order_breakable(const NM &nm, const TAB &ids, CB cb /*(int id, const char *name)*/)
{
  for (int id : ids)
  {
    IterateStatus s = cb(id, nm.getName(id));
    if (s != IterateStatus::Continue)
      return s;
  }
  return IterateStatus::StopDoneAll;
}
template <typename CB, bool CI>
static inline void iterate_names_in_lexical_order(const OAHashNameMap<CI, DefaultOAHasher<CI>> &nm,
  CB cb /*(int id, const char *name)*/)
{
  SmallTab<uint32_t> sorted_ids;
  gather_ids_in_lexical_order(sorted_ids, nm);
  iterate_names_in_order(nm, sorted_ids, cb);
}
template <typename CB, typename NM>
static inline uint32_t iterate_names(const NM &nm, CB cb /*(int id, const char *name)*/)
{
  return iterate_names_in_id_order(nm, cb);
}
template <typename CB, typename NM>
static inline IterateStatus iterate_names_breakable(const NM &nm, CB cb /*(int id, const char *name)*/)
{
  const uint32_t count = nm.nameCount();
  for (int id = 0; id < count; id++)
  {
    IterateStatus s = cb(id, nm.getStringDataUnsafe(id));
    if (s != IterateStatus::Continue)
      return s;
  }
  return IterateStatus::StopDoneAll;
}


#include <supp/dag_undef_COREIMP.h>
