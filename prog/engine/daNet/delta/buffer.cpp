// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/delta/buffer.h>

#include "diff_impl.h"

namespace net
{

namespace delta
{
const uint8_t versionMask = 0x7f;
const uint8_t versionIncrement = 0x01;
const uint8_t fullVersionMask = 0x80;

Buffer::Buffer() : version(0), confirmedVersion(0) { reset(); }

void Buffer::reset()
{
  base.Reset();
  incrementVersion();
  setFullVersion();
  confirmedVersion = ~version;
}

void Buffer::incrementVersion() { version = ((version & versionMask) + versionIncrement) & versionMask; }

void Buffer::setFullVersion() { version = (version & versionMask) | fullVersionMask; }

void Buffer::setBase(const danet::BitStream &data)
{
  base = data;
  incrementVersion();
}

void Buffer::setBase(danet::BitStream &&data)
{
  base = eastl::move(data);
  incrementVersion();
}

danet::BitStream Buffer::getDiff(const danet::BitStream &new_ver) const { return diff_impl(base, new_ver); }

danet::BitStream Buffer::applyPatch(const danet::BitStream &delta)
{
  danet::BitStream result = diff_impl(base, delta);
  setBase(result);
  return result;
}

bool Buffer::checkVersionAndNeedApply(uint8_t incomming_version)
{
  if (incomming_version & fullVersionMask)
  {
    reset();
    version = incomming_version;
    return true;
  }
  return incomming_version == version;
}

} // namespace delta

} // namespace net
