//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class DataBlock;

namespace dataexport
{
class DataExportInterface
{
public:
  virtual void init(const DataBlock &cfg) = 0;
  virtual void destroy() = 0;
  virtual void send(void *buf, short len) = 0;
};

DataExportInterface *create(const DataBlock &cfg);
}; // namespace dataexport
