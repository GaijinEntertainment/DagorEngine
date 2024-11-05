// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daProfilerInternal.h"

namespace da_profiler
{

inline uint32_t ProfilerData::getTLSDescription(const char *name, const char *file_name, int line, uint32_t flags, uint32_t color)
{
  ThreadStorage *storage = interlocked_relaxed_load_ptr(tls_storage);
  if (!storage)
    return (uint32_t)0;
  ThreadStorage::local_hash_t hash = wyhash(name, strlen(name), file_name ? wyhash(file_name, strlen(file_name), 0) : 1);
  hash = wyhash64(wyhash64(flags, color), wyhash64(hash, line));
  hash = hash ? hash : (ThreadStorage::local_hash_t(1) << ThreadStorage::local_hash_t(sizeof(hash) * 8 - 1));

  uint32_t description = (uint32_t)storage->hashToDescrId.findOr(hash, 0);
  if (DAGOR_LIKELY(description))
    return description;
  description = (uint32_t)descriptions.createCopy(name, file_name, line, flags, color);
  storage->hashToDescrId.emplace_new(hash, description);
  return description;
}


desc_id_t add_description(const char *file_name, int line, uint32_t flags, const char *name, uint32_t color)
{
  return (desc_id_t)the_profiler.descriptions.create(name, file_name, line, flags, color);
}
desc_id_t add_copy_description(const char *file_name, int line, uint32_t flags, const char *name, uint32_t color)
{
  return (desc_id_t)the_profiler.descriptions.createCopy(name, file_name, line, flags, color);
}

desc_id_t get_tls_description(const char *file_name, int line, uint32_t flags, const char *name, uint32_t color)
{
  return (desc_id_t)the_profiler.getTLSDescription(name, file_name, line, flags, color);
}

const char *get_description(desc_id_t i, uint32_t &color_and_flags) { return the_profiler.descriptions.get(i, color_and_flags); }

}; // namespace da_profiler