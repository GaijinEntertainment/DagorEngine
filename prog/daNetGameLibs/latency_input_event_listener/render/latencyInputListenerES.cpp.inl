// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <latencyFlash/latencyInputListener.h>
#include <ecs/core/entityManager.h>


ECS_DECLARE_BOXED_TYPE(LatencyInputEventListener);
ECS_REGISTER_BOXED_TYPE(LatencyInputEventListener, nullptr);
ECS_AUTO_REGISTER_COMPONENT(LatencyInputEventListener, "latency_input_event_listener", nullptr, 0);