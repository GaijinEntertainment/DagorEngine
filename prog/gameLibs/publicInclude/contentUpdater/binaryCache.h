//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <contentUpdater/version.h>
#include <EASTL/string.h>

namespace updater
{
namespace binarycache
{
// Copy the lastes vroms version to cache folder.
// The cache folder must contain the most recent version of the vroms.
// Final step fo content
enum class SyncMode
{
  NON_RECURSIVE,
  RECURSIVE,
};

int sync_vroms_in_folders(const char *src, const char *dst, SyncMode mode);
int sync_remote_vroms_with_cache(const char *remote_folder, const char *cache_folder, SyncMode mode);

bool set_cache_version(const char *folder, Version version);
Version get_cache_version(const char *folder);

void early_init(Version build_version, const eastl::string &cache_folder);
void init(Version build_version, const eastl::string &remote_folder, const eastl::string &cache_folder);

void purge(const eastl::string &cache_folder);
} // namespace binarycache
} // namespace updater
