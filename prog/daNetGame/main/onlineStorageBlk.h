// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/event.h>

class DataBlock;

namespace online_storage
{
class DataExchangeBLK
{
public:
  virtual ~DataExchangeBLK() {}
  virtual const char *getDataTypeName() const = 0;
  virtual bool applyGeneralBlk(const DataBlock &container) const = 0;
  virtual bool fillGeneralBlk(DataBlock &container) const = 0;
};
} // namespace online_storage

#define ONLINE_STORAGE_ECS_EVENTS                                                                                            \
  ONLINE_STORAGE_ECS_EVENT(OnlineStorageAddExchangeEvent, eastl::unique_ptr<online_storage::DataExchangeBLK> * /*exchange*/) \
  ONLINE_STORAGE_ECS_EVENT(SaveToOnlineStorageEvent, bool /*send_to_srv*/, bool * /*dest_result*/)

#define ONLINE_STORAGE_ECS_EVENT ECS_BROADCAST_EVENT_TYPE
ONLINE_STORAGE_ECS_EVENTS
#undef ONLINE_STORAGE_ECS_EVENT
