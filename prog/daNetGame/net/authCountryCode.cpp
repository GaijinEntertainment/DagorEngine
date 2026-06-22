// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "authCountryCode.h"
#include "authEvents.h"
#include <daECS/core/entityManager.h>

void auth_get_country_code(ecs::EntityManager &mgr, eastl::string &country)
{
  mgr.broadcastEventImmediate(NetAuthGetCountryCodeEvent{&country});
}
