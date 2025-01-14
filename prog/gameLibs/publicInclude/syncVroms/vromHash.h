//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <hashUtils/hashUtils.h>
#include <vromfsHash/vromfsHash.h>

struct VromHasher : GenericHasher<SHA_DIGEST_LENGTH>
{
  using SHA1VromHashCalculator = VromfsHashCalculator<SHA1Hasher>;
  using Blake3ShortVromHashCalculator = VromfsHashCalculator<Blake3Hasher<Length>>;

  using HasherBuffer = eastl::array<uint8_t, eastl::max(sizeof(SHA1VromHashCalculator), sizeof(Blake3ShortVromHashCalculator))>;
  alignas(16) HasherBuffer buffer;

  VromHasher();
  ~VromHasher();
  void update(const uint8_t *data, size_t size);
  Value finalize();
};

using VromHash = GenericHash<VromHasher>;

extern const VromHash EMPTY_VROM_HASH;
