//
// Dagor Engine 6.5
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

constexpr ecs::hash_str_t ecs_str_hash(const char *b) { return str_hash_fnv1a(b); }
constexpr ecs::hash_str_t ecs_mem_hash(const char *b, size_t len) { return mem_hash_fnv1a(b, len); }


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
