//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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

struct SchemelessEvent;
typedef eastl::optional<SchemelessEvent> MaybeSchemelessEvent;
void serialize_to(const SchemelessEvent &, danet::BitStream &bs);
MaybeSchemelessEvent deserialize_from(const danet::BitStream &bs);

} // namespace ecs
