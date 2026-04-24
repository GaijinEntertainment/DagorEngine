//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daNet/bitStream.h>

namespace net
{

namespace delta
{

class Buffer
{
  void incrementVersion();
  void setFullVersion();

public:
  Buffer();

  void reset();

  uint8_t getVersion() const { return version; }
  uint8_t getConfirmedVersion() const { return confirmedVersion; }
  void setConfirmedVersion(uint8_t vers) { confirmedVersion = vers; }

  void setBase(const danet::BitStream &data);
  void setBase(danet::BitStream &&data);

  danet::BitStream getDiff(const danet::BitStream &new_ver) const;
  danet::BitStream applyPatch(const danet::BitStream &delta);

  // check version: full packet make reset and need apply, different version skip - return false.
  bool checkVersionAndNeedApply(uint8_t incomming_version);

private:
  danet::BitStream base;
  uint8_t version;
  uint8_t confirmedVersion;
};

} // namespace delta

} // namespace net
