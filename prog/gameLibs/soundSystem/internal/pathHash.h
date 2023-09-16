#pragma once

#include <hash/wyhash.h>

namespace sndsys
{
struct FMODGUID;
typedef uint64_t path_hash_t;
typedef uint64_t guid_hash_t;
__forceinline path_hash_t hash_fun(const char *path, size_t path_len) { return wyhash(path, path_len, 0); }
__forceinline guid_hash_t hash_fun(const FMODGUID &event_id) { return wyhash(&event_id, sizeof(event_id), 0); }
} // namespace sndsys
