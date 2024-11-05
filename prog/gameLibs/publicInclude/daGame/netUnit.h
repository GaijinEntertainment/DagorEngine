//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gamePhys/phys/physActor.h>
#include <util/dag_bitArray.h>

class NetUnit : public IPhysActor
{
private:
  uint8_t authorityUnitVersion = 0;

protected:
  float timeToResyncVersion = -1.f;
  int resyncVersionRequestsNumber = 0;
  Bitarray resyncVersionRequests;

public:
  uint8_t getAuthorityUnitVersion() const override { return authorityUnitVersion; }
  void setAuthorityUnitVersion(uint8_t ver) { authorityUnitVersion = ver; }

  void onAuthorityUnitVersion(uint8_t remote_version_unsafe) override;
  bool isUnitVersionMatching(uint8_t version) const override;
  void resetUnitVersion();

  void onUnitVersionMatch(uint8_t version, uint32_t sender_player_id);
  void onUnitVersionMismatch(uint8_t client_version, uint8_t server_version, uint32_t sender_player_id);

  virtual void sendUnitVersionMatch(uint8_t /*version*/) {}
  virtual void sendUnitVersionMismatch(uint8_t /*remote_version_unsafe*/, uint8_t /*version*/) {}
};
