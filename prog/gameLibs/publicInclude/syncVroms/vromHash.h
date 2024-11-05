//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <syncVroms/hashUtils.h>

struct VromHasher : GenericHasher<SHA_DIGEST_LENGTH>
{
  using HasherBuffer = eastl::array<uint8_t, eastl::max(sizeof(SHA1Hasher), sizeof(Blake3Hasher<Length>))>;
  alignas(16) HasherBuffer buffer;

  VromHasher();
  void update(const uint8_t *data, size_t size);
  Value finalize();
};

using VromHash = GenericHash<VromHasher>;

extern const VromHash EMPTY_VROM_HASH;
