// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/textureGen/graph/texgenGraph_api.h>

#include <EASTL/string.h>

class DataBlock;

// The das front-end of the texgen::Graph IR (prog/gameLibs/textureGen/_doc/graph_text_format.md, sections 2, 5 and 6).
// base_nodes_blk is the descriptor registry after preprocess_node_descriptor, as graph_ir.h expects.

// out_das is the module texgen_nodes.das: one function per node descriptor not hidden or named block, comment or <graph properties>,
// output() and export() from the first final templates without and with "file format", and the operators that operator:t names.
// out_sheet is the DSL sheet (Markdown, ASCII). A descriptor whose name has no identifier form and no alias:t,
// or whose identifier repeats an earlier one, and an operator that does not fit, are left out with one line each appended to errors.
void generate_das_nodes(const DataBlock &base_nodes_blk, eastl::string &out_das, eastl::string &out_sheet, eastl::string &errors);

// A straight-line script that builds g again, except g.subgraphs: `[export] def graph()` with one `let v<id>` per node
// in topological order. A node takes its generated function when the function can carry all of it, else the raw node() form.
// Deterministic. False, with one line per problem appended to errors, when validate() fails or the nodes form a loop.
bool graph_to_das(const texgen::Graph &g, const DataBlock &base_nodes_blk, eastl::string &out, eastl::string &errors);
