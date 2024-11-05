// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <syncVroms/vromHash.h>


using HasherImpl = SHA1Hasher;


VromHasher::VromHasher() { new (buffer.data()) HasherImpl; }


void VromHasher::update(const uint8_t *data, size_t size) { ((HasherImpl *)buffer.data())->update(data, size); }


VromHasher::Value VromHasher::finalize() { return ((HasherImpl *)buffer.data())->finalize(); }
