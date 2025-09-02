// Copyright (C) Gaijin Games KFT.  All rights reserved.

// Note: not freestanding header (i.e don't include it directly) therefore doesn't need `#pragma once`

template <bool ignore_case, typename Hasher>
int OAHashNameMap<ignore_case, Hasher>::getNameId(const char *name, size_t name_len, const oa_hash_t hash) const
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

template <bool ignore_case, typename Hasher>
int OAHashNameMap<ignore_case, Hasher>::addNameId(const char *name, size_t name_len, oa_hash_t hash)
{
  int it = -1;
  if (DAGOR_LIKELY(noCollisions()))
  {
    it = hashToStringId.findOr(hash, -1);
    if (DAGOR_UNLIKELY(it != -1 && !string_equal(name, name_len, it)))
    {
      hasCollisions() = 1;
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
