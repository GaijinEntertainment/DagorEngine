// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/textureGen/graph/texgenGraph_api.h>

#include <EASTL/string.h>

struct GraphData;
class DataBlock;

// Conversions between the editor graph and the texgen::Graph IR. base_nodes_blk is the descriptor registry after
// preprocess_node_descriptor, so the texture size, type and wrap properties are in it. An IR node id is the GraphData
// node id + 1, because GraphData ids start at 0 and NodeId 0 is none.
//
// A node is a texgen::Final when its template has one in pin with data{final:b=yes} and no out pins; its
// "texture name" is the final name, and a template with "file format" on a node that stores "channels" makes it an export
// of "<texture name><file format>" in "channels", skipped when "skip when exporting" is "true". Pin indices in the IR
// count the template's pins in descriptor order, so the saved pin order of a node does not matter. Canvas state
// (positions, block, comment and subgraph separator nodes, pin comments) and the per instance pin data are not converted.

// False when a node has no template in base_nodes_blk, an edge does not join an out pin to an in pin or names a pin its
// template lacks, an in pin is fed twice, a node's texture size or wrap or the graph's wrap holds an unknown item,
// or validate() fails. errors gets one line per problem; validate()'s lines use IR ids.
bool graph_data_to_ir(const GraphData &gd, const DataBlock &base_nodes_blk, texgen::Graph &out, eastl::string &errors);

// Clears out, builds nodes from their templates with the descriptor defaults, then applies params, rules, finals and edges
// (with fresh edge ids) and runs auto_layout. g.subgraphs is not converted: a GraphData holds one graph file. False when
// validate() fails, a template is not in base_nodes_blk, a node's template is a final template or a final's is not,
// a ref names a pin its template lacks, or an export's template has no "file format".
bool ir_to_graph_data(const texgen::Graph &g, const DataBlock &base_nodes_blk, GraphData &out, eastl::string &errors);

// Sets x and y only: column = longest path from a source, row order by the mean row of the inputs, fixed pitch.
void auto_layout(GraphData &gd);

inline constexpr const char *PROP_TEXTURE_WIDTH = "texture width";
inline constexpr const char *PROP_TEXTURE_HEIGHT = "texture height";
inline constexpr const char *PROP_TEXTURE_DEPTH = "texture depth";
inline constexpr const char *PROP_TEXTURE_TYPE = "texture type";
inline constexpr const char *PROP_TEXTURE_WRAP = "texture wrap";
inline constexpr const char *PROP_FILE_FORMAT = "file format";

// A template's pins in descriptor order, hidden ones included, separators skipped: out pins, and the rest (in, any, ctrl) as inputs.
struct TemplatePins
{
  eastl::vector<const DataBlock *> ins;
  eastl::vector<const DataBlock *> outs;
};
void collect_template_pins(const DataBlock &desc, TemplatePins &out);
bool is_final_template(const TemplatePins &pins);

texgen::Value::Type value_type_for(const DataBlock *prop_desc);
// On false, out keeps its value. allow_width admits the "width" item, which only height and depth take.
bool parse_size_rule(const eastl::string &s, bool allow_width, texgen::SizeRule &out);
bool parse_fmt_rule(const eastl::string &s, texgen::FmtRule &out);
bool parse_wrap_rule(const eastl::string &s, texgen::WrapRule &out);
