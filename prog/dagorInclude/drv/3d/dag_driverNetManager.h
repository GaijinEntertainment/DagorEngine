//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

class DataBlock;

namespace Json
{
class Value;
}

/**
 * @brief The DriverNetManager interface
 *
 * This class is responsible for providing network related functionality for d3d drivers.
 * Some drivers need to send data from clients for debugging and infrastructure purposes.
 * Methods of the class are very specific currently because we have a very strict set of use cases that we want to cover.
 */
class DriverNetManager
{
public:
  virtual ~DriverNetManager() = default; ///< Destructor
  /**
   * @brief Send a cache block to the server
   *
   * This method is used to send a cache block to the server.
   * A call of this method blocks the thread execution until the data is sent or the request is timed out.
   * The blocking behavior is necessarty to ensure that all the data is send and it doesn't affect the players experience since we
   * are sending data only when the driver shuts down.
   *
   * @param cache_blk The cache data block to send
   */
  virtual void sendPsoCacheBlkSync(const DataBlock &cache_blk) = 0;

  /**
   * @brief Send an event log to the server
   *
   * This method is used to send gpu crash dump from DX12 driver, but other event log collections also
   * could be used.
   */
  virtual void sendHttpEventLog(const char *type, const void *data, uint32_t size, Json::Value *meta) = 0;

  /**
   * @brief Adds a file to the crash report
   *
   * It is used to send GPU crash dumps from DX12 driver.
   */
  virtual void addFileToCrashReport(const char *path) = 0;
};
