// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <ecs/terraform/terraform.h>

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::TerraformComponent::PrimMode, TerraformPrimMode);

MAKE_TYPE_FACTORY(TerraformComponent, TerraformComponent);
