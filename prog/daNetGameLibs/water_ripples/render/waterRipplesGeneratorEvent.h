// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "waterRipplesGenerator.h"
#include <gamePhys/common/loc.h>
#include <fftWater/fftWater.h>
#include <daECS/core/event.h>
#include <daECS/core/entitySystem.h>
#include <ecs/core/entityManager.h>

extern bool test_box_is_in_water(FFTWater &water, const TMatrix &transform, const BBox3 &box);

ECS_BROADCAST_EVENT_TYPE(GatherEmittersForWaterRipples, GatherEmittersEventCtx *)
