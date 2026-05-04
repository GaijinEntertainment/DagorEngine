// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <assets/asset.h>

ECS_BROADCAST_EVENT_TYPE(AssetAddedEvent, const DagorAsset *);
ECS_BROADCAST_EVENT_TYPE(AssetChangedEvent, const DagorAsset *);
ECS_BROADCAST_EVENT_TYPE(AssetRemovedEvent, const DagorAsset *);
