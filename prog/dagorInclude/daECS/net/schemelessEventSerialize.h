//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/schemelessEvent.h>
#include <EASTL/optional.h>

namespace danet
{
class BitStream;
}

namespace ecs
{

class EntityManager;
struct SchemelessEvent;
typedef eastl::optional<SchemelessEvent> MaybeSchemelessEvent;
void serialize_to(EntityManager &mgr, const SchemelessEvent &, danet::BitStream &bs);
MaybeSchemelessEvent deserialize_from(EntityManager &mgr, const danet::BitStream &bs);

} // namespace ecs
