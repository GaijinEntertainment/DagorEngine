// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/event.h>

ECS_BROADCAST_EVENT_TYPE(OnStoreApiEarlyInit)
ECS_BROADCAST_EVENT_TYPE(OnStoreApiInit)
ECS_BROADCAST_EVENT_TYPE(OnStoreApiTerm)
ECS_BROADCAST_EVENT_TYPE(OnStoreApiUpdate)
ECS_BROADCAST_EVENT_TYPE(StoreApiReportContentCorruption)
