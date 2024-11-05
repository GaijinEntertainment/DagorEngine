//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// dynamic const char*-> fixed const char* (non moveable memory)
// or dynamic const char*-> uint32_t (unique id)
// basically it stores hashmap of const_char *->uint32_t ids (which are page_no + page_offset)
// i.e. strings interning hashmap table
// allows no collision option (should be default for 64bit mum hashes with ~1000 names)
// ignore_case makes things much slower, as it forcing to use FNV1 hash + table access

#include <util/dag_nameHashers.h>
#include <util/dag_hashedKeyMap.h>
#include <util/dag_stringTableAllocator.h>

template <typename HashType = uint32_t,
  bool can_have_collisions = (sizeof(HashType) < 8), // by default we assume that 32bits hashes are not good enough, while 64bit are
                                                     // bullet-proof
  bool ignore_case = false, typename Hasher = DefaultOAHasher<ignore_case, HashType>>
struct UniqueHashedNames
{
  typedef HashType hash_t;
  static constexpr uint32_t invalid_id = ~0u;
  HashedKeyMap<hash_t, uint32_t> hashToStringId;
  StringTableAllocator allocator;
  uint8_t noCollisions() const { return !can_have_collisions || allocator.padding; }
  void setHasCollision()
  {
#if DAGOR_DBGLEVEL > 1
    G_ASSERT(can_have_collisions);
#endif
    if (can_have_collisions)
      allocator.padding = 1;
  }
  uint32_t addStringId(const char *name, size_t name_len)
  {
    const uint32_t len = (uint32_t)name_len + 1; // term zero, or one symbol after len. todo: fixme, so we don't memcpy from it
    const uint32_t at = allocator.addDataRaw(name, len);
    char *ret = allocator.head.data.get() + allocator.get_page_ofs(at);
    ret[name_len] = 0; // ensure term zero!
    return at;
  }
  uint32_t addStringId(const char *name) { return addStringId(name, strlen(name)); }
  const char *getStringFromId(uint32_t id) const { return allocator.getDataRawUnsafe(id); }
  const char *getStringFromIdSafe(uint32_t id) const { return id == invalid_id ? nullptr : allocator.getDataRawUnsafe(id); }

  const char *addString(const char *name, size_t name_len)
  {
    const uint32_t len = (uint32_t)name_len + 1; // term zero, or one symbol after len. todo: fixme, so we don't memcpy from it
    const uint32_t at = allocator.addDataRaw(name, len);
    char *ret = allocator.head.data.get() + allocator.get_page_ofs(at);
    ret[name_len] = 0; // ensure term zero!
    return ret;
  }
  const char *addString(const char *name) { return addString(name, strlen(name)); }
  static inline hash_t string_hash(const char *s, size_t len) { return Hasher::hash_data(s, len); }
  static inline hash_t string_hash(const char *s) { return string_hash(s, strlen(s)); }
  inline bool string_equal(const char *name, size_t name_len, const char *str) const
  {
    G_FAST_ASSERT(name != nullptr);
    if (ignore_case)
    {
      const unsigned char *to_lower_lut = dd_local_cmp_lwtab;
      for (; name_len; ++name, ++str, --name_len)
        if (to_lower_lut[uint8_t(*name)] != to_lower_lut[uint8_t(*str)])
          return false;
      return true;
    }
    // memcmp is faster but we can only call it, when there is enough data in str
    return strncmp(str, name, name_len) == 0; // we actually probably don't ever collide
  }
  inline bool string_equal(const char *name, size_t name_len, const uint32_t id) const
  {
    return string_equal(name, name_len, getStringFromId(id));
  }
  const char *getName(const char *name, size_t name_len, const hash_t hash) const
  {
    if (DAGOR_LIKELY(noCollisions()))
    {
      const char *it = getStringFromIdSafe(hashToStringId.findOr(hash, invalid_id));
      if (!can_have_collisions && it != nullptr)
      {
#if DAGOR_DBGLEVEL > 1
        G_ASSERT(string_equal(name, name_len, it));
#endif
        return it;
      }
      if (it == nullptr || !string_equal(name, name_len, it))
        return nullptr;
      return it;
    }
    // check hash collision
    return getStringFromIdSafe(hashToStringId.findOr(hash, invalid_id, [&](uint32_t id) { return string_equal(name, name_len, id); }));
  }
  const char *getName(const char *name, size_t name_len) const { return getName(name, name_len, string_hash(name, name_len)); }
  const char *getName(const char *name) const { return getName(name, strlen(name)); }
  uint32_t addNameId(const char *name, size_t name_len, hash_t hash)
  {
    uint32_t id = invalid_id;
    if (DAGOR_LIKELY(noCollisions()))
    {
      id = hashToStringId.findOr(hash, invalid_id);
      if (id != invalid_id)
      {
        if (!can_have_collisions)
        {
#if DAGOR_DBGLEVEL > 1
          G_ASSERT(string_equal(name, name_len, id));
#endif
          return id;
        }
        if (DAGOR_UNLIKELY(!string_equal(name, name_len, id))) // now we have collisions!
        {
          setHasCollision();
          id = invalid_id;
        }
      }
    }
    else
      id = hashToStringId.findOr(hash, invalid_id, [&](const uint32_t id) { return string_equal(name, name_len, id); });
    if (id == invalid_id)
    {
      id = addStringId(name, name_len);
      hashToStringId.emplace(hash, eastl::move(id));
    }
    return id;
  }
  uint32_t addNameId(const char *name, size_t name_len) { return addNameId(name, name_len, string_hash(name, name_len)); }
  uint32_t addNameId(const char *name) { return addNameId(name, strlen(name)); }

  const char *addName(const char *name, size_t name_len, hash_t hash) { return getStringFromIdSafe(addNameId(name, name_len, hash)); }
  const char *addName(const char *name, size_t name_len) { return addName(name, name_len, string_hash(name, name_len)); }
  const char *addName(const char *name) { return addName(name, strlen(name)); }

  size_t totalAllocated() const
  {
    return allocator.totalAllocated() + hashToStringId.bucket_count() * (sizeof(hash_t) + sizeof(uint32_t));
  }
  size_t totalUsed() const { return allocator.totalUsed(); }

  void shrink_to_fit()
  {
    hashToStringId.shrink_to_fit();
    allocator.shrink_to_fit();
  }
  void reset(bool erase_only = true)
  {
    hashToStringId.clear();
    allocator.clear();
    if (!erase_only)
      shrink_to_fit();
  }
};

#include <supp/dag_undef_KRNLIMP.h>
