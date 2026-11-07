// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// Hosts pull it: NEED_MODULE(TexgenModule)
namespace texgen
{
struct Graph;

// The file name a host serves the generated nodes module under.
inline constexpr const char *NODES_DAS = "texgen_nodes.das";

// The graph a script builds, owned by the calling thread. The reference stays valid until the next texgen_graph_reset();
// a host resets the graph before it runs each script.
Graph &texgen_graph();
void texgen_graph_reset();
} // namespace texgen
