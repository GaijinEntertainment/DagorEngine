// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// Command identifiers registered with the daEditorX EditorCommands registry
// (see GraphEditorPlg::registerEditorCommands). The id strings are also the
// keys used by the rebind UI and by de3_hotkeys.blk persistence.

inline constexpr const char *SHOW_GRAPH = "Plugin.GraphEditor.ShowGraph";
inline constexpr const char *SHOW_TEXTURE_PREVIEW = "Plugin.GraphEditor.ShowTexturePreview";
inline constexpr const char *SHOW_HISTOGRAM = "Plugin.GraphEditor.ShowHistogram";
inline constexpr const char *SHOW_LANDSCAPE_PREVIEW = "Plugin.GraphEditor.ShowLandscapePreview";
inline constexpr const char *SHOW_NODE_LIBRARY = "Plugin.GraphEditor.ShowNodeLibrary";
inline constexpr const char *SHOW_PROPERTIES = "Plugin.GraphEditor.ShowProperties";
inline constexpr const char *SHOW_SHORTCUTS = "Plugin.GraphEditor.ShowShortcuts";
inline constexpr const char *NEW_GRAPH = "Plugin.GraphEditor.NewGraph";
inline constexpr const char *LOAD_GRAPH_BLK = "Plugin.GraphEditor.LoadGraphBlk";
inline constexpr const char *SAVE_GRAPH_BLK = "Plugin.GraphEditor.SaveGraphBlk";
inline constexpr const char *SAVE_AS_SUBGRAPH_BLK = "Plugin.GraphEditor.SaveAsSubgraphBlk";
inline constexpr const char *SAVE_TEXTURES = "Plugin.GraphEditor.SaveTextures";
inline constexpr const char *SAVE_TEXTURES_BY_SUBSTRING = "Plugin.GraphEditor.SaveTexturesBySubstring";
inline constexpr const char *RELOAD_TEXTURES = "Plugin.GraphEditor.ReloadTextures";
inline constexpr const char *FORCE_REBUILD = "Plugin.GraphEditor.ForceRebuild";
inline constexpr const char *TOGGLE_GRAPH_PROPERTIES = "Plugin.GraphEditor.ToggleGraphProperties";

// Canvas-local shortcuts. Dispatched by graph_tab.cpp via ImGui::Shortcut with
// RouteFocused; imgui-node-editor's matching built-ins are suppressed at editor
// creation (see apply_editor_defaults).
inline constexpr const char *CANVAS_DELETE_SELECTED = "Plugin.GraphEditor.Canvas.DeleteSelected";
inline constexpr const char *CANVAS_COPY = "Plugin.GraphEditor.Canvas.Copy";
inline constexpr const char *CANVAS_CUT = "Plugin.GraphEditor.Canvas.Cut";
inline constexpr const char *CANVAS_PASTE = "Plugin.GraphEditor.Canvas.Paste";
inline constexpr const char *CANVAS_ZOOM_AND_CENTER = "Plugin.GraphEditor.Canvas.ZoomAndCenter";
inline constexpr const char *CANVAS_SELECT_NODES_NO_OUTPUTS = "Plugin.GraphEditor.Canvas.SelectNodesWithNoOutputs";
inline constexpr const char *CANVAS_SHOW_NEXT_SELECTED = "Plugin.GraphEditor.Canvas.ShowNextSelectedNode";
inline constexpr const char *CANVAS_REMOVE_KEEP_CONNECTIONS = "Plugin.GraphEditor.Canvas.RemoveKeepConnections";
inline constexpr const char *CANVAS_REMOVE_EDGES_AT_PIN = "Plugin.GraphEditor.Canvas.RemoveEdgesAtPin";
inline constexpr const char *CANVAS_MODIFY_EDGE_AT_PIN = "Plugin.GraphEditor.Canvas.ModifyEdgeAtPin";
inline constexpr const char *CANVAS_JUMP_OPPOSITE_PIN = "Plugin.GraphEditor.Canvas.JumpToOppositePin";
inline constexpr const char *CANVAS_COMMENT_PIN = "Plugin.GraphEditor.Canvas.CommentPin";
inline constexpr const char *CANVAS_DUPLICATE = "Plugin.GraphEditor.Canvas.Duplicate";
inline constexpr const char *CANVAS_TOGGLE_AUTOUPDATE = "Plugin.GraphEditor.Canvas.ToggleAutoUpdate";
inline constexpr const char *CANVAS_ADD_NODE_AT_PIN = "Plugin.GraphEditor.Canvas.AddNodeAtPin";
inline constexpr const char *CANVAS_ADD_TRANSIT_NODE = "Plugin.GraphEditor.Canvas.AddTransitNode";
inline constexpr const char *CANVAS_CREATE_SUBGRAPH = "Plugin.GraphEditor.Canvas.CreateSubgraph";
inline constexpr const char *CANVAS_OPEN_SUBGRAPH = "Plugin.GraphEditor.Canvas.OpenSubgraph";
inline constexpr const char *CANVAS_EXPLODE_SUBGRAPH = "Plugin.GraphEditor.Canvas.ExplodeSubgraph";
inline constexpr const char *CANVAS_GO_TO_PARENT_GRAPH = "Plugin.GraphEditor.Canvas.GoToParentGraph";
