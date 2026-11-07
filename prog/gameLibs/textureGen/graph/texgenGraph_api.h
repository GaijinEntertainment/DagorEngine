// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/fixed_vector.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <float.h>
#include <stdint.h>

// The texture graph as both front-ends (the editor's GraphData and a das script) build it: templates, params, edges,
// finals and options, with no canvas state and no descriptors. See _doc/graph_text_format.md section 1.
namespace texgen
{
// 0 is none. Unique among the graph's current nodes and finals; front-ends never see indices.
struct NodeId
{
  int v = 0;
  explicit operator bool() const { return v != 0; }
  bool operator==(const NodeId &o) const { return v == o.v; }
};

// A descriptor templateUid (base_nodes.blk `templateUid:t`), "subgraph::<stem>" for a subgraph instance.
struct TemplateId
{
  eastl::string uid;
};

struct Value
{
  enum Type : uint8_t
  {
    Int,
    Bool,
    Float,
    Float2,
    Float3,
    Float4,
    String,
  };
  // The fields a type does not use keep their defaults.
  float f[4] = {};
  int i = 0;
  Type type = String;
  bool b = false;
  eastl::string s;

  static Value makeInt(int v);
  static Value makeFloat(float v);
  static Value makeBool(bool v);
  static Value makeFloats(Type t, const float *v); // t is Float, Float2, Float3 or Float4
  static Value makeString(const char *v);

  bool operator==(const Value &o) const;
};

// The text form: "%d", the shortest finite float text that parses back, "true"/"false", comma separated components, the string.
eastl::string value_to_text(const Value &v);
// A text that does not parse fully as `type` gives a String value, so no text is lost.
Value value_from_text(Value::Type type, const char *text);

struct Param
{
  eastl::string name;
  Value value;
};

// The out pin that feeds an input. pin indexes the template's out pins in descriptor order.
struct PinRef
{
  NodeId node; // none: unconnected
  uint16_t pin = 0;
  bool muted = false; // a muted input carries no data
};

enum class PinType : uint8_t
{
  Unknown = 0,
  Bool,
  Int,
  Uint,
  Float,
  Float2,
  Float3,
  Float4,
  Texture1D,
  Texture2D,
  Texture3D,
  Texture2DArray,
  Texture2DShdArray,
  Particles,
  BiomeData,
  NBSGbuffer,
  MaterialT,
  LayerT,
  MaskT,
  CtrlT,
};

PinType parse_pin_type(const char *type_name);
// Replaces out with the tokens of a comma separated types:t, an unknown one as Unknown.
void parse_pin_types_csv(const char *types_csv, eastl::fixed_vector<PinType, 4, true> &out);
bool is_texture_or_particles(PinType t);

// The pin types a node can inherit its texture size from: the array forms included, particles not.
bool is_texture_type_for_inherit(PinType t);

// 1 / 2 / 3, particles counting as 2, and 0 for the other types, the arrays included.
int texture_out_dimension(PinType t);

// Inverse of parse_pin_type; "" for Unknown.
const char *pin_type_to_string(PinType t);

struct SizeRule
{
  enum Kind : uint8_t
  {
    Explicit,  // value
    Parent,    // the size of the main input, else the graph size
    Graph,     // the graph size
    ParentMul, // main input size * value, else the graph size
    ParentDiv, // main input size / value, else the graph size
    Width,     // the node's own width; height and depth only
  };
  Kind kind = Parent;
  int value = 0;
  bool operator==(const SizeRule &o) const { return kind == o.kind && value == o.value; }
};

struct FmtRule
{
  enum Kind : uint8_t
  {
    Explicit, // fmt
    Parent,   // the format of the main input, else the graph format
    Graph,    // the graph format
  };
  Kind kind = Parent;
  eastl::string fmt; // Explicit only
  bool operator==(const FmtRule &o) const { return kind == o.kind && fmt == o.fmt; }
};

struct WrapRule
{
  enum Kind : uint8_t
  {
    Explicit, // wrap
    Parent,   // the wrap of the main input, else the graph wrap
    Graph,    // the graph wrap
  };
  Kind kind = Parent;
  bool wrap = true; // Explicit only; false is clamp
  bool operator==(const WrapRule &o) const { return kind == o.kind && wrap == o.wrap; }
};

// The texture size, type and wrap items of a template that produces a texture, and of the graph options.
inline constexpr const char *BASE_SIZES[] = {
  "= 1", "= 2", "= 4", "= 8", "= 16", "= 32", "= 64", "= 128", "= 256", "= 512", "= 1024", "= 2048", "= 4096", "= 8192"};
inline constexpr const char *BASE_TYPES[] = {
  "R8", "R16", "R16F", "R32F", "RG16", "RG16F", "RG32F", "ARGB8", "ARGB16", "ARGB16F", "ARGB32F", "R11G11B10F"};
inline constexpr const char *BASE_WRAPS[] = {"wrap", "clamp"};

// The relative items of a node only; they have no meaning at the graph level. Item 0 is the parent form and item 1 the
// graph form; "x N" / "/ N" scale the parent size.
inline constexpr const char *SIZE_PREFIXES[] = {
  "parent size", "graph size", "x 2", "x 4", "x 8", "x 16", "x 32", "/ 2", "/ 4", "/ 8", "/ 16", "/ 32"};
inline constexpr const char *TYPE_PREFIXES[] = {"parent type", "graph type"};
inline constexpr const char *WRAP_PREFIXES[] = {"parent wrap", "graph wrap"};
// The height and depth item that copies the node's own width.
inline constexpr const char *SIZE_SAME_AS_WIDTH = "width";

// The item text of a rule; a numbered size rule gives "= N", "x N" or "/ N" for any N.
eastl::string size_rule_text(const SizeRule &r);
eastl::string fmt_rule_text(const FmtRule &r);
eastl::string wrap_rule_text(const WrapRule &r);

// The rules apply only to a node whose template produces a texture; the others keep the defaults.
struct Node
{
  NodeId id;
  TemplateId tmpl;
  eastl::string label;
  eastl::vector<Param> params;
  eastl::vector<PinRef> inputs; // one per template in pin, in descriptor order, hidden pins kept, separators skipped
  SizeRule width;
  SizeRule height{SizeRule::Width};
  SizeRule depth{SizeRule::Width};
  FmtRule fmt;
  WrapRule wrap;
};

struct Export
{
  eastl::string file; // stored "<Final::name><file format>"; the compile resolves %property% references
  eastl::string fmt;  // stored channels text, "ARGB8", "R16", ...
  bool skipSaving = false;
};

// A final of the graph, src.node 0 while its pin is open. It is a template node with one final in pin and no out pins,
// so it has an id and a template like a Node; params holds what name and exportSpec do not.
struct Final
{
  NodeId id;
  TemplateId tmpl;
  PinRef src;
  eastl::string name;
  eastl::optional<Export> exportSpec;
  eastl::vector<Param> params;
};

// Splits e.file after a final_name prefix, else at the last dot.
void split_export_file(const Export &e, const eastl::string &final_name, eastl::string &name, eastl::string &format);

struct GraphOptions
{
  static constexpr float UNSET = FLT_MAX; // a heightmap field the graph does not set
  int width = 1024;
  int height = 1024;
  int depth = 1024;
  eastl::string fmt = "R16F";
  bool wrap = true; // false is clamp
  eastl::string renderDir;
  eastl::string entityDir;
  float heightmapScale = UNSET;
  float heightmapMin = UNSET;
  float heightmapCellSize = UNSET;
};

struct Subgraph;

struct Graph
{
  GraphOptions options;
  eastl::vector<Node> nodes;
  eastl::vector<Final> finals;
  // The graphs that instances name by tmpl.uid. A Graph can refer to a template it does not hold here.
  eastl::vector<Subgraph> subgraphs;

  // max(id of every node and final) + 1.
  NodeId newNodeId() const;
  const Node *findNode(NodeId id) const;
  const Final *findFinal(NodeId id) const;
};

struct Subgraph
{
  eastl::string uid;
  Graph graph;
};

// Checks ids (non-zero, unique over nodes and finals), that every input and final source names an existing node,
// that a muted ref is connected, and that float params and heightmap options are finite. Out pin indices need the
// descriptors, so they are not checked. Appends one line per problem to errors.
bool validate(const Graph &g, eastl::string &errors);
} // namespace texgen
