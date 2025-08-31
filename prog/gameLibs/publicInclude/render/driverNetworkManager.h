//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>

#include <drv/3d/dag_driverNetManager.h>

class DriverNetworkManager : public DriverNetManager
{
  eastl::string gameVersion;

public:
  DriverNetworkManager(const char *game_version) : gameVersion(game_version) {}

  void sendPsoCacheBlkSync(const DataBlock &cache_blk) override;
  void sendHttpEventLog(const char *type, const void *data, uint32_t size, Json::Value *meta) override;
  void addFileToCrashReport(const char *path) override;
};
