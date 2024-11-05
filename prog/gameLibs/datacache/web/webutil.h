// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <openssl/sha.h>

class IGenLoad;
class IGenSave;

namespace datacache
{

const char *hashstr(const uint8_t hash[SHA_DIGEST_LENGTH], char buf[SHA_DIGEST_LENGTH * 2 + 1]);
bool hashstream(IGenLoad *stream, uint8_t out_hash[SHA_DIGEST_LENGTH], IGenSave *save = NULL);
bool copy_stream(IGenLoad *read, IGenSave *save);

}; // namespace datacache
