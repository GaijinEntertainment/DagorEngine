// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>

#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <float.h>
#include <stdint.h>

// Shared between the daEditorX GraphEditor plugin (which owns the instance and populates it from
// JSON) and the in-editor texgen service (which reads the BLKs + dirs to drive generation). Lives
// in sharedInclude so the dynamically-loaded plugin and the editor binary can both include it
// without crossing source-tree boundaries.
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
bool is_texture_or_particles(PinType t);

// Pin direction role. Mirrors the descriptor "role:t" string ("in" / "out" / "any" / "ctrl").
// `isInput` on Pin is derived from this: isInput == (role != PinRole::Out).
enum class PinRole : uint8_t
{
  In,
  Out,
  Any,
  Ctrl,
};

struct GraphData
{
  // `name` and `customTextureName` are per-instance state -- written by the loader / drag-drop
  // creator and persisted to JSON. `pins[]` preserves JSON pin order so edges (which reference
  // pins by index) keep pointing to the right slot across descriptor evolutions.
  //
  // `type` / `types` / `typeGroup` / `role` / `isInput` / `singleConnect` / `hidden` are a *cache*
  // resolved from base_nodes.blk by `resolve_node_pins` at well-defined moments (graph load,
  // drag-drop insert, BLK reload). They are NOT authoritative -- the descriptor is. Never
  // serialize them back to JSON. `hidden` defaults to true so a pin not present in the
  // descriptor renders as hidden (safe default for unknown / dangling pins).
  struct Pin
  {
    eastl::string name;
    eastl::string customTextureName;             // texgen register name for output pins (empty otherwise)
    eastl::fixed_vector<PinType, 4, true> types; // full multi-type list from descriptor; drives validation
    eastl::string typeGroup;                     // empty when pin is not in a typeGroup
    PinType type = PinType::Unknown;             // first listed type, used for color
    PinRole role = PinRole::In;                  // source of truth for direction
    bool isInput = true;                         // derived: role != PinRole::Out
    bool singleConnect = false;
    bool hidden = true;
    // Per-instance pin data, populated from the JSON pin's "data" object.
    // For regular nodes this overlaps with the descriptor's pin data sub-block in
    // base_nodes.blk; for shader_editor sub-graph nodes (mask_from_index etc.)
    // the descriptor isn't in base_nodes.blk and instance data is the only source
    // of blk_code / customParam / particleCode / etc. compile_graph_to_blks
    // prefers this over the descriptor's data when both are present.
    DataBlock data;
  };
  struct Node
  {
    int id = 0;
    // Position in imgui-node-editor canvas-space. Loaded from JSON view.x/view.y as-is
    // (the JS editor saves canvas-space coords); drag-drop sets it from ne::ScreenToCanvas.
    // First render after load/insert pushes it to ne via SetNodePosition; from then on
    // imgui-node-editor owns the live position (user drags don't write back here).
    float x = 0.0f;
    float y = 0.0f;
    // templateUid is the stable key into base_nodes.blk (`templateUid:t` per node{} block).
    // All descriptor lookup goes through this. descName is the human-readable hint; it's
    // refreshed from the descriptor's current `name:t` on load and persisted to saved files
    // so the BLK is readable by hand, but it is NOT used for resolution. A template can be
    // renamed in base_nodes.blk without breaking saved graphs as long as templateUid stays.
    // Legacy graphs (no templateUid) trigger a one-time descName-based fallback in the
    // loaders, which populates templateUid in memory; the next save upgrades the file.
    eastl::string templateUid;
    eastl::string descName;
    eastl::string plugin; // from JSON "plugin" field; e.g. "shader_editor"; empty for built-ins
    eastl::vector<Pin> pins;
    // Property values keyed by name. Constraints (type / minVal / maxVal / items[]) live in
    // the descriptor. Values are stored as strings so the descriptor's declared type drives
    // parsing at consumption time.
    eastl::vector<eastl::pair<eastl::string, eastl::string>> propertyValues;
    bool allowLoop = false; // descriptor "allowLoop:b"; cycle-detection skip flag
    // User-resizable extent for "block" comment-container nodes (descName == "block"). Ignored
    // for every other descriptor. Default matches the JS editor's initial blockWidth/blockHeight
    // at tools/graphEditor/editorScripts/graphEditor.js (svgBlock starts 500x300, clamped to >=200
    // by the corner-drag resize). Persisted in BLK only for block nodes.
    float blockWidth = 500.0f;
    float blockHeight = 300.0f;
  };
  struct Edge
  {
    int id = 0;
    int elemA = 0;
    int pinA = 0;
    int elemB = 0;
    int pinB = 0;
  };

  // Display data (consumed by GraphPanel).
  eastl::vector<Node> nodes;
  eastl::vector<Edge> edges;

  // Texgen pipeline input (consumed by IGraphTexGenService).
  DataBlock mainGraphBlk;
  DataBlock shaderListBlk;

  // Output directories extracted from the graph JSON markers.
  eastl::string textureRootDir;
  eastl::string renderDir;
  eastl::string entityDir;

  // Heightmap metadata extracted from the graph JSON markers (for landscape preview).
  // FLT_MAX when the marker is absent.
  float heightmapScale = FLT_MAX;
  float heightmapMin = FLT_MAX;
  float heightmapCellSize = FLT_MAX;

  // Graph-level texture defaults applied by compile_graph_to_blks when a node's own
  // sizing properties say "parent size" / "graph size" / "graph type" / "graph wrap"
  // or no upstream node has propagated a size yet. Mirrors the `<graph properties>`
  // descriptor defaults at tools/graphEditor/mainNodes.js:3006-3007 ("= 1024" and
  // types[2] = "R16F"). Get these wrong and every node compiles against the wrong
  // render target -- e.g. R16 on a heightmap pipeline that expects R16F renders a
  // saturated red plane with single-pixel spikes from quantisation overflow.
  int graphTextureWidth = 1024;
  int graphTextureHeight = 1024;
  int graphTextureDepth = 1024;
  eastl::string graphTextureType = "R16F";
  eastl::string graphTextureWrap = "wrap";

  // Source path, retained for display.
  eastl::string sourcePath;
};

// `base_nodes_blk` is the loaded `tools/dagor_cdk/commonData/graphEditor/base_nodes.blk`. The
// loader uses it to fill the resolved-cache fields on each Node's pins (type / isInput / hidden)
// via `resolve_node_pins`. Pass nullptr to leave pins unresolved (all stay `hidden = true`).
bool load_graph_data(GraphData &out, const char *json_path, const char *shader_includes_dir,
  const DataBlock *base_nodes_blk = nullptr);

// Fills the resolved-cache fields (type, isInput, hidden) on each pin of `node` by looking
// up `node.descName` in `base_nodes_blk` and matching pins by name. Pins whose name is not in
// the descriptor stay `hidden = true`. If `base_nodes_blk` is null or the descriptor is not
// found, all pins stay `hidden = true`. Also evaluates the shader_editor visibility rule
// (`plugin == "shader_editor"` outputs whose first type is not texture/particles -> hidden),
// since the descriptor's types and the node's plugin are both available here.
void resolve_node_pins(GraphData::Node &node, const DataBlock *base_nodes_blk);

// Resets `out` to an empty state.
void clear_graph_data(GraphData &out);

// Serializes GraphData source-of-truth (nodes, edges, propertyValues, dirs,
// heightmap, graph-level texture defaults) as a text DataBlock at `blk_path`.
// Does NOT write `sourcePath`, and does NOT persist `mainGraphBlk` /
// `shaderListBlk` -- those are compiled output of the source-of-truth and must
// be regenerated on load by calling compile_graph_to_blks (in the GraphEditor
// plugin). Returns false on I/O failure.
bool save_graph_data_blk(const GraphData &data, const char *blk_path);

// Loads a graph previously produced by `save_graph_data_blk`. Calls
// `clear_graph_data` first, re-derives the descriptor-cache pin fields via
// `resolve_node_pins`, and sets `sourcePath = blk_path`. `mainGraphBlk` /
// `shaderListBlk` are left empty; the caller must invoke
// compile_graph_to_blks to repopulate them. `shader_includes_dir` is unused
// (kept for signature parity with the JSON loader).
bool load_graph_data_blk(GraphData &out, const char *blk_path, const char *shader_includes_dir,
  const DataBlock *base_nodes_blk = nullptr);
