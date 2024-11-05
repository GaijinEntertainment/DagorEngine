// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/aotEcs.h>
#include <dasModules/aotDagorMath.h>

#include <game/capsuleApproximation.h>
#include <render/capsulesAO.h>
#include <generic/dag_staticTab.h>

typedef decltype(CapsuleApproximation::capsuleDatas) CapsuleDatas;

MAKE_TYPE_FACTORY(CapsuleApproximation, CapsuleApproximation);
MAKE_TYPE_FACTORY(CapsuleData, CapsuleData);
DAS_BIND_STATIC_TAB(CapsuleDatas, CapsuleDatas, CapsuleData, "CapsuleDatas");
