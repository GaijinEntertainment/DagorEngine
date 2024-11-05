// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <id/idIndexedMapping.h>
#include <backend/intermediateRepresentation.h>


namespace dabfg
{

enum class PassColor : uint32_t
{
};

using PassColoring = IdIndexedMapping<intermediate::NodeIndex, PassColor>;

// Performs a best-effort coloring of nodes such that nodes that should
// be within a single render pass will be colored the same
PassColoring perform_coloring(const intermediate::Graph &graph);

} // namespace dabfg
