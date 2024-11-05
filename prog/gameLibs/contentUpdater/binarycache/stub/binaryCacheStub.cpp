// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <contentUpdater/binaryCache.h>
#include <contentUpdater/version.h>

namespace updater
{
void binarycache::early_init(Version, const eastl::string &) {}
void binarycache::init(Version, const eastl::string &, const eastl::string &) {}
bool binarycache::set_cache_version(const char *, Version) { return false; }
Version binarycache::get_cache_version(const char *) { return Version{}; }
void binarycache::purge(const eastl::string &) {}
} // namespace updater
