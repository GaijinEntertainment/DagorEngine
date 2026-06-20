// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>

namespace ecs
{
class EntityManager;
}

void auth_get_country_code(ecs::EntityManager &mgr, eastl::string &country);
