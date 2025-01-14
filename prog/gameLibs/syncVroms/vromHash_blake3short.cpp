// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <syncVroms/vromHash.h>


using HasherImpl = VromHasher::Blake3ShortVromHashCalculator;


VromHasher::VromHasher() { new (buffer.data()) HasherImpl; }
VromHasher::~VromHasher() { ((HasherImpl *)buffer.data())->~HasherImpl(); }

void VromHasher::update(const uint8_t *data, size_t size)
{
  ((HasherImpl *)buffer.data())->update(make_span_const((const char *)data, size));
}


VromHasher::Value VromHasher::finalize() { return ((HasherImpl *)buffer.data())->finalizeRaw(); }
