// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/hash_map.h>
#include <util/dag_stdint.h>
#include <openssl/sha.h>
#include <util/dag_simpleString.h>

class IGenLoad;
class IGenSave;

namespace datacache
{

struct WebEntry
{
  SimpleString path;
  int64_t size;
  int64_t lastModified;
  uint8_t hash[SHA_DIGEST_LENGTH];
};

typedef eastl::hash_map<size_t, WebEntry> EntriesMap;

void parse_index(EntriesMap &entries, IGenLoad *stream);

}; // namespace datacache
