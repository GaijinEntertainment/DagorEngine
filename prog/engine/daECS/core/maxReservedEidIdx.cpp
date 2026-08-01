// Copyright (C) Gaijin Games KFT.  All rights reserved.

namespace ecs
{
// Default cap of the reserved (server) eid index range; kept ALONE in its own
// module so a game can override it just by defining this symbol - the linker
// then uses the game's value and skips this file (an archive object links only
// when its symbol is still undefined; a shared TU would be a redefinition).
// Not constexpr (so it stays overridable); LTO still inlines the resolved value.
extern const unsigned MAX_RESERVED_EID_IDX_CONST = 65535;
} // namespace ecs
