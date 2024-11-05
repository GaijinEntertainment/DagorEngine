// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcs.h>
#include <blood_puddles/public/render/bloodPuddles.h>
#include <dasModules/aotRendInst.h>

MAKE_TYPE_FACTORY(BloodPuddles, BloodPuddles);
MAKE_TYPE_FACTORY(PuddleCtx, BloodPuddles::PuddleCtx);

DAS_BIND_ENUM_CAST(BloodPuddles::DecalGroup);
