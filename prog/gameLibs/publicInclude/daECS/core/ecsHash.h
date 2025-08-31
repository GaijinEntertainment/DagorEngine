//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_hash.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>

namespace ecs
{
typedef uint32_t hash_str_t;
}

template <int HashBits>
constexpr HashVal<HashBits> ecs_str_hash(const char *s, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return str_hash_fnv1<HashBits>(s, result);
}

template <int HashBits>
constexpr HashVal<HashBits> ecs_mem_hash(const char *b, size_t len, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return mem_hash_fnv1<HashBits>(b, len, result);
}

constexpr ecs::hash_str_t ecs_mem_hash(const char *b, size_t len) { return ecs_mem_hash<32>(b, len); }
constexpr ecs::hash_str_t ecs_str_hash(const char *b) { return ecs_str_hash<32>(b); }


namespace ecs
{
struct HashedConstString
{
  const char *str;
  hash_str_t hash;
};

#define ECS_HASH(a)      (ecs::HashedConstString{a, eastl::integral_constant<ecs::hash_str_t, ecs_str_hash(a)>::value})
#define ECS_HASH_SLOW(a) (ecs::HashedConstString{a, ecs_str_hash(a)})

inline hash_str_t ecs_hash(eastl::string_view str) { return ecs_mem_hash(str.data(), str.length()); }

struct EcsHasher
{
  size_t operator()(const eastl::string &str) const { return ecs_hash(str); }
};
struct EcsSvHasher
{
  size_t operator()(eastl::string_view str) const { return ecs_hash(str); }
};

} // namespace ecs
